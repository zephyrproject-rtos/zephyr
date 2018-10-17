#!/usr/bin/env python3
#
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import argparse
import sys
import pprint
import logging
from pathlib import Path
from copy import deepcopy
from os import path

if "ZEPHYR_BASE" not in os.environ:
    logging.error("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)
else:
    zephyr_base = os.environ.get('ZEPHYR_BASE')

sys.path.append(os.path.join(zephyr_base,'scripts'))
sys.path.append(os.path.join(zephyr_base,'scripts/dts'))

from dts.edtsdatabase import EDTSDatabase

rdh = argparse.RawDescriptionHelpFormatter
parser = argparse.ArgumentParser(description=__doc__, formatter_class=rdh)

parser.add_argument("-e", "--edts", nargs=1, required=True,
                    help="edts json file")

args =  parser.parse_args()
edts_file = Path(args.edts[0])
if not edts_file.is_file():
    raise self._get_error_exception(
        "Generated extended device tree database file '{}' not found/ no access.".
        format(edts_file), 2)
edts = EDTSDatabase()
edts.load(str(edts_file))

# Test: get_device_by_device_id - get an id that doesn't exit
d = edts.get_device_by_device_id("/foobar")
if d is not None:
    print("Error: get_device_by_device_id expected None, got %s" % d)
    sys.exit(1)

# Test: get_device_by_device_id - get an id that exists
d = edts.get_device_by_device_id("/soc/serial@41000000")
if d is None:
    print("Error: get_device_by_device_id got None, expect a device")
    sys.exit(1)

# Test: get_device_ids_by_compatible - get a compat that doesn't exist
d = edts.get_device_ids_by_compatible("foobar")
if d:
    print("Error: get_device_ids_by_compatible expected None, got %s" % d)
    sys.exit(1)

# Test: get_device_ids_by_compatible - get a compat that exists
d = edts.get_device_ids_by_compatible("refsoc,uart")
if d is None:
    print("Error: get_device_ids_by_compatible got None, expect a device")
    sys.exit(1)

# all tests passed
print("OK")
