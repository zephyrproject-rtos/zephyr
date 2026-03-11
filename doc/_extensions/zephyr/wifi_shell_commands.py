# SPDX-FileCopyrightText: 2026 Zephyr Project members and individual contributors
# SPDX-License-Identifier: Apache-2.0

"""
Sphinx extension to auto-generate a table of Wi-Fi shell commands and their
help from subsys/net/l2/wifi/wifi_shell.c. Use the directive so the table
stays in sync with the source and does not require manual maintenance.

Usage in an .rst file::

   .. wifi-shell-commands::

Configuration in conf.py::

   wifi_shell_commands_source = str(ZEPHYR_BASE / "subsys" / "net" / "l2" / "wifi" / "wifi_shell.c")
"""

from pathlib import Path
from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

try:
    from gen_wifi_shell_commands import (
        WIFI_SHELL_GROUP_LABELS,
        WIFI_SHELL_GROUP_ORDER,
        WifiShellCommand,
        parse_wifi_shell_commands,
    )
except ImportError:
    parse_wifi_shell_commands = None
    WifiShellCommand = None
    WIFI_SHELL_GROUP_ORDER = ()
    WIFI_SHELL_GROUP_LABELS = {}

__version__ = "0.1.0"


class WifiShellCommandsDirective(SphinxDirective):
    """Emit a list-table of wifi shell commands and help from wifi_shell.c."""

    has_content = False
    required_arguments = 0
    optional_arguments = 0

    def run(self) -> list[nodes.Element]:
        if parse_wifi_shell_commands is None:
            return [
                nodes.warning(
                    "",
                    nodes.paragraph(
                        "",
                        "",
                        nodes.Text(
                            "wifi-shell-commands: gen_wifi_shell_commands module not found."
                        ),
                    ),
                )
            ]
        source = self.env.config.wifi_shell_commands_source
        if source is None:
            return [
                nodes.warning(
                    "",
                    nodes.paragraph(
                        "",
                        "",
                        nodes.Text(
                            "wifi-shell-commands: wifi_shell_commands_source not set in conf.py"
                        ),
                    ),
                )
            ]
        source = Path(source)
        if not source.is_file():
            return [
                nodes.warning(
                    "",
                    nodes.paragraph(
                        "",
                        "",
                        nodes.Text(f"wifi-shell-commands: source file not found: {source}"),
                    ),
                )
            ]
        self.env.note_dependency(str(source))
        try:
            commands = parse_wifi_shell_commands(source)
        except Exception as e:
            return [
                nodes.warning(
                    "",
                    nodes.paragraph(
                        "",
                        "",
                        nodes.Text(f"wifi-shell-commands: parse error: {e}"),
                    ),
                )
            ]
        by_group: dict[str, list[WifiShellCommand]] = {}
        for cmd in commands:
            by_group.setdefault(cmd.group, []).append(cmd)
        result = []
        filter_section = nodes.section(ids=["wifi-shell-filters"])
        jump_par = nodes.paragraph(classes=["wifi-shell-filter"])
        jump_par += nodes.Text("Filter by group: ")
        first = True
        for group_key in WIFI_SHELL_GROUP_ORDER:
            if group_key not in by_group:
                continue
            if not first:
                jump_par += nodes.Text(" | ")
            first = False
            ref = nodes.reference(
                "",
                "",
                nodes.Text(WIFI_SHELL_GROUP_LABELS.get(group_key, group_key)),
                refid=f"wifi-shell-cmd-{group_key}",
                internal=True,
            )
            jump_par += ref
        if jump_par.children:
            filter_section += jump_par
            result.append(filter_section)
        for group_key in WIFI_SHELL_GROUP_ORDER:
            group_cmds = by_group.get(group_key, [])
            if not group_cmds:
                continue
            label = WIFI_SHELL_GROUP_LABELS.get(group_key, group_key)
            section = nodes.section(ids=[f"wifi-shell-cmd-{group_key}"])
            title = nodes.title("", "", nodes.Text(label))
            section += title
            table = nodes.table()
            tgroup = nodes.tgroup(cols=3)
            tgroup += nodes.colspec(colwidth=1)
            tgroup += nodes.colspec(colwidth=1)
            tgroup += nodes.colspec(colwidth=4)
            thead = nodes.thead()
            row = nodes.row()
            row += nodes.entry("", nodes.paragraph("", "", nodes.Text("Command")))
            row += nodes.entry("", nodes.paragraph("", "", nodes.Text("Description")))
            row += nodes.entry("", nodes.paragraph("", "", nodes.Text("Usage")))
            thead += row
            tgroup += thead
            tbody = nodes.tbody()
            for cmd in group_cmds:
                row = nodes.row()
                row += nodes.entry(
                    "", nodes.paragraph("", "", nodes.literal("", cmd.command))
                )
                row += nodes.entry(
                    "", nodes.paragraph("", "", nodes.Text(cmd.summary))
                )
                usage_entry = nodes.entry()
                for line in cmd.usage.split("\n"):
                    line = line.strip()
                    if line:
                        usage_entry += nodes.paragraph("", "", nodes.Text(line))
                row += usage_entry
                tbody += row
            tgroup += tbody
            table += tgroup
            section += table
            back_par = nodes.paragraph()
            back_ref = nodes.reference(
                "",
                "",
                nodes.Text("↑ Back to filters"),
                refid="wifi-shell-filters",
                internal=True,
            )
            back_par += back_ref
            section += back_par
            result.append(section)
        return result


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("wifi_shell_commands_source", None, "env")
    directives.register_directive("wifi-shell-commands", WifiShellCommandsDirective)
    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
