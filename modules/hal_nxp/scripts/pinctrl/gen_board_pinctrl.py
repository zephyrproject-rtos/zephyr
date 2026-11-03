#!/usr/bin/env python3
#
# Copyright 2023, NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
Wrapper around support libraries for i.MX RT, LPC, and Kinetis SOCs.
Allows user to generate pin control board files from MCUXpresso config
tools (MEX) files
"""

# Standard python libraries
import argparse
import datetime
import pathlib
import re
import sys
import tempfile
import zipfile

# SOC configuration data support libraries
from imx import imx_cfg_utils
from kinetis import kinetis_cfg_utils
from lpc import lpc_cfg_utils

# Shared MEX utilities
from shared import mex_utils

HELPSTR = """
Processes NXP MEX configuration files
Given a mex file, generates the board level pinctrl groups required for
Zephyr. This tool is intended to be used with the configuration data packs
downloaded from NXP's MCUXpresso SDK builder, as well as MEX files created
by the user using MCUXpresso configuration tools
"""


def parse_args():
    """
    Parses arguments, and returns object with parsed arguments
    as properties
    """
    parser = argparse.ArgumentParser(
        description=HELPSTR, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument('mex_file', metavar='MEX', type=str, help='path to source MEX file')
    parser.add_argument(
        'data_pack', metavar='DATA_PACK', type=str, help='Path to downloaded data package zip'
    )
    parser.add_argument('--copyright', action='store_true', help='Enable default NXP copyright')
    parser.add_argument(
        '--board-output', metavar='BOARD_OUT', type=str, help='Output path for board level DTS file'
    )
    parser.add_argument(
        '--controller',
        metavar='CTRL',
        type=str,
        help=("SOC pin controller type.Currently supports: [IOMUX, IOCON, PORT]"),
    )

    return parser.parse_args()


def processor_to_controller(processor_name):
    """
    Returns pin controller type for processor, or None if
    processor is unknown
    """
    # Select family of pin controller based on SOC type
    if "IMXRT1" in processor_name:
        # Use IMX config tools
        return 'IOMUX'
    if "IMXRT6" in processor_name:
        # LPC config tools
        return 'IOCON'
    if "IMXRT5" in processor_name:
        # LPC config tools
        return 'IOCON'
    if "LPC55" in processor_name:
        # LPC config tools
        return 'IOCON'
    if "MK" in processor_name:
        # Kinetis config tools
        return 'PORT'
    if "MCX" in processor_name:
        # Kinetis config tools
        return 'PORT'
    # Unknown processor family
    return "UNKNOWN"


def get_pack_version(pack_dir):
    """
    Gets datapack version
    @param pack_dir: root directory data pack is in
    """
    # Check version of the config tools archive
    npi_data = pathlib.Path(pack_dir) / 'npidata.mf'
    data_version = 0.0
    with open(npi_data, encoding='UTF8') as stream:
        line = stream.readline()
        while line != '':
            match = re.search(r'data_version=([\d\.]+)', line)
            if match:
                data_version = float(match.group(1))
                break
            line = stream.readline()
    return data_version


def main():
    """
    Main entry point. Will process arguments, and generate board pin control
    files.
    """
    args = parse_args()
    utils = mex_utils.NxpMexUtil(args.mex_file)
    pins_version = utils.get_pins_version()

    if round(pins_version) != 14:
        print(
            "Warning: This tool is only verified for MEX files version 14, "
            "other versions may not work"
        )

    board_name = utils.get_board_name()
    processor_name = utils.get_processor_name()
    package_name = utils.get_package_name()

    # Extract the Data pack to a temporary directory
    temp_dir = tempfile.TemporaryDirectory()
    zipfile.ZipFile(args.data_pack).extractall(temp_dir.name)
    proc_root = (
        pathlib.Path(temp_dir.name) / 'processors' / processor_name / 'ksdk2_0' / package_name
    )
    if not proc_root.exists():
        print(f"Error: Data pack does not contain processor data for {processor_name}")
        sys.exit(255)
    data_version = get_pack_version(temp_dir.name)

    print(
        f"Found data pack version {data_version}, pins version {pins_version} "
        f"for processor {processor_name}"
    )
    if round(data_version) != 14:
        print(
            "Warning: This tool is only verified for data pack version 14, "
            "other versions may not work"
        )

    if not args.board_output:
        args.board_output = f"{board_name.lower().replace('-', '_')}-pinctrl.dtsi"

    # Initialize SOC specific utility, and generate board pin control
    if args.copyright:
        # Add default copyright
        nxp_copyright = (
            f"Copyright {datetime.datetime.today().year}, NXP\n"
            f" * SPDX-License-Identifier: Apache-2.0"
        )
    else:
        nxp_copyright = ''

    if args.controller is None:
        args.controller = processor_to_controller(processor_name)

    # If controller is unknown, this is an unknown SOC and user did not
    # manually select a controller
    if args.controller == "UNKNOWN":
        print(f"Error: processor {processor_name} is not currently supported by this tool")
        print(
            "You can try specifying your pin controller family manually, but this is unsupported!"
        )
        sys.exit(255)

    # Select correct config tool script for the MEX file
    if args.controller == 'IOMUX':
        cfg_util = imx_cfg_utils.NXPSdkUtil(str(proc_root), copyright_header=nxp_copyright)
    elif args.controller == 'IOCON':
        cfg_util = lpc_cfg_utils.NXPSdkUtil(str(proc_root), copyright_header=nxp_copyright)
    elif args.controller == 'PORT':
        cfg_util = kinetis_cfg_utils.NXPSdkUtil(str(proc_root), copyright_header=nxp_copyright)
    else:
        print("Error: unknown controller type")
        sys.exit(255)
    print(f"Generating configuration for {board_name}")
    cfg_util.write_pinctrl_groups(args.mex_file, args.board_output)
    print(f"Wrote board pinctrl dts file to {args.board_output}")


if __name__ == "__main__":
    main()
