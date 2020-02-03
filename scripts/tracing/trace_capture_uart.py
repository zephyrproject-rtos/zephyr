#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to capture tracing data with UART backend.
"""

import sys
import serial
import argparse

def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-d", "--serial_port", required=True,
                        help="serial port")
    parser.add_argument("-b", "--serial_baudrate", required=True,
                        help="serial baudrate")
    parser.add_argument("-o", "--output", default='channel0_0',
                        required=False, help="tracing data output file")
    args = parser.parse_args()

def main():
    parse_args()
    serial_port = args.serial_port
    serial_baudrate = args.serial_baudrate
    output_file = args.output
    try:
        ser = serial.Serial(serial_port, serial_baudrate)
        ser.isOpen()
    except serial.SerialException as e:
        sys.exit("{}".format(e))

    print("serial open success")

    #enable device tracing
    ser.write("enable\r".encode())

    with open(output_file, "wb") as file_desc:
        while True:
            count = ser.inWaiting()
            if count > 0:
                while count > 0:
                    data = ser.read()
                    file_desc.write(data)
                    count -= 1

    ser.close()

if __name__=="__main__":
    try:
        main()
    except KeyboardInterrupt:
        print('Data capture interrupted, data saved into {}'.format(args.output))
        sys.exit(0)
