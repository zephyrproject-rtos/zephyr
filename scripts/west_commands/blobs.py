# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import hashlib
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

    def __init__(self):
        super().__init__(
            'blobs',
            # Keep this in sync with the string in west-commands.yml.
            'work with binary blobs',
            'Work with binary blobs',
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        default_fmt = '{module} {status} {path} {type} {abspath}'
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

            "{default_fmt}"

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

        parser.add_argument('-f', '--format', default=default_fmt,
                            help='''Format string to use to list each blob;
                                    see FORMAT STRINGS below.''')

        parser.add_argument('modules', metavar='MODULE', nargs='*',
                            help='''zephyr modules to operate on;
                            all modules will be used if not given''')

        return parser

    def get_status(self, path, sha256):
        if not path.is_file():
            return 'D'
        with path.open('rb') as f:
            m = hashlib.sha256()
            m.update(f.read())
            if sha256.lower() == m.hexdigest():
                return 'A'
            else:
                return 'M'

    def get_blobs(self, args):
        blobs = []
        modules = args.modules
        for module in zephyr_module.parse_modules(ZEPHYR_BASE, self.manifest):
            mblobs = module.meta.get('blobs', None)
            if not mblobs:
                continue

            # Filter by module
            module_name = module.meta.get('name', None)
            if len(modules) and module_name not in modules:
                continue

            blobs_path = Path(module.project) / zephyr_module.MODULE_BLOBS_PATH
            for blob in mblobs:
                blob['module'] = module_name
                blob['abspath'] = blobs_path / Path(blob['path'])
                blob['status'] = self.get_status(blob['abspath'], blob['sha256'])
                blobs.append(blob)

        return blobs

    def list(self, args):
        blobs = self.get_blobs(args)
        for blob in blobs:
            log.inf(args.format.format(**blob))

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
                log.inf('Blob {module}: {abspath} is up to date'.format(**blob))
                continue
            log.inf('Fetching blob {module}: {abspath}'.format(**blob))
            self.fetch_blob(blob['url'], blob['abspath'])

    def clean(self, args):
        blobs = self.get_blobs(args)
        for blob in blobs:
            if blob['status'] == 'D':
                log.inf('Blob {module}: {abspath} not in filesystem'.format(**blob))
                continue
            log.inf('Deleting blob {module}: {status} {abspath}'.format(**blob))
            blob['abspath'].unlink()

    def do_run(self, args, _):
        log.dbg(f'{args.subcmd[0]} {args.modules}')

        subcmd = getattr(self, args.subcmd[0])
        subcmd(args)
