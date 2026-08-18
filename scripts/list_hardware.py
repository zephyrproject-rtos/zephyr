#!/usr/bin/env python3

# Copyright (c) 2023-2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path, PurePath

import jsonschema
import yaml
from jsonschema.exceptions import best_match

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader


SOC_SCHEMA_PATH = str(Path(__file__).parent / 'schemas' / 'soc-schema.yaml')
with open(SOC_SCHEMA_PATH) as f:
    soc_schema = yaml.load(f.read(), Loader=SafeLoader)

ARCH_SCHEMA_PATH = str(Path(__file__).parent / 'schemas' / 'arch-schema.yaml')
with open(ARCH_SCHEMA_PATH) as f:
    arch_schema = yaml.load(f.read(), Loader=SafeLoader)

validator_class = jsonschema.validators.validator_for(soc_schema)
validator_class.check_schema(soc_schema)
soc_validator = validator_class(soc_schema)

validator_class = jsonschema.validators.validator_for(arch_schema)
validator_class.check_schema(arch_schema)
arch_validator = validator_class(arch_schema)

SOC_YML = 'soc.yml'
ARCHS_YML_PATH = PurePath('arch/archs.yml')

class Systems:

    def __init__(self, folder='', soc_yaml=None):
        self._socs = []
        self._series = []
        self._families = []
        self._extended_socs = []
        self._based_socs = {}

        if soc_yaml is None:
            return

        data = yaml.load(soc_yaml, Loader=SafeLoader)
        errors = list(soc_validator.iter_errors(data))
        if errors:
            sys.exit('ERROR: Malformed soc YAML file: \n'
                        f'{soc_yaml}\n'
                        f'{best_match(errors).message} in {best_match(errors).json_path}')

        for f in data.get('family', []):
            family = Family(f['name'], [folder], [], [])
            for s in f.get('series', []):
                series = Series(s['name'], [folder], f['name'], [])
                socs = self._make_socs(s.get('socs', []), folder, s['name'], f['name'])
                series.socs.extend(socs)
                self._series.append(series)
                self._socs.extend(socs)
                family.series.append(series)
                family.socs.extend(socs)
            socs = self._make_socs(f.get('socs', []), folder, None, f['name'])
            self._socs.extend(socs)
            self._families.append(family)

        for s in data.get('series', []):
            series = Series(s['name'], [folder], '', [])
            socs = self._make_socs(s.get('socs', []), folder, s['name'], '')
            series.socs.extend(socs)
            self._series.append(series)
            self._socs.extend(socs)

        for soc in data.get('socs', []):
            if soc.get('base') is not None:
                # A SoC based on another one adds no configuration of its own, so it is recorded
                # as a pointer to the SoC it is based on rather than as a SoC in its own right.
                self._based_socs[soc['name']] = BasedSoc(soc['name'], soc['base'], folder)
            elif soc.get('name') is not None:
                self._socs.append(Soc(soc['name'], [c['name'] for c in soc.get('cpuclusters', [])],
                                  [folder], '', ''))
            elif soc.get('extend') is not None:
                self._extended_socs.append(Soc(soc['extend'],
                                           [c['name'] for c in soc.get('cpuclusters', [])],
                                           [folder], '', ''))
            else:
                # This should not happen if schema validation passed
                sys.exit(f'ERROR: Malformed "socs" section in SoC file: {soc_yaml}\n'
                         f'SoC entry must have either "name" or "extend" property.')

        # Ensure that any runner configuration matches socs and cpuclusters declared in the same
        # soc.yml file
        if 'runners' in data and 'run_once' in data['runners']:
            for grp in data['runners']['run_once']:
                for item_data in data['runners']['run_once'][grp]:
                    for group in item_data['groups']:
                        for qualifiers in group['qualifiers']:
                            soc_name = qualifiers.split('/')[0]
                            found_match = False

                            for soc in self._socs + self._extended_socs:
                                if re.match(fr'^{soc_name}$', soc.name) is not None:
                                    found_match = True
                                    break
                            else:
                                # A SoC declared with 'base' is a valid board qualifier too.
                                found_match = any(re.match(fr'^{soc_name}$', name) is not None
                                                  for name in self._based_socs)

                            if found_match is False:
                                sys.exit(f'ERROR: SoC qualifier match unresolved: {qualifiers}')

    @staticmethod
    def from_file(socs_file):
        '''Load SoCs from a soc.yml file.
        '''
        try:
            with open(socs_file) as f:
                socs_yaml = f.read()
        except FileNotFoundError as e:
            sys.exit(f'ERROR: socs.yml file not found: {socs_file.as_posix()}', e)

        return Systems(str(socs_file.parent), socs_yaml)

    @staticmethod
    def from_yaml(socs_yaml):
        '''Load socs from a string with YAML contents.
        '''
        return Systems('', socs_yaml)

    def _make_socs(self, entries, folder, series, family):
        '''Build the SoCs of a "socs" list, setting aside the ones declared with "base".'''
        socs = []
        for soc in entries:
            if soc.get('base') is not None:
                # A SoC based on another one adds no configuration of its own, so it is recorded
                # as a pointer to the SoC it is based on rather than as a SoC in its own right.
                self._based_socs[soc['name']] = BasedSoc(soc['name'], soc['base'], folder)
                continue
            socs.append(Soc(soc['name'], [c['name'] for c in soc.get('cpuclusters', [])],
                            [folder], series, family))
        return socs

    def extend(self, systems):
        self._families.extend(systems.get_families())
        self._series.extend(systems.get_series())

        for name, based in systems.get_based_socs().items():
            existing = self._based_socs.get(name)
            if existing is not None and existing.base != based.base:
                sys.exit(f"ERROR: SoC '{name}' is declared twice, based on '{existing.base}' in "
                         f"{existing.folder} and on '{based.base}' in {based.folder}.")
            self._based_socs[name] = based

        for es in self._extended_socs + systems.get_extended_socs():
            based = self._based_socs.get(es.name)
            if based is not None:
                sys.exit(f"ERROR: SoC '{es.name}' is declared with 'base' in {based.folder} and "
                         "carries no configuration of its own, so it cannot be extended. Extend "
                         f"SoC '{based.base}' instead.")

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

    def get_based_socs(self):
        return self._based_socs

    def resolve_soc_name(self, name):
        '''Return the name of the SoC that provides the configuration for the given SoC.

        A SoC declared with 'base' carries no configuration of its own, so it resolves to the SoC
        it is based on. That SoC must be a real one, which keeps resolution one step deep.
        '''
        based = self._based_socs.get(name)
        if based is None:
            return name

        if based.base in self._based_socs:
            sys.exit(f"ERROR: SoC '{name}' in {based.folder} is based on '{based.base}', which is "
                     "itself based on another SoC. 'base' must name a SoC.")

        if not any(s.name == based.base for s in self._socs):
            sys.exit(f"ERROR: SoC '{name}' in {based.folder} is based on SoC '{based.base}', "
                     "which is not found. Please ensure that the SoC exists and that the "
                     "soc-root containing it has been correctly defined.")

        return based.base

    def get_loaded_trees(self, names):
        '''Return the SoCs, series and families described by the soc.yml trees holding the given
        SoCs.
        '''
        folders = {f for n in names for f in self.get_soc(n).folder}
        return (
            [s for s in self._socs if folders.intersection(s.folder)],
            [s for s in self._series if folders.intersection(s.folder)],
            [f for f in self._families if folders.intersection(f.folder)],
        )

    def get_soc(self, name):
        if name in self._based_socs:
            # Keep the name the board target uses, but take everything else from the SoC it is
            # based on, so it resolves to that SoC's tree and CPU clusters.
            base = self.get_soc(self.resolve_soc_name(name))
            return Soc(name, list(base.cpuclusters), list(base.folder), base.series, base.family)

        try:
            return next(s for s in self._socs if s.name == name)
        except StopIteration:
            sys.exit(f"ERROR: SoC '{name}' is not found, please ensure that the SoC exists "
                     f"and that soc-root containing '{name}' has been correctly defined.")



