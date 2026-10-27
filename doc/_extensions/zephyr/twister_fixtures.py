# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Twister Fixtures Table
======================

This extension renders the twister fixture registry (the ``tests/fixtures/<name>.yaml``
files) as a table.

Usage
*****

This extension introduces a new directive: ``twister-fixtures-table``::

    .. twister-fixtures-table::

Options
*******

- ``twister_fixtures_registry``: Path to the fixture registry directory.
"""

from pathlib import Path
from typing import Any

import yaml
from docutils import nodes
from docutils.parsers.rst import directives
from docutils.statemachine import StringList
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

__version__ = "0.1.0"


class TwisterFixturesTable(SphinxDirective):
    """Table of the registered twister fixtures."""

    def _cell(self, text: str) -> nodes.entry:
        entry = nodes.entry()
        self.state.nested_parse(StringList(text.splitlines()), self.content_offset, entry)
        return entry

    def run(self) -> list[nodes.Element]:
        registry = Path(self.env.config.twister_fixtures_registry)
        fixtures = {}
        for path in sorted(registry.glob("*.yaml")):
            self.env.note_dependency(str(path))
            fixtures[path.stem] = yaml.safe_load(path.read_text(encoding="utf-8"))

        table = nodes.table()
        tgroup = nodes.tgroup(cols=2)
        tgroup += nodes.colspec(colwidth=1)
        tgroup += nodes.colspec(colwidth=3)
        table += tgroup

        thead = nodes.thead()
        tgroup += thead
        row = nodes.row()
        thead += row
        for title in ("Fixture", "Description"):
            entry = nodes.entry()
            entry += nodes.paragraph(text=title)
            row += entry

        tbody = nodes.tbody()
        tgroup += tbody
        for name, data in fixtures.items():
            row = nodes.row()
            tbody += row
            entry = nodes.entry()
            entry += nodes.paragraph("", "", nodes.literal(text=name))
            row += entry
            text = data["description"]
            if "notes" in data:
                text += f"\n\n*Notes:* {data['notes']}"
            for reference in data.get("references", []):
                text += f"\n\n{reference}"
            row += self._cell(text)

        return [table]


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("twister_fixtures_registry", None, "env")

    directives.register_directive("twister-fixtures-table", TwisterFixturesTable)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
