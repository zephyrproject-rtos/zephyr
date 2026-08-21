"""
llms.txt
########

Copyright (c) 2026 Eoin Jordan
SPDX-License-Identifier: Apache-2.0

Introduction
============

This extension writes an ``llms.txt`` file into the HTML output directory at the
end of a build, following the `llms.txt convention <https://llmstxt.org/>`_.

``llms.txt`` is a single Markdown file listing a project's documentation as
titled links, grouped by section. It gives language models and documentation
assistants an accurate, curated map of the docs, rather than leaving them to
infer one by crawling rendered HTML — where navigation chrome, version banners
and the generated API surface all compete with the prose that actually answers
questions.

Zephyr already publishes ``sitemap.xml`` for search engines via
``sphinx_sitemap``. This is the same idea for a different consumer: a sitemap
is a flat list of URLs with no titles or structure, which is what a crawler
needs; ``llms.txt`` carries the titles and the section grouping, which is what a
retrieval system needs to decide what is worth reading.

The file is generated from the toctree, so it reflects the documentation's own
structure and stays correct as pages are added, moved or removed. Nothing is
hand-maintained.

Configuration options
=====================

- ``llms_txt_enable``: Whether to generate the file. Defaults to ``True``.
- ``llms_txt_filename``: Output filename. Defaults to ``llms.txt``.
- ``llms_txt_summary``: Short description placed under the title. Defaults to
  the first paragraph of the root document.
- ``llms_txt_exclude``: List of docname prefixes to leave out. Defaults to
  release notes and the API surface, which are large, generated, and better
  reached from the pages that link to them.
"""

from __future__ import annotations

import os
from typing import Any

from sphinx.application import Sphinx
from sphinx.util import logging

__version__ = "0.1.0"

logger = logging.getLogger(__name__)

# Sections whose pages are numerous and generated rather than written. Listing
# every release note and Doxygen group would make the file enormous and bury the
# documentation a reader actually wants.
DEFAULT_EXCLUDE = [
    "releases/",
    "doxygen/",
    "_doxygen/",
]


def _clean_title(title: str) -> str:
    """Collapse whitespace so a wrapped RST title becomes one line."""
    return " ".join(title.split())


def _summary(app: Sphinx) -> str:
    configured = app.config.llms_txt_summary
    if configured:
        return _clean_title(configured)
    return (
        "Zephyr is a small, scalable, real-time operating system for connected, "
        "resource-constrained devices, supporting multiple architectures and "
        "released under the Apache 2.0 licence."
    )


def _walk_toctree(app: Sphinx, docname: str, seen: set[str]) -> list[str]:
    """Return docnames reachable from ``docname``, in toctree order."""
    env = app.builder.env
    ordered: list[str] = []
    pending = [docname]

    while pending:
        current = pending.pop(0)
        if current in seen:
            continue
        seen.add(current)
        ordered.append(current)
        # ``toctree_includes`` preserves the author-chosen ordering, which is
        # the whole reason for generating from the toctree rather than globbing
        # the source tree.
        for child in env.toctree_includes.get(current, []):
            if child not in seen:
                pending.append(child)

    return ordered


def _excluded(docname: str, patterns: list[str]) -> bool:
    """Match a section root as well as the pages under it.

    A prefix test alone misses the section's own landing page: with a pattern of
    ``releases/``, the docname ``releases`` does not start with it, so the
    section header was emitted with the index page beneath it while the notes
    themselves were dropped.
    """
    for pattern in patterns:
        stem = pattern.rstrip("/")
        if docname == stem or docname.startswith(f"{stem}/"):
            return True
    return False


def write_llms_txt(app: Sphinx, exception: Exception | None) -> None:
    if exception is not None or not app.config.llms_txt_enable:
        return
    if app.builder.name not in ("html", "dirhtml"):
        return

    env = app.builder.env
    root = app.config.root_doc
    exclude = list(app.config.llms_txt_exclude)
    base_url = (app.config.html_baseurl or "").rstrip("/")

    lines: list[str] = [
        f"# {app.config.project}",
        "",
        f"> {_summary(app)}",
        "",
        (
            f"Generated from the {app.config.project} {app.config.version} "
            "documentation build. Each link is a documentation page; the "
            "grouping follows the documentation's own structure."
        ),
        "",
    ]

    seen: set[str] = set()
    top_level = env.toctree_includes.get(root, [])
    if not top_level:
        logger.warning("llms.txt: no toctree entries under %s, file not written", root)
        return

    total = 0
    for section in top_level:
        if _excluded(section, exclude):
            continue

        section_title = _clean_title(env.titles[section].astext()) if section in env.titles else section
        pages = _walk_toctree(app, section, seen)
        entries: list[str] = []

        for docname in pages:
            if _excluded(docname, exclude) or docname not in env.titles:
                continue
            title = _clean_title(env.titles[docname].astext())
            suffix = "/" if app.builder.name == "dirhtml" else ".html"
            target = f"{docname}{suffix}"
            url = f"{base_url}/{target}" if base_url else target
            entries.append(f"- [{title}]({url})")

        if not entries:
            continue

        lines.append(f"## {section_title}")
        lines.append("")
        lines.extend(entries)
        lines.append("")
        total += len(entries)

    output = os.path.join(app.outdir, app.config.llms_txt_filename)
    with open(output, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))

    logger.info("llms.txt: wrote %d entries to %s", total, output)


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("llms_txt_enable", True, "html")
    app.add_config_value("llms_txt_filename", "llms.txt", "html")
    app.add_config_value("llms_txt_summary", "", "html")
    app.add_config_value("llms_txt_exclude", DEFAULT_EXCLUDE, "html")

    app.connect("build-finished", write_llms_txt)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
