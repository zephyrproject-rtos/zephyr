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

Using --twister-out <filename> an argument file for twister script will
be generated which would point to test and sample roots available in modules
that can be included during a twister run. This allows testing code
maintained in modules in addition to what is available in the main Zephyr tree.
'''

import argparse
import hashlib
import os
import re
import subprocess
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
  name:
    required: false
    type: str
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
      cmake-ext:
        required: false
        type: bool
        default: false
      kconfig-ext:
        required: false
        type: bool
        default: false
      sysbuild-cmake:
        required: false
        type: str
      sysbuild-kconfig:
        required: false
        type: str
      sysbuild-cmake-ext:
        required: false
        type: bool
        default: false
      sysbuild-kconfig-ext:
        required: false
        type: bool
        default: false
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
          snippet_root:
            required: false
            type: str
          soc_root:
            required: false
            type: str
          arch_root:
            required: false
            type: str
          module_ext_root:
            required: false
            type: str
          sca_root:
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
  blobs:
    required: false
    type: seq
    sequence:
      - type: map
        mapping:
          path:
            required: true
            type: str
          sha256:
            required: true
            type: str
          type:
            required: true
            type: str
            enum: ['img', 'lib']
          version:
            required: true
            type: str
          license-path:
            required: true
            type: str
          url:
            required: true
            type: str
          description:
            required: true
            type: str
          doc-url:
            required: false
            type: str
'''

MODULE_YML_PATH = PurePath('zephyr/module.yml')
# Path to the blobs folder
MODULE_BLOBS_PATH = PurePath('zephyr/blobs')
BLOB_PRESENT = 'A'
BLOB_NOT_PRESENT = 'D'
BLOB_OUTDATED = 'M'

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

    # The input is a module if zephyr/module.{yml,yaml} is a valid yaml file
    # or if both zephyr/CMakeLists.txt and zephyr/Kconfig are present.

    for module_yml in [module_path / MODULE_YML_PATH,
                       module_path / MODULE_YML_PATH.with_suffix('.yaml')]:
        if Path(module_yml).is_file():
            with Path(module_yml).open('r') as f:
                meta = yaml.safe_load(f.read())

            try:
                pykwalify.core.Core(source_data=meta, schema_data=schema)\
                    .validate()
            except pykwalify.errors.SchemaError as e:
                sys.exit('ERROR: Malformed "build" section in file: {}\n{}'
                        .format(module_yml.as_posix(), e))

            meta['name'] = meta.get('name', module_path.name)
            meta['name-sanitized'] = re.sub('[^a-zA-Z0-9]', '_', meta['name'])
            return meta

    if Path(module_path.joinpath('zephyr/CMakeLists.txt')).is_file() and \
       Path(module_path.joinpath('zephyr/Kconfig')).is_file():
        return {'name': module_path.name,
                'name-sanitized': re.sub('[^a-zA-Z0-9]', '_', module_path.name),
                'build': {'cmake': 'zephyr', 'kconfig': 'zephyr/Kconfig'}}

    return None


def process_cmake(module, meta):
    section = meta.get('build', dict())
    module_path = PurePath(module)
    module_yml = module_path.joinpath('zephyr/module.yml')

    cmake_extern = section.get('cmake-ext', False)
    if cmake_extern:
        return('\"{}\":\"{}\":\"{}\"\n'
               .format(meta['name'],
                       module_path.as_posix(),
                       "${ZEPHYR_" + meta['name-sanitized'].upper() + "_CMAKE_DIR}"))

    cmake_setting = section.get('cmake', None)
    if not validate_setting(cmake_setting, module, 'CMakeLists.txt'):
        sys.exit('ERROR: "cmake" key in {} has folder value "{}" which '
                 'does not contain a CMakeLists.txt file.'
                 .format(module_yml.as_posix(), cmake_setting))

    cmake_path = os.path.join(module, cmake_setting or 'zephyr')
    cmake_file = os.path.join(cmake_path, 'CMakeLists.txt')
    if os.path.isfile(cmake_file):
        return('\"{}\":\"{}\":\"{}\"\n'
               .format(meta['name'],
                       module_path.as_posix(),
                       Path(cmake_path).resolve().as_posix()))
    else:
        return('\"{}\":\"{}\":\"\"\n'
               .format(meta['name'],
                       module_path.as_posix()))


