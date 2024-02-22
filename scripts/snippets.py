#!/usr/bin/env python3
#
# Copyright (c) 2022, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Internal snippets tool.
This is part of the build system's support for snippets.
It is not meant for use outside of the build system.

Output CMake variables:

- SNIPPET_NAMES: CMake list of discovered snippet names
- SNIPPET_FOUND_{snippet}: one per discovered snippet
'''

from collections import defaultdict, UserDict
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath
from typing import Dict, Iterable, List, Set
import argparse
import logging
import os
import pykwalify.core
import pykwalify.errors
import re
import sys
import textwrap
import yaml
import platform

# Marker type for an 'append:' configuration. Maps variables
# to the list of values to append to them.
Appends = Dict[str, List[str]]

def _new_append():
    return defaultdict(list)

def _new_board2appends():
    return defaultdict(_new_append)

@dataclass
class Snippet:
    '''Class for keeping track of all the settings discovered for an
    individual snippet.'''

    name: str
    appends: Appends = field(default_factory=_new_append)
    board2appends: Dict[str, Appends] = field(default_factory=_new_board2appends)

    def process_data(self, pathobj: Path, snippet_data: dict):
        '''Process the data in a snippet.yml file, after it is loaded into a
        python object and validated by pykwalify.'''
        def append_value(variable, value):
            if variable in ('EXTRA_DTC_OVERLAY_FILE', 'EXTRA_CONF_FILE'):
                path = pathobj.parent / value
                if not path.is_file():
                    _err(f'snippet file {pathobj}: {variable}: file not found: {path}')
                return f'"{path}"'
            if variable in ('DTS_EXTRA_CPPFLAGS'):
                return f'"{value}"'
            _err(f'unknown append variable: {variable}')

        for variable, value in snippet_data.get('append', {}).items():
            self.appends[variable].append(append_value(variable, value))
        for board, settings in snippet_data.get('boards', {}).items():
            if board.startswith('/') and not board.endswith('/'):
                _err(f"snippet file {pathobj}: board {board} starts with '/', so "
                     "it must end with '/' to use a regular expression")
            for variable, value in settings.get('append', {}).items():
                self.board2appends[board][variable].append(
                    append_value(variable, value))

class Snippets(UserDict):
    '''Type for all the information we have discovered about all snippets.
    As a dict, this maps a snippet's name onto the Snippet object.
    Any additional global attributes about all snippets go here as
    instance attributes.'''

    def __init__(self, requested: Iterable[str] = None):
        super().__init__()
        self.paths: Set[Path] = set()
        self.requested: List[str] = list(requested or [])

class SnippetsError(Exception):
    '''Class for signalling expected errors'''

    def __init__(self, msg):
        self.msg = msg

class SnippetToCMakePrinter:
    '''Helper class for printing a Snippets's semantics to a .cmake
    include file for use by snippets.cmake.'''

    def __init__(self, snippets: Snippets, out_file):
        self.snippets = snippets
        self.out_file = out_file
        self.section = '#' * 79

    def print_cmake(self):
        '''Print to the output file provided to the constructor.'''
        # TODO: add source file info
        snippets = self.snippets
        snippet_names = sorted(snippets.keys())

        if platform.system() == "Windows":
            # Change to linux-style paths for windows to avoid cmake escape character code issues
            snippets.paths = set(map(lambda x: str(PurePosixPath(x)), snippets.paths))

            for this_snippet in snippets:
                for snippet_append in (snippets[this_snippet].appends):
                    snippets[this_snippet].appends[snippet_append] = \
                        set(map(lambda x: str(x.replace("\\", "/")), \
                            snippets[this_snippet].appends[snippet_append]))

        snippet_path_list = " ".join(
            sorted(f'"{path}"' for path in snippets.paths))

        self.print('''\
# WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY!
#
# This file contains build system settings derived from your snippets.
# Its contents are an implementation detail that should not be used outside
# of Zephyr's snippets CMake module.
#
# See the Snippets guide in the Zephyr documentation for more information.
''')

        self.print(f'''\
{self.section}
# Global information about all snippets.