@dataclass
class BasedSoc:
    '''A SoC built on another SoC, adding no configuration of its own.'''
    name: str
    base: str
    folder: str


@dataclass
class Soc:
    name: str
    cpuclusters: list[str]
    folder: list[str]
    series: str = ''
    family: str = ''

    def extend(self, soc):
        if self.name == soc.name:
            self.cpuclusters.extend(soc.cpuclusters)
            self.folder.extend(soc.folder)


@dataclass
class Series:
    name: str
    folder: list[str]
    family: str
    socs: list[Soc]


@dataclass
class Family:
    name: str
    folder: list[str]
    series: list[Series]
    socs: list[Soc]


def unique_paths(paths):
    # Using dict keys ensures both uniqueness and a deterministic order.
    yield from dict.fromkeys(map(Path.resolve, paths)).keys()


def find_v2_archs(args):
    ret = {'archs': []}
    for root in unique_paths(args.arch_roots):
        archs_yml = root / ARCHS_YML_PATH

        if Path(archs_yml).is_file():
            with Path(archs_yml).open('r', encoding='utf-8') as f:
                archs = yaml.load(f.read(), Loader=SafeLoader)

            errors = list(arch_validator.iter_errors(archs))
            if errors:
                sys.exit('ERROR: Malformed arch YAML file: '
                         f'{archs_yml.as_posix()}\n'
                         f'{best_match(errors).message} in {best_match(errors).json_path}')

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
    parser.add_argument("--soc", dest='socs_lookup', default=[], action='append',
                        help='lookup the specific soc, may be given more than once')
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
                # Below is non existing for arch but is defined here to support
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
                # Below is non existing for arch but is defined here to support
                # common formatting string.
                series='',
                family='',
                arch='',
                vendor=''
            )

        print(info)