def process_sysbuildcmake(module, meta):
    section = meta.get('build', dict())
    module_path = PurePath(module)
    module_yml = module_path.joinpath('zephyr/module.yml')

    cmake_extern = section.get('sysbuild-cmake-ext', False)
    if cmake_extern:
        return('\"{}\":\"{}\":\"{}\"\n'
               .format(meta['name'],
                       module_path.as_posix(),
                       "${SYSBUILD_" + meta['name-sanitized'].upper() + "_CMAKE_DIR}"))

    cmake_setting = section.get('sysbuild-cmake', None)
    if not validate_setting(cmake_setting, module, 'CMakeLists.txt'):
        sys.exit('ERROR: "cmake" key in {} has folder value "{}" which '
                 'does not contain a CMakeLists.txt file.'
                 .format(module_yml.as_posix(), cmake_setting))

    if cmake_setting is None:
        return ""

    cmake_path = os.path.join(module, cmake_setting or 'zephyr')
    cmake_file = os.path.join(cmake_path, 'CMakeLists.txt')
    if os.path.isfile(cmake_file):
        return('\"{}\":\"{}\":\"{}\"\n'
               .format(meta['name'],
                       module_path.as_posix(),
                       Path(cmake_path).resolve().as_posix()))
    else:
        return('\"{}\":\"{}\":\"\"\n'
               .format(meta['name'],
                       module_path.as_posix()))


def process_settings(module, meta):
    section = meta.get('build', dict())
    build_settings = section.get('settings', None)
    out_text = ""

    if build_settings is not None:
        for root in ['board', 'dts', 'snippet', 'soc', 'arch', 'module_ext', 'sca']:
            setting = build_settings.get(root+'_root', None)
            if setting is not None:
                root_path = PurePath(module) / setting
                out_text += f'"{root.upper()}_ROOT":'
                out_text += f'"{root_path.as_posix()}"\n'

    return out_text


def get_blob_status(path, sha256):
    if not path.is_file():
        return BLOB_NOT_PRESENT
    with path.open('rb') as f:
        m = hashlib.sha256()
        m.update(f.read())
        if sha256.lower() == m.hexdigest():
            return BLOB_PRESENT
        else:
            return BLOB_OUTDATED


def process_blobs(module, meta):
    blobs = []
    mblobs = meta.get('blobs', None)
    if not mblobs:
        return blobs

    blobs_path = Path(module) / MODULE_BLOBS_PATH
    for blob in mblobs:
        blob['module'] = meta.get('name', None)
        blob['abspath'] = blobs_path / Path(blob['path'])
        blob['status'] = get_blob_status(blob['abspath'], blob['sha256'])
        blobs.append(blob)

    return blobs


def kconfig_snippet(meta, path, kconfig_file=None, blobs=False, sysbuild=False):
    name = meta['name']
    name_sanitized = meta['name-sanitized']

    snippet = [f'menu "{name} ({path.as_posix()})"',
               f'osource "{kconfig_file.resolve().as_posix()}"' if kconfig_file
               else f'osource "$(SYSBUILD_{name_sanitized.upper()}_KCONFIG)"' if sysbuild is True
	       else f'osource "$(ZEPHYR_{name_sanitized.upper()}_KCONFIG)"',
               f'config ZEPHYR_{name_sanitized.upper()}_MODULE',
               '	bool',
               '	default y',
               'endmenu\n']

    if blobs:
        snippet.insert(-1, '	select TAINT_BLOBS')
    return '\n'.join(snippet)


