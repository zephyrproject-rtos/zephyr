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


def _collect_links(xml_dir: Path) -> dict[str, dict[str, set[str]]]:
    """Parse the Doxygen XML and collect, per relationship, the mapping
    ``symbol name -> {requirement UID, ...}``."""
    links: dict[str, dict[str, set[str]]] = {rel: {} for rel in LINK_RELATIONSHIPS.values()}
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
            name = name_el.text
            for tag, rel in LINK_RELATIONSHIPS.items():
                uids = {
                    req.get("refid", "")[len(REQ_REFID_PREFIX):]
                    for link in member.findall(tag)
                    for req in link.findall("requirement")
                    if req.get("refid", "").startswith(REQ_REFID_PREFIX)
                }
                uids.discard("")
                if uids:
                    links[rel].setdefault(name, set()).update(uids)
    return links


def _render_page(links: dict[str, dict[str, set[str]]]) -> str:
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

    # One item per symbol, carrying all of its relationships.
    by_symbol: dict[str, dict[str, set[str]]] = {}
    for rel, mapping in links.items():
        for symbol, uids in mapping.items():
            by_symbol.setdefault(symbol, {}).setdefault(rel, set()).update(uids)

    for symbol in sorted(by_symbol):
        out.append(f".. item:: {symbol}")
        for rel in sorted(by_symbol[symbol]):
            uids = " ".join(sorted(by_symbol[symbol][rel]))
            out.append(f"   :{rel}: {uids}")
        out.append("")

    if not by_symbol:
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
        links: dict[str, dict[str, set[str]]] = {rel: {} for rel in LINK_RELATIONSHIPS.values()}
    else:
        links = _collect_links(xml_dir)

    n_links = sum(len(m) for m in links.values())
    logger.info("requirement_traceability: %d traceable symbols", n_links)

    out_file.parent.mkdir(parents=True, exist_ok=True)
    out_file.write_text(_render_page(links))


def setup(app: Sphinx) -> dict:
    # Run after doxyrunner (which produces the XML) and external_content (which
    # syncs the source tree), both connected at the default priority of 500.
    app.connect("builder-inited", generate_traceability, priority=600)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
