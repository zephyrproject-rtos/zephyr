# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''west "change" command
Command allowing to make changes to the manifest'''

import ruamel.yaml
from west.commands import WestCommand
from west.manifest import Manifest, ImportFlag


class Change(WestCommand):

    def __init__(self):
        super().__init__(
            'change',
            # Keep this in sync with the string in west-commands.yml.
            'Update projects information',
            'Change project config in west.yml.',
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help, 
            description=self.description
        )

        # Add some example options using the standard argparse module API.
        parser.add_argument(
            "name",
            help="Which project to change.",
        )
        parser.add_argument(
            "--revision",
            help="What revision to use.",
        )
        parser.add_argument(
            "--path",
            help="What path to use.",
        )
        parser.add_argument(
            "--url",
            help="What url to use.",
        )
        return parser  # gets stored as self.parser

    def do_run(self, args, unknown_args):

        manifest = Manifest.from_file(import_flags=ImportFlag.IGNORE_PROJECTS)
        m = ruamel.yaml.YAML()
        m.indent(mapping=2, sequence=4, offset=2)
        m.preserve_quotes = True
        with open(manifest.path, "r") as fh:
            x = m.load(fh.read())
            for p in x["manifest"]["projects"]:
                if p["name"] == args.name:
                    if args.revision:
                        p["revision"] = args.revision
                    if args.path:
                        p["path"] = args.path
                    if args.url:
                        p["url"] = args.url

        with open(manifest.path, "w") as fh:
            m.dump(x, fh)
