#!/usr/bin/env python3

# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Set, Union
import argparse
import contextlib
import glob
import os
import subprocess
import sys
import tempfile

# TODO: include changes to child bindings

HERE = Path(__file__).parent.resolve()
ZEPHYR_BASE = HERE.parent.parent
SCRIPTS = ZEPHYR_BASE / 'scripts'

sys.path.insert(0, str(SCRIPTS / 'dts' / 'python-devicetree' / 'src'))

from devicetree.edtlib import Binding, bindings_from_paths, load_vendor_prefixes_txt

# The Compat type is a (compatible, on_bus) pair, which is used as a
# lookup key for bindings. The name "compat" matches edtlib's internal
# variable for this; it's a bit of a misnomer, but let's be
# consistent.
@dataclass
class Compat:
    compatible: str
    on_bus: Optional[str]

    def __hash__(self):
        return hash((self.compatible, self.on_bus))

class BindingChange:
    '''Marker type for an individual change that happened to a
    binding between the start and end commits. See subclasses
    below for concrete changes.
    '''

Compat2Binding = Dict[Compat, Binding]
Binding2Changes = Dict[Binding, List[BindingChange]]

@dataclass
class Changes:
    '''Container for all the changes that happened between the
    start and end commits.'''

    vnds: List[str]
    vnd2added: Dict[str, Compat2Binding]
    vnd2removed: Dict[str, Compat2Binding]
    vnd2changes: Dict[str, Binding2Changes]

@dataclass
class ModifiedSpecifier2Cells(BindingChange):
    space: str
    start: List[str]
    end: List[str]

@dataclass
class ModifiedBuses(BindingChange):
    start: List[str]
    end: List[str]

@dataclass
class AddedProperty(BindingChange):
    property: str

@dataclass
class RemovedProperty(BindingChange):
    property: str

@dataclass
class ModifiedPropertyType(BindingChange):
    property: str
    start: str
    end: str

@dataclass
class ModifiedPropertyEnum(BindingChange):
    property: str
    start: Any
    end: Any

@dataclass
class ModifiedPropertyConst(BindingChange):
    property: str
    start: Any
    end: Any

@dataclass
class ModifiedPropertyDefault(BindingChange):
    property: str
    start: Any
    end: Any

@dataclass
class ModifiedPropertyDeprecated(BindingChange):
    property: str
    start: bool
    end: bool

@dataclass
class ModifiedPropertyRequired(BindingChange):
    property: str
    start: bool
    end: bool

def get_changes_between(
        compat2binding_start: Compat2Binding,
        compat2binding_end: Compat2Binding
) -> Changes:
    vnd2added: Dict[str, Compat2Binding] = \
        group_compat2binding_by_vnd({
            compat: compat2binding_end[compat]
            for compat in compat2binding_end
            if compat not in compat2binding_start
        })

    vnd2removed: Dict[str, Compat2Binding] = \
        group_compat2binding_by_vnd({
            compat: compat2binding_start[compat]
            for compat in compat2binding_start
            if compat not in compat2binding_end
        })

    vnd2changes = group_binding2changes_by_vnd(
        get_binding2changes(compat2binding_start,
                            compat2binding_end))

    vnds_set: Set[str] = set()
    vnds_set.update(set(vnd2added.keys()),
                    set(vnd2removed.keys()),
                    set(vnd2changes.keys()))

    return Changes(vnds=sorted(vnds_set),
                   vnd2added=vnd2added,
                   vnd2removed=vnd2removed,
                   vnd2changes=vnd2changes)

def group_compat2binding_by_vnd(
        compat2binding: Compat2Binding
) -> Dict[str, Compat2Binding]:
    '''Convert *compat2binding* to a dict mapping vendor prefixes
    to the subset of *compat2binding* with that vendor prefix.'''
    ret: Dict[str, Compat2Binding] = defaultdict(dict)

    for compat, binding in compat2binding.items():
        ret[get_vnd(binding.compatible)][compat] = binding

    return ret

