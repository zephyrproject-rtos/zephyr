# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import doxmlparser

from docutils import nodes
from doxmlparser.compound import DoxCompoundKind
from pathlib import Path
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective
from typing import Any, Dict


class ApiOverview(SphinxDirective):
    """
    This is a Zephyr directive to generate a table containing an overview
    of all APIs. This table will show the API name, version and since which
    version it is present - all information extracted from Doxygen XML output.

    It is exclusively used by the doc/develop/api/overview.rst page.

    Configuration options:

    api_overview_doxygen_xml_dir: Doxygen xml output directory
    api_overview_doxygen_base_url: Doxygen base html directory
    """

    def run(self):
        return [self.env.api_overview_table]


def get_group(innergroup, all_groups):
    try:
        return [
            g
            for g in all_groups
            if g.get_compounddef()[0].get_id() == innergroup.get_refid()
        ][0]
    except IndexError as e:
        raise Exception(f"Unexpected group {innergroup.get_refid()}") from e


def visit_group(app, group, all_groups, rows, indent=0):
    version = since = ""
    github_uri = "https://github.com/zephyrproject-rtos/zephyr/releases/tag/"
    cdef = group.get_compounddef()[0]

    ssects = [
        s for p in cdef.get_detaileddescription().get_para() for s in p.get_simplesect()
    ]
    for sect in ssects:
        if sect.get_kind() == "since":
            since = sect.get_para()[0].get_valueOf_()
        elif sect.get_kind() == "version":
            version = sect.get_para()[0].get_valueOf_()

    if since:
        since_url = nodes.inline()
        reference = nodes.reference(text=f"v{since.strip()}.0", refuri=f"{github_uri}/v{since.strip()}.0")
        reference.attributes["internal"] = True
        since_url += reference
    else:
        since_url = nodes.Text("")

    url_base = Path(app.config.api_overview_doxygen_base_url)
    url = url_base / f"{cdef.get_id()}.html"

    title = cdef.get_title()

    row_node = nodes.row()

    # Next entry will contain the spacer and the link with API name
    entry = nodes.entry()
    span = nodes.Text("".join(["\U000000A0"] * indent))
    entry += span

    # API name with link
    inline = nodes.inline()
    reference = nodes.reference(text=title, refuri=str(url))
    reference.attributes["internal"] = True
    inline += reference
    entry += inline
    row_node += entry

    version_node = nodes.Text(version)
    # Finally, add version and since
    for cell in [version_node, since_url]:
        entry = nodes.entry()
        entry += cell
        row_node += entry
    rows.append(row_node)

    for innergroup in cdef.get_innergroup():
        visit_group(
            app, get_group(innergroup, all_groups), all_groups, rows, indent + 6
        )


def parse_xml_dir(dir_name):
    groups = []
    root = doxmlparser.index.parse(Path(dir_name) / "index.xml", True)
    for compound in root.get_compound():
        if compound.get_kind() == DoxCompoundKind.GROUP:
            file_name = Path(dir_name) / f"{compound.get_refid()}.xml"
            groups.append(doxmlparser.compound.parse(file_name, True))

    return groups


def generate_table(app, toplevel, groups):
    table = nodes.table()
    tgroup = nodes.tgroup()

    thead = nodes.thead()
    thead_row = nodes.row()
    for header_name in ["API", "Version", "Available in Zephyr Since"]:
        colspec = nodes.colspec()
        tgroup += colspec

        entry = nodes.entry()
        entry += nodes.Text(header_name)
        thead_row += entry
    thead += thead_row
    tgroup += thead

    rows = []
    tbody = nodes.tbody()
    for t in toplevel:
        visit_group(app, t, groups, rows)
    tbody.extend(rows)
    tgroup += tbody

    table += tgroup

    return table


def sync_contents(app: Sphinx) -> None:
    if app.config.doxyrunner_outdir:
        doxygen_out_dir = Path(app.config.doxyrunner_outdir)
    else:
        doxygen_out_dir = Path(app.outdir) / "_doxygen"

    if not app.env.doxygen_input_changed:
        return

    doxygen_xml_dir = doxygen_out_dir / "xml"
    groups = parse_xml_dir(doxygen_xml_dir)

    toplevel = [
        g
        for g in groups
        if g.get_compounddef()[0].get_id()
        not in [
            i.get_refid()
            for h in [j.get_compounddef()[0].get_innergroup() for j in groups]
            for i in h
        ]
    ]

    app.builder.env.api_overview_table = generate_table(app, toplevel, groups)


def setup(app) -> Dict[str, Any]:
    app.add_config_value("api_overview_doxygen_xml_dir", "html/doxygen/xml", "env")
    app.add_config_value("api_overview_doxygen_base_url", "../../doxygen/html", "env")

    app.add_directive("api-overview-table", ApiOverview)

    app.connect("builder-inited", sync_contents)

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
