"""
Doxygen to Sphinx cross-reference resolver
##########################################

Copyright (c) 2026 The Linux Foundation
SPDX-License-Identifier: Apache-2.0

This extension makes it possible to reference Sphinx documentation content
(e.g. Kconfig options, Devicetree bindings, or any ``:ref:`` target) from
Doxygen comments, and have the Doxygen-generated HTML pages link back to the
corresponding pages of the Sphinx-based documentation.

Doxygen aliases (defined in ``zephyr.doxyfile.in``) such as ``@kconfig{}``,
``@dtcompatible{}`` or ``@rstref{}`` expand to HTML placeholders like::

    <code class='doxyxref' data-stype='kconfig:option'
          data-starget='CONFIG_GPIO'>CONFIG_GPIO</code>

Once the Sphinx HTML build is finished, this extension scans the
Doxygen-generated HTML pages and resolves such placeholders into actual
hyperlinks, using the Sphinx object inventory (``objects.inv``) produced by
the very same build. Links are written as relative URLs so that they remain
valid wherever the documentation set is hosted (docs.zephyrproject.org/latest,
versioned copies, local builds, downstream distributions, ...).

Placeholders that cannot be resolved are left untouched (they degrade to
plain, code-styled text) and a Sphinx warning is emitted for each of them.

The module can also be used as a standalone command line tool, which is
useful when only building the Doxygen documentation (e.g. the ``doxygen``
CMake target). In that case, placeholders can be resolved against the object
inventory of an already published documentation set::

    python doc/_extensions/zephyr/doxyxref.py \\
        --html-dir doc/_build/doxygen/html \\
        --inventory https://docs.zephyrproject.org/latest/objects.inv \\
        --base-url https://docs.zephyrproject.org/latest/

Configuration options
=====================

- ``doxyxref_projects``: Dictionary of Doxygen projects, mapping project name
  to the Doxygen output directory (same format as ``doxybridge_projects``).
"""

from __future__ import annotations

import argparse
import html
import os
import re
import sys
import zlib
from pathlib import Path

__version__ = "0.1.0"

PLACEHOLDER_RE = re.compile(
    r"<(?P<tag>code|span) class='doxyxref' "
    r"data-stype='(?P<stype>[^']+)' "
    r"data-starget='(?P<starget>[^']*)'>"
    r".*?</(?P=tag)>",
    re.DOTALL,
)
"""Placeholder markup, as emitted by the ALIASES defined in zephyr.doxyfile.in."""


def split_titled_ref(raw: str) -> tuple[str | None, str]:
    """Split an explicit-title reference into its title and target parts.

    Explicit titles use the same ``custom text <target>`` notation as Sphinx
    roles. When no explicit title is present, the returned title is ``None``
    and the whole input is the target.
    """
    if raw.endswith(">"):
        open_pos = raw.rfind("<")
        title = raw[:open_pos].rstrip() if open_pos > 0 else ""
        target = raw[open_pos + 1 : -1]
        if title and target and ">" not in target:
            return title, target

    return None, raw


def parse_inventory_entry(line: str) -> tuple[str, str, str, str] | None:
    """Parse an entry line of a version 2 Sphinx object inventory.

    Entries have the form ``name type priority location dispname``, where both
    the name and the display name may contain spaces.

    Returns:
        The ``(name, type, location, dispname)`` tuple, or ``None`` if the
        line is not a valid inventory entry.
    """
    words = line.split(" ")

    # the name is the shortest sequence of words followed by a type (which
    # always contains a colon), a priority (an integer), and a location
    for i in range(1, len(words) - 2):
        if ":" not in words[i]:
            continue
        try:
            int(words[i + 1])
        except ValueError:
            continue

        name = " ".join(words[:i])
        location = words[i + 2]
        dispname = " ".join(words[i + 3 :]) or "-"
        return name, words[i], location, dispname

    return None


def parse_inventory(data: bytes) -> dict[str, dict[str, tuple[str, str]]]:
    """Parse a version 2 Sphinx object inventory (``objects.inv``).

    Args:
        data: Raw content of the inventory file.

    Returns:
        A mapping of object type (e.g. ``std:label``) to a dictionary mapping
        object name to a ``(uri, display name)`` tuple. URIs are relative to
        the documentation root.
    """
    header, _, remainder = data.partition(b"\n")
    if header.rstrip() != b"# Sphinx inventory version 2":
        raise ValueError(f"Unsupported inventory format: {header.decode(errors='replace')}")

    # skip project name/version lines, check compression marker
    try:
        _, _, compression, compressed = remainder.split(b"\n", 3)
    except ValueError as e:
        raise ValueError("Malformed inventory header") from e
    if b"zlib" not in compression:
        raise ValueError("Unsupported inventory compression")

    inventory: dict[str, dict[str, tuple[str, str]]] = {}
    for line in zlib.decompress(compressed).decode("utf-8").splitlines():
        entry = parse_inventory_entry(line.rstrip())
        if entry is None:
            continue
        name, stype, location, dispname = entry
        if location.endswith("$"):
            location = location[:-1] + name
        inventory.setdefault(stype, {})[name] = (location, dispname)

    return inventory


def lookup(
    inventory: dict[str, dict[str, tuple[str, str]]], stype: str, target: str
) -> tuple[str, str] | None:
    """Look up a reference target in the object inventory.

    ``std:ref`` targets are resolved like the Sphinx ``:ref:`` role, i.e.
    against reference labels (which are stored lowercased in the inventory),
    with document names (``:doc:`` targets) as a fallback.
    """
    if stype == "std:ref":
        entry = inventory.get("std:label", {}).get(target.lower())
        if entry is None:
            entry = inventory.get("std:doc", {}).get(target.strip("/"))
        return entry

    return inventory.get(stype, {}).get(target)