def process_kconfig(module, meta):
    blobs = process_blobs(module, meta)
    taint_blobs = len(tuple(filter(lambda b: b['status'] != 'D', blobs))) != 0
    section = meta.get('build', dict())
    module_path = PurePath(module)
    module_yml = module_path.joinpath('zephyr/module.yml')
    kconfig_extern = section.get('kconfig-ext', False)
    if kconfig_extern:
        return kconfig_snippet(meta, module_path, blobs=taint_blobs)

    kconfig_setting = section.get('kconfig', None)
    if not validate_setting(kconfig_setting, module):
        sys.exit('ERROR: "kconfig" key in {} has value "{}" which does '
                 'not point to a valid Kconfig file.'
                 .format(module_yml, kconfig_setting))

    kconfig_file = os.path.join(module, kconfig_setting or 'zephyr/Kconfig')
    if os.path.isfile(kconfig_file):
        return kconfig_snippet(meta, module_path, Path(kconfig_file),
                               blobs=taint_blobs)
    else:
        return ""


def process_sysbuildkconfig(module, meta):
    section = meta.get('build', dict())
    module_path = PurePath(module)
    module_yml = module_path.joinpath('zephyr/module.yml')
    kconfig_extern = section.get('sysbuild-kconfig-ext', False)
    if kconfig_extern:
        return kconfig_snippet(meta, module_path, sysbuild=True)

    kconfig_setting = section.get('sysbuild-kconfig', None)
    if not validate_setting(kconfig_setting, module):
        sys.exit('ERROR: "kconfig" key in {} has value "{}" which does '
                 'not point to a valid Kconfig file.'
                 .format(module_yml, kconfig_setting))

    if kconfig_setting is None:
        return ""

    kconfig_file = os.path.join(module, kconfig_setting)
    if os.path.isfile(kconfig_file):
        return kconfig_snippet(meta, module_path, Path(kconfig_file))
    else:
        return ""


def process_twister(module, meta):

    out = ""
    tests = meta.get('tests', [])
    samples = meta.get('samples', [])
    boards = meta.get('boards', [])

    for pth in tests + samples:
        if pth:
            dir = os.path.join(module, pth)
            out += '-T\n{}\n'.format(PurePath(os.path.abspath(dir))
                                     .as_posix())

    for pth in boards:
        if pth:
            dir = os.path.join(module, pth)
            out += '--board-root\n{}\n'.format(PurePath(os.path.abspath(dir))
                                               .as_posix())

    return out


def process_meta(zephyr_base, west_projs, modules, extra_modules=None,
                 propagate_state=False):
    # Process zephyr_base, projects, and modules and create a dictionary
    # with meta information for each input.
    #
    # The dictionary will contain meta info in the following lists:
    # - zephyr:        path and revision
    # - modules:       name, path, and revision
    # - west-projects: path and revision
    #
    # returns the dictionary with said lists

    meta = {'zephyr': None, 'modules': None, 'workspace': None}

    workspace_dirty = False
    workspace_extra = extra_modules is not None
    workspace_off = False

    def git_revision(path):
        rc = subprocess.Popen(['git', 'rev-parse', '--is-inside-work-tree'],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              cwd=path).wait()
        if rc == 0:
            # A git repo.
            popen = subprocess.Popen(['git', 'rev-parse', 'HEAD'],
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE,
                                     cwd=path)
            stdout, stderr = popen.communicate()
            stdout = stdout.decode('utf-8')

            if not (popen.returncode or stderr):
                revision = stdout.rstrip()

                rc = subprocess.Popen(['git', 'diff-index', '--quiet', 'HEAD',
                                       '--'],
                                      stdout=None,
                                      stderr=None,
                                      cwd=path).wait()
                if rc:
                    return revision + '-dirty', True
                return revision, False
        return None, False

    zephyr_revision, zephyr_dirty = git_revision(zephyr_base)
    zephyr_project = {'path': zephyr_base,
                      'revision': zephyr_revision}
    meta['zephyr'] = zephyr_project
    meta['workspace'] = {}
    workspace_dirty |= zephyr_dirty

    if west_projs is not None:
        from west.manifest import MANIFEST_REV_BRANCH
        projects = west_projs['projects']
        meta_projects = []

        # Special treatment of manifest project.
        manifest_proj_path = PurePath(projects[0].posixpath).as_posix()
        manifest_revision, manifest_dirty = git_revision(manifest_proj_path)
        workspace_dirty |= manifest_dirty
        manifest_project = {'path': manifest_proj_path,
                            'revision': manifest_revision}
        meta_projects.append(manifest_project)

        for project in projects[1:]:
            project_path = PurePath(project.posixpath).as_posix()
            revision, dirty = git_revision(project_path)
            workspace_dirty |= dirty
            if project.sha(MANIFEST_REV_BRANCH) != revision:
                revision += '-off'
                workspace_off = True
            meta_project = {'path': project_path,
                            'revision': revision}
            meta_projects.append(meta_project)

        meta.update({'west': {'manifest': west_projs['manifest_path'],
                              'projects': meta_projects}})
        meta['workspace'].update({'off': workspace_off})

    meta_projects = []
    for module in modules:
        module_path = PurePath(module.project).as_posix()
        revision, dirty = git_revision(module_path)
        workspace_dirty |= dirty
        meta_project = {'name': module.meta['name'],
                        'path': module_path,
                        'revision': revision}
        meta_projects.append(meta_project)
    meta['modules'] = meta_projects

    meta['workspace'].update({'dirty': workspace_dirty,
                              'extra': workspace_extra})

    if propagate_state:
        if workspace_dirty and not zephyr_dirty:
            zephyr_revision += '-dirty'
        if workspace_extra:
            zephyr_revision += '-extra'
        if workspace_off:
            zephyr_revision += '-off'
        zephyr_project.update({'revision': zephyr_revision})

        if west_projs is not None:
            if workspace_dirty and not manifest_dirty:
                manifest_revision += '-dirty'
            if workspace_extra:
                manifest_revision += '-extra'
            if workspace_off:
                manifest_revision += '-off'
            manifest_project.update({'revision': manifest_revision})

    return meta


