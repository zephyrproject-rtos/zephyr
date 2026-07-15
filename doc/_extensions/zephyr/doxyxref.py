# SPDX-FileCopyrightText: Copyright (c) 2026 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

"""
Doxygen to Sphinx cross-reference resolver
##########################################

This extension makes it possible to reference Sphinx documentation content (e.g. Kconfig options,
Devicetree bindings, or any ``:ref:`` target) from Doxygen comments, and have the Doxygen-generated
HTML pages link back to the corresponding pages of the Sphinx-based documentation.

Doxygen aliases (defined in ``zephyr.doxyfile.in``) such as ``@kconfig{}``, ``@dtcompatible{}`` or
``@rstref{}`` expand to HTML placeholders like::

    <code class='doxyxref' data-stype='kconfig:option' data-starget='CONFIG_GPIO'>CONFIG_GPIO</code>

Once the Sphinx HTML build is finished, this extension scans the Doxygen-generated HTML pages and
resolves such placeholders into actual hyperlinks, using the Sphinx object inventory
(``objects.inv``) produced by the very same build. Links are written as relative URLs so that they
remain valid wherever the documentation set is hosted (docs.zephyrproject.org/latest, versioned
copies, local builds, downstream distributions, ...).

Placeholders that cannot be resolved are left untouched (they degrade to plain, code-styled text)
and a Sphinx warning is emitted for each of them.

Configuration options
=====================

- ``doxyxref_projects``: Dictionary of Doxygen projects, mapping project name to the Doxygen output
  directory (same format as ``doxybridge_projects``).
"""

from __future__ import annotations

import html
import os
import re
import zlib
from dataclasses import dataclass
from enum import StrEnum
from pathlib import Path
from typing import TYPE_CHECKING
from urllib.parse import quote

if TYPE_CHECKING:
    from sphinx.util.typing import Inventory

__version__ = "0.1.0"


class ObjType(StrEnum):
    """Sphinx object types (``domain:objtype``, as used to key the object inventory).

    Only the types this extension gives special treatment to are listed. Any other type is looked up
    in the inventory as-is, so new Doxygen aliases can be added to ``zephyr.doxyfile.in`` without
    any change here.
    """

    KCONFIG_OPTION = "kconfig:option"
    KCONFIG_OPTION_REGEX = "kconfig:option-regex"
    REF = "std:ref"
    LABEL = "std:label"
    DOC = "std:doc"


PLACEHOLDER_RE = re.compile(
    r"<(?P<tag>code|span) class='doxyxref' "
    r"data-stype='(?P<stype>[^']+)' "
    r"data-starget='(?P<starget>[^']*)'>"
    r".*?</(?P=tag)>",
    re.DOTALL,
)
"""Placeholder markup, as emitted by the ALIASES defined in zephyr.doxyfile.in."""


@dataclass(frozen=True)
class Xref:
    """A cross-reference from a Doxygen page back to the main documentation.

    Attributes:
        stype: Sphinx object type of the target (e.g. ``kconfig:option``, ``std:ref``).
        target: Reference target, with any explicit link title removed.
        uri: Location of the target relative to the documentation root, or ``None`` when the
            reference could not be resolved.
        text: Text to display for the resolved link.
    """

    stype: str
    target: str
    uri: str | None = None
    text: str = ""

    @property
    def resolved(self) -> bool:
        """Whether the reference was resolved to a documentation URI."""
        return self.uri is not None


def split_titled_ref(raw: str) -> tuple[str | None, str]:
    """Split an explicit-title reference into its title and target parts.

    Explicit titles use the same ``custom text <target>`` notation as Sphinx roles.

    Args:
        raw: Reference, with or without an explicit title.

    Returns:
        The title and the target. When no explicit title is present, the title is ``None`` and the
        whole input is the target.
    """
    if raw.endswith(">"):
        open_pos = raw.rfind("<")
        title = raw[:open_pos].rstrip() if open_pos > 0 else ""
        target = raw[open_pos + 1 : -1]
        if title and target and ">" not in target:
            return title, target

    return None, raw


def lookup_kconfig_regex(inventory: Inventory, target: str) -> str | None:
    """Resolve a Kconfig regex reference to the Kconfig search page.

    The link points to the Kconfig search page pre-filled with the given pattern, mirroring the
    Sphinx ``:kconfig:option-regex:`` role. The reference only resolves if at least one Kconfig
    option matches, using the same semantics as the search page: whitespace-separated,
    case-insensitive regular expressions searched within option names.

    Args:
        inventory: Parsed object inventory.
        target: Whitespace-separated regular expressions to search option names for.

    Returns:
        The search-page URI (relative to the documentation root), or ``None`` if no option matches.
    """
    options = inventory.get(ObjType.KCONFIG_OPTION, {})
    if not options:
        return None

    try:
        patterns = [re.compile(term.lower()) for term in target.split()]
    except re.error:
        return None

    if not patterns:
        return None

    if not any(all(pattern.search(name.lower()) for pattern in patterns) for name in options):
        return None

    # all options live on the search page: derive its URI from any entry
    page_uri = next(iter(options.values())).uri.partition("#")[0]
    return f"{page_uri}#!{quote(target)}"


