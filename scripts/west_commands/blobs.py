# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
from pathlib import Path
import sys
import textwrap
from urllib.parse import urlparse

from west import log
from west.commands import WestCommand

from zephyr_ext_common import ZEPHYR_BASE

sys.path.append(os.fspath(Path(__file__).parent.parent))
import zephyr_module

class Blobs(WestCommand):

    DEFAULT_LIST_FMT = '{module} {status} {path} {type} {abspath}'

    def __init__(self):
        super().__init__(
            'blobs',
            # Keep this in sync with the string in west-commands.yml.
            'work with binary blobs',
            'Work with binary blobs',
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
            epilog=textwrap.dedent(f'''\
            FORMAT STRINGS
            --------------

            Blobs are listed using a Python 3 format string. Arguments
            to the format string are accessed by name.

            The default format string is:

            "{self.DEFAULT_LIST_FMT}"

            The following arguments are available:

            - module: name of the module that contains this blob
            - abspath: blob absolute path
            - status: short status (A: present, M: hash failure, D: not present)
            - path: blob local path from <module>/zephyr/blobs/
            - sha256: blob SHA256 hash in hex
            - type: type of blob
            - version: version string
            - license_path: path to the license file for the blob
            - uri: URI to the remote location of the blob
            - description: blob text description
            - doc-url: URL to the documentation for this blob
            '''))

        # Remember to update west-completion.bash if you add or remove
        # flags
        parser.add_argument('subcmd', nargs=1,
                            choices=['list', 'fetch', 'clean'],
                            help='sub-command to execute')

        parser.add_argument('modules', metavar='MODULE', nargs='*',
                            help='''zephyr modules to operate on;
                            all modules will be used if not given''')

        group = parser.add_argument_group('west blob list options')
        group.add_argument('-f', '--format',
                            help='''format string to use to list each blob;
                                    see FORMAT STRINGS below''')

        return parser

    def get_blobs(self, args):
        blobs = []
        modules = args.modules
        for module in zephyr_module.parse_modules(ZEPHYR_BASE, self.manifest):
            # Filter by module
            module_name = module.meta.get('name', None)
            if len(modules) and module_name not in modules:
                continue

            blobs += zephyr_module.process_blobs(module.project, module.meta)

        return blobs

    def list(self, args):
        blobs = self.get_blobs(args)
        fmt = args.format or self.DEFAULT_LIST_FMT
        for blob in blobs:
            log.inf(fmt.format(**blob))

    def ensure_folder(self, path):
        path.parent.mkdir(parents=True, exist_ok=True)

    def fetch_blob(self, url, path):
        scheme = urlparse(url).scheme
        log.dbg(f'Fetching {path} with {scheme}')
        import fetchers
        fetcher = fetchers.get_fetcher_cls(scheme)

        log.dbg(f'Found fetcher: {fetcher}')
        inst = fetcher()
        self.ensure_folder(path)
        inst.fetch(url, path)

    def fetch(self, args):
        blobs = self.get_blobs(args)
        for blob in blobs:
            if blob['status'] == 'A':
                log.dbg('Blob {module}: {abspath} is up to date'.format(**blob))
                continue
            log.inf('Fetching blob {module}: {abspath}'.format(**blob))
            self.fetch_blob(blob['url'], blob['abspath'])

    def clean(self, args):
        blobs = self.get_blobs(args)
        for blob in blobs:
            if blob['status'] == 'D':
                log.dbg('Blob {module}: {abspath} not in filesystem'.format(**blob))
                continue
            log.inf('Deleting blob {module}: {status} {abspath}'.format(**blob))
            blob['abspath'].unlink()

    def do_run(self, args, _):
        log.dbg(f'subcmd: \'{args.subcmd[0]}\' modules: {args.modules}')

        subcmd = getattr(self, args.subcmd[0])

        if args.subcmd[0] != 'list' and args.format is not None:
            log.die(f'unexpected --format argument; this is a "west blobs list" option')

        subcmd(args)
