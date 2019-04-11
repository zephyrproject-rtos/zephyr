#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import sys, traceback
import getopt
import serial
import os
import time
from threading import Thread
from threading import Timer
import click
import signal

class uart_test(Thread):
	def __init__(self, tty_port, baud):
		signal.signal(signal.SIGTERM, self.handleSignal)
		signal.signal(signal.SIGINT, self.handleSignal)
		signal.signal(signal.SIGTSTP, self.handleSignal)

		Thread.__init__(self)

		self.console = serial.Serial(
			port=tty_port,
			baudrate=baud,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout = 0.01, # 10 ms
			)
		self.running = True
		if self.console.isOpen():
			self.console.flushInput()
			self.console.flushOutput()
		self.start()

	def __del__(self):
		self.console.close()

	def close(self):
		self.running = False
		time.sleep(1)
		self.join()
		if self.console.isOpen():
			self.console.close()

	def packet_send(self, cmd):
		if self.console.isOpen():
			self.console.write(cmd.encode())

	def handleSignal(self, signum, frame):
		self.close()

	def tx_test(self):
		if self.running:
			my_packet = "This is a stm32 F429zi packet!\r\n"
			print("Write: " + my_packet)
			self.packet_send(my_packet)
			self.tx_timer = Timer(1.0, self.tx_test)
			self.tx_timer.start();

	def run(self):

		self.tx_test()
		while self.running:
			if self.console.inWaiting() > 0:
				input_str = self.console.readline()
				print("rcv: %s" % input_str)

		self.tx_timer.cancel();
		self.console.close();


@click.command("Start dma test")
@click.option('-b', '--baud', default=115200, type=int, help='Baud rate')
@click.argument("tty_port")
def main(tty_port, baud):
	try:
		test_shell =  uart_test(tty_port, baud)

	except KeyboardInterrupt:
		test_shell.close()
		exit()
	except serial.SerialException:
		print("Open serial port %s failed!" % tty_port)
	except SystemExit:
		print('Exiting...')
	except:
		traceback.print_exc()
		sys.exit(3)

if __name__ == "__main__":
	main()
