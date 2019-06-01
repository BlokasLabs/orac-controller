import argparse
import random
import time
import rtmidi
from enum import Enum

from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer
from pythonosc import osc_message_builder
from pythonosc import udp_client

parser = argparse.ArgumentParser()
parser.add_argument("--ip", default="127.0.0.1", help="The IP of the DisplayOSC server")
parser.add_argument("--port", type=int, default=6100, help="The port the DisplayOSC server is listening on")
args = parser.parse_args()

def send_text(a, b, c):
	return

class Orac:
	def __init__(self, ip, port):
		self.lines = [""]*6
		self.selectedLine = 0

		self.client = udp_client.SimpleUDPClient(args.ip, args.port)
		self.client.send_message("/Connect", 6111)

		self.oscDispatcher = Dispatcher()

		self.oscDispatcher.map("/text", self.textHandler)
		self.oscDispatcher.map("/selectText", self.selectTextHandler)
		self.oscDispatcher.map("/clearText", self.clearTextHandler)
		self.oscDispatcher.map("/*", self.allOtherHandler)

		self.server = BlockingOSCUDPServer(('', 6111), self.oscDispatcher)

		self.linesClearedCallbacks = []
		self.lineChangedCallbacks = []

	def navigationActivate(self):
		self.client.send_message("/NavActivate", 1.0)

	def navigationNext(self):
		self.client.send_message("/NavNext", 1.0)

	def navigationPrevious(self):
		self.client.send_message("/NavPrev", 1.0)

	def moduleNext(self):
		self.client.send_message("/ModuleNext", 1.0)

	def modulePrevious(self):
		self.client.send_message("/ModulePrev", 1.0)

	def addLinesClearedCallback(self, cb):
		self.linesClearedCallbacks.append(cb)

	def addLineChangedCallback(self, cb):
		self.lineChangedCallbacks.append(cb)

	def notifyLinesCleared(self):
		for cb in self.linesClearedCallbacks:
			cb(self)

	def notifyLineChanged(self, line, text, selected):
		for cb in self.lineChangedCallbacks:
			cb(self, line, text, selected)

	def run(self):
		self.server.serve_forever()

	def textHandler(self, address, *osc_arguments):
		print("%d: %s" % (osc_arguments[0], osc_arguments[1]))
		i = osc_arguments[0]
		if self.lines[i] != osc_arguments[1]:
			self.lines[i] = osc_arguments[1]
			self.notifyLineChanged(i, self.lines[i], self.selectedLine == i)

	def selectTextHandler(self, address, *osc_arguments):
		print("select %d" % osc_arguments[0])
		i = osc_arguments[0]
		if self.selectedLine != i:
			self.notifyLineChanged(self.selectedLine, self.lines[self.selectedLine], False)
			self.selectedLine = i
			self.notifyLineChanged(i, self.lines[i], True)

	def clearTextHandler(self, address, *osc_arguments):
		self.lines = [""]*6
		self.notifyLinesCleared()

	def allOtherHandler(self, address, *osc_arguments):
		print(address, osc_arguments)

# Class for interfacing with Midiboy
class OracCtl:
	class Button(Enum):
		B     = 0
		A     = 1
		Right = 2
		Down  = 3
		Left  = 4
		Up    = 5

	@staticmethod
	def findOracCtlPort(port):
		available_ports = port.get_ports()
		i = 0
		for p in available_ports:
			if "ORAC-CTL" in p:
				return i
			i = i+1

		raise Exception('ORAC-CTL port not found!')

	def __init__(self):
		self.midiOut = rtmidi.MidiOut()
		self.midiIn = rtmidi.MidiIn()

		self.midiOut.open_port(OracCtl.findOracCtlPort(self.midiOut))
		self.midiIn.open_port(OracCtl.findOracCtlPort(self.midiIn))
		self.midiIn.set_callback(self.midiInCallback)

		self.inputCallbacks = []

	def __del__(self):
		self.midiOut.close_port()
		del self.midiOut
		self.midiIn.close_port()
		del self.midiIn

	def addInputCallback(self, callback):
		self.inputCallbacks.append(callback)

	def notifyInput(self, button, down):
		for c in self.inputCallbacks:
			c(self, button, down)

	def midiInCallback(self, event, data=None):
		message = event[0]

		if message[0] != 0x90 and message[0] != 0x80 and (message[1] < 0 or message[1] >= 6):
			print("Ignoring unexpected message:", message)
			return

		down = message[0] == 0x90
		self.notifyInput(OracCtl.Button(message[1]), down)

		#value = 1.0 if pressedDown else 0.0
		#
		#if message[1] == 1:
		#	client.send_message("/NavActivate", value)
		#elif message[1] == 3:
		#	client.send_message("/NavNext", value)
		#elif message[1] == 5:
		#	client.send_message("/NavPrev", value)
		#elif message[1] == 2:
		#	client.send_message("/ModuleNext", value)
		#elif message[1] == 4:
		#	client.send_message("/ModulePrev", value)

	def printLine(self, line, text, inverted):
		msg = [0xf0, 0x01 if inverted else 0x00, line]

		for c in bytes(text if text else "", encoding='utf-8'):
			msg.append(c if c <= 0x7f else '_')

		msg.append(0xf7)

		self.midiOut.send_message(msg)

	def clearText(self):
		for i in range(6):
			self.printLine(i, "", False)

class Controller:
	def __init__(self, orac, oracCtl):
		self.orac = orac
		self.oracCtl = oracCtl
		self.oracCtl.addInputCallback(self.onButtonEvent)
		self.orac.addLineChangedCallback(self.onLineChanged)
		self.orac.addLinesClearedCallback(self.onLinesCleared)

	def onLinesCleared(self, sender):
		self.oracCtl.clearText()

	def onLineChanged(self, sender, line, text, inverted):
		self.oracCtl.printLine(line, text, inverted)

	def onButtonEvent(self, sender, button, down):
		if not down:
			return

		if button == OracCtl.Button.A:
			self.orac.navigationActivate()
		elif button == OracCtl.Button.Up:
			self.orac.navigationPrevious()
		elif button == OracCtl.Button.Down:
			self.orac.navigationNext()
		elif button == OracCtl.Button.Left:
			self.orac.modulePrevious()
		elif button == OracCtl.Button.Right:
			self.orac.moduleNext()

orac = Orac(args.ip, args.port)
oracCtl = OracCtl()
ctrl = Controller(orac, oracCtl)

try:
	orac.run()
finally:
	print("Done!")
