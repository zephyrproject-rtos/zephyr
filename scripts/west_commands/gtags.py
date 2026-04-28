# Copyright (c) 2025 Qualcomm Innovation Center, Inc.
# SPDX-License-Identifier: Apache-2.0

"""gtags.py

A west extension for creating tags files (GTAGS) for GNU Global.
For more information on Global, see: https://www.gnu.org/software/global
"""

import argparse
import os.path
import subprocess
import tempfile

from west.commands import WestCommand


class Gtags(WestCommand):
    def __init__(self):
        super().__init__(
            "gtags",
            "create a GNU Global tags file for the current workspace",
            """\
Indexes source code files in the west workspace using GNU Global's
"gtags" tool. For more information on Global and gtags, see:

    https://www.gnu.org/software/global/

The index can be useful to find definitions of functions, etc.,
especially across repository boundaries. One example is
finding the definition of a vendor HAL function that is
provided by a Zephyr module for the HAL.

By default, this west command only indexes files that are
tracked by git projects defined in the west. Inactive west
projects are ignored by default. For more information on
projects etc., see the west documentation.""",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
            formatter_class=argparse.RawDescriptionHelpFormatter,
        )

        parser.add_argument(
            "projects",
            nargs="*",
            metavar="PROJECT",
            help="""Name of west project to index, or
            its path. May be given more than once. Use
            "manifest" to refer to the manifest
            repository""",
        )

        return parser

    def do_run(self, args, unknown_args):
        all_files = []
        for project in self.manifest.get_projects(args.projects):
            all_files.extend(self.files_in_project(project))

        with tempfile.TemporaryDirectory(suffix="gtags") as d:
            gtags_files = os.path.join(d, "gtags.files")
            with open(gtags_files, "w") as f:
                # Due to what looks like a limitation in GNU Global,
                # this won't work if there are newlines in file names.
                # Unlike xargs and other commands, though, gtags
                # doesn't seem to have a way to accept a NUL-delimited
                # list of input file; its manpage says file names must
                # be delimited by newlines.
                f.write("\n".join(all_files))
            subprocess.run(
                # Note that "gtags -f -" and passing files via stdin
                # could run into issues on windows, and there seem to
                # be win32 builds of global out there
                ["gtags", "-f", gtags_files],
                cwd=self.manifest.topdir,
                check=True,
            )

    def files_in_project(self, project):
        if not project.is_cloned() or not self.manifest.is_active(project):
            return []
        ls_files = (
            project.git(["ls-files", "**"], capture_stdout=True).stdout.decode("utf-8").splitlines()
        )
        ret = []
        for filename in ls_files:
            absolute = os.path.join(project.abspath, filename)
            # Filter out directories from the git ls-files output.
            # There didn't seem to be a way to tell it to do that by
            # itself.
            if os.path.isfile(absolute):
                ret.append(absolute)
        return ret
