# Copyright (c) 2023 Basalte bv
#
# SPDX-License-Identifier: Apache-2.0

"""Sphinx extension to glob files and insert in place"""


import glob
import os

from sphinx.directives.other import Include as BaseInclude


class ZephyrGlobFiles(BaseInclude):
    r"""
    This is a Zephyr directive for including content from other files in place.

    For example, to include all files matching the pattern
    release-notes-3.6/net_misc_*.rst use::

       .. zephyr-glob-files:: release-notes-3.6/net_misc_*.rst

    """

    def run(self):
        source = self.state_machine.input_lines.source(
            self.lineno - self.state_machine.input_offset - 1
        )
        source_dir = os.path.dirname(os.path.abspath(source))

        content = []

        files = glob.glob(self.arguments[0], root_dir=source_dir)
        for file in files:
            self.arguments[0] = file
            content.extend(super().run())

        return content


def setup(app):
    app.add_directive("zephyr-glob-files", ZephyrGlobFiles)

    return {"version": "1.0", "parallel_read_safe": True, "parallel_write_safe": True}