def west_projects(manifest = None):
    manifest_path = None
    projects = []
    # West is imported here, as it is optional
    # (and thus maybe not installed)
    # if user is providing a specific modules list.
    try:
        from west.manifest import Manifest
        from west.util import WestNotFound
        from west.version import __version__ as WestVersion
    except ImportError:
        # West is not installed, so don't return any projects.
        return None

    from packaging import version
    try:
        if not manifest:
            manifest = Manifest.from_file()
        if version.parse(WestVersion) >= version.parse('0.9.0'):
            projects = [p for p in manifest.get_projects([])
                        if manifest.is_active(p)]
        else:
            projects = manifest.get_projects([])
        manifest_path = manifest.path
        return {'manifest_path': manifest_path, 'projects': projects}
    except WestNotFound:
        # Only accept WestNotFound, meaning we are not in a west
        # workspace. Such setup is allowed, as west may be installed
        # but the project is not required to use west.
        pass
    return None


def parse_modules(zephyr_base, manifest=None, west_projs=None, modules=None,
                  extra_modules=None):

    if modules is None:
        west_projs = west_projs or west_projects(manifest)
        modules = ([p.posixpath for p in west_projs['projects']]
                   if west_projs else [])

    if extra_modules is None:
        extra_modules = []

    Module = namedtuple('Module', ['project', 'meta', 'depends'])

    all_modules_by_name = {}
    # dep_modules is a list of all modules that has an unresolved dependency
    dep_modules = []
    # start_modules is a list modules with no depends left (no incoming edge)
    start_modules = []
    # sorted_modules is a topological sorted list of the modules
    sorted_modules = []

    for project in modules + extra_modules:
        # Avoid including Zephyr base project as module.
        if project == zephyr_base:
            continue

        meta = process_module(project)
        if meta:
            depends = meta.get('build', {}).get('depends', [])
            all_modules_by_name[meta['name']] = Module(project, meta, depends)

        elif project in extra_modules:
            sys.exit(f'{project}, given in ZEPHYR_EXTRA_MODULES, '
                     'is not a valid zephyr module')

    for module in all_modules_by_name.values():
        if not module.depends:
            start_modules.append(module)
        else:
            dep_modules.append(module)

    # This will do a topological sort to ensure the modules are ordered
    # according to dependency settings.
    while start_modules:
        node = start_modules.pop(0)
        sorted_modules.append(node)
        node_name = node.meta['name']
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

    return sorted_modules


