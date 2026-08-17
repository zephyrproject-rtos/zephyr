# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Convert a StrictDoc JSON export of the Zephyr requirements (from the ``reqmgmt``
module) into documentation artifacts consumed by the Zephyr doc build:

- reStructuredText pages for the Sphinx HTML/LaTeX build, with a cross-reference
  target per requirement so other pages can link to a requirement by its UID.
- A Doxygen ``.dox`` file using the native ``\\requirement`` command, so that
  API and test comments can trace to requirements via ``\\satisfies`` /
  ``\\verifies`` and Doxygen renders its requirements/traceability page.

The JSON is produced by ``strictdoc export <dir> --formats json``, which writes a
single ``json/index.json`` with a top-level ``DOCUMENTS`` list. Each document has
a ``NODES`` tree (sections may nest) where requirement nodes are tagged
``_NODE_TYPE == "REQUIREMENT"``.

When the reqmgmt module is not present (``--module-present 0``), only a
placeholder ``index.rst`` is written so the strict (``-W``) Sphinx build still
succeeds.
"""

import argparse
import json
import logging
import re
import sys
from pathlib import Path

logger = logging.getLogger("gen_requirements")

# Requirement fields rendered (in order) as a field list in RST / labelled lines
# in Doxygen. The STATEMENT and TITLE are handled separately. Any other present
# field is rendered generically after these.
ORDERED_FIELDS = ["STATUS", "TYPE", "COMPONENT", "LEVEL", "TAGS", "RATIONALE", "USER_STORY"]

# JSON keys that are metadata (prefixed with '_') or structural, not content.
SKIP_FIELDS = {"UID", "TITLE", "STATEMENT", "RELATIONS"}


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "--json",
        type=Path,
        help="Path to the StrictDoc JSON export (json/index.json).",
    )
    parser.add_argument(
        "--rst-out",
        required=True,
        type=Path,
        help="Output directory for the generated reStructuredText pages.",
    )
    parser.add_argument(
        "--dox-out",
        required=True,
        type=Path,
        help="Output directory for the generated Doxygen .dox file.",
    )
    parser.add_argument(
        "--module-present",
        type=int,
        choices=(0, 1),
        default=1,
        help="Whether the reqmgmt module is available (1) or not (0).",
    )
    return parser.parse_args()


def iter_requirements(nodes):
    """Yield REQUIREMENT nodes from a document's NODES tree, recursing into
    sections, preserving document order."""
    for node in nodes:
        if not isinstance(node, dict):
            continue
        if node.get("_NODE_TYPE") == "REQUIREMENT":
            yield node
        # Sections (and any other container) nest their children under NODES.
        children = node.get("NODES")
        if isinstance(children, list):
            yield from iter_requirements(children)


def slugify(title):
    slug = re.sub(r"[^a-z0-9]+", "_", title.lower()).strip("_")
    return slug or "requirements"


def parents(req):
    return [
        rel.get("VALUE")
        for rel in req.get("RELATIONS", [])
        if rel.get("TYPE") == "Parent" and rel.get("VALUE")
    ]


def clean(text):
    """Collapse a (possibly multi-line) StrictDoc statement into a single
    trimmed string."""
    return " ".join((text or "").split())


# -- reStructuredText generation ---------------------------------------------

RST_ESCAPE = re.compile(r"([*`|\\])")


def rst_escape(text):
    return RST_ESCAPE.sub(r"\\\1", text)


def render_rst_document(doc, reqs):
    title = doc.get("TITLE", "Requirements")
    out = []
    out.append(title)
    out.append("#" * len(title))
    out.append("")

    for req in reqs:
        uid = req.get("UID")
        if not uid:
            continue
        heading = f"{uid}: {req.get('TITLE', '')}".rstrip(": ").strip()
        # Cross-reference target so other docs can use :ref:`<UID>`.
        out.append(f".. _{uid}:")
        out.append("")
        out.append(heading)
        out.append("=" * len(heading))
        out.append("")

        statement = clean(req.get("STATEMENT"))
        if statement:
            out.append(rst_escape(statement))
            out.append("")

        # Field list with the remaining metadata.
        fields = []
        for key in ORDERED_FIELDS:
            value = clean(req.get(key))
            if value:
                fields.append((key.replace("_", " ").title(), value))
        rels = parents(req)
        if rels:
            links = ", ".join(f":ref:`{p} <{p}>`" for p in rels)
            fields.append(("Parents", links))
        for name, value in fields:
            out.append(f":{name}: {value}")
        if fields:
            out.append("")

    return "\n".join(out).rstrip() + "\n"


def write_rst(rst_out, documents):
    rst_out.mkdir(parents=True, exist_ok=True)

    pages = []
    for doc in documents:
        reqs = list(iter_requirements(doc.get("NODES", [])))
        if not reqs:
            continue
        name = slugify(doc.get("TITLE", "requirements"))
        (rst_out / f"{name}.rst").write_text(render_rst_document(doc, reqs))
        pages.append((doc.get("TITLE", name), name))

    write_rst_index(rst_out, pages)


def write_rst_index(rst_out, pages):
    rst_out.mkdir(parents=True, exist_ok=True)
    title = "Requirements Catalog"
    out = []
    if not pages:
        # No content to link from a toctree; mark orphan so the strict build
        # does not flag it as missing from the document hierarchy.
        out.append(":orphan:")
        out.append("")
    out += [
        ".. _requirements_catalog_generated:",
        "",
        title,
        "#" * len(title),
        "",
    ]
    if pages:
        out.append("The following requirements are imported from the Zephyr")
        out.append("requirements repository (``reqmgmt``).")
        out.append("")
        out.append(".. toctree::")
        out.append("   :maxdepth: 2")
        out.append("")
        for _, name in pages:
            out.append(f"   {name}")
    else:
        out.append("No requirements are available in this build.")
    out.append("")
    (rst_out / "index.rst").write_text("\n".join(out))


# -- Doxygen generation ------------------------------------------------------

# Escape characters that Doxygen would otherwise interpret as commands or markup.
DOX_ESCAPE = re.compile(r"([\\@&<>#%*])")


def dox_escape(text):
    return DOX_ESCAPE.sub(r"\\\1", text)


def render_dox(documents):
    out = ["/**", " * @file", " */", ""]
    for doc in documents:
        reqs = list(iter_requirements(doc.get("NODES", [])))
        if not reqs:
            continue
        for req in reqs:
            uid = req.get("UID")
            if not uid:
                continue
            title = clean(req.get("TITLE"))
            out.append("/**")
            if title:
                out.append(f" * \\requirement {uid} ({dox_escape(title)})")
            else:
                out.append(f" * \\requirement {uid}")
            out.append(" *")

            statement = clean(req.get("STATEMENT"))
            if statement:
                out.append(f" * {dox_escape(statement)}")
                out.append(" *")

            # Metadata is rendered as a Markdown bullet list. Note: \par must not
            # be used inside a \requirement block (Doxygen 1.17 emits mismatched
            # </div> nesting warnings for it).
            for key in ORDERED_FIELDS:
                value = clean(req.get(key))
                if value:
                    label = key.replace("_", " ").title()
                    out.append(f" * - **{label}:** {dox_escape(value)}")
            rels = parents(req)
            if rels:
                links = ", ".join(f"\\ref {p}" for p in rels)
                out.append(f" * - **Parents:** {links}")
            out.append(" */")
            out.append("")
    return "\n".join(out) + "\n"


def write_dox(dox_out, documents):
    dox_out.mkdir(parents=True, exist_ok=True)
    (dox_out / "requirements.dox").write_text(render_dox(documents))


def main():
    logging.basicConfig(level=logging.INFO, format="%(name)s: %(message)s")
    args = parse_args()

    if not args.module_present:
        logger.info("reqmgmt module not present; writing placeholder only")
        write_rst_index(args.rst_out, [])
        args.dox_out.mkdir(parents=True, exist_ok=True)
        return 0

    if not args.json or not args.json.is_file():
        logger.error("JSON export not found: %s", args.json)
        return 1

    data = json.loads(args.json.read_text())
    documents = data.get("DOCUMENTS", [])

    write_rst(args.rst_out, documents)
    write_dox(args.dox_out, documents)

    total = sum(len(list(iter_requirements(d.get("NODES", [])))) for d in documents)
    logger.info("generated %d requirements", total)
    return 0


if __name__ == "__main__":
    sys.exit(main())