# The name of every snippet that was discovered.
set(SNIPPET_NAMES {' '.join(f'"{name}"' for name in snippet_names)})
# The paths to all the snippet.yml files. One snippet
# can have multiple snippet.yml files.
set(SNIPPET_PATHS {snippet_path_list})

# Create variable scope for snippets build variables
zephyr_create_scope(snippets)
''')

        for snippet_name in snippets.requested:
            self.print_cmake_for(snippets[snippet_name])
            self.print()

    def print_cmake_for(self, snippet: Snippet):
        self.print(f'''\
{self.section}
# Snippet '{snippet.name}'

# Common variable appends.''')
        self.print_appends(snippet.appends, 0)
        for board, appends in snippet.board2appends.items():
            self.print_appends_for_board(board, appends)

    def print_appends_for_board(self, board: str, appends: Appends):
        if board.startswith('/'):
            board_re = board[1:-1]
            self.print(f'''\
# Appends for board regular expression '{board_re}'
if("${{BOARD}}${{BOARD_IDENTIFIER}}" MATCHES "^{board_re}$")''')
        else:
            self.print(f'''\
# Appends for board '{board}'
if("${{BOARD}}${{BOARD_IDENTIFIER}}" STREQUAL "{board}")''')
        self.print_appends(appends, 1)
        self.print('endif()')

    def print_appends(self, appends: Appends, indent: int):
        space = '  ' * indent
        for name, values in appends.items():
            for value in values:
                self.print(f'{space}zephyr_set({name} {value} SCOPE snippets APPEND)')

    def print(self, *args, **kwargs):
        kwargs['file'] = self.out_file
        print(*args, **kwargs)

# Name of the file containing the pykwalify schema for snippet.yml
# files.
SCHEMA_PATH = str(Path(__file__).parent / 'schemas' / 'snippet-schema.yml')
with open(SCHEMA_PATH, 'r') as f:
    SNIPPET_SCHEMA = yaml.safe_load(f.read())

# The name of the file which contains metadata about the snippets
# being defined in a directory.
SNIPPET_YML = 'snippet.yml'

# Regular expression for validating snippet names. Snippet names must
# begin with an alphanumeric character, and may contain alphanumeric
# characters or underscores. This is intentionally very restrictive to
# keep things consistent and easy to type and remember. We can relax
# this a bit later if needed.
SNIPPET_NAME_RE = re.compile('[A-Za-z0-9][A-Za-z0-9_-]*')

# Logger for this module.
LOG = logging.getLogger('snippets')

def _err(msg):
    raise SnippetsError(f'error: {msg}')

def parse_args():
    parser = argparse.ArgumentParser(description='snippets helper',
                                     allow_abbrev=False)
    parser.add_argument('--snippet-root', default=[], action='append', type=Path,
                        help='''a SNIPPET_ROOT element; may be given
                        multiple times''')
    parser.add_argument('--snippet', dest='snippets', default=[], action='append',
                        help='''a SNIPPET element; may be given
                        multiple times''')
    parser.add_argument('--cmake-out', type=Path,
                        help='''file to write cmake output to; include()
                        this file after calling this script''')
    return parser.parse_args()

def setup_logging():
    # Silence validation errors from pykwalify, which are logged at
    # logging.ERROR level. We want to handle those ourselves as
    # needed.
    logging.getLogger('pykwalify').setLevel(logging.CRITICAL)
    logging.basicConfig(level=logging.INFO,
                        format='  %(name)s: %(message)s')

def process_snippets(args: argparse.Namespace) -> Snippets:
    '''Process snippet.yml files under each *snippet_root*
    by recursive search. Return a Snippets object describing
    the results of the search.
    '''
    # This will contain information about all the snippets
    # we discover in each snippet_root element.
    snippets = Snippets(requested=args.snippets)

    # Process each path in snippet_root in order, adjusting
    # snippets as needed for each one.
    for root in args.snippet_root:
        process_snippets_in(root, snippets)

    return snippets

