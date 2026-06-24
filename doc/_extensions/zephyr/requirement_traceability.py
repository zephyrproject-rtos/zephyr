# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Sphinx extension that turns the Doxygen requirement traceability links into
mlx.traceability items and views.

The requirements themselves are emitted as ``item`` directives by
``_scripts/gen_requirements.py`` (from the StrictDoc export). This extension adds
the *test side*: it parses the Doxygen XML produced by the ``doxyrunner`` plugin
(which already runs at ``builder-inited``) and, for every test function carrying
``\\verifies`` / ``\\satisfies`` links, emits an mlx ``item`` with a
``validates`` / ``implements`` relationship to the requirement. It then writes a
traceability page with a coverage chart, a requirement<->test matrix and a
requirement hierarchy tree.

Doxygen only emits a resolvable ``refid`` for links that point at a real
requirement, so the XML is effectively pre-validated.

Everything is gated on the ``reqmgmt`` Sphinx tag (set by the doc CMake build
when the reqmgmt module is present); when the tag is absent the extension is a
no-op.
"""

import xml.etree.ElementTree as ET
from pathlib import Path

from sphinx.application import Sphinx
from sphinx.util import logging

__version__ = "0.1.0"

logger = logging.getLogger(__name__)

# Doxygen XML <name> prefix for requirement compounds.
REQ_REFID_PREFIX = "requirement_"

# Map Doxygen link element -> mlx relationship (test/impl item -> requirement).
LINK_RELATIONSHIPS = {
    "verifies": "validates",
    "satisfies": "implements",
}


def _xml_dir(app: Sphinx) -> Path | None:
    """Locate the Doxygen XML output produced by the doxyrunner plugin."""
    projects = getattr(app.config, "doxyrunner_projects", None) or {}
    project = projects.get("zephyr")
    if not project:
        return None
    xml_dir = Path(project["outdir"]) / "xml"
    return xml_dir if xml_dir.is_dir() else None


# Doxygen simplesect kinds rendered as labelled paragraphs.
SIMPLESECT_LABELS = {
    "see": "See also",
    "note": "Note",
    "attention": "Attention",
    "warning": "Warning",
    "return": "Returns",
}


def _inline_text(el: ET.Element) -> str:
    """Concatenate all text of an element and its descendants."""
    text = el.text or ""
    for child in el:
        text += _inline_text(child)
        text += child.tail or ""
    return text


def _description_to_rst(desc: ET.Element | None) -> list[str]:
    """Convert a Doxygen ``<...description>`` element into RST body lines
    (unindented). Handles paragraphs, bullet lists and ``\\see``-style sections."""
    if desc is None:
        return []
    lines: list[str] = []
    for para in desc.findall("para"):
        buf = para.text or ""
        for child in para:
            if child.tag in ("itemizedlist", "orderedlist"):
                flushed = " ".join(buf.split())
                if flushed:
                    lines += [flushed, ""]
                buf = ""
                for item in child.findall("listitem"):
                    item_text = " ".join(_inline_text(item).split())
                    if item_text:
                        lines.append(f"- {item_text}")
                lines.append("")
                buf = child.tail or ""
            elif child.tag == "simplesect":
                flushed = " ".join(buf.split())
                if flushed:
                    lines += [flushed, ""]
                buf = ""
                kind = child.get("kind", "")
                label = SIMPLESECT_LABELS.get(kind, kind.capitalize())
                sect = " ".join(_inline_text(child).split())
                if sect:
                    lines += [f"*{label}:* {sect}", ""]
                buf = child.tail or ""
            else:
                buf += _inline_text(child) + (child.tail or "")
        flushed = " ".join(buf.split())
        if flushed:
            lines += [flushed, ""]
    while lines and lines[-1] == "":
        lines.pop()
    return lines


def _link_uids(member: ET.Element, tag: str) -> set[str]:
    uids = {
        req.get("refid", "")[len(REQ_REFID_PREFIX):]
        for link in member.findall(tag)
        for req in link.findall("requirement")
        if req.get("refid", "").startswith(REQ_REFID_PREFIX)
    }
    uids.discard("")
    return uids


def _collect(xml_dir: Path) -> dict[str, dict]:
    """Parse the Doxygen XML and collect, per documented symbol, its
    relationships and its brief/detailed description (as RST lines)."""
    symbols: dict[str, dict] = {}
    for xml_file in xml_dir.glob("*.xml"):
        # Cheap pre-filter: only parse files that actually contain a link.
        text = xml_file.read_text(encoding="utf-8", errors="ignore")
        if not any(f"<{tag}>" in text for tag in LINK_RELATIONSHIPS):
            continue
        try:
            root = ET.fromstring(text)
        except ET.ParseError:
            continue
        for member in root.iter("memberdef"):
            if member.get("kind") != "function":
                continue
            name_el = member.find("name")
            if name_el is None or not name_el.text:
                continue
            rels = {
                rel: _link_uids(member, tag)
                for tag, rel in LINK_RELATIONSHIPS.items()
                if _link_uids(member, tag)
            }
            if not rels:
                continue  # only document symbols that trace to a requirement
            entry = symbols.setdefault(
                name_el.text, {"rels": {}, "brief": "", "body": []}
            )
            for rel, uids in rels.items():
                entry["rels"].setdefault(rel, set()).update(uids)
            if not entry["brief"]:
                entry["brief"] = " ".join(_inline_text(member.find("briefdescription")).split())
            if not entry["body"]:
                entry["body"] = _description_to_rst(member.find("detaileddescription"))
    return symbols


def _render_page(symbols: dict[str, dict]) -> str:
    title = "Requirements Traceability"
    out = [
        title,
        "#" * len(title),
        "",
        "Traceability between the imported requirements and the tests that verify",
        "them (and the code that implements them), derived from the ``@verifies`` /",
        "``@satisfies`` links in the source.",
        "",
        "Coverage",
        "========",
        "",
        ".. item-piechart:: Requirement verification coverage",
        "   :id_set: ZEP-SRS test_",
        "   :label_set: unverified, verified",
        "",
        "Requirement / test matrix",
        "=========================",
        "",
        ".. item-matrix:: Requirements and their verifying tests",
        "   :source: ZEP-S",
        "   :target: test_",
        "   :sourcetitle: Requirement",
        "   :targettitle: Verifying test / implementation",
        "   :type: validated_by implemented_by",
        "   :stats:",
        "",
        "Requirement hierarchy",
        "=====================",
        "",
        ".. item-tree:: Requirement hierarchy",
        "   :top: ZEP-SYRS",
        "   :type: backtrace",
        "",
        "Verifying tests and implementations",
        "===================================",
        "",
    ]

    for name in sorted(symbols):
        entry = symbols[name]
        caption = f".. item:: {name} {entry['brief']}".rstrip()
        out.append(caption)
        for rel in sorted(entry["rels"]):
            out.append(f"   :{rel}: {' '.join(sorted(entry['rels'][rel]))}")
        out.append("")
        # Link to the full test/implementation documentation in the Doxygen
        # output (resolved by the doxybridge extension).
        out.append(f"   Documented at :c:func:`{name}`.")
        out.append("")
        for line in entry["body"]:
            out.append(f"   {line}" if line else "")
        out.append("")

    if not symbols:
        out.append("No traceability links were found in this build.")
        out.append("")

    return "\n".join(out)


def generate_traceability(app: Sphinx) -> None:
    if not app.tags.has("reqmgmt"):
        return

    out_file = Path(app.srcdir) / "build" / "requirements" / "traceability.rst"

    xml_dir = _xml_dir(app)
    if xml_dir is None:
        logger.warning("requirement_traceability: Doxygen XML not found; writing empty page")
        symbols: dict[str, dict] = {}
    else:
        symbols = _collect(xml_dir)

    logger.info("requirement_traceability: %d traceable symbols", len(symbols))

    out_file.parent.mkdir(parents=True, exist_ok=True)
    out_file.write_text(_render_page(symbols))


def setup(app: Sphinx) -> dict:
    # Run after doxyrunner (which produces the XML) and external_content (which
    # syncs the source tree), both connected at the default priority of 500.
    app.connect("builder-inited", generate_traceability, priority=600)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
