#!/usr/bin/env python3
#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0

"""Import files from an NXP MCUXpresso SDK archive into Zephyr

The MCUXpresso SDK provides device header files and peripheral drivers for NXP
Kinetis, LPC, and i.MX SoCs. Zephyr drivers for these SoCs are shims that adapt
MCUXpresso SDK APIs to Zephyr APIs.

This script automates updating Zephyr to a newer version of the MCUXpresso SDK.
"""

import argparse
import os
import re
import shutil
import sys
import tempfile

if "ZEPHYR_BASE" not in os.environ:
    sys.stderr.write("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)

ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]

class Sdk:
    '''MCUXpresso SDK class'''
    def __init__(self, root):
        self.root = root

    def get_devices(self):
        '''Get a list of devices in the SDK'''
        return os.listdir(os.path.join(self.root, 'devices'))

    def get_device_headers(self, device):
        '''Get a list of device header files in the SDK'''
        device_root = os.path.join(self.root, 'devices', device)
        patterns = [ device, 'fsl_device_registers' ]
        pattern = "|".join(patterns)
        device_headers = []
        for f in os.listdir(device_root):
            if re.search(pattern, f):
                device_headers.append(os.path.join(device_root, f))
        return device_headers

    def get_drivers(self, device, shared):
        '''Get a list of driver files in the SDK

        Many drivers can be shared across multiple SoCs, but at least one
        driver is device-specific. The 'shared' argument allows us to select
        either device-specific drivers or shared drivers.
        '''
        drivers_root = os.path.join(self.root, 'devices', device, 'drivers')
        device_specific_pattern = 'fsl_clock'
        drivers = []
        for f in os.listdir(drivers_root):
            if bool(re.search(device_specific_pattern, f)) ^ bool(shared):
                drivers.append(os.path.join(drivers_root,f))
        return drivers

    def get_shared_drivers(self, device):
        return self.get_drivers(device, True)

    def get_clock_driver(self, device):
        return self.get_drivers(device, False)

def get_soc_family(device):
    if device.startswith('MK'):
        return 'kinetis'
    elif device.startswith('LPC'):
        return 'lpc'
    elif device.startswith('MIMX'):
        return 'imx'

def copy_files(files, dst):
    for f in files:
        shutil.copy2(f, dst)

def import_sdk(directory):
    sdk = Sdk(directory)

    for device in sdk.get_devices():
        family = get_soc_family(device)
        shared_dst = os.path.join(ZEPHYR_BASE, 'ext/hal/nxp/mcux/drivers', family)
        device_dst = os.path.join(ZEPHYR_BASE, 'ext/hal/nxp/mcux/devices', device)

        print('Importing {} device headers'.format(device))
        device_headers = sdk.get_device_headers(device)
        copy_files(device_headers, device_dst)

        print('Importing {} clock driver'.format(device))
        clock_driver = sdk.get_clock_driver(device)
        copy_files(clock_driver, device_dst)

        print('Importing {} family shared drivers'.format(family))
        shared_drivers = sdk.get_shared_drivers(device)
        copy_files(shared_drivers, shared_dst)

def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument("-f", "--file", required=True,
                        help="MCUXpresso SDK archive file to import from")

    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as d:
        print('Extracting MCUXpresso SDK into temporary directory {}'.format(d))
        shutil.unpack_archive(args.file, d)
        import_sdk(d)

def main():
    parse_args()

if __name__ == "__main__":
    main()
