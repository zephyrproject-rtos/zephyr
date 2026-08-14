# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Sphinx extension that turns the Doxygen requirement traceability links into
sphinx-needs needs and views.

The requirements themselves are emitted as ``req`` needs by
``_scripts/gen_requirements.py`` (from the StrictDoc export). This extension adds
the *test side*: it parses the Doxygen XML produced by the ``doxyrunner`` plugin
(which already runs at ``builder-inited``) and, for every test function carrying
``\\verifies`` / ``\\satisfies`` links, emits a ``test`` or ``impl`` need with a
``validates`` / ``implements`` link to the requirement. It then writes a
traceability page with verification and implementation coverage charts, a
requirement<->test matrix, a requirement<->implementation matrix, a
requirement<->design matrix and a requirement hierarchy tree.

Doxygen only emits a resolvable ``refid`` for links that point at a real
requirement, so the XML is effectively pre-validated.

Kernel internals (``z_*``/``_*`` symbols) are excluded from the Doxygen output
by ``EXCLUDE_SYMBOLS``, so requirement links carried by their doc comments never
reach the XML. A source-level fallback scan picks those up from the kernel
sources directly; their UIDs are validated against the requirement compounds in
the Doxygen XML, keeping the "only real requirements" guarantee.

Everything is gated on the ``reqmgmt`` Sphinx tag (set by the doc CMake build
when the reqmgmt module is present); when the tag is absent the extension is a
no-op.
"""

import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

from docutils import nodes
from docutils.parsers.rst import Directive, directives
from docutils.statemachine import StringList
from sphinx.application import Sphinx
from sphinx.util import logging

__version__ = "0.1.0"

logger = logging.getLogger(__name__)

# Relationships/attributes a ``design`` element may declare. ``fulfills`` (to a
# requirement) is the primary one; the others are accepted so design and
# architecture documents can express richer links when needed. ``satisfies`` and
# ``realizes`` are folded into ``fulfills`` for the sphinx-needs model.
DESIGN_OPTIONS = ("fulfills", "implements", "satisfies", "realizes", "trace", "status")
DESIGN_LINKS = ("fulfills", "implements", "trace")
DESIGN_OPTION_MAP = {"satisfies": "fulfills", "realizes": "fulfills"}

# Doxygen XML <name> prefix for requirement compounds.
REQ_REFID_PREFIX = "requirement_"

# Software-requirement UID -> component prefix (ZEP-SRS-5-2 -> ZEP-SRS-5).
REQ_PREFIX_RE = re.compile(r"^(ZEP-S\w*-\d+)-\d+$")


def _slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_") or "component"

# Map Doxygen link element -> needs link (test/impl need -> requirement).
LINK_RELATIONSHIPS = {
    "verifies": "validates",
    "satisfies": "implements",
}

# Doxygen memberdef/compounddef kinds that may carry requirement links, and the
# C-domain role doxybridge resolves them with.
MEMBER_KIND_ROLES = {
    "function": "func",
    "define": "macro",
    "typedef": "type",
    "enum": "enum",
    "variable": "var",
}
COMPOUND_KIND_ROLES = {
    "group": "group",
    "struct": "struct",
    "union": "union",
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
        elements = [
            (member, MEMBER_KIND_ROLES.get(member.get("kind", "")), "name")
            for member in root.iter("memberdef")
        ] + [
            (compound, COMPOUND_KIND_ROLES.get(compound.get("kind", "")), "compoundname")
            for compound in root.iter("compounddef")
        ]
        for element, role, name_tag in elements:
            if role is None:
                continue
            name_el = element.find(name_tag)
            if name_el is None or not name_el.text:
                continue
            rels = {
                rel: _link_uids(element, tag)
                for tag, rel in LINK_RELATIONSHIPS.items()
                if _link_uids(element, tag)
            }
            if not rels:
                continue  # only document symbols that trace to a requirement
            entry = symbols.setdefault(
                name_el.text, {"rels": {}, "brief": "", "body": [], "role": role}
            )
            for rel, uids in rels.items():
                entry["rels"].setdefault(rel, set()).update(uids)
            if not entry["brief"]:
                entry["brief"] = " ".join(_inline_text(element.find("briefdescription")).split())
            if not entry["brief"]:
                entry["brief"] = (element.findtext("title") or "").strip()
            if not entry["body"]:
                entry["body"] = _description_to_rst(element.find("detaileddescription"))
    return symbols


# Source directories (relative to ZEPHYR_BASE) scanned by the fallback pass for
# requirement links Doxygen dropped (EXCLUDE_SYMBOLS, undocumented internals).
FALLBACK_SOURCE_DIRS = ("kernel", "include/zephyr")

# Doc comment followed by the start of the documented declaration. The
# declaration window must not run into the next comment, or it would swallow
# adjacent doc blocks (finditer matches are non-overlapping).
DOC_BLOCK_RE = re.compile(
    r"/\*\*(?P<block>.*?)\*/\s*(?P<decl>[^/](?:(?!/\*).){0,300})", re.S
)
LINK_TAG_RE = re.compile(r"[@\\](verifies|satisfies)\s+(ZEP-[A-Za-z0-9-]+)")
BRIEF_RE = re.compile(r"[@\\]brief\s+(.+)")
IDENT = r"[A-Za-z_][A-Za-z0-9_]*"


def _decl_name(decl: str) -> str | None:
    """Best-effort name of the declaration a doc comment documents."""
    # Preprocessor conditionals and attributes may precede the declaration.
    decl = re.sub(r"^\s*#\s*(?:if|ifdef|ifndef|el|endif)[^\n]*$", "", decl, flags=re.M)
    decl = re.sub(r"__attribute__\s*\(\(.*?\)\)", "", decl, flags=re.S)
    decl = decl.lstrip()
    m = re.match(rf"#\s*define\s+({IDENT})", decl)
    if m:
        return m.group(1)
    m = re.match(rf"typedef\b[^;(]*\(\s*\*\s*({IDENT})\s*\)", decl, re.S)
    if m:
        return m.group(1)
    m = re.match(rf"(?:typedef\s+)?(?:struct|union|enum)\s+({IDENT})\s*\{{", decl)
    if m:
        return m.group(1)
    # Function or variable: last identifier before '(' / '=' / ';'.
    head = re.split(r"[(=;{]", decl, maxsplit=1)[0]
    idents = re.findall(IDENT, head)
    return idents[-1] if idents else None


def _collect_source_fallback(
    xml_dir: Path, symbols: dict[str, dict]
) -> dict[str, dict]:
    """Scan the kernel sources for requirement links whose symbols never made
    it into the Doxygen XML (e.g. internals hidden by ``EXCLUDE_SYMBOLS``).
    UIDs are validated against the requirement compounds in the XML."""
    zephyr_base = Path(__file__).resolve().parents[3]
    extra: dict[str, dict] = {}
    for src_dir in FALLBACK_SOURCE_DIRS:
        for src in sorted((zephyr_base / src_dir).rglob("*.[ch]")):
            text = src.read_text(encoding="utf-8", errors="ignore")
            if "satisfies" not in text and "verifies" not in text:
                continue
            for m in DOC_BLOCK_RE.finditer(text):
                links = LINK_TAG_RE.findall(m.group("block"))
                if not links:
                    continue
                name = _decl_name(m.group("decl"))
                if not name or name in symbols or name in extra:
                    continue
                rels: dict[str, set[str]] = {}
                for tag, uid in links:
                    if not (xml_dir / f"{REQ_REFID_PREFIX}{uid}.xml").is_file():
                        logger.warning(
                            "%s: reference to unknown requirement '%s' (on %s)",
                            src.relative_to(zephyr_base), uid, name,
                        )
                        continue
                    rels.setdefault(LINK_RELATIONSHIPS[tag], set()).add(uid)
                if not rels:
                    continue
                brief_m = BRIEF_RE.search(m.group("block"))
                if brief_m:
                    brief = " ".join(brief_m.group(1).split())
                else:
                    # No @brief: use the block's first plain text line.
                    brief = next(
                        (
                            stripped
                            for line in m.group("block").splitlines()
                            if (stripped := line.strip(" \t*"))
                            and not stripped.startswith(("@", "\\"))
                        ),
                        "",
                    )
                line = text.count("\n", 0, m.start("decl")) + 1
                extra[name] = {
                    "rels": rels,
                    "brief": brief,
                    "body": [],
                    "role": None,
                    "source": f"{src.relative_to(zephyr_base)}:{line}",
                }
    return extra


def _heading(text: str, char: str) -> list[str]:
    return [text, char * max(len(text), 3), ""]


# The standalone view pages (one matrix/chart family each), keyed by file stem.
# Splitting these onto separate pages keeps each one small and quick to load,
# and lets a reader jump straight to the view they need.
def _view_pages() -> dict[str, list[str]]:
    return {
        "coverage": [
            *_heading("Coverage", "#"),
            "Share of software requirements with at least one verifying test, and",
            "with at least one implementing symbol.",
            "",
            ".. needpie:: Requirement verification coverage",
            "   :labels: unverified, verified",
            "",
            "   id.startswith('ZEP-SRS') and len(validates_back) == 0",
            "   id.startswith('ZEP-SRS') and len(validates_back) > 0",
            "",
            ".. needpie:: Requirement implementation coverage",
            "   :labels: unimplemented, implemented",
            "",
            "   id.startswith('ZEP-SRS') and len(implements_back) == 0",
            "   id.startswith('ZEP-SRS') and len(implements_back) > 0",
            "",
        ],
        "matrix_test": [
            *_heading("Requirement / test matrix", "#"),
            "Software requirements and the tests that verify them (``@verifies``).",
            "",
            ".. needtable:: Requirements and their verifying tests",
            "   :types: req",
            "   :filter: id.startswith('ZEP-SRS')",
            "   :columns: id, title, validates_back",
            "   :style: table",
            "",
        ],
        "matrix_implementation": [
            *_heading("Requirement / implementation matrix", "#"),
            "Software requirements and the source symbols (functions and macros)",
            "that implement them (``@satisfies``).",
            "",
            ".. needtable:: Requirements and the code that implements them",
            "   :types: req",
            "   :filter: id.startswith('ZEP-SRS')",
            "   :columns: id, title, implements_back",
            "   :style: table",
            "",
        ],
        "matrix_design": [
            *_heading("Requirement / design matrix", "#"),
            "Design and architecture elements (``DESIGN-*`` needs declared with the",
            "``design`` directive in the design documents) and the requirements",
            "they fulfill.",
            "",
            ".. needtable:: Requirements and the design elements that fulfill them",
            "   :types: req",
            "   :filter: id.startswith('ZEP-S')",
            "   :columns: id, title, fulfills_back",
            "   :style: table",
            "",
        ],
        "hierarchy": [
            *_heading("Requirement hierarchy", "#"),
            "System requirements and the software requirements that refine them.",
            "",
            ".. needtable:: Requirement hierarchy",
            "   :types: req",
            "   :filter: id.startswith('ZEP-SYRS')",
            "   :columns: id, title, trace_back",
            "   :style: table",
            "",
        ],
    }


def _render_symbols_page(title: str, symbols: dict[str, dict]) -> str:
    out = _heading(f"{title} tests and implementations", "#")
    for name in sorted(symbols):
        entry = symbols[name]
        directive = "test" if "validates" in entry["rels"] else "impl"
        out.append(f".. {directive}:: {entry['brief'] or name}".rstrip())
        out.append(f"   :id: {name}")
        for rel in sorted(entry["rels"]):
            out.append(f"   :{rel}: {', '.join(sorted(entry['rels'][rel]))}")
        out.append("")
        if entry.get("role"):
            # Link to the full documentation in the Doxygen output (doxybridge).
            out.append(f"   Documented at :c:{entry['role']}:`{name}`.")
        else:
            # Internal symbol excluded from the Doxygen output.
            out.append(f"   Internal symbol, defined in ``{entry['source']}``.")
        out.append("")
        for line in entry["body"]:
            out.append(f"   {line}" if line else "")
        out.append("")
    return "\n".join(out)


def _render_index(symbol_pages: list[tuple[str, str]], have_symbols: bool) -> str:
    title = "Requirements Traceability"
    out = [
        *_heading(title, "#"),
        "Traceability between the imported requirements and the tests that verify",
        "them and the code that implements them, derived from the ``@verifies`` and",
        "``@satisfies`` links in the source. Each view below is a separate page.",
        "",
        ".. toctree::",
        "   :maxdepth: 1",
        "   :caption: Coverage and matrices",
        "",
        "   coverage",
        "   matrix_test",
        "   matrix_implementation",
        "   matrix_design",
        "   hierarchy",
        "",
    ]
    if symbol_pages:
        out += [
            ".. toctree::",
            "   :maxdepth: 1",
            "   :caption: Verifying tests and implementations",
            "",
        ]
        out += [f"   {stem}" for _name, stem in symbol_pages]
        out.append("")
    elif not have_symbols:
        out += ["No traceability links were found in this build.", ""]
    return "\n".join(out)


def _component_of(entry: dict, components: dict) -> tuple[str, str]:
    """Assign a symbol to a component (slug, name) by the most frequent
    requirement-UID prefix among its links; fall back to a generic group."""
    counts: dict[str, int] = {}
    for uids in entry["rels"].values():
        for uid in uids:
            m = REQ_PREFIX_RE.match(uid)
            if m:
                counts[m.group(1)] = counts.get(m.group(1), 0) + 1
    if not counts:
        return "other", "Other"
    prefix = max(counts, key=lambda k: (counts[k], k))
    comp = components.get(prefix)
    if comp:
        return comp["slug"], comp["name"]
    return _slug(prefix), prefix


def generate_traceability(app: Sphinx) -> None:
    if not app.tags.has("reqmgmt"):
        return

    base = Path(app.srcdir) / "build" / "requirements" / "traceability"
    base.mkdir(parents=True, exist_ok=True)
    # Prune pages from earlier runs so renamed component pages do not linger
    # as orphaned (not-in-any-toctree) pages.
    for stale in base.glob("*.rst"):
        stale.unlink()

    xml_dir = _xml_dir(app)
    if xml_dir is None:
        logger.warning("requirement_traceability: Doxygen XML not found; empty views")
        symbols: dict[str, dict] = {}
    else:
        symbols = _collect(xml_dir)
        symbols.update(_collect_source_fallback(xml_dir, symbols))

    # Component map (prefix -> {name, slug}) shared with gen_requirements so the
    # per-symbol pages are grouped by the same components as the requirements.
    components: dict[str, dict] = {}
    comp_file = base.parent / "components.json"
    if comp_file.is_file():
        try:
            components = json.loads(comp_file.read_text())
        except (ValueError, OSError):
            logger.warning("requirement_traceability: could not read components.json")

    # Group the traceable symbols by component.
    groups: dict[str, dict] = {}
    for name, entry in symbols.items():
        slug, cname = _component_of(entry, components)
        group = groups.setdefault(slug, {"name": cname, "symbols": {}})
        group["symbols"][name] = entry

    # Standalone matrix/coverage/tree pages.
    for stem, lines in _view_pages().items():
        (base / f"{stem}.rst").write_text("\n".join(lines))

    # Per-component "tests and implementations" pages.
    symbol_pages: list[tuple[str, str]] = []
    for slug in sorted(groups, key=lambda s: groups[s]["name"].lower()):
        group = groups[slug]
        stem = f"symbols_{slug}"
        (base / f"{stem}.rst").write_text(_render_symbols_page(group["name"], group["symbols"]))
        symbol_pages.append((group["name"], stem))

    (base / "index.rst").write_text(_render_index(symbol_pages, bool(symbols)))

    logger.info(
        "requirement_traceability: %d symbols across %d component pages",
        len(symbols), len(symbol_pages),
    )


class DesignDirective(Directive):
    """A ``design`` item linking a design/architecture element to requirements.

    Usage in a design document (e.g. under ``doc/kernel/``)::

        .. design:: DESIGN-THREAD-SUSPENSION Thread Suspension
           :fulfills: ZEP-SRS-1-3 ZEP-SRS-1-4

           Optional prose describing the design element.

    When the ``reqmgmt`` tag is active the directive renders a sphinx-needs
    ``design_need`` (so the design element participates in the
    requirement/design matrices and JSON export). When the tag is absent it is a
    no-op: any body content is rendered as ordinary prose and no traceability
    item is created, so the directive never interferes with the documentation
    flow and never errors when the requirements tooling is disabled.
    """

    required_arguments = 1
    optional_arguments = 0
    # Allow "ID Caption with spaces" as a single argument (the ID is split off).
    final_argument_whitespace = True
    has_content = True
    option_spec = {opt: directives.unchanged for opt in DESIGN_OPTIONS}

    def run(self):
        env = self.state.document.settings.env

        if not env.app.tags.has("reqmgmt"):
            # No-op: keep any prose, drop the traceability marker entirely.
            if self.content:
                container = nodes.container()
                self.state.nested_parse(self.content, self.content_offset, container)
                return container.children
            return []

        # Emit a sphinx-needs ``design_need`` and let sphinx-needs register it.
        # The argument is "ID Caption..."; the ID becomes the need
        # id and the caption its title.
        arg = self.arguments[0].strip().split(None, 1)
        need_id = arg[0]
        caption = arg[1] if len(arg) > 1 else need_id
        lines = [f".. design_need:: {caption}", f"   :id: {need_id}"]
        merged: dict[str, list[str]] = {}
        for opt in DESIGN_OPTIONS:
            if opt not in self.options:
                continue
            name = DESIGN_OPTION_MAP.get(opt, opt)
            if name in DESIGN_LINKS:
                # link values are whitespace-separated in the documents;
                # sphinx-needs expects a comma-separated list
                merged.setdefault(name, []).extend(self.options[opt].split())
            else:
                lines.append(f"   :{name}: {self.options[opt]}")
        for name, values in merged.items():
            lines.append(f"   :{name}: {', '.join(values)}")
        lines.append("")
        for line in self.content:
            lines.append(f"   {line}" if line else "")

        container = nodes.container()
        self.state.nested_parse(
            StringList(lines, source="design"), self.content_offset, container
        )
        return container.children


def setup(app: Sphinx) -> dict:
    # Run after doxyrunner (which produces the XML) and external_content (which
    # syncs the source tree), both connected at the default priority of 500.
    app.connect("builder-inited", generate_traceability, priority=600)

    # Always register the directive so design documents that use it parse
    # cleanly whether or not the requirements tooling is enabled.
    app.add_directive("design", DesignDirective)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
