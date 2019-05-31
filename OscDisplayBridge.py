import argparse
import random
import time
import rtmidi

from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer
from pythonosc import osc_message_builder
from pythonosc import udp_client

parser = argparse.ArgumentParser()
parser.add_argument("--ip", default="127.0.0.1", help="The IP of the DisplayOSC server")
parser.add_argument("--port", type=int, default=6100, help="The port the DisplayOSC server is listening on")
args = parser.parse_args()

class Orac:
	def __init__(self, osc_dispatcher):
		self.lines = [""]*6
		self.selectedLine = 0

		osc_dispatcher.map("/text", self.text_handler)
		osc_dispatcher.map("/selectText", self.select_text_handler)
		osc_dispatcher.map("/clearText", self.clear_text_handler)
		osc_dispatcher.map("/*", self.all_other_handler)

	def text_handler(self, address, *osc_arguments):
		print("%d: %s" % (osc_arguments[0], osc_arguments[1]))
		i = osc_arguments[0]
		self.lines[i] = osc_arguments[1]
		send_text(i, self.lines[i], i == self.selectedLine)

	def select_text_handler(self, address, *osc_arguments):
		print("select %d" % osc_arguments[0])
		i = osc_arguments[0]
		if self.selectedLine != i:
			send_text(self.selectedLine, self.lines[self.selectedLine], False)
			self.selectedLine = i
			send_text(i, self.lines[i], True)		

	def clear_text_handler(self, address, *osc_arguments):
		for i in range(6):
			self.lines[i] = ""
			send_text(i, "", False)

	def all_other_handler(self, address, *osc_arguments):
		print(address, osc_arguments)

client = udp_client.SimpleUDPClient(args.ip, args.port)

def find_orac_ctl_port(port):
	available_ports = port.get_ports()
	i = 0
	for p in available_ports:
		if "ORAC-CTL" in p:
			return i
		i = i+1

	return -1

midiout = rtmidi.MidiOut()
midiout.open_port(find_orac_ctl_port(midiout))

midiin = rtmidi.MidiIn()
midiin.open_port(find_orac_ctl_port(midiin))

def midiin_callback(event, data=None):
	# 0 1 2     3    4    5
	# B A Right Down Left Up
	message = event[0]
	pressedDown = message[0] == 0x90

	#if not pressedDown:
	#	return

	value = 1.0 if pressedDown else 0.0

	if message[1] == 1:
		client.send_message("/NavActivate", value)
	elif message[1] == 3:
		client.send_message("/NavNext", value)
	elif message[1] == 5:
		client.send_message("/NavPrev", value)
	elif message[1] == 2:
		client.send_message("/ModuleNext", value)
	elif message[1] == 4:
		client.send_message("/ModulePrev", value)

midiin.set_callback(midiin_callback)

def send_text(line, text, inverted):
	msg = [0xf0, 0x01 if inverted else 0x00, line]

	for c in bytes(text if text else "", encoding='utf-8'):
		msg.append(c if c <= 0x7f else '_')

	msg.append(0xf7)

	midiout.send_message(msg)

dispatcher = Dispatcher()


#async def loop():
#	"""Example main loop that only runs for 10 iterations before finishing"""
#	for i in range(10):
#		print(f"Loop {i}")
#		await asyncio.sleep(1)

client.send_message("/Connect", 6111)

orac = Orac(dispatcher)

#async def init_main():
server = BlockingOSCUDPServer(('', 6111), dispatcher)

try:
	server.serve_forever()
finally:
	print("Cleaning up MIDI.")
	midiin.close_port()
	del midiin
	midiout.close_port()
	del midiout
	print("Done!")
