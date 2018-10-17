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

x = edts.get_device_by_device_id("/soc/sub_range/serial@1000000")
print("reg %x" % x.get_reg_addr())
print("foo %s" % x.get_property('reg/0'))
print("bar %s" % x.properties_flattened())

# Test 1: 

