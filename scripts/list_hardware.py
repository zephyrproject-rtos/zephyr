#!/usr/bin/env python3

# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
from dataclasses import dataclass
from pathlib import Path, PurePath
import pykwalify.core
import sys
from typing import List
import yaml
import re

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader


SOC_SCHEMA_PATH = str(Path(__file__).parent / 'schemas' / 'soc-schema.yml')
with open(SOC_SCHEMA_PATH, 'r') as f:
    soc_schema = yaml.load(f.read(), Loader=SafeLoader)

ARCH_SCHEMA_PATH = str(Path(__file__).parent / 'schemas' / 'arch-schema.yml')
with open(ARCH_SCHEMA_PATH, 'r') as f:
    arch_schema = yaml.load(f.read(), Loader=SafeLoader)

SOC_YML = 'soc.yml'
ARCHS_YML_PATH = PurePath('arch/archs.yml')

class Systems:

    def __init__(self, folder='', soc_yaml=None):
        self._socs = []
        self._series = []
        self._families = []
        self._extended_socs = []

        if soc_yaml is None:
            return

        try:
            data = yaml.load(soc_yaml, Loader=SafeLoader)
            pykwalify.core.Core(source_data=data,
                                schema_data=soc_schema).validate()
        except (yaml.YAMLError, pykwalify.errors.SchemaError) as e:
            sys.exit(f'ERROR: Malformed yaml {soc_yaml.as_posix()}', e)

        for f in data.get('family', []):
            family = Family(f['name'], [folder], [], [])
            for s in f.get('series', []):
                series = Series(s['name'], [folder], f['name'], [])
                socs = [(Soc(soc['name'],
                             [c['name'] for c in soc.get('cpuclusters', [])],
                             [folder], s['name'], f['name']))
                        for soc in s.get('socs', [])]
                series.socs.extend(socs)
                self._series.append(series)
                self._socs.extend(socs)
                family.series.append(series)
                family.socs.extend(socs)
            socs = [(Soc(soc['name'],
                         [c['name'] for c in soc.get('cpuclusters', [])],
                         [folder], None, f['name']))
                    for soc in f.get('socs', [])]
            self._socs.extend(socs)
            self._families.append(family)

        for s in data.get('series', []):
            series = Series(s['name'], [folder], '', [])
            socs = [(Soc(soc['name'],
                         [c['name'] for c in soc.get('cpuclusters', [])],
                         [folder], s['name'], ''))
                    for soc in s.get('socs', [])]
            series.socs.extend(socs)
            self._series.append(series)
            self._socs.extend(socs)

        for soc in data.get('socs', []):
            mutual_exclusive = {'name', 'extend'}
            if len(mutual_exclusive - soc.keys()) < 1:
                sys.exit(f'ERROR: Malformed content in SoC file: {soc_yaml}\n'
                         f'{mutual_exclusive} are mutual exclusive at this level.')
            if soc.get('name') is not None:
                self._socs.append(Soc(soc['name'], [c['name'] for c in soc.get('cpuclusters', [])],
                                  [folder], '', ''))
            elif soc.get('extend') is not None:
                self._extended_socs.append(Soc(soc['extend'],
                                           [c['name'] for c in soc.get('cpuclusters', [])],
                                           [folder], '', ''))
            else:
                sys.exit(f'ERROR: Malformed "socs" section in SoC file: {soc_yaml}\n'
                         f'Cannot find one of required keys {mutual_exclusive}.')

        # Ensure that any runner configuration matches socs and cpuclusters declared in the same
        # soc.yml file
        if 'runners' in data and 'run_once' in data['runners']:
            for grp in data['runners']['run_once']:
                for item_data in data['runners']['run_once'][grp]:
                    for group in item_data['groups']:
                        for qualifiers in group['qualifiers']:
                            soc_name, *components = qualifiers.split('/')
                            found_match = False

                            # Allow 'ns' as final qualifier until "virtual" CPUs are ported to soc.yml
                            # https://github.com/zephyrproject-rtos/zephyr/issues/70721
                            if components and components[-1] == 'ns':
                                components.pop()

                            for soc in self._socs + self._extended_socs:
                                if re.match(fr'^{soc_name}$', soc.name) is not None:
                                    if soc.cpuclusters and components:
                                        check_string = '/'.join(components)
                                        for cpucluster in soc.cpuclusters:
                                            if re.match(fr'^{check_string}$', cpucluster) is not None:
                                                found_match = True
                                                break
                                    elif not soc.cpuclusters and not components:
                                        found_match = True
                                        break

                            if found_match is False:
                                sys.exit(f'ERROR: SoC qualifier match unresolved: {qualifiers}')

    @staticmethod
    def from_file(socs_file):
        '''Load SoCs from a soc.yml file.
        '''
        try:
            with open(socs_file, 'r') as f:
                socs_yaml = f.read()
        except FileNotFoundError as e:
            sys.exit(f'ERROR: socs.yml file not found: {socs_file.as_posix()}', e)

        return Systems(str(socs_file.parent), socs_yaml)

    @staticmethod
    def from_yaml(socs_yaml):
        '''Load socs from a string with YAML contents.
        '''
        return Systems('', socs_yaml)

    def extend(self, systems):
        self._families.extend(systems.get_families())
        self._series.extend(systems.get_series())

        for es in self._extended_socs[:]:
            for s in systems.get_socs():
                if s.name == es.name:
                    s.extend(es)
                    self._extended_socs.remove(es)
                    break
        self._socs.extend(systems.get_socs())

        for es in systems.get_extended_socs():
            for s in self._socs:
                if s.name == es.name:
                    s.extend(es)
                    break
            else:
                self._extended_socs.append(es)

    def get_families(self):
        return self._families

    def get_series(self):
        return self._series

    def get_socs(self):
        return self._socs

    def get_extended_socs(self):
        return self._extended_socs

    def get_soc(self, name):
        try:
            return next(s for s in self._socs if s.name == name)
        except StopIteration:
            sys.exit(f"ERROR: SoC '{name}' is not found, please ensure that the SoC exists "
                     f"and that soc-root containing '{name}' has been correctly defined.")