def group_binding2changes_by_vnd(
        binding2changes: Binding2Changes
) -> Dict[str, Binding2Changes]:
    '''Convert *binding2chages* to a dict mapping vendor prefixes
    to the subset of *binding2changes* with that vendor prefix.'''
    ret: Dict[str, Binding2Changes] = defaultdict(dict)

    for binding, changes in binding2changes.items():
        ret[get_vnd(binding.compatible)][binding] = changes

    return ret

def get_vnd(compatible: str) -> str:
    '''Return the vendor prefix or the empty string.'''
    if ',' not in compatible:
        return ''

    return compatible.split(',')[0]

def get_binding2changes(
        compat2binding_start: Compat2Binding,
        compat2binding_end: Compat2Binding
) -> Binding2Changes:
    ret: Binding2Changes = {}

    for compat, binding in compat2binding_end.items():
        if compat not in compat2binding_start:
            continue

        binding_start = compat2binding_start[compat]
        binding_end = compat2binding_end[compat]

        binding_changes: List[BindingChange] = \
            get_binding_changes(binding_start, binding_end)
        if binding_changes:
            ret[binding] = binding_changes

    return ret

def get_binding_changes(
        binding_start: Binding,
        binding_end: Binding
) -> List[BindingChange]:
    '''Enumerate the changes to a binding given its start and end values.'''
    ret: List[BindingChange] = []

    assert binding_start.compatible == binding_end.compatible
    assert binding_start.on_bus == binding_end.on_bus

    common_props: Set[str] = set(binding_start.prop2specs).intersection(
        set(binding_end.prop2specs))

    ret.extend(get_modified_specifier2cells(binding_start, binding_end))
    ret.extend(get_modified_buses(binding_start, binding_end))
    ret.extend(get_added_properties(binding_start, binding_end))
    ret.extend(get_removed_properties(binding_start, binding_end))
    ret.extend(get_modified_property_type(binding_start, binding_end,
                                          common_props))
    ret.extend(get_modified_property_enum(binding_start, binding_end,
                                          common_props))
    ret.extend(get_modified_property_const(binding_start, binding_end,
                                           common_props))
    ret.extend(get_modified_property_default(binding_start, binding_end,
                                             common_props))
    ret.extend(get_modified_property_deprecated(binding_start, binding_end,
                                                common_props))
    ret.extend(get_modified_property_required(binding_start, binding_end,
                                              common_props))

    return ret

def get_modified_specifier2cells(
        binding_start: Binding,
        binding_end: Binding
) -> List[BindingChange]:
    ret: List[BindingChange] = []
    start = binding_start.specifier2cells
    end = binding_end.specifier2cells

    if start == end:
        return []

    for space, cells_end in end.items():
        cells_start = start.get(space)
        if cells_start != cells_end:
            ret.append(ModifiedSpecifier2Cells(space,
                                               start=cells_start,
                                               end=cells_end))
    for space, cells_start in start.items():
        if space not in end:
            ret.append(ModifiedSpecifier2Cells(space,
                                               start=cells_start,
                                               end=None))

    return ret

def get_modified_buses(
        binding_start: Binding,
        binding_end: Binding
) -> List[BindingChange]:
    start = binding_start.buses
    end = binding_end.buses

    if start == end:
        return []

    return [ModifiedBuses(start=start, end=end)]

def get_added_properties(
        binding_start: Binding,
        binding_end: Binding
) -> List[BindingChange]:
    return [AddedProperty(prop) for prop in binding_end.prop2specs
            if prop not in binding_start.prop2specs]

def get_removed_properties(
        binding_start: Binding,
        binding_end: Binding
) -> List[BindingChange]:
    return [RemovedProperty(prop) for prop in binding_start.prop2specs
            if prop not in binding_end.prop2specs]

def get_modified_property_type(
        binding_start: Binding,
        binding_end: Binding,
        common_props: Set[str]
) -> List[BindingChange]:
    return get_modified_property_helper(
        common_props,
        lambda prop: binding_start.prop2specs[prop].type,
        lambda prop: binding_end.prop2specs[prop].type,
        ModifiedPropertyType)

