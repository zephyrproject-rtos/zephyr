#!/usr/bin/env python3

# SPDX-FileCopyrightText: (C) 2026 NXP

# SPDX-License-Identifier: Apache-2.0

"""
Wrapper around imx_cfg_utils.py to provide arguments to module
"""

import argparse
import pathlib
import sys
from errno import EINVAL

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
import imx_cfg_utils

HELPSTR = """
Processes NXP iMX.RT signal configuration files

Given a processor folder, generates the SOC level gpio DTSI mux options used
within Zephyr. These definitions should be copied where appropriate.

This tool is intended to be used with the configuration data downloaded
from NXP's website. One way to extract this config data is to use the
MCUXpresso configuration tool to download processor definitions, locate
those definitions on the disk. Alternatively, individual processor config
data packs can be downloaded from the MCUXpresso SDK builder tool. Either way,
the path to the 'processors' directory must be provided as the 'cfg-tool-root'
parameter.
"""


parser = argparse.ArgumentParser(
    description=HELPSTR,
    formatter_class=argparse.RawDescriptionHelpFormatter,
    allow_abbrev=False,
)
parser.add_argument(
    'processor_root',
    metavar='PROC_ROOT',
    type=str,
    help='folder with processor signal configuration files',
)

args = parser.parse_args()

# Attempt to locate the signal XML files we will generate from
proc_root = pathlib.Path(args.processor_root) / 'ksdk2_0'
if not proc_root.exists():
    print(f"Error: Path {args.processor_root} provided for processor root is invalid")
    sys.exit(EINVAL)
# Iterate through all processor package signal.xml files
for package_root in proc_root.iterdir():
    signal_path = package_root / 'signal_configuration.xml'
    # Check if this is a processor package folder
    if signal_path.exists():
        # Generate SOC GPIO file
        util = imx_cfg_utils.NXPSdkUtil(str(package_root))
        out_path = f"./{util.get_part_num().lower()}-gpio.dtsi"
        util.write_gpio_mux(out_path)
        print(f"Wrote SOC GPIO file to {out_path}")
