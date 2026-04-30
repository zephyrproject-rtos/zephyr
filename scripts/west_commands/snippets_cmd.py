# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import re
import shutil
import sys
import textwrap
from pathlib import Path

from west.commands import WestCommand

from zephyr_ext_common import ZEPHYR_BASE

sys.path.append(os.fspath(Path(__file__).parent.parent))
import zephyr_module
from snippets import find_snippets_in_roots


class Snippets(WestCommand):
    def __init__(self):
        super().__init__(
            'snippets',
            '',
            description='Display list of available snippets',
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
            epilog=textwrap.dedent('''\
            FORMAT STRINGS
            --------------

            Snippets are listed using a Python 3 format string. Arguments
            to the format string are accessed by name.

            The following arguments are available:

            - name: snippet name
            - description: snippet description (may be empty)
            - dirs: directories (";" separated) that contain the snippet definition
            - boards: list of board patterns the snippet targets (may be empty)

            For example:
                west snippets --format="{name}: {dirs}"

            When no format string is given, snippets are listed with their
            names and descriptions aligned in colums.
            '''),
        )

        parser.add_argument(
            '-f',
            '--format',
            default=None,
            help='''Format string to use to list each snippet;
                    see FORMAT STRINGS below.''',
        )
        parser.add_argument(
            '-n',
            '--name',
            dest='name_re',
            help='''a regular expression; only snippets whose
                    names match NAME_RE will be listed''',
        )
        parser.add_argument(
            '--snippet-root',
            dest='snippet_roots',
            default=[],
            type=Path,
            action='append',
            help='add a snippet root, may be given more than once',
        )

        return parser

    def do_run(self, args, _):
        if args.name_re is not None:
            name_re = re.compile(args.name_re)
        else:
            name_re = None

        # User-supplied --snippet-root values come first, then auto-discovered roots.
        auto_roots = [ZEPHYR_BASE]

        for module in zephyr_module.parse_modules(ZEPHYR_BASE, self.manifest):
            snippet_root = module.meta.get('build', {}).get('settings', {}).get('snippet_root')
            if snippet_root is not None:
                auto_roots.append(Path(module.project) / snippet_root)

        args.snippet_roots += auto_roots

        snippets = find_snippets_in_roots(requested_snippets=[], snippet_roots=args.snippet_roots)
        filtered = [
            snippet
            for _, snippet in snippets.items()
            if name_re is None or name_re.search(snippet.name)
        ]
        filtered.sort(key=lambda snippet: snippet.name)

        if args.format is not None:
            for snippet in filtered:
                self.inf(
                    args.format.format(
                        name=snippet.name,
                        description=snippet.description or '',
                        dirs=';'.join(str(dir) for dir in snippet.dirs),
                        boards=list(snippet.board2appends.keys()) or [],
                    )
                )
        else:
            self._print_default(filtered)

    def _print_default(self, snippets):
        if not snippets:
            return

        max_name = max(len(s.name) for s in snippets)
        desc_col = max_name + 2
        line_width = shutil.get_terminal_size().columns if sys.stdout.isatty() else None

        for snippet in snippets:
            desc = snippet.description or ''
            if not desc:
                self.inf(snippet.name)
                continue

            prefix = f'{snippet.name + ":":{desc_col}}'
            if line_width is None:
                self.inf(prefix + desc)
                continue

            self.inf(
                textwrap.fill(
                    desc,
                    width=line_width,
                    initial_indent=prefix,
                    subsequent_indent=f'{"":{desc_col}}',
                )
            )