def get_modified_property_enum(
        binding_start: Binding,
        binding_end: Binding,
        common_props: Set[str]
) -> List[BindingChange]:
    return get_modified_property_helper(
        common_props,
        lambda prop: binding_start.prop2specs[prop].enum,
        lambda prop: binding_end.prop2specs[prop].enum,
        ModifiedPropertyEnum)

def get_modified_property_const(
        binding_start: Binding,
        binding_end: Binding,
        common_props: Set[str]
) -> List[BindingChange]:
    return get_modified_property_helper(
        common_props,
        lambda prop: binding_start.prop2specs[prop].const,
        lambda prop: binding_end.prop2specs[prop].const,
        ModifiedPropertyConst)

def get_modified_property_default(
        binding_start: Binding,
        binding_end: Binding,
        common_props: Set[str]
) -> List[BindingChange]:
    return get_modified_property_helper(
        common_props,
        lambda prop: binding_start.prop2specs[prop].default,
        lambda prop: binding_end.prop2specs[prop].default,
        ModifiedPropertyDefault)

def get_modified_property_deprecated(
        binding_start: Binding,
        binding_end: Binding,
        common_props: Set[str]
) -> List[BindingChange]:
    return get_modified_property_helper(
        common_props,
        lambda prop: binding_start.prop2specs[prop].deprecated,
        lambda prop: binding_end.prop2specs[prop].deprecated,
        ModifiedPropertyDeprecated)

def get_modified_property_required(
        binding_start: Binding,
        binding_end: Binding,
        common_props: Set[str]
) -> List[BindingChange]:
    return get_modified_property_helper(
        common_props,
        lambda prop: binding_start.prop2specs[prop].required,
        lambda prop: binding_end.prop2specs[prop].required,
        ModifiedPropertyRequired)

def get_modified_property_helper(
        common_props: Set[str],
        start_fn: Callable[[str], Any],
        end_fn: Callable[[str], Any],
        change_constructor: Callable[[str, Any, Any], BindingChange]
) -> List[BindingChange]:

    ret = []
    for prop in common_props:
        start = start_fn(prop)
        end = end_fn(prop)
        if start != end:
            ret.append(change_constructor(prop, start, end))
    return ret

def load_compat2binding(commit: str) -> Compat2Binding:
    '''Load a map from compatible to binding with that compatible,
    based on the bindings in zephyr at the given commit.'''

    @contextlib.contextmanager
    def git_worktree(directory: os.PathLike, commit: str):
        fspath = os.fspath(directory)
        subprocess.run(['git', 'worktree', 'add', '--detach', fspath, commit],
                       check=True)
        yield
        print('removing worktree...')
        subprocess.run(['git', 'worktree', 'remove', fspath], check=True)

    ret: Compat2Binding = {}
    with tempfile.TemporaryDirectory(prefix='dt_bindings_worktree') as tmpdir:
        with git_worktree(tmpdir, commit):
            tmpdir_bindings = Path(tmpdir) / 'dts' / 'bindings'
            binding_files = []
            binding_files.extend(glob.glob(f'{tmpdir_bindings}/**/*.yml',
                                           recursive=True))
            binding_files.extend(glob.glob(f'{tmpdir_bindings}/**/*.yaml',
                                           recursive=True))
            bindings: List[Binding] = bindings_from_paths(
                binding_files, ignore_errors=True)
            for binding in bindings:
                compat = Compat(binding.compatible, binding.on_bus)
                assert compat not in ret
                ret[compat] = binding

    return ret

def compatible_sort_key(data: Union[Compat, Binding]) -> str:
    '''Sort key used by Printer.'''
    return (data.compatible, data.on_bus or '')

