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

def get_soc_family(device):
    if device.startswith('MK'):
        return 'kinetis'
    elif device.startswith('LPC'):
        return 'lpc'
    elif device.startswith('MIMX'):
        return 'imx'

def get_files(src, pattern):
    matches = []
    nonmatches = []
    for filename in os.listdir(src):
        path = os.path.join(src, filename)
        if re.search(pattern, filename):
            matches.append(path)
        else:
            nonmatches.append(path)

    return [matches, nonmatches]

def copy_files(files, dst):
    for f in files:
        shutil.copy2(f, dst)

def import_sdk(directory):
    devices = os.listdir(os.path.join(directory, 'devices'))

    for device in devices:
        family = get_soc_family(device)
        shared_dst = os.path.join(ZEPHYR_BASE, 'ext/hal/nxp/mcux/drivers', family)
        device_dst = os.path.join(ZEPHYR_BASE, 'ext/hal/nxp/mcux/devices', device)

        device_src = os.path.join(directory, 'devices', device)
        device_pattern = "|".join([device, 'fsl_device_registers'])
        [device_headers, ignore] = get_files(device_src, device_pattern)

        drivers_src = os.path.join(directory, 'devices', device, 'drivers')
        drivers_pattern = "fsl_clock|fsl_iomuxc"
        [device_drivers, shared_drivers] = get_files(drivers_src, drivers_pattern)

        print('Importing {} device headers to {}'.format(device, device_dst))
        copy_files(device_headers, device_dst)

        print('Importing {} device-specific drivers to {}'.format(device, device_dst))
        copy_files(device_drivers, device_dst)

        print('Importing {} family shared drivers to {}'.format(family, shared_dst))
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
