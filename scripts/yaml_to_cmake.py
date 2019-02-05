#!/usr/bin/env python3
#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Tool for a simple parsing of YAML files and return a ;-list of <key>=<value>
pairs to use within a CMake build file.
'''

import argparse
import sys
import yaml
import pykwalify.core


METADATA_SCHEMA = '''\
## A pykwalify schema for basic validation of the structure of a
## metadata YAML file.
##
# The zephyr/module.yml file is a simple list of key value pairs to be used by
# the build system.
type: map
mapping:
  build:
    required: true
    type: map
    mapping:
      cmake:
        required: false
        type: str
      kconfig:
        required: false
        type: str
'''


def main():
    parser = argparse.ArgumentParser(description='''
    Converts YAML to a CMake list''')

    parser.add_argument('-i', '--input', required=True,
                        help='YAML file with data')
    parser.add_argument('-o', '--output', required=True,
                        help='File to write with CMake data')
    parser.add_argument('-s', '--section', required=True,
                        help='Section in YAML file to parse')
    args = parser.parse_args()

    with open(args.input, 'r') as f:
        meta = yaml.safe_load(f.read())

    pykwalify.core.Core(source_data=meta,
                        schema_data=yaml.safe_load(METADATA_SCHEMA)).validate()

    val_str = ''

    section = meta.get(args.section)
    if section is not None:
        for key in section:
            val_str += '{}={}\n'.format(key, section[key])

    with open(args.output, 'w') as f:
        f.write(val_str)


if __name__ == "__main__":
    main()