def find_snippets_in_roots(requested_snippets, snippet_roots) -> Snippets:
    '''Process snippet.yml files under each *snippet_root*
    by recursive search. Return a Snippets object describing
    the results of the search.
    '''
    # This will contain information about all the snippets
    # we discover in each snippet_root element.
    snippets = Snippets(requested=requested_snippets)

    # Process each path in snippet_root in order, adjusting
    # snippets as needed for each one.
    for root in snippet_roots:
        process_snippets_in(root, snippets)

    return snippets

def process_snippets_in(root_dir: Path, snippets: Snippets) -> None:
    '''Process snippet.yml files in *root_dir*,
    updating *snippets* as needed.'''

    if not root_dir.is_dir():
        LOG.warning(f'SNIPPET_ROOT {root_dir} '
                    'is not a directory; ignoring it')
        return

    snippets_dir = root_dir / 'snippets'
    if not snippets_dir.is_dir():
        return

    for dirpath, _, filenames in os.walk(snippets_dir):
        if SNIPPET_YML not in filenames:
            continue

        snippet_yml = Path(dirpath) / SNIPPET_YML
        snippet_data = load_snippet_yml(snippet_yml)
        name = snippet_data['name']
        if name not in snippets:
            snippets[name] = Snippet(name=name)
        snippets[name].process_data(snippet_yml, snippet_data)
        snippets.paths.add(snippet_yml)

def load_snippet_yml(snippet_yml: Path) -> dict:
    '''Load a snippet.yml file *snippet_yml*, validate the contents
    against the schema, and do other basic checks. Return the dict
    of the resulting YAML data.'''

    with open(snippet_yml, 'r') as f:
        try:
            snippet_data = yaml.safe_load(f.read())
        except yaml.scanner.ScannerError:
            _err(f'snippets file {snippet_yml} is invalid YAML')

    def pykwalify_err(e):
        return f'''\
invalid {SNIPPET_YML} file: {snippet_yml}
{textwrap.indent(e.msg, '  ')}
'''

    try:
        pykwalify.core.Core(source_data=snippet_data,
                            schema_data=SNIPPET_SCHEMA).validate()
    except pykwalify.errors.PyKwalifyException as e:
        _err(pykwalify_err(e))

    name = snippet_data['name']
    if not SNIPPET_NAME_RE.fullmatch(name):
        _err(f"snippet file {snippet_yml}: invalid snippet name '{name}'; "
             'snippet names must begin with a letter '
             'or number, and may only contain letters, numbers, '
             'dashes (-), and underscores (_)')

    return snippet_data

def check_for_errors(snippets: Snippets) -> None:
    unknown_snippets = sorted(snippet for snippet in snippets.requested
                              if snippet not in snippets)
    if unknown_snippets:
        all_snippets = '\n    '.join(sorted(snippets))
        _err(f'''\
snippets not found: {', '.join(unknown_snippets)}
  Please choose from among the following snippets:
    {all_snippets}''')

def write_cmake_out(snippets: Snippets, cmake_out: Path) -> None:
    '''Write a cmake include file to *cmake_out* which
    reflects the information in *snippets*.

    The contents of this file should be considered an implementation
    detail and are not meant to be used outside of snippets.cmake.'''
    if not cmake_out.parent.exists():
        cmake_out.parent.mkdir()
    with open(cmake_out, 'w', encoding="utf-8") as f:
        SnippetToCMakePrinter(snippets, f).print_cmake()

def main():
    args = parse_args()
    setup_logging()
    try:
        snippets = process_snippets(args)
        check_for_errors(snippets)
    except SnippetsError as e:
        LOG.critical(e.msg)
        sys.exit(1)
    write_cmake_out(snippets, args.cmake_out)

if __name__ == "__main__":
    main()
