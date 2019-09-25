#!/usr/bin/env python3
#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Tool for parsing a list of projects to determine if they are Zephyr
projects. If no projects are given then the output from `west list` will be
used as project list.

Include file is generated for Kconfig using --kconfig-out.
A <name>:<path> text file is generated for use with CMake using --cmake-out.
'''

import argparse
import os
import sys
import yaml
import pykwalify.core
import subprocess
import re
from pathlib import PurePath

METADATA_SCHEMA = '''
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

schema = yaml.safe_load(METADATA_SCHEMA)


def validate_setting(setting, module_path, filename=None):
    if setting is not None:
        if filename is not None:
            checkfile = os.path.join(module_path, setting, filename)
        else:
            checkfile = os.path.join(module_path, setting)
        if not os.path.isfile(checkfile):
            return False
    return True


def process_module(module, cmake_out=None, kconfig_out=None):
    cmake_setting = None
    kconfig_setting = None

    module_yml = os.path.join(module, 'zephyr/module.yml')
    if os.path.isfile(module_yml):
        with open(module_yml, 'r') as f:
            meta = yaml.safe_load(f.read())

        try:
            pykwalify.core.Core(source_data=meta, schema_data=schema)\
                .validate()
        except pykwalify.errors.SchemaError as e:
            sys.exit('ERROR: Malformed "build" section in file: {}\n{}'
                     .format(module_yml, e))

        section = meta.get('build', dict())
        cmake_setting = section.get('cmake', None)
        if not validate_setting(cmake_setting, module, 'CMakeLists.txt'):
            sys.exit('ERROR: "cmake" key in {} has folder value "{}" which '
                     'does not contain a CMakeLists.txt file.'
                     .format(module_yml, cmake_setting))

        kconfig_setting = section.get('kconfig', None)
        if not validate_setting(kconfig_setting, module):
            sys.exit('ERROR: "kconfig" key in {} has value "{}" which does '
                     'not point to a valid Kconfig file.'
                     .format(module.yml, kconfig_setting))

    cmake_path = os.path.join(module, cmake_setting or 'zephyr')
    cmake_file = os.path.join(cmake_path, 'CMakeLists.txt')
    if os.path.isfile(cmake_file) and cmake_out is not None:
        cmake_out.write('\"{}\":\"{}\"\n'.format(os.path.basename(module),
                                                 os.path.abspath(cmake_path)))

    kconfig_file = os.path.join(module, kconfig_setting or 'zephyr/Kconfig')
    if os.path.isfile(kconfig_file) and kconfig_out is not None:
        kconfig_out.write('osource "{}"\n\n'
                          .format(PurePath(
                              os.path.abspath(kconfig_file)).as_posix()))


def main():
    kconfig_out_file = None
    cmake_out_file = None

    parser = argparse.ArgumentParser(description='''
    Process a list of projects and create Kconfig / CMake include files for
    projects which are also a Zephyr module''')

    parser.add_argument('--kconfig-out',
                        help='File to write with resulting KConfig import'
                             'statements.')
    parser.add_argument('--cmake-out',
                        help='File to write with resulting <name>:<path>'
                             'values to use for including in CMake')
    parser.add_argument('-m', '--modules', nargs='+',
                        help='List of modules to parse instead of using `west'
                             'list`')
    parser.add_argument('-x', '--extra-modules', nargs='+',
                        help='List of extra modules to parse')
    parser.add_argument('-w', '--west-path', default='west',
                        help='Path to west executable')
    args = parser.parse_args()

    if args.modules is None:
        p = subprocess.Popen([args.west_path, 'list', '--format={posixpath}'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        if p.returncode == 0:
            projects = out.decode(sys.getdefaultencoding()).splitlines()
        elif re.match((r'Error: .* is not in a west installation\.'
                        '|FATAL ERROR: no west installation found from .*'),
                      err.decode(sys.getdefaultencoding())):
            # Only accept the error from bootstrapper in the event we are
            # outside a west managed project.
            projects = []
        else:
            print(err.decode(sys.getdefaultencoding()))
            # A real error occurred, raise an exception
            raise subprocess.CalledProcessError(cmd=p.args,
                                                returncode=p.returncode)
    else:
        projects = args.modules

    if args.extra_modules is not None:
        projects += args.extra_modules

    if args.kconfig_out:
        kconfig_out_file = open(args.kconfig_out, 'w', encoding="utf-8")

    if args.cmake_out:
        cmake_out_file = open(args.cmake_out, 'w', encoding="utf-8")

    try:
        for project in projects:
            # Avoid including Zephyr base project as module.
            if project != os.environ.get('ZEPHYR_BASE'):
                process_module(project, cmake_out_file, kconfig_out_file)
    finally:
        if args.kconfig_out:
            kconfig_out_file.close()
        if args.cmake_out:
            cmake_out_file.close()


if __name__ == "__main__":
    main()
