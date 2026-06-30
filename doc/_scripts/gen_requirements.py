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


def req_uid(req):
    """Return a requirement's UID with surrounding whitespace removed.

    Some upstream requirements carry trailing whitespace in their UID; that
    space would otherwise leak into the generated Doxygen ``\\requirement`` id
    and RST item id, where it can never match a ``@verifies``/``@satisfies``
    reference (Doxygen tokenizes those on whitespace). Normalizing here keeps
    traceability robust against such data quirks."""
    return (req.get("UID") or "").strip()


def parents(req):
    return [
        rel.get("VALUE").strip()
        for rel in req.get("RELATIONS", [])
        if rel.get("TYPE") == "Parent" and rel.get("VALUE")
    ]


def clean(text):
    """Collapse a (possibly multi-line) StrictDoc statement into a single
    trimmed string."""
    return " ".join((text or "").split())


# Number of requirements contained in a node subtree (used to skip empty
# sections and requirement-less documents).
def count_reqs(node):
    return sum(1 for _ in iter_requirements([node]))


def children(node):
    kids = node.get("NODES")
    return [c for c in kids if isinstance(c, dict)] if isinstance(kids, list) else []


# -- reStructuredText generation ---------------------------------------------

RST_ESCAPE = re.compile(r"([*`|\\])")

# Heading underline characters by absolute level (0 = document title).
RST_LEVELS = ["#", "*", "=", "-", "~", "^", '"', "+"]


def rst_escape(text):
    return RST_ESCAPE.sub(r"\\\1", text)


def rst_heading(out, text, level):
    char = RST_LEVELS[min(level, len(RST_LEVELS) - 1)]
    out.append(text)
    out.append(char * max(len(text), 3))
    out.append("")


# Mapping of StrictDoc requirement fields to mlx.traceability item attributes.
ITEM_ATTRIBUTES = [("STATUS", "status"), ("TYPE", "rtype"), ("COMPONENT", "component")]


def render_requirement_rst(out, req):
    """Render a requirement as an mlx.traceability ``item`` directive. The UID is
    the item id (and cross-reference target); StrictDoc Parent relations become a
    ``trace`` relationship so the requirement hierarchy can be rendered."""
    uid = req_uid(req)
    if not uid:
        return
    title = clean(req.get("TITLE"))
    out.append(f".. item:: {uid} {title}".rstrip())
    for key, attr in ITEM_ATTRIBUTES:
        value = clean(req.get(key))
        if value:
            out.append(f"   :{attr}: {value}")
    rels = parents(req)
    if rels:
        out.append(f"   :trace: {' '.join(rels)}")
    out.append("")

    statement = clean(req.get("STATEMENT"))
    if statement:
        out.append(f"   {rst_escape(statement)}")
        out.append("")
    user_story = clean(req.get("USER_STORY"))
    if user_story:
        out.append(f"   *User story:* {rst_escape(user_story)}")
        out.append("")


def render_node_rst(out, node, level):
    node_type = node.get("_NODE_TYPE")
    if node_type == "REQUIREMENT":
        render_requirement_rst(out, node)
    elif node_type == "SECTION":
        if count_reqs(node) == 0:
            return
        rst_heading(out, clean(node.get("TITLE")) or "Section", level)
        for child in children(node):
            render_node_rst(out, child, level + 1)


def render_rst_document(doc):
    out = []
    rst_heading(out, doc.get("TITLE", "Requirements"), 0)
    for node in children(doc):
        render_node_rst(out, node, 1)
    return "\n".join(out).rstrip() + "\n"


# A document with more than this many requirements is split into one page per
# top-level section (component) instead of a single large page, so the catalog
# is browsable. Smaller documents (e.g. the system requirements) stay whole.
SPLIT_REQ_THRESHOLD = 60

# UID of a software requirement -> its component prefix (e.g. ZEP-SRS-5-2 ->
# ZEP-SRS-5), used to key the component map shared with the traceability views.
SECTION_PREFIX_RE = re.compile(r"^(ZEP-S\w*-\d+)-\d+$")


def section_prefix(section):
    for req in iter_requirements([section]):
        m = SECTION_PREFIX_RE.match(req_uid(req))
        if m:
            return m.group(1)
    return None


def render_section_page(section):
    """Render a single top-level section (component) as its own page."""
    out = []
    rst_heading(out, clean(section.get("TITLE")) or "Requirements", 0)
    for child in children(section):
        render_node_rst(out, child, 1)
    return "\n".join(out).rstrip() + "\n"


def write_rst(rst_out, documents):
    rst_out.mkdir(parents=True, exist_ok=True)

    groups = []          # [(group_title, [(page_title, slug), ...]), ...]
    components = {}       # prefix -> {"name", "slug"} (shared with traceability)
    for doc in documents:
        if count_reqs(doc) == 0:
            continue
        doc_title = doc.get("TITLE", "Requirements")
        top_sections = [
            c for c in children(doc)
            if c.get("_NODE_TYPE") == "SECTION" and count_reqs(c) > 0
        ]
        if count_reqs(doc) > SPLIT_REQ_THRESHOLD and len(top_sections) >= 4:
            # Split into one page per component (top-level section).
            pages = []
            for sec in top_sections:
                stitle = clean(sec.get("TITLE")) or "Section"
                slug = slugify(f"{doc_title} {stitle}")
                (rst_out / f"{slug}.rst").write_text(render_section_page(sec))
                pages.append((stitle, slug))
                prefix = section_prefix(sec)
                if prefix:
                    components[prefix] = {"name": stitle, "slug": slug}
            groups.append((doc_title, pages))
        else:
            slug = slugify(doc_title)
            (rst_out / f"{slug}.rst").write_text(render_rst_document(doc))
            groups.append((doc_title, [(doc_title, slug)]))

    # Component map consumed by the zephyr.requirement_traceability extension to
    # group the per-symbol traceability pages by the same components.
    (rst_out / "components.json").write_text(json.dumps(components, indent=2))
    write_rst_index(rst_out, groups)


