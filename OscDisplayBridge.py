import argparse
import random
import time
import rtmidi
from threading import Timer
from enum import IntEnum

from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer
from pythonosc import osc_message_builder
from pythonosc import udp_client

parser = argparse.ArgumentParser()
parser.add_argument("--ip", default="127.0.0.1", help="The IP of the DisplayOSC server")
parser.add_argument("--port", type=int, default=6100, help="The port the DisplayOSC server is listening on")
args = parser.parse_args()

class Orac:
	MAX_LINES = 5
	MAX_PARAMS = 8

	def __init__(self, ip, port):
		self.lines = [""]*Orac.MAX_LINES
		self.selectedLine = 0

		self.params = [{"name": "", "value": "", "ctrl": 0.0} for _ in range(Orac.MAX_PARAMS)]

		self.client = udp_client.SimpleUDPClient(args.ip, args.port)
		self.client.send_message("/Connect", 6111)

		self.oscDispatcher = Dispatcher()

		self.oscDispatcher.map("/text", self.textHandler)
		self.oscDispatcher.map("/selectText", self.selectTextHandler)
		self.oscDispatcher.map("/clearText", self.clearTextHandler)
		self.oscDispatcher.map("/P*Desc", self.paramDescHandler)
		self.oscDispatcher.map("/P*Ctrl", self.paramCtrlHandler)
		self.oscDispatcher.map("/P*Value", self.paramValueHandler)
		self.oscDispatcher.map("/*", self.allOtherHandler)

		self.server = BlockingOSCUDPServer(('', 6111), self.oscDispatcher)

		self.linesClearedCallbacks = []
		self.lineChangedCallbacks = []
		self.paramNameChangedCallbacks = []
		self.paramValueChangedCallbacks = []
		self.paramCtrlChangedCallbacks = []

		self.lineChangedNotificationsEnabled = True
		self.timer = None
		self.linesSnapshot = None

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

	def addParamNameChangedCallback(self, cb):
		self.paramNameChangedCallbacks.append(cb)

	def addParamValueChangedCallback(self, cb):
		self.paramValueChangedCallbacks.append(cb)

	def addParamCtrlChangedCallback(self, cb):
		self.paramCtrlChangedCallbacks.append(cb)

	def notifyLinesCleared(self):
		for cb in self.linesClearedCallbacks:
			cb(self)

	def notifyLineChanged(self, line, text, selected):
		for cb in self.lineChangedCallbacks:
			cb(self, line, text, selected)

	def notifyParamNameChanged(self, i, name):
		for cb in self.paramNameChangedCallbacks:
			cb(self, i, name)

	def notifyParamValueChanged(self, i, value):
		for cb in self.paramValueChangedCallbacks:
			cb(self, i, value)

	def notifyParamCtrlChanged(self, i, ctrl):
		for cb in self.paramCtrlChangedCallbacks:
			cb(self, i, ctrl)

	def run(self):
		self.server.serve_forever()

	def textHandler(self, address, *osc_arguments):
		print("%d: %s" % (osc_arguments[0], osc_arguments[1]))
		i = osc_arguments[0]-1
		if self.lines[i] != osc_arguments[1]:
			self.lines[i] = osc_arguments[1]
			if self.lineChangedNotificationsEnabled:
				self.notifyLineChanged(i, self.lines[i], self.selectedLine == i)

	def selectTextHandler(self, address, *osc_arguments):
		print("select %d" % osc_arguments[0])
		i = osc_arguments[0]-1
		if self.selectedLine != i:
			if self.lineChangedNotificationsEnabled:
				self.notifyLineChanged(self.selectedLine, self.lines[self.selectedLine], False)
			self.selectedLine = i
			if self.lineChangedNotificationsEnabled:
				self.notifyLineChanged(i, self.lines[i], True)

	def handleScreenUpdate(self):
		if self.lines == [""]*Orac.MAX_LINES:
			self.notifyLinesCleared()
		else:
			for i in range(Orac.MAX_LINES):
				if self.linesSnapshot[i] != self.lines[i]:
					self.notifyLineChanged(i, self.lines[i], i == self.selectedLine)

		self.lineChangedNotificationsEnabled = True
		self.timer = None

	def clearTextHandler(self, address, *osc_arguments):
		self.lineChangedNotificationsEnabled = False

		if self.timer != None:
			self.timer.cancel()
		else:
			self.linesSnapshot = self.lines.copy()

		self.timer = Timer(0.2, self.handleScreenUpdate)
		self.timer.start()

		self.lines = [""]*Orac.MAX_LINES

	@staticmethod
	def decodeParamId(oscAddress):
		return ord(oscAddress[2]) - ord('1')

	def paramDescHandler(self, address, *osc_arguments):
		i = Orac.decodeParamId(address)
		if self.params[i]["name"] != osc_arguments[0]:
			self.params[i]["name"] = osc_arguments[0]
			self.notifyParamNameChanged(i, osc_arguments[0])

	def paramValueHandler(self, address, *osc_arguments):
		i = Orac.decodeParamId(address)
		if self.params[i]["value"] != osc_arguments[0]:
			self.params[i]["value"] = osc_arguments[0]
			self.notifyParamValueChanged(i, osc_arguments[0])

	def paramCtrlHandler(self, address, *osc_arguments):
		i = Orac.decodeParamId(address)
		if self.params[i]["ctrl"] != osc_arguments[0]:
			self.params[i]["ctrl"] = osc_arguments[0]
			self.notifyParamCtrlChanged(i, osc_arguments[0])

	def allOtherHandler(self, address, *osc_arguments):
		print(address, osc_arguments)