def process_html_content(
    content: str,
    inventory: dict[str, dict[str, tuple[str, str]]],
    url_base: str,
) -> tuple[str, list[tuple[str, str]]]:
    """Resolve all cross-reference placeholders found in an HTML document.

    Args:
        content: HTML content to process.
        inventory: Parsed object inventory.
        url_base: Prefix prepended to the documentation-root-relative URI of
            each resolved target. Can be an absolute URL or a relative path
            (with trailing slash).

    Returns:
        The processed content, and the list of ``(type, target)`` tuples that
        could not be resolved.
    """
    unresolved: list[tuple[str, str]] = []

    def _replace(m: re.Match) -> str:
        stype = m.group("stype")
        raw_target = m.group("starget")

        if stype == "std:ref":
            title, target = split_titled_ref(raw_target)
        else:
            title, target = None, raw_target

        entry = lookup(inventory, stype, target)
        if entry is None:
            unresolved.append((stype, target))
            return m.group(0)

        uri, dispname = entry
        if title is None:
            title = dispname if dispname and dispname != "-" else target

        text = html.escape(title)
        if m.group("tag") == "code":
            text = f"<code>{text}</code>"

        href = html.escape(url_base + uri, quote=True)
        return f'<a class="doxyxref" href="{href}">{text}</a>'

    return PLACEHOLDER_RE.sub(_replace, content), unresolved


def process_html_dir(
    html_dir: Path,
    inventory: dict[str, dict[str, tuple[str, str]]],
    url_base: str | None = None,
    docs_root: Path | None = None,
) -> list[tuple[str, str]]:
    """Resolve cross-reference placeholders in all HTML files of a directory.

    Args:
        html_dir: Directory containing the Doxygen-generated HTML files.
        inventory: Parsed object inventory.
        url_base: Fixed URL prefix for resolved links (e.g.
            ``https://docs.zephyrproject.org/latest/``). Mutually exclusive
            with ``docs_root``.
        docs_root: Root directory of the Sphinx HTML output. When given,
            links are computed relative to each processed file.

    Returns:
        List of ``(type, target)`` tuples that could not be resolved.
    """
    if (url_base is None) == (docs_root is None):
        raise ValueError("Exactly one of url_base and docs_root must be provided")

    unresolved: list[tuple[str, str]] = []

    for file in sorted(html_dir.rglob("*.html")):
        content = file.read_text(encoding="utf-8", errors="surrogateescape")
        if "class='doxyxref'" not in content:
            continue

        if docs_root is not None:
            rel = os.path.relpath(docs_root, file.parent).replace(os.sep, "/")
            base = "" if rel == "." else f"{rel}/"
        else:
            base = url_base

        new_content, missing = process_html_content(content, inventory, base)
        unresolved.extend(missing)

        if new_content != content:
            file.write_text(new_content, encoding="utf-8", errors="surrogateescape")

    return unresolved


def doxyxref_resolve(app, exception) -> None:
    """Sphinx ``build-finished`` hook resolving Doxygen cross-references."""
    from sphinx.util import logging

    logger = logging.getLogger(__name__)

    if exception is not None or app.builder.format != "html":
        return

    outdir = Path(app.outdir)
    inventory_file = outdir / "objects.inv"
    if not inventory_file.exists():
        logger.warning(f"doxyxref: object inventory not found: {inventory_file}")
        return

    inventory = parse_inventory(inventory_file.read_bytes())

    for project, project_outdir in (app.config.doxyxref_projects or {}).items():
        html_dir = Path(project_outdir) / "html"
        if not html_dir.is_dir():
            logger.warning(f"doxyxref: no Doxygen HTML output found for project '{project}'")
            continue

        logger.info(f"doxyxref: resolving Sphinx cross-references for project '{project}'...")
        unresolved = process_html_dir(html_dir, inventory, docs_root=outdir)
        for stype, target in sorted(set(unresolved)):
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


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Resolve Sphinx cross-reference placeholders in Doxygen HTML output.",
        allow_abbrev=False,
    )
    parser.add_argument(
        "--html-dir",
        type=Path,
        required=True,
        help="Directory containing the Doxygen-generated HTML files",
    )
    parser.add_argument(
        "--inventory",
        required=True,
        help="Sphinx object inventory (objects.inv): local file path or http(s) URL",
    )
    parser.add_argument(
        "--base-url",
        required=True,
        help="Base URL of the documentation set links should point to, e.g. "
        "https://docs.zephyrproject.org/latest/",
    )
    args = parser.parse_args()

    if not args.html_dir.is_dir():
        sys.exit(f"error: not a directory: {args.html_dir}")

    if args.inventory.startswith(("http://", "https://")):
        from urllib.request import urlopen

        with urlopen(args.inventory) as f:
            data = f.read()
    else:
        data = Path(args.inventory).read_bytes()

    inventory = parse_inventory(data)

    base_url = args.base_url if args.base_url.endswith("/") else f"{args.base_url}/"
    unresolved = process_html_dir(args.html_dir, inventory, url_base=base_url)

    for stype, target in sorted(set(unresolved)):
        print(f"warning: unresolved {stype} cross-reference: '{target}'", file=sys.stderr)

    return 0


if __name__ == "__main__":
    sys.exit(main())
