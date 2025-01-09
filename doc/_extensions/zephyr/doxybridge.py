"""
Copyright (c) 2021 Nordic Semiconductor ASA
Copyright (c) 2024 The Linux Foundation
SPDX-License-Identifier: Apache-2.0
"""

import concurrent.futures
import os
from typing import Any

import doxmlparser
from docutils import nodes
from docutils.parsers.rst import directives
from doxmlparser.compound import DoxCompoundKind, DoxMemberKind
from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.domains.c import CXRefRole
from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective

logger = logging.getLogger(__name__)


KIND_D2S = {
    DoxMemberKind.DEFINE: "macro",
    DoxMemberKind.VARIABLE: "var",
    DoxMemberKind.TYPEDEF: "type",
    DoxMemberKind.ENUM: "enum",
    DoxMemberKind.FUNCTION: "func",
}


class DoxygenGroupDirective(SphinxDirective):
    has_content = False
    required_arguments = 1
    optional_arguments = 0
    option_spec = {
        "project": directives.unchanged,
    }

    def run(self):

        desc_node = addnodes.desc()
        desc_node["domain"] = "c"
        desc_node["objtype"] = "group"

        title_signode = addnodes.desc_signature()
        group_xref = addnodes.pending_xref(
            "",
            refdomain="c",
            reftype="group",
            reftarget=self.arguments[0],
            refwarn=True,
            project=self.options.get("project")
        )
        group_xref += nodes.Text(self.arguments[0])
        title_signode += group_xref

        desc_node.append(title_signode)

        return [desc_node]


class DoxygenReferencer(SphinxPostTransform):
    """Mapping between Doxygen memberdef kind and Sphinx kinds"""

    default_priority = 5

    def run(self, **kwargs: Any) -> None:
        for node in self.document.traverse(addnodes.pending_xref):
            if node.get("refdomain") != "c":
                continue

            reftype = node.get("reftype")

            # "member", "data" and "var" are equivalent as per Sphinx documentation for C domain
            if reftype in ("member", "data"):
                reftype = "var"

            found_name = None
            found_id = None
            for name in self.app.config.doxybridge_projects:
                entry = self.app.env.doxybridge_cache[name].get(reftype)
                if not entry:
                    continue

                reftarget = node.get("reftarget").replace(".", "::").rstrip("()")
                id = entry.get(reftarget)
                if not id:
                    if reftype == "func":
                        # macros are sometimes referenced as functions, so try that
                        id = self.app.env.doxybridge_cache[name].get("macro").get(reftarget)
                        if not id:
                            continue
                    else:
                        continue

                found_name = name
                found_id = id
                break

            if not found_name or not found_id:
                continue

            if reftype in ("struct", "union", "group"):
                doxygen_target = f"{id}.html"
            else:
                split = found_id.split("_")
                doxygen_target = f"{'_'.join(split[:-1])}.html#{split[-1][1:]}"

            doxygen_target = (
                str(self.app.config.doxybridge_projects[found_name]) + "/html/" + doxygen_target
            )

            doc_dir = os.path.dirname(self.document.get("source"))
            doc_dest = os.path.join(
                self.app.outdir,
                os.path.relpath(doc_dir, self.app.srcdir),
            )
            rel_uri = os.path.relpath(doxygen_target, doc_dest)

            refnode = nodes.reference("", "", internal=True, refuri=rel_uri, reftitle="")

            refnode.append(node[0].deepcopy())

            if reftype == "group":
                refnode["classes"].append("doxygroup")
                title = self.app.env.doxybridge_group_titles[found_name].get(reftarget, "group")
                refnode[0] = nodes.Text(title)

            node.replace_self([refnode])


def parse_members(sectiondef):
    cache = {}

    for memberdef in sectiondef.get_memberdef():
        kind = KIND_D2S.get(memberdef.get_kind())
        if not kind:
            continue

        id = memberdef.get_id()
        if memberdef.get_kind() == DoxMemberKind.VARIABLE:
            name = memberdef.get_qualifiedname() or memberdef.get_name()
        else:
            name = memberdef.get_name()

        cache.setdefault(kind, {})[name] = id

        if memberdef.get_kind() == DoxMemberKind.ENUM:
            for enumvalue in memberdef.get_enumvalue():
                enumname = enumvalue.get_name()
                enumid = enumvalue.get_id()
                cache.setdefault("enumerator", {})[enumname] = enumid

    return cache


def parse_sections(compounddef):
    cache = {}

    for sectiondef in compounddef.get_sectiondef():
        members = parse_members(sectiondef)
        for kind, data in members.items():
            cache.setdefault(kind, {}).update(data)

    return cache


def parse_compound(inDirName, baseName) -> dict:
    rootObj = doxmlparser.compound.parse(inDirName + "/" + baseName + ".xml", True)
    cache = {}
    group_titles = {}

    for compounddef in rootObj.get_compounddef():
        name = compounddef.get_compoundname()
        id = compounddef.get_id()
        kind = None
        if compounddef.get_kind() == DoxCompoundKind.STRUCT:
            kind = "struct"
        elif compounddef.get_kind() == DoxCompoundKind.UNION:
            kind = "union"
        elif compounddef.get_kind() == DoxCompoundKind.GROUP:
            kind = "group"
            group_titles[name] = compounddef.get_title()

        if kind:
            cache.setdefault(kind, {})[name] = id

        sections = parse_sections(compounddef)
        for kind, data in sections.items():
            cache.setdefault(kind, {}).update(data)

    return cache, group_titles


def parse_index(app: Sphinx, name, inDirName):
    rootObj = doxmlparser.index.parse(inDirName + "/index.xml", True)
    compounds = rootObj.get_compound()

    with concurrent.futures.ProcessPoolExecutor() as executor:
        futures = [
            executor.submit(parse_compound, inDirName, compound.get_refid())
            for compound in compounds
        ]
        for future in concurrent.futures.as_completed(futures):
            cache, group_titles = future.result()
            for kind, data in cache.items():
                app.env.doxybridge_cache[name].setdefault(kind, {}).update(data)
            app.env.doxybridge_group_titles[name].update(group_titles)


def doxygen_parse(app: Sphinx) -> None:
    if not hasattr(app.env, "doxybridge_cache"):
        app.env.doxybridge_cache = dict()

    if not hasattr(app.env, "doxybridge_group_titles"):
        app.env.doxybridge_group_titles = dict()

    for project, path in app.config.doxybridge_projects.items():
        if project in app.env.doxygen_input_changed and not app.env.doxygen_input_changed[project]:
            return

        app.env.doxybridge_cache[project] = {
            "macro": {},
            "var": {},
            "type": {},
            "enum": {},
            "enumerator": {},
            "func": {},
            "union": {},
            "struct": {},
            "group": {},
        }

        app.env.doxybridge_group_titles[project] = dict()

        parse_index(app, project, str(path / "xml"))


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("doxybridge_projects", None, "env")

    app.add_directive("doxygengroup", DoxygenGroupDirective)

    app.add_role_to_domain("c", "group", CXRefRole())

    app.add_post_transform(DoxygenReferencer)
    app.connect("builder-inited", doxygen_parse)

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
