"""
Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import os
from pathlib import Path
from typing import Any, Dict

from docutils import nodes

from sphinx.addnodes import pending_xref
from sphinx.application import Sphinx
from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx.util import logging

import xml.etree.ElementTree as ET


logger = logging.getLogger(__name__)


KIND_D2S = {
    "define": "macro",
    "variable": "var",
    "typedef": "type",
    "enum": "enum",
    "function": "func",
}
"""Mapping between Doxygen memberdef kind and Sphinx kinds"""


class DoxygenReferencer(SphinxPostTransform):

    default_priority = 5

    def run(self, **kwargs: Any) -> None:
        for node in self.document.traverse(pending_xref):
            if node.get("refdomain") != "c":
                continue

            reftype = node.get("reftype")
            entry = self.app.env.doxybridge_cache.get(reftype)
            if not entry:
                continue

            reftarget = node.get("reftarget")
            id = entry.get(reftarget)
            if not id:
                continue

            if reftype in ("struct", "union"):
                doxygen_target = f"{id}.html"
            else:
                split = id.split("_")
                doxygen_target = f"{'_'.join(split[:-1])}.html#{split[-1][1:]}"

            doxygen_target = (
                str(self.app.config.doxybridge_dir) + "/html/" + doxygen_target
            )

            doc_dir = os.path.dirname(self.document.get("source"))
            doc_dest = os.path.join(
                self.app.outdir,
                os.path.relpath(doc_dir, self.app.srcdir),
            )
            rel_uri = os.path.relpath(doxygen_target, doc_dest)

            refnode = nodes.reference(
                "", "", internal=True, refuri=rel_uri, reftitle=""
            )
            refnode.append(node[0].deepcopy())

            node.replace_self([refnode])


def doxygen_parse(app: Sphinx) -> None:
    app.env.doxybridge_cache = {
        "macro": {},
        "var": {},
        "type": {},
        "enum": {},
        "func": {},
        "union": {},
        "struct": {},
    }

    xmldir = Path(app.config.doxybridge_dir) / "xml"
    for file in xmldir.glob("*_8[a-z].xml"):
        root = ET.parse(file).getroot()

        compounddef = root.find("compounddef")
        if not compounddef or compounddef.attrib["kind"] != "file":
            logger.warning(f"Unexpected file content in {file}")
            continue

        memberdefs = compounddef.findall(".//memberdef")
        for memberdef in memberdefs:
            kind = KIND_D2S.get(memberdef.attrib["kind"])
            if not kind:
                logger.warning(f"Unsupported kind: {kind}")
                continue

            id = memberdef.attrib["id"]
            name = memberdef.find("name").text

            app.env.doxybridge_cache[kind][name] = id

        innerclasses = compounddef.findall(".//innerclass")
        for innerclass in innerclasses:
            refid = innerclass.get("refid")
            if refid.startswith("union"):
                kind = "union"
            elif refid.startswith("struct"):
                kind = "struct"
            else:
                continue

            name = innerclass.text
            app.env.doxybridge_cache[kind][name] = refid


def setup(app: Sphinx) -> Dict[str, Any]:
    app.add_config_value("doxybridge_dir", None, "env")

    app.add_post_transform(DoxygenReferencer)
    app.connect("builder-inited", doxygen_parse)

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
