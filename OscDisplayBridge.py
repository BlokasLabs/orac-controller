import argparse
import random
import time
import rtmidi

from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer
from pythonosc import osc_message_builder
from pythonosc import udp_client

def find_orac_ctl_port():
	midiout = rtmidi.MidiOut()
	available_ports = midiout.get_ports()
	i = 0
	for p in available_ports:
		if "ORAC-CTL" in p:
			return i
		i = i+1

	return -1

midiout = rtmidi.MidiOut()
midiout.open_port(find_orac_ctl_port())

def text_handler(address, *osc_arguments):
	print(osc_arguments)
	midiout.send_message([0x90, 0x40, 0x30])

parser = argparse.ArgumentParser()
parser.add_argument("--ip", default="127.0.0.1", help="The IP of the DisplayOSC server")
parser.add_argument("--port", type=int, default=6100, help="The port the DisplayOSC server is listening on")
args = parser.parse_args()

client = udp_client.SimpleUDPClient(args.ip, args.port)

dispatcher = Dispatcher()
dispatcher.map("/text", text_handler)

#async def loop():
#	"""Example main loop that only runs for 10 iterations before finishing"""
#	for i in range(10):
#		print(f"Loop {i}")
#		await asyncio.sleep(1)


#async def init_main():
server = BlockingOSCUDPServer(('127.0.0.1', 6111), dispatcher)
server.serve_forever()