# Class for interfacing with Midiboy
class OracCtl:
	class Button(IntEnum):
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

	def printLine(self, line, text, inverted):
		msg = [0xf0, 0x01 if inverted else 0x00, line]

		for c in bytes(text if text else "", encoding='utf-8'):
			msg.append(c if c <= 0x7f else '_')

		msg.append(0xf7)

		self.midiOut.send_message(msg)

	def printParam(self, i, name, value):
		if not name or not value:
			self.printLine(i, "", False)
		else:
			self.printLine(i, "%s: %s" % (name, value), False)

	def printCtrl(self, i, ctrl):
		msg = [0xf0, 0x02, i, int(ctrl * 127), 0xf7]
		self.midiOut.send_message(msg)

	def clearScreen(self):
		msg = [0xf0, 0x03, 0xf7]
		self.midiOut.send_message(msg)

	def setViewMode(self, mode):
		msg = [0xf0, 0x04, int(mode), 0xf7]
		print(msg)
		self.midiOut.send_message(msg)

class Controller:
	class Mode(IntEnum):
		UNKNOWN = 0
		MENU    = 1
		PARAMS  = 2

	def __init__(self, orac, oracCtl):
		self.mode = Controller.Mode.UNKNOWN
		self.lines = [{"text": "", "inverted": False} for _ in range(Orac.MAX_LINES)]
		self.params = [{"name": "", "value": "", "ctrl": 0.0} for _ in range(Orac.MAX_PARAMS)]

		self.orac = orac
		self.oracCtl = oracCtl
		self.oracCtl.addInputCallback(self.onButtonEvent)
		self.orac.addLineChangedCallback(self.onLineChanged)
		self.orac.addLinesClearedCallback(self.onLinesCleared)
		self.orac.addParamNameChangedCallback(self.onParamNameChanged)
		self.orac.addParamValueChangedCallback(self.onParamValueChanged)
		self.orac.addParamCtrlChangedCallback(self.onParamCtrlChanged)

		self.setMode(Controller.Mode.MENU)

	def setMode(self, mode):
		print(self.mode, "->", mode)
		if self.mode == mode:
			return

		self.oracCtl.clearScreen()
		self.oracCtl.setViewMode(mode)

		if mode == Controller.Mode.MENU:
			for i in range(Orac.MAX_LINES):
				self.oracCtl.printLine(i, self.lines[i]["text"], self.lines[i]["inverted"])
		elif mode == Controller.Mode.PARAMS:
			for i in range(Orac.MAX_PARAMS):
				if self.params[i]["name"] or self.params[i]["value"]:
					self.oracCtl.printParam(i, self.params[i]["name"], self.params[i]["value"])
					self.oracCtl.printCtrl(i, self.params[i]["ctrl"])

		self.mode = mode

	def onLinesCleared(self, sender):
		self.lines = [{"text": "", "inverted": False} for _ in range(Orac.MAX_LINES)]
		print("linesCleared")
		if self.mode == Controller.Mode.MENU:
			self.oracCtl.clearScreen()

	def onLineChanged(self, sender, line, text, inverted):
		self.lines[line]["text"] = text
		self.lines[line]["inverted"] = inverted
		print("onLineChanged", line, self.lines[line])
		if self.mode == Controller.Mode.MENU:
			self.oracCtl.printLine(line, text, inverted)

		print(self.lines)

	def onParamNameChanged(self, sender, i, name):
		self.params[i]["name"] = name
		if self.mode == Controller.Mode.PARAMS:
			self.oracCtl.printParam(i, self.params[i]["name"], self.params[i]["value"])

	def onParamValueChanged(self, sender, i, value):
		self.params[i]["value"] = value
		if self.mode == Controller.Mode.PARAMS:
			self.oracCtl.printParam(i, self.params[i]["name"], self.params[i]["value"])

	def onParamCtrlChanged(self, sender, i, ctrl):
		self.params[i]["ctrl"] = ctrl
		if self.mode == Controller.Mode.PARAMS:
			self.oracCtl.printCtrl(i, self.params[i]["ctrl"])

	def onButtonEvent(self, sender, button, down):
		if not down:
			return

		if button == OracCtl.Button.B:
			print("opa", self.mode)
			#if self.mode == Controller.Mode.PARAMS:
			#	self.setMode(Controller.Mode.MENU)
			#else:
			#	self.setMode(Controller.Mode.PARAMS)
			self.setMode(Controller.Mode.MENU if self.mode == Controller.Mode.PARAMS else Controller.Mode.PARAMS)
			print("opa", self.mode)

		if self.mode != Controller.Mode.MENU:
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
	del oracCtl
	oracCtl = None
	print("Done!")