def dump_v2_system(args, type, system):
    if args.soc_family is not None and (type != "soc" or system.family is None or \
       system.family != args.soc_family):
        return

    if args.soc_series is not None and (type != "soc" or system.series is None or \
       system.series != args.soc_series):
        return

    if type == "soc" and system.family is not None:
        family = system.family
    else:
        family = ""

    if type == "soc" and system.series is not None:
        series = system.series
    else:
        series = ""

    if args.cmakeformat is not None:
        info = args.cmakeformat.format(
           TYPE='TYPE;' + type,
           NAME='NAME;' + system.name,
           DIR='DIR;' + ';'.join([Path(x).as_posix() for x in system.folder]),
           HWM='HWM;' + 'v2',
           FAMILY='FAMILY;' + family,
           SERIES='SERIES;' + series
        )
    else:
        info = args.format.format(
           type=type,
           name=system.name,
           dir=system.folder,
           hwm='v2',
           family=family,
           series=series
        )

    print(info)


def dump_v2_systems(args):
    systems = find_v2_systems(args)

    if args.socs_lookup:
        socs, series, families = systems.get_loaded_trees(args.socs_lookup)
    else:
        socs = systems.get_socs()
        families = systems.get_families()
        series = systems.get_series()

    for f in families:
        dump_v2_system(args, 'family', f)

    for s in series:
        dump_v2_system(args, 'series', s)

    for s in socs:
        dump_v2_system(args, 'soc', s)


if __name__ == '__main__':
    args = parse_args()
    if any([args.socs, args.socs_lookup, args.soc_series, args.soc_family]):
        dump_v2_systems(args)
    if args.archs or args.arch is not None:
        dump_v2_archs(args)