def write_rst_index(rst_out, groups):
    rst_out.mkdir(parents=True, exist_ok=True)
    title = "Requirements Catalog"
    out = []
    if not groups:
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
    if groups:
        out += [
            "The following requirements are imported from the Zephyr",
            "requirements repository (``reqmgmt``).",
            "",
        ]
        # One captioned toctree per document; component pages sorted by title.
        for group_title, pages in groups:
            out += [".. toctree::", "   :maxdepth: 1", f"   :caption: {group_title}", ""]
            for _title, slug in sorted(pages, key=lambda p: p[0].lower()):
                out.append(f"   {slug}")
            out.append("")
        # Traceability views (matrices/coverage/tree/per-component symbol pages)
        # are generated by the zephyr.requirement_traceability extension.
        out += [
            ".. toctree::",
            "   :maxdepth: 1",
            "   :caption: Traceability",
            "",
            "   traceability/index",
            "",
        ]
    else:
        out += ["No requirements are available in this build.", ""]
    (rst_out / "index.rst").write_text("\n".join(out))


# -- Doxygen generation ------------------------------------------------------

# Escape characters that Doxygen would otherwise interpret as commands or markup.
DOX_ESCAPE = re.compile(r"([\\@&<>#%*])")


def dox_escape(text):
    return DOX_ESCAPE.sub(r"\\\1", text)


# Doxygen sectioning commands by depth (it supports four nested levels).
DOX_SECTIONS = ["section", "subsection", "subsubsection", "paragraph"]


def render_requirement_block_dox(out, req):
    """Emit a flat \\requirement block. These are collected by Doxygen onto a
    single requirements page and are the anchors for \\satisfies / \\verifies."""
    uid = req_uid(req)
    if not uid:
        return
    title = clean(req.get("TITLE"))
    out.append("/**")
    out.append(f" * \\requirement {uid} ({dox_escape(title)})" if title else f" * \\requirement {uid}")
    out.append(" *")

    statement = clean(req.get("STATEMENT"))
    if statement:
        out.append(f" * {dox_escape(statement)}")
        out.append(" *")

    # Metadata is rendered as a Markdown bullet list. Note: \par must not be used
    # inside a \requirement block (Doxygen 1.17 emits mismatched </div> nesting
    # warnings for it).
    for key in ORDERED_FIELDS:
        value = clean(req.get(key))
        if value:
            out.append(f" * - **{key.replace('_', ' ').title()}:** {dox_escape(value)}")
    rels = parents(req)
    if rels:
        links = ", ".join("\\ref " + p for p in rels)
        out.append(f" * - **Parents:** {links}")
    out.append(" */")
    out.append("")


def render_node_dox_page(out, node, depth, counter):
    """Render a section/requirement into a grouped index page. Sections become
    Doxygen section headings; requirements become \\ref links into the
    requirements page so the catalog is browsable by category."""
    node_type = node.get("_NODE_TYPE")
    if node_type == "REQUIREMENT":
        uid = req_uid(node)
        if uid:
            title = clean(node.get("TITLE")).replace('"', "")
            out.append(f' * - \\ref {uid} "{uid}: {title}"')
    elif node_type == "SECTION":
        if count_reqs(node) == 0:
            return
        cmd = DOX_SECTIONS[min(depth, len(DOX_SECTIONS) - 1)]
        counter[0] += 1
        out.append(" *")
        out.append(f" * \\{cmd} reqsec_{counter[0]} {dox_escape(clean(node.get('TITLE')) or 'Section')}")
        for child in children(node):
            render_node_dox_page(out, child, depth + 1, counter)


def render_dox(documents):
    out = ["/**", " * @file", " */", ""]

    # Flat \requirement blocks (the requirements page + traceability anchors).
    for doc in documents:
        for req in iter_requirements(doc.get("NODES", [])):
            render_requirement_block_dox(out, req)

    # Grouped, browsable index pages mirroring the StrictDoc section hierarchy.
    docs_with_reqs = [d for d in documents if count_reqs(d) > 0]
    page_ids = []
    counter = [0]
    for doc in docs_with_reqs:
        page_id = "reqcat_" + slugify(doc.get("TITLE", "requirements"))
        page_ids.append(page_id)
        out.append("/**")
        out.append(f" * \\page {page_id} {dox_escape(clean(doc.get('TITLE')) or 'Requirements')}")
        for node in children(doc):
            render_node_dox_page(out, node, 0, counter)
        out.append(" */")
        out.append("")

    if page_ids:
        out.append("/**")
        out.append(" * \\page reqcat Requirements Catalog")
        out.append(" *")
        out.append(" * Requirements imported from the Zephyr requirements repository")
        out.append(" * (reqmgmt), grouped by category.")
        out.append(" *")
        for page_id in page_ids:
            out.append(f" * \\subpage {page_id}")
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