def main():
    parser = argparse.ArgumentParser(description='''
    Process a list of projects and create Kconfig / CMake include files for
    projects which are also a Zephyr module''', allow_abbrev=False)

    parser.add_argument('--kconfig-out',
                        help="""File to write with resulting KConfig import
                             statements.""")
    parser.add_argument('--twister-out',
                        help="""File to write with resulting twister
                             parameters.""")
    parser.add_argument('--cmake-out',
                        help="""File to write with resulting <name>:<path>
                             values to use for including in CMake""")
    parser.add_argument('--sysbuild-kconfig-out',
                        help="""File to write with resulting KConfig import
                             statements.""")
    parser.add_argument('--sysbuild-cmake-out',
                        help="""File to write with resulting <name>:<path>
                             values to use for including in CMake""")
    parser.add_argument('--meta-out',
                        help="""Write a build meta YaML file containing a list
                             of Zephyr modules and west projects.
                             If a module or project is also a git repository
                             the current SHA revision will also be written.""")
    parser.add_argument('--meta-state-propagate', action='store_true',
                        help="""Propagate state of modules and west projects
                             to the suffix of the Zephyr SHA and if west is
                             used, to the suffix of the manifest SHA""")
    parser.add_argument('--settings-out',
                        help="""File to write with resulting <name>:<value>
                             values to use for including in CMake""")
    parser.add_argument('-m', '--modules', nargs='+',
                        help="""List of modules to parse instead of using `west
                             list`""")
    parser.add_argument('-x', '--extra-modules', nargs='+',
                        help='List of extra modules to parse')
    parser.add_argument('-z', '--zephyr-base',
                        help='Path to zephyr repository')
    args = parser.parse_args()

    kconfig = ""
    cmake = ""
    sysbuild_kconfig = ""
    sysbuild_cmake = ""
    settings = ""
    twister = ""

    west_projs = west_projects()
    modules = parse_modules(args.zephyr_base, None, west_projs,
                            args.modules, args.extra_modules)

    for module in modules:
        kconfig += process_kconfig(module.project, module.meta)
        cmake += process_cmake(module.project, module.meta)
        sysbuild_kconfig += process_sysbuildkconfig(module.project, module.meta)
        sysbuild_cmake += process_sysbuildcmake(module.project, module.meta)
        settings += process_settings(module.project, module.meta)
        twister += process_twister(module.project, module.meta)

    if args.kconfig_out:
        with open(args.kconfig_out, 'w', encoding="utf-8") as fp:
            fp.write(kconfig)

    if args.cmake_out:
        with open(args.cmake_out, 'w', encoding="utf-8") as fp:
            fp.write(cmake)

    if args.sysbuild_kconfig_out:
        with open(args.sysbuild_kconfig_out, 'w', encoding="utf-8") as fp:
            fp.write(sysbuild_kconfig)

    if args.sysbuild_cmake_out:
        with open(args.sysbuild_cmake_out, 'w', encoding="utf-8") as fp:
            fp.write(sysbuild_cmake)

    if args.settings_out:
        with open(args.settings_out, 'w', encoding="utf-8") as fp:
            fp.write('''\
# WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY!
#
# This file contains build system settings derived from your modules.
#
# Modules may be set via ZEPHYR_MODULES, ZEPHYR_EXTRA_MODULES,
# and/or the west manifest file.
#
# See the Modules guide for more information.
''')
            fp.write(settings)

    if args.twister_out:
        with open(args.twister_out, 'w', encoding="utf-8") as fp:
            fp.write(twister)

    if args.meta_out:
        meta = process_meta(args.zephyr_base, west_projs, modules,
                            args.extra_modules, args.meta_state_propagate)

        with open(args.meta_out, 'w', encoding="utf-8") as fp:
            fp.write(yaml.dump(meta))


if __name__ == "__main__":
    main()