class Printer:
    '''Helper class for formatting output.'''

    def __init__(self, outfile):
        self.outfile = outfile
        self.vnd2vendor_name = load_vendor_prefixes_txt(
            ZEPHYR_BASE / 'dts' / 'bindings' / 'vendor-prefixes.txt')

    def print(self, *args, **kwargs):
        kwargs['file'] = self.outfile
        print(*args, **kwargs)

    def print_changes(self, changes: Changes):
        for vnd in changes.vnds:
            if vnd:
                vnd_fmt = f' ({vnd})'
            else:
                vnd_fmt = ''
            self.print(f'* {self.vendor_name(vnd)}{vnd_fmt}:\n')

            added = changes.vnd2added[vnd]
            if added:
                self.print('  * New bindings:\n')
                self.print_compat2binding(
                    added,
                    lambda binding: f':dtcompatible:`{binding.compatible}`'
                )

            removed = changes.vnd2removed[vnd]
            if removed:
                self.print('  * Removed bindings:\n')
                self.print_compat2binding(
                    removed,
                    lambda binding: f'``{binding.compatible}``'
                )

            modified = changes.vnd2changes[vnd]
            if modified:
                self.print('  * Modified bindings:\n')
                self.print_binding2changes(modified)

    def print_compat2binding(
            self,
            compat2binding: Compat2Binding,
            formatter: Callable[[Binding], str]
    ) -> None:
        for compat in sorted(compat2binding, key=compatible_sort_key):
            self.print(f'    * {formatter(compat2binding[compat])}')
        self.print()

    def print_binding2changes(self, binding2changes: Binding2Changes) -> None:
        for binding, changes in binding2changes.items():
            on_bus = f' (on {binding.on_bus} bus)' if binding.on_bus else ''
            self.print(f'    * :dtcompatible:`{binding.compatible}`{on_bus}:\n')
            for change in changes:
                self.print_change(change)
            self.print()

    def print_change(self, change: BindingChange) -> None:
        def print(msg):
            self.print(f'          * {msg}')
        def print_prop_change(details):
            print(f'property ``{change.property}`` {details} changed from '
                  f'{change.start} to {change.end}')
        if isinstance(change, ModifiedSpecifier2Cells):
            print(f'specifier cells for space "{change.space}" '
                  f'are now named: {change.end} (old value: {change.start})')
        elif isinstance(change, ModifiedBuses):
            print(f'bus list changed from {change.start} to {change.end}')
        elif isinstance(change, AddedProperty):
            print(f'new property: ``{change.property}``')
        elif isinstance(change, RemovedProperty):
            print(f'removed property: ``{change.property}``')
        elif isinstance(change, ModifiedPropertyType):
            print_prop_change('type')
        elif isinstance(change, ModifiedPropertyEnum):
            print_prop_change('enum value')
        elif isinstance(change, ModifiedPropertyConst):
            print_prop_change('const value')
        elif isinstance(change, ModifiedPropertyDefault):
            print_prop_change('default value')
        elif isinstance(change, ModifiedPropertyDeprecated):
            print_prop_change('deprecation status')
        elif isinstance(change, ModifiedPropertyRequired):
            if not change.start and change.end:
                print(f'property ``{change.property}`` is now required')
            else:
                print(f'property ``{change.property}`` is no longer required')
        else:
            raise ValueError(f'unknown type for {change}: {type(change)}')

    def vendor_name(self, vnd: str) -> str:
        # Necessary due to the patch for openthread.

        if vnd == 'openthread':
            # FIXME: we have to go beyond the dict since this
            # compatible isn't in vendor-prefixes.txt, but we have
            # binding(s) for it. We need to fix this in CI by
            # rejecting unknown vendors in a bindings check.
            return 'OpenThread'
        if vnd == '':
            return 'Generic or vendor-independent'
        return self.vnd2vendor_name[vnd]

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description='''
Print human-readable descriptions of changes to devicetree
bindings between two commits, in .rst format suitable for copy/pasting
into the release notes.
''',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('start', metavar='START-COMMIT',
                        help='''what you want to compare bindings against
                        (typically the previous release's tag)''')
    parser.add_argument('end', metavar='END-COMMIT',
                        help='''what you want to know bindings changes for
                        (typically 'main')''')
    parser.add_argument('file', help='where to write the .rst output to')
    return parser.parse_args()

def main():
    args = parse_args()

    compat2binding_start = load_compat2binding(args.start)
    compat2binding_end = load_compat2binding(args.end)
    changes = get_changes_between(compat2binding_start,
                                  compat2binding_end)

    with open(args.file, 'w') as outfile:
        Printer(outfile).print_changes(changes)

if __name__ == '__main__':
    main()
