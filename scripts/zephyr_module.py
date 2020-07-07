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
from pathlib import Path, PurePath
from collections import namedtuple

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
      depends:
        required: false
        type: seq
        sequence:
          - type: str
      settings:
        required: false
        type: map
        mapping:
          board_root:
            required: false
            type: str
          dts_root:
            required: false
            type: str
          soc_root:
            required: false
            type: str
          arch_root:
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

def process_settings(module, meta):
    section = meta.get('build', dict())
    build_settings = section.get('settings', None)
    out_text = ""

    if build_settings is not None:
        for root in ['board', 'dts', 'soc', 'arch']:
            setting = build_settings.get(root+'_root', None)
            if setting is not None:
                root_path = PurePath(module) / setting
                out_text += f'"{root.upper()}_ROOT":"{root_path}"\n'

    return out_text

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
    parser.add_argument('--settings-out',
                        help="""File to write with resulting <name>:<value>
                             values to use for including in CMake""")
    parser.add_argument('-m', '--modules', nargs='+',
                        help="""List of modules to parse instead of using `west
                             list`""")
    parser.add_argument('-x', '--extra-modules', nargs='+', default=[],
                        help='List of extra modules to parse')
    parser.add_argument('-z', '--zephyr-base',
                        help='Path to zephyr repository')
    args = parser.parse_args()

    if args.modules is None:
        # West is imported here, as it is optional (and thus maybe not installed)
        # if user is providing a specific modules list.
        from west.manifest import Manifest
        from west.util import WestNotFound
        try:
            manifest = Manifest.from_file()
            projects = [p.posixpath for p in manifest.get_projects([])]
        except WestNotFound:
            # Only accept WestNotFound, meaning we are not in a west
            # workspace. Such setup is allowed, as west may be installed
            # but the project is not required to use west.
            projects = []
    else:
        projects = args.modules.copy()

    projects += args.extra_modules
    extra_modules = set(args.extra_modules)

    kconfig = ""
    cmake = ""
    settings = ""
    sanitycheck = ""

    Module = namedtuple('Module', ['project', 'meta', 'depends'])
    # dep_modules is a list of all modules that has an unresolved dependency
    dep_modules = []
    # start_modules is a list modules with no depends left (no incoming edge)
    start_modules = []
    # sorted_modules is a topological sorted list of the modules
    sorted_modules = []

    for project in projects:
        # Avoid including Zephyr base project as module.
        if project == args.zephyr_base:
            continue

        meta = process_module(project)
        if meta:
            section = meta.get('build', dict())
            deps = section.get('depends', [])
            if not deps:
                start_modules.append(Module(project, meta, []))
            else:
                dep_modules.append(Module(project, meta, deps))
        elif project in extra_modules:
            sys.exit(f'{project}, given in ZEPHYR_EXTRA_MODULES, '
                     'is not a valid zephyr module')

    # This will do a topological sort to ensure the modules are ordered
    # according to dependency settings.
    while start_modules:
        node = start_modules.pop(0)
        sorted_modules.append(node)
        node_name = PurePath(node.project).name
        to_remove = []
        for module in dep_modules:
            if node_name in module.depends:
                module.depends.remove(node_name)
                if not module.depends:
                    start_modules.append(module)
                    to_remove.append(module)
        for module in to_remove:
            dep_modules.remove(module)

    if dep_modules:
        # If there are any modules with unresolved dependencies, then the
        # modules contains unmet or cyclic dependencies. Error out.
        error = 'Unmet or cyclic dependencies in modules:\n'
        for module in dep_modules:
            error += f'{module.project} depends on: {module.depends}\n'
        sys.exit(error)

    for module in sorted_modules:
        kconfig += process_kconfig(module.project, module.meta)
        cmake += process_cmake(module.project, module.meta)
        settings += process_settings(module.project, module.meta)
        sanitycheck += process_sanitycheck(module.project, module.meta)

    if args.kconfig_out:
        with open(args.kconfig_out, 'w', encoding="utf-8") as fp:
            fp.write(kconfig)

    if args.cmake_out:
        with open(args.cmake_out, 'w', encoding="utf-8") as fp:
            fp.write(cmake)

    if args.settings_out:
        with open(args.settings_out, 'w', encoding="utf-8") as fp:
            fp.write(settings)

    if args.sanitycheck_out:
        with open(args.sanitycheck_out, 'w', encoding="utf-8") as fp:
            fp.write(sanitycheck)

if __name__ == "__main__":
    main()