def resolve(inventory: Inventory, stype: str, raw_target: str) -> Xref:
    """Parse and resolve a single cross-reference placeholder into an :class:`Xref`.

    ``std:ref`` references may carry an explicit ``title <target>`` title (as the Sphinx ``:ref:``
    role does) and are resolved against reference labels (stored lowercased in the inventory), with
    document names (``:doc:`` targets) as a fallback.

    Args:
        inventory: Parsed object inventory.
        stype: Sphinx object type of the target, as carried by the placeholder.
        raw_target: Reference target, possibly carrying an explicit title.

    Returns:
        The cross-reference, left unresolved if the target is not in the inventory.
    """
    if stype == ObjType.KCONFIG_OPTION_REGEX:
        uri = lookup_kconfig_regex(inventory, raw_target)
        return Xref(stype, raw_target, uri, raw_target)

    if stype == ObjType.REF:
        title, target = split_titled_ref(raw_target)
        item = inventory.get(ObjType.LABEL, {}).get(target.lower())
        if item is None:
            item = inventory.get(ObjType.DOC, {}).get(target.strip("/"))
    else:
        title, target = None, raw_target
        item = inventory.get(stype, {}).get(target)

    if item is None:
        return Xref(stype, target)

    text = title or (item.display_name if item.display_name not in ("", "-") else target)
    return Xref(stype, target, item.uri, text)


def render_link(tag: str, xref: Xref, url_base: str) -> str:
    """Render a resolved :class:`Xref` as an HTML anchor.

    Args:
        tag: Tag of the placeholder being replaced. A ``code`` placeholder keeps its monospace
            styling by wrapping the link text.
        xref: Resolved cross-reference.
        url_base: Prefix prepended to the documentation-root-relative URI of the target.

    Returns:
        The HTML anchor.
    """
    text = html.escape(xref.text)
    if tag == "code":
        text = f"<code>{text}</code>"

    href = html.escape(url_base + (xref.uri or ""), quote=True)
    return f'<a class="doxyxref" href="{href}">{text}</a>'


def process_html_content(
    content: str,
    inventory: Inventory,
    url_base: str,
) -> tuple[str, list[Xref]]:
    """Resolve all cross-reference placeholders found in an HTML document.

    Args:
        content: HTML content to process.
        inventory: Parsed object inventory.
        url_base: Prefix prepended to the documentation-root-relative URI of each resolved target.
            Can be an absolute URL or a relative path (with trailing slash).

    Returns:
        The processed content, and the list of references that could not be resolved.
    """
    unresolved: list[Xref] = []

    def _replace(m: re.Match) -> str:
        xref = resolve(inventory, m.group("stype"), m.group("starget").strip())
        if not xref.resolved:
            unresolved.append(xref)
            return m.group(0)

        return render_link(m.group("tag"), xref, url_base)

    return PLACEHOLDER_RE.sub(_replace, content), unresolved


def process_html_dir(
    html_dir: Path,
    inventory: Inventory,
    docs_root: Path,
) -> list[Xref]:
    """Resolve cross-reference placeholders in all HTML files of a directory.

    Args:
        html_dir: Directory containing the Doxygen-generated HTML files.
        inventory: Parsed object inventory.
        docs_root: Root directory of the Sphinx HTML output. Links are computed relative to each
            processed file.

    Returns:
        List of references that could not be resolved.
    """
    unresolved: list[Xref] = []

    for file in sorted(html_dir.rglob("*.html")):
        content = file.read_text(encoding="utf-8", errors="surrogateescape")
        if "class='doxyxref'" not in content:
            continue

        rel = os.path.relpath(docs_root, file.parent).replace(os.sep, "/")
        base = "" if rel == "." else f"{rel}/"

        new_content, missing = process_html_content(content, inventory, base)
        unresolved.extend(missing)

        if new_content != content:
            file.write_text(new_content, encoding="utf-8", errors="surrogateescape")

    return unresolved


def doxyxref_resolve(app, exception) -> None:
    """Sphinx ``build-finished`` hook resolving Doxygen cross-references.

    Args:
        app: Sphinx application instance.
        exception: Exception raised by the build, if it failed.
    """
    from sphinx.util import logging
    from sphinx.util.inventory import InventoryFile

    logger = logging.getLogger(__name__)

    if exception is not None or app.builder.format != "html":
        return

    outdir = Path(app.outdir)
    inventory_file = outdir / "objects.inv"
    if not inventory_file.exists():
        logger.warning(f"doxyxref: object inventory not found: {inventory_file}")
        return

    try:
        # uri="" keeps the target locations relative to the documentation root
        inventory = InventoryFile.loads(inventory_file.read_bytes(), uri="").data
    except (OSError, ValueError, zlib.error) as e:
        logger.warning(f"doxyxref: could not read object inventory {inventory_file}: {e}")
        return

    for project, project_outdir in (app.config.doxyxref_projects or {}).items():
        html_dir = Path(project_outdir) / "html"
        if not html_dir.is_dir():
            logger.warning(f"doxyxref: no Doxygen HTML output found for project '{project}'")
            continue

        logger.info(f"doxyxref: resolving Sphinx cross-references for project '{project}'...")
        unresolved = process_html_dir(html_dir, inventory, docs_root=outdir)
        for stype, target in sorted({(xref.stype, xref.target) for xref in unresolved}):
            logger.warning(
                f"doxyxref: unresolved {stype} cross-reference: '{target}' (project '{project}')",
                type="doxyxref",
            )


def setup(app):
    app.add_config_value("doxyxref_projects", {}, "env")

    app.connect("build-finished", doxyxref_resolve)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