@dataclass
class Soc:
    name: str
    cpuclusters: List[str]
    folder: List[str]
    series: str = ''
    family: str = ''

    def extend(self, soc):
        if self.name == soc.name:
            self.cpuclusters.extend(soc.cpuclusters)
            self.folder.extend(soc.folder)


@dataclass
class Series:
    name: str
    folder: List[str]
    family: str
    socs: List[Soc]


@dataclass
class Family:
    name: str
    folder: List[str]
    series: List[Series]
    socs: List[Soc]


def unique_paths(paths):
    # Using dict keys ensures both uniqueness and a deterministic order.
    yield from dict.fromkeys(map(Path.resolve, paths)).keys()


def find_v2_archs(args):
    ret = {'archs': []}
    for root in unique_paths(args.arch_roots):
        archs_yml = root / ARCHS_YML_PATH

        if Path(archs_yml).is_file():
            with Path(archs_yml).open('r') as f:
                archs = yaml.load(f.read(), Loader=SafeLoader)

            try:
                pykwalify.core.Core(source_data=archs, schema_data=arch_schema).validate()
            except pykwalify.errors.SchemaError as e:
                sys.exit('ERROR: Malformed "build" section in file: {}\n{}'
                         .format(archs_yml.as_posix(), e))

            if args.arch is not None:
                archs = {'archs': list(filter(
                    lambda arch: arch.get('name') == args.arch, archs['archs']))}
            for arch in archs['archs']:
                arch.update({'path': root / 'arch' / arch['path']})
                arch.update({'hwm': 'v2'})
                arch.update({'type': 'arch'})

            ret['archs'].extend(archs['archs'])

    return ret


def find_v2_systems(args):
    yml_files = []
    systems = Systems()
    for root in unique_paths(args.soc_roots):
        yml_files.extend(sorted((root / 'soc').rglob(SOC_YML)))

    for soc_yml in yml_files:
        if soc_yml.is_file():
            systems.extend(Systems.from_file(soc_yml))

    return systems


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    add_args(parser)
    return parser.parse_args()


def add_args(parser):
    default_fmt = '{name}'

    parser.add_argument("--soc-root", dest='soc_roots', default=[],
                        type=Path, action='append',
                        help='add a SoC root, may be given more than once')
    parser.add_argument("--soc", default=None, help='lookup the specific soc')
    parser.add_argument("--soc-series", default=None, help='lookup the specific soc series')
    parser.add_argument("--soc-family", default=None, help='lookup the specific family')
    parser.add_argument("--socs", action='store_true', help='lookup all socs')
    parser.add_argument("--arch-root", dest='arch_roots', default=[],
                        type=Path, action='append',
                        help='add a arch root, may be given more than once')
    parser.add_argument("--arch", default=None, help='lookup the specific arch')
    parser.add_argument("--archs", action='store_true', help='lookup all archs')
    parser.add_argument("--format", default=default_fmt,
                        help='''Format string to use to list each soc.''')
    parser.add_argument("--cmakeformat", default=None,
                        help='''CMake format string to use to list each arch/soc.''')


def dump_v2_archs(args):
    archs = find_v2_archs(args)

    for arch in archs['archs']:
        if args.cmakeformat is not None:
            info = args.cmakeformat.format(
                TYPE='TYPE;' + arch['type'],
                NAME='NAME;' + arch['name'],
                DIR='DIR;' + str(arch['path'].as_posix()),
                HWM='HWM;' + arch['hwm'],
                # Below is non exising for arch but is defined here to support
                # common formatting string.
                SERIES='',
                FAMILY='',
                ARCH='',
                VENDOR=''
            )
        else:
            info = args.format.format(
                type=arch.get('type'),
                name=arch.get('name'),
                dir=arch.get('path'),
                hwm=arch.get('hwm'),
                # Below is non exising for arch but is defined here to support
                # common formatting string.
                series='',
                family='',
                arch='',
                vendor=''
            )

        print(info)


def dump_v2_system(args, type, system):
    if args.cmakeformat is not None:
        info = args.cmakeformat.format(
           TYPE='TYPE;' + type,
           NAME='NAME;' + system.name,
           DIR='DIR;' + ';'.join([Path(x).as_posix() for x in system.folder]),
           HWM='HWM;' + 'v2'
        )
    else:
        info = args.format.format(
           type=type,
           name=system.name,
           dir=system.folder,
           hwm='v2'
        )

    print(info)


def dump_v2_systems(args):
    systems = find_v2_systems(args)

    for f in systems.get_families():
        dump_v2_system(args, 'family', f)

    for s in systems.get_series():
        dump_v2_system(args, 'series', s)

    for s in systems.get_socs():
        dump_v2_system(args, 'soc', s)


if __name__ == '__main__':
    args = parse_args()
    if any([args.socs, args.soc, args.soc_series, args.soc_family]):
        dump_v2_systems(args)
    if args.archs or args.arch is not None:
        dump_v2_archs(args)
