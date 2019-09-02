#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sathish Kuttan <sathish.k.kuttan@intel.com>

# This script sends a command to either start or stop audio streams
# to an Intel Sue Creek S1000 target connected to a Linux host
# over USB.
# The target should be running the sample audio application from the
# sample project to which this script belongs.

import hid
import yaml
import os
import argparse

class Device:
    def __init__(self):
        """
        Reads configuration file to determine the USB VID/PID of
        the Sue Creek target.
        When the script is run using sudo permission, the
        manufacturer and product strings are printed
        """
        config_file = os.path.join(os.path.dirname(__file__), 'config.yml')
        with open(config_file, 'r') as ymlfile:
            config = yaml.safe_load(ymlfile)
        self.name = config['general']['name']
        self.usb_vid = config['usb']['vid']
        self.usb_pid = config['usb']['pid']
        self.hid_dev = hid.device()
        if self.hid_dev is None:
            print('Device not found')
        else:
            self.hid_dev.open(self.usb_vid, self.usb_pid)

    def start_audio(self):
        """
        Sends a command to start the audio transfers
        in the Sue Creek target.
        """
        # space for 1 byte report id, 2 bytes of padding and 1 byte report size
        command = 'start_audio'.encode('utf-8') + b'\x00'
        command += b'\x00' * (56 - len(command))
        cmd_len = len(command) // 4 + 1
        command = b'\x01\x00' + cmd_len.to_bytes(2, byteorder='little') + \
                command
        command = b'\x01\x00\x00\x38' + command
        print('Starting Audio on %s ...' % self.hid_dev.get_product_string())
        self.hid_dev.send_feature_report(command)
        self.hid_dev.read(len(command))

    def stop_audio(self):
        """
        Sends a command to stop the running audio transfers
        in the Sue Creek target.
        """
        # space for 1 byte report id, 2 bytes of padding and 1 byte report size
        command = 'stop_audio'.encode('utf-8') + b'\x00'
        command += b'\x00' * (56 - len(command))
        cmd_len = len(command) // 4 + 1
        command = b'\x02\x00' + cmd_len.to_bytes(2, byteorder='little') + \
                command
        command = b'\x01\x00\x00\x38' + command
        print('Stopping Audio on %s ...' % self.hid_dev.get_product_string())
        self.hid_dev.send_feature_report(command)
        self.hid_dev.read(len(command))

def main():
    parser = argparse.ArgumentParser(epilog='NOTE: use sudo -E %(prog)s to run the script')
    parser.add_argument('command', choices=['start', 'stop'],
            help='start or stop audio streams')
    args = parser.parse_args()
    sue_creek = Device()
    if args.command == 'start':
        sue_creek.start_audio()
    if args.command == 'stop':
        sue_creek.stop_audio()

if __name__ == '__main__':
    main()
