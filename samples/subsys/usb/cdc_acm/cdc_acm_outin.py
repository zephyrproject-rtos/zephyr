#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
# Copyright (c) 2020 PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

from time import sleep
import tqdm
import serial
import sys

ser_dev = serial.Serial('/dev/ttyACM1', 115200, timeout=0.1, writeTimeout=0.1)
max_len = 256
wait_time = 0.001
numof_retries = 100

def test_outin():

    for payload_len in tqdm.trange(1, max_len, ascii=True, desc="progress",
                                   dynamic_ncols=True, unit="bytes",
                                   unit_scale=numof_retries*(max_len)/2):

        for n in range(0, numof_retries):
            data_in = None

            data_out = serial.to_bytes(range(payload_len))
            to_device = 0

            if ser_dev is None:
                sys.exit(1)

            if not ser_dev.writable():
                print("OUT: not ready, droped block of length: ", payload_len)
                continue

            try:
                to_device = ser_dev.write(data_out)
                sleep(wait_time)
            except serial.SerialTimeoutException:
                print("OUT: timeout, droped block of length: ", payload_len)
                continue
            except serial.SerialException:
                print("OUT: exception, droped block of length: ", payload_len)
                continue

            while not ser_dev.readable() or not ser_dev.inWaiting() > 0:
                print("IN: not ready, block length: ", payload_len)
                sleep(wait_time)
                continue

            try:
                data_in = ser_dev.read(payload_len);
                #ser_dev.flushInput()
            except serial.SerialTimeoutException:
                print("IN: timeout, block length", payload_len)
                continue
            except serial.SerialException:
                print("IN: exception, block length", payload_len)
                continue

            if data_out != data_in or payload_len != len(data_in):
                print("Transfer errror OUT: %d IN: %d"
                      % (payload_len, len(data_in)))
                sys.exit(1)

def main():
    test_outin()

if ser_dev is None:
    print("Failed to open serial port.")
    sys.exit(0)

#ser_dev.dtr = False
ser_dev.flushInput()
ser_dev.flushOutput()
main()
