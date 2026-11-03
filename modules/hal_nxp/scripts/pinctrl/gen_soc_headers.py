#!/usr/bin/env python3
#
# Copyright 2023-2026 NXP
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

import dedup_pinctrl

# SOC configuration data support libraries
from imx import imx_cfg_utils, imx_fixup_pinmux
from kinetis import kinetis_cfg_utils
from lpc import lpc_cfg_utils
from shared import pinctrl_layout

HELPSTR = """
Processes NXP signal configuration files

Given a processor data pack, generates the SOC level pinctrl DTSI definitions
required for Zephyr. This tool is intended to be used with the configuration
data downloaded from NXP's MCUXpresso SDK builder.
"""

# Data pack version this tool is verified against. Only used for a warning:
# other versions usually work, but the layout of the register and signal
# description files is not a stable interface.
SUPPORTED_DATA_VERSION = 26.06


def parse_args():
    """
    Parses arguments, and returns object with parsed arguments
    as properties
    """
    parser = argparse.ArgumentParser(
        description=HELPSTR,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        'data_pack', metavar='DATA_PACK', type=str, help='Path to downloaded data package zip'
    )
    parser.add_argument(
        '--copyright',
        action='store_true',
        default=True,
        help='Include default NXP copyright (enabled by default)',
    )
    parser.add_argument(
        '--soc-output',
        metavar='SOC_OUT',
        type=str,
        help=(
            'Override the family/series output root (default: '
            'Zephyr dt-bindings/pinctrl/nxp for .h, '
            'dts/arm/nxp for .dtsi)'
        ),
    )
    parser.add_argument(
        '--controller',
        metavar='CTRL',
        type=str,
        help=("SOC pin controller type.Currently supports: [IOMUX, IOCON, PORT]"),
    )
    parser.add_argument(
        '--iomuxc-file',
        metavar='IOMUXC',
        type=str,
        help=(
            'Path to fsl_iomuxc.h file for daisy register fixup. '
            'Required for IOMUX controller (i.MX RT 4-digit parts only)'
        ),
    )
    parser.add_argument(
        '--no-dedup',
        action='store_true',
        help=(
            'Do not deduplicate the generated files. By default, '
            'part number files with identical pin mux data are replaced '
            'by one-line wrappers that include a single canonical file '
            '(see dedup_pinctrl.py)'
        ),
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
    if "IMXRT2" in processor_name:
        # i.MX RT266x (RT2660 family) uses IOMUXC, same IMX config tools flow
        return 'IOMUX'
    if "IMXRT7" in processor_name:
        # LPC config tools
        return 'IOCON'
    if "IMXRT6" in processor_name:
        # LPC config tools
        return 'IOCON'
    if "IMXRT5" in processor_name:
        # LPC config tools
        return 'IOCON'
    if "IMX8M" in processor_name:
        # IMX config tools
        return 'IOMUX'
    if "IMX9" in processor_name:
        # IMX config tools
        return 'IOMUX'
    if "LPC5" in processor_name:
        # LPC config tools
        return 'IOCON'
    if "MK" in processor_name:
        # Kinetis config tools
        return 'PORT'
    if "K32" in processor_name:
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
    Main entry point. Will process data pack, and generate board pin control
    headers
    """
    args = parse_args()
    if args.soc_output and not pathlib.Path(args.soc_output).is_dir():
        print('SOC output path must be a directory')
        sys.exit(255)

    # Extract the Data pack to a temporary directory
    temp_dir = tempfile.TemporaryDirectory()
    zipfile.ZipFile(args.data_pack).extractall(temp_dir.name)

    data_version = get_pack_version(temp_dir.name)
    print(f"Found data pack version {data_version}")
    if abs(data_version - SUPPORTED_DATA_VERSION) > 1e-9:
        print(
            f"Warning: This tool is verified for data pack version "
            f"{SUPPORTED_DATA_VERSION}, other versions may not work"
        )

    # Attempt to locate the signal XML files we will generate from
    proc_root = pathlib.Path(temp_dir.name) / 'processors'
    search_pattern = "*/ksdk2_0/*/signal_configuration.xml"
    # Pathlib glob returns an iteration, so use sum to count the length
    package_count = sum(1 for _ in proc_root.glob(search_pattern))
    if package_count == 0:
        search_pattern = "*/i_mx_2_0/*/signal_configuration.xml"
        package_count = sum(1 for _ in proc_root.glob(search_pattern))
        if package_count == 0:
            print("No signal configuration files were found in this data pack")
            sys.exit(255)
    if args.copyright:
        # Add default copyright in the machine-parsable SPDX / REUSE form.
        nxp_copyright = (
            f"SPDX-FileCopyrightText: Copyright "
            f"{datetime.datetime.today().year} NXP\n"
            f" *\n"
            f" * SPDX-License-Identifier: Apache-2.0"
        )
    else:
        nxp_copyright = ""
    written_files = []
    for signal_xml in proc_root.glob(search_pattern):
        package_root = signal_xml.parent
        package_name = package_root.name
        if args.controller is None:
            # Determine pin controller type using SOC package name
            args.controller = processor_to_controller(package_name)
        # If controller is still unknown, error out
        if args.controller == "UNKNOWN":
            print(f"Error: package {package_name} is not currently supported by this tool")
            print(
                "You can try specifying your pin controller family manually, "
                "but this is unsupported!"
            )
            sys.exit(255)

        if args.controller == 'IOMUX' and not args.iomuxc_file:
            print(
                "Error: --iomuxc-file is required for IOMUX controller to fix daisy register values"
            )
            sys.exit(255)

        # Select correct config tool script for the signal file
        if args.controller == 'IOMUX':
            cfg_util = imx_cfg_utils.NXPSdkUtil(str(package_root), copyright_header=nxp_copyright)
            part_num = cfg_util.get_part_num()
            file_name = f"{part_num.lower()}-pinctrl.dtsi"
        elif args.controller == 'IOCON':
            cfg_util = lpc_cfg_utils.NXPSdkUtil(str(package_root), copyright_header=nxp_copyright)
            part_num = cfg_util.get_part_num()
            file_name = f"{part_num.upper()}-pinctrl.h"
        elif args.controller == 'PORT':
            cfg_util = kinetis_cfg_utils.NXPSdkUtil(
                str(package_root), copyright_header=nxp_copyright
            )
            part_num = cfg_util.get_part_num()
            file_name = f"{part_num.upper()}-pinctrl.h"
        else:
            print("Error: unknown controller type")
            sys.exit(255)

        suffix = pathlib.Path(file_name).suffix
        out_dir = (
            pathlib.Path(args.soc_output) if args.soc_output else pinctrl_layout.output_root(suffix)
        )
        subdir = pinctrl_layout.resolve_subdir(part_num, suffix)
        if subdir is None:
            sys.exit(f"Error: could not classify pinctrl family for {part_num}")
        target_dir = out_dir / subdir
        target_dir.mkdir(parents=True, exist_ok=True)
        out_path = str(target_dir / file_name)

        if args.controller == 'IOMUX':
            # Build the IOMUXC select-input maps from the register data before
            # writing anything: they complete the daisy column of the generated
            # entries, and an unrecognized description means the map would be
            # silently incomplete, which must not be papered over.
            daisy_maps = cfg_util.build_daisy_maps()
            if daisy_maps['unparsed']:
                print(
                    "Error: unrecognized IOMUXC select-input descriptions, "
                    "the daisy map would be incomplete:"
                )
                for periph_name, reg_name, desc in daisy_maps['unparsed'][:10]:
                    print(f"  {periph_name}.{reg_name}: \"{desc}\"")
                sys.exit(1)
            recovered = cfg_util.write_pinctrl_defs(out_path, daisy_maps)
        else:
            daisy_maps = None
            recovered = cfg_util.write_pinctrl_defs(out_path)
        written_files.append(out_path)
        print(f"Wrote pinctrl headers to {out_path}")
        if recovered:
            print(
                f"  recovered {len(recovered)} mux option(s) from the "
                "select-input candidate lists: " + ", ".join(recovered)
            )

        if args.controller == 'IOMUX':
            ground_truth = imx_fixup_pinmux.parse_iomuxc_ground_truth(args.iomuxc_file)
            report = imx_fixup_pinmux.validate_pinctrl_file(out_path, daisy_maps, ground_truth)
            for report_line in imx_fixup_pinmux.format_report(report, out_path):
                print(report_line)
            if imx_fixup_pinmux.report_is_fatal(report):
                print("Error: daisy validation failed, see above")
                sys.exit(1)
    if not args.no_dedup and written_files:
        # Deduplicate the generated files against each other AND against
        # the pinctrl files already present in the output directories, so
        # a single-NPI data pack can reuse an existing part's data (or
        # become the new canonical if it is the higher-numbered flagship).
        dedup_set = set()
        for out_file in written_files:
            dedup_set.add(pathlib.Path(out_file))
            dedup_set.update(pathlib.Path(out_file).parent.glob("*-pinctrl.*"))
        clusters, deltas, stale, failures = dedup_pinctrl.dedup_files(sorted(dedup_set), apply=True)
        wrapper_count = sum(len(c["wrappers"]) for c in clusters)
        saved = sum(c["bytes_saved"] for c in clusters) + sum(d["saved"] for d in deltas)
        print(
            f"Dedup: {wrapper_count} files became wrappers and "
            f"{len(deltas)} became delta wrappers "
            f"({saved / 1024:.0f}KB saved)"
        )
        if failures:
            print(
                f"Error: {len(failures)} deduplicated file(s) failed "
                f"content verification and were reverted to their full "
                f"form (see VERIFY FAILED above)"
            )
            sys.exit(1)
        if stale:
            print(
                f"Error: {len(stale)} existing wrapper(s) reference "
                f"regenerated content that changed; regenerate those "
                f"parts' data packs as well (see STALE WRAPPER above)"
            )
            sys.exit(1)


if __name__ == "__main__":
    main()
