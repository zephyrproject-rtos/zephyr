#!/usr/bin/env python3

# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2024 SILA Embedded Solutions GmbH
# SPDX-License-Identifier: Apache-2.0

# This script uses edtlib to generate a pickled edt from a devicetree
# (.dts) file. Information from binding files in YAML format is used
# as well.
#
# Bindings are files that describe devicetree nodes. Devicetree nodes are
# usually mapped to bindings via their 'compatible = "..."' property.
#
# See Zephyr's Devicetree user guide for details.
#
# Note: Do not access private (_-prefixed) identifiers from edtlib here (and
# also note that edtlib is not meant to expose the dtlib API directly).
# Instead, think of what API you need, and add it as a public documented API in
# edtlib. This will keep this script simple.

import argparse
import os
import pickle
import sys
from typing import NoReturn

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-devicetree',
                                'src'))

import edtlib_logger
from devicetree import edtlib


def main():
    args = parse_args()

    edtlib_logger.setup_edtlib_logging()

    vendor_prefixes = {}
    for prefixes_file in args.vendor_prefixes:
        vendor_prefixes.update(edtlib.load_vendor_prefixes_txt(prefixes_file))

    try:
        edt = edtlib.EDT(args.dts, args.bindings_dirs,
                         # Suppress this warning if it's suppressed in dtc
                         warn_reg_unit_address_mismatch=
                             "-Wno-simple_bus_reg" not in args.dtc_flags,
                         default_prop_types=True,
                         infer_binding_for_paths=["/zephyr,user"],
                         werror=args.edtlib_Werror,
                         vendor_prefixes=vendor_prefixes)
    except edtlib.EDTError as e:
        sys.exit(f"devicetree error: {e}")

    # Save merged DTS source, as a debugging aid
    with open(args.dts_out, "w", encoding="utf-8") as f:
        print(edt.dts_source, file=f)

    write_pickled_edt(edt, args.edt_pickle_out)


def parse_args() -> argparse.Namespace:
    # Returns parsed command-line arguments

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--dts", required=True, help="DTS file")
    parser.add_argument("--dtc-flags",
                        help="'dtc' devicetree compiler flags, some of which "
                             "might be respected here")
    parser.add_argument("--bindings-dirs", nargs='+', required=True,
                        help="directory with bindings in YAML format, "
                        "we allow multiple")
    parser.add_argument("--dts-out", required=True,
                        help="path to write merged DTS source code to (e.g. "
                             "as a debugging aid)")
    parser.add_argument("--edt-pickle-out",
                        help="path to write pickled edtlib.EDT object to", required=True)
    parser.add_argument("--vendor-prefixes", action='append', default=[],
                        help="vendor-prefixes.txt path; used for validation; "
                             "may be given multiple times")
    parser.add_argument("--edtlib-Werror", action="store_true",
                        help="if set, edtlib-specific warnings become errors. "
                             "(this does not apply to warnings shared "
                             "with dtc.)")

    return parser.parse_args()


def write_pickled_edt(edt: edtlib.EDT, out_file: str) -> None:
    # Writes the edt object in pickle format to out_file.

    with open(out_file, 'wb') as f:
        # Pickle protocol version 4 is the default as of Python 3.8
        # and was introduced in 3.4, so it is both available and
        # recommended on all versions of Python that Zephyr supports
        # (at time of writing, Python 3.6 was Zephyr's minimum
        # version, and 3.10 the most recent CPython release).
        #
        # Using a common protocol version here will hopefully avoid
        # reproducibility issues in different Python installations.
        pickle.dump(edt, f, protocol=4)


def err(s: str) -> NoReturn:
    raise Exception(s)


if __name__ == "__main__":
    main()
