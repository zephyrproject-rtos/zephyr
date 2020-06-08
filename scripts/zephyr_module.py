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

Using --sanitycheck-out <filename> an argument file for sanitycheck script will
be generated which would point to test and sample roots available in modules
that can be included during a sanitycheck run. This allows testing code
maintained in modules in addition to what is available in the main Zephyr tree.
'''

import argparse
import os
import sys
import yaml
import pykwalify.core
import subprocess
import re
from pathlib import Path, PurePath

METADATA_SCHEMA = '''
## A pykwalify schema for basic validation of the structure of a
## metadata YAML file.
##
# The zephyr/module.yml file is a simple list of key value pairs to be used by
# the build system.
type: map
mapping:
  build:
    required: false
    type: map
    mapping:
      cmake:
        required: false
        type: str
      kconfig:
        required: false
        type: str
  tests:
    required: false
    type: seq
    sequence:
      - type: str
  samples:
    required: false
    type: seq
    sequence:
      - type: str
  boards:
    required: false
    type: seq
    sequence:
      - type: str
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


def process_module(module):
    module_path = PurePath(module)
    module_yml = module_path.joinpath('zephyr/module.yml')

    # The input is a module if zephyr/module.yml is a valid yaml file
    # or if both zephyr/CMakeLists.txt and zephyr/Kconfig are present.

    if Path(module_yml).is_file():
        with Path(module_yml).open('r') as f:
            meta = yaml.safe_load(f.read())

        try:
            pykwalify.core.Core(source_data=meta, schema_data=schema)\
                .validate()
        except pykwalify.errors.SchemaError as e:
            sys.exit('ERROR: Malformed "build" section in file: {}\n{}'
                     .format(module_yml.as_posix(), e))

        return meta

    if Path(module_path.joinpath('zephyr/CMakeLists.txt')).is_file() and \
       Path(module_path.joinpath('zephyr/Kconfig')).is_file():
        return {'build': {'cmake': 'zephyr', 'kconfig': 'zephyr/Kconfig'}}

    return None


def process_cmake(module, meta):
    section = meta.get('build', dict())
    module_path = PurePath(module)
    module_yml = module_path.joinpath('zephyr/module.yml')
    cmake_setting = section.get('cmake', None)
    if not validate_setting(cmake_setting, module, 'CMakeLists.txt'):
        sys.exit('ERROR: "cmake" key in {} has folder value "{}" which '
                    'does not contain a CMakeLists.txt file.'
                    .format(module_yml.as_posix(), cmake_setting))

    cmake_path = os.path.join(module, cmake_setting or 'zephyr')
    cmake_file = os.path.join(cmake_path, 'CMakeLists.txt')
    if os.path.isfile(cmake_file):
        return('\"{}\":\"{}\"\n'
                        .format(module_path.name, Path(cmake_path).resolve().as_posix()))
    else:
        return ""

def process_kconfig(module, meta):
    section = meta.get('build', dict())
    module_path = PurePath(module)
    module_yml = module_path.joinpath('zephyr/module.yml')

    kconfig_setting = section.get('kconfig', None)
    if not validate_setting(kconfig_setting, module):
        sys.exit('ERROR: "kconfig" key in {} has value "{}" which does '
                    'not point to a valid Kconfig file.'
                    .format(module_yml, kconfig_setting))


    kconfig_file = os.path.join(module, kconfig_setting or 'zephyr/Kconfig')
    if os.path.isfile(kconfig_file):
        return 'osource "{}"\n\n'.format(Path(kconfig_file).resolve().as_posix())
    else:
        return ""

def process_sanitycheck(module, meta):

    out = ""
    tests = meta.get('tests', [])
    samples = meta.get('samples', [])
    boards = meta.get('boards', [])

    for pth in tests + samples:
        if pth:
            dir = os.path.join(module, pth)
            out += '-T\n{}\n'.format(PurePath(os.path.abspath(dir)).as_posix())

    for pth in boards:
        if pth:
            dir = os.path.join(module, pth)
            out += '--board-root\n{}\n'.format(PurePath(os.path.abspath(dir)).as_posix())

    return out


def main():
    parser = argparse.ArgumentParser(description='''
    Process a list of projects and create Kconfig / CMake include files for
    projects which are also a Zephyr module''')

    parser.add_argument('--kconfig-out',
                        help="""File to write with resulting KConfig import
                             statements.""")
    parser.add_argument('--sanitycheck-out',
                        help="""File to write with resulting sanitycheck parameters.""")
    parser.add_argument('--cmake-out',
                        help="""File to write with resulting <name>:<path>
                             values to use for including in CMake""")
    parser.add_argument('-m', '--modules', nargs='+',
                        help="""List of modules to parse instead of using `west
                             list`""")
    parser.add_argument('-x', '--extra-modules', nargs='+', default=[],
                        help='List of extra modules to parse')
    parser.add_argument('-w', '--west-path', default='west',
                        help='Path to west executable')
    parser.add_argument('-z', '--zephyr-base',
                        help='Path to zephyr repository')
    args = parser.parse_args()

    if args.modules is None:
        p = subprocess.Popen([args.west_path, 'list', '--format={posixpath}'],
                             cwd=args.zephyr_base,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        if p.returncode == 0:
            projects = out.decode(sys.getdefaultencoding()).splitlines()
        elif re.match((r'Error: .* is not in a west installation\.'
                       '|FATAL ERROR: no west installation found from .*'
                       '|FATAL ERROR: no west workspace.*'),
                      err.decode(sys.getdefaultencoding())):
            # Only accept the error the event we are outside a west
            # workspace.
            #
            # TODO: we can just use "west topdir" instead if we can
            # depend on west 0.7.0 or later.
            projects = []
        else:
            print(err.decode(sys.getdefaultencoding()))
            # A real error occurred, raise an exception
            raise subprocess.CalledProcessError(cmd=p.args,
                                                returncode=p.returncode)
    else:
        projects = args.modules

    projects += args.extra_modules
    extra_modules = set(args.extra_modules)

    kconfig = ""
    cmake = ""
    sanitycheck = ""

    for project in projects:
        # Avoid including Zephyr base project as module.
        if project == os.environ.get('ZEPHYR_BASE'):
            continue

        meta = process_module(project)
        if meta:
            kconfig += process_kconfig(project, meta)
            cmake += process_cmake(project, meta)
            sanitycheck += process_sanitycheck(project, meta)
        elif project in extra_modules:
            sys.exit(f'{project}, given in ZEPHYR_EXTRA_MODULES, '
                     'is not a valid zephyr module')

    if args.kconfig_out:
        with open(args.kconfig_out, 'w', encoding="utf-8") as fp:
            fp.write(kconfig)

    if args.cmake_out:
        with open(args.cmake_out, 'w', encoding="utf-8") as fp:
            fp.write(cmake)

    if args.sanitycheck_out:
        with open(args.sanitycheck_out, 'w', encoding="utf-8") as fp:
            fp.write(sanitycheck)

if __name__ == "__main__":
    main()
