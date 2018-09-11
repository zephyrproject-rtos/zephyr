#!/usr/bin/env python3
#
# Copyright (c) 2017, Linaro Limited
# Copyright (c) 2018, Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# vim: ai:ts=4:sw=4

import sys
import pprint
from devicetree import parse_file

def convert_string_to_label(s):
    # Transmute ,-@/ to _
    s = s.replace("-", "_")
    s = s.replace(",", "_")
    s = s.replace("@", "_")
    s = s.replace("/", "_")
    # Uppercase the string
    s = s.upper()
    return s

def main(args):
    if len(args) == 1:
        print('Usage: %s filename.dts' % args[0])
        return 1

    with open(args[1], "r") as fd:
        dts = parse_file(fd)
        root_compat = dts['/']['props']['compatible']
        soc_compat = dts['/']['children']['soc']['props']['compatible']

        if not isinstance(soc_compat, list):
            soc_compat = [soc_compat]

        if not isinstance(root_compat, list):
            root_compat = [root_compat]

        for k in root_compat:
            if k != 'simple-bus':
                sys.stdout.write('-D%s ' % convert_string_to_label(k))

        for k in soc_compat:
            if k != 'simple-bus':
                sys.stdout.write('-DSOC_%s ' % convert_string_to_label(k))

        sys.stdout.write('\n')
        sys.stdout.flush()

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
