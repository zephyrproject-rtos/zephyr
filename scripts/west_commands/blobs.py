# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import re
import sys
import textwrap
from pathlib import Path
from urllib.parse import urlparse

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
            - license-abspath: absolute path to the license file for the blob
            - click-through: need license click-through or not
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

        group = parser.add_argument_group('west blobs fetch options')
        group.add_argument(
            '-l',
            '--allow-regex',
            help='''Regex pattern to apply to the blob local path.
                    Only local paths matching this regex will be fetched.
                    Note that local paths are relative to the module directory''',
        )
        group.add_argument('-a', '--auto-accept', action='store_true',
                            help='''auto accept license if the fetching needs click-through''')

        return parser

    def get_blobs(self, args):
        blobs = []
        modules = args.modules
        all_modules = zephyr_module.parse_modules(ZEPHYR_BASE, self.manifest)
        all_names = [m.meta.get('name', None) for m in all_modules]

        unknown = set(modules) - set(all_names)

        if len(unknown):
            self.die(f'Unknown module(s): {unknown}')

        for module in all_modules:
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
            self.inf(fmt.format(**blob))

    def ensure_folder(self, path):
        path.parent.mkdir(parents=True, exist_ok=True)

    def fetch_blob(self, url, path):
        scheme = urlparse(url).scheme
        self.dbg(f'Fetching {path} with {scheme}')
        import fetchers
        fetcher = fetchers.get_fetcher_cls(scheme)

        self.dbg(f'Found fetcher: {fetcher}')
        inst = fetcher()
        self.ensure_folder(path)
        inst.fetch(url, path)

    # Compare the checksum of a file we've just downloaded
    # to the digest in blob metadata, warn user if they differ.
    def verify_blob(self, blob) -> bool:
        self.dbg(f"Verifying blob {blob['module']}: {blob['abspath']}")

        status = zephyr_module.get_blob_status(blob['abspath'], blob['sha256'])
        if status == zephyr_module.BLOB_OUTDATED:
            self.err(textwrap.dedent(
                f'''\
                The checksum of the downloaded file does not match that
                in the blob metadata:
                - if it is not certain that the download was successful,
                  try running 'west blobs fetch {blob['module']}'
                  to re-download the file
                - if the error persists, please consider contacting
                  the maintainers of the module so that they can check
                  the corresponding blob metadata

                Module: {blob['module']}
                Blob:   {blob['path']}
                URL:    {blob['url']}
                Info:   {blob['description']}'''))
            return False
        return True

    def fetch(self, args):
        bad_checksum_count = 0
        blobs = self.get_blobs(args)
        for blob in blobs:
            if blob['status'] == zephyr_module.BLOB_PRESENT:
                self.dbg(f"Blob {blob['module']}: {blob['abspath']} is up to date")
                continue

            # if args.allow_regex is set, use it to filter the blob by path
            if args.allow_regex and not re.match(args.allow_regex, blob['path']):
                self.dbg(
                    f"Blob {blob['module']}: {blob['abspath']} does not match regex "
                    f"'{args.allow_regex}', skipping"
                )
                continue
            self.inf(f"Fetching blob {blob['module']}: {blob['abspath']}")

            if blob['click-through'] and not args.auto_accept:
                while True:
                    user_input = input("For this blob, need to read and accept "
                                       "license to continue. Read it?\n"
                                       "Please type 'y' or 'n' and press enter to confirm: ")
                    if user_input.upper() == "Y" or user_input.upper() == "N":
                        break

                if user_input.upper() != "Y":
                    self.wrn('Skip fetching this blob.')
                    continue

                with open(blob['license-abspath']) as license_file:
                    license_content = license_file.read()
                    print(license_content)

                while True:
                    user_input = input("Accept license to continue?\n"
                                       "Please type 'y' or 'n' and press enter to confirm: ")
                    if user_input.upper() == "Y" or user_input.upper() == "N":
                        break

                if user_input.upper() != "Y":
                    self.wrn('Skip fetching this blob.')
                    continue

            self.fetch_blob(blob['url'], blob['abspath'])
            if not self.verify_blob(blob):
                bad_checksum_count += 1

        if bad_checksum_count:
            self.err(f"{bad_checksum_count} blobs have bad checksums")
            sys.exit(os.EX_DATAERR)

    def clean(self, args):
        blobs = self.get_blobs(args)
        for blob in blobs:
            if blob['status'] == zephyr_module.BLOB_NOT_PRESENT:
                self.dbg(f"Blob {blob['module']}: {blob['abspath']} not in filesystem")
                continue
            self.inf(f"Deleting blob {blob['module']}: {blob['status']} {blob['abspath']}")
            blob['abspath'].unlink()

    def do_run(self, args, _):
        self.dbg(f"subcmd: '{args.subcmd[0]}' modules: {args.modules}")

        subcmd = getattr(self, args.subcmd[0])

        if args.subcmd[0] != 'list' and args.format is not None:
            self.die('unexpected --format argument; this is a "west blobs list" option')

        subcmd(args)
