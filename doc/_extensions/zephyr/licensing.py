"""
SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0

Licensing exceptions page generator
====================================

This extension renders the :ref:`Zephyr_Licensing` page directly from the machine-readable metadata
that already lives in the repository's ``REUSE.toml`` file, so the page can never drift from the
actual licensing of the tree.

Rationale
*********

Zephyr as a whole is licensed under Apache-2.0, but it imports or reuses a handful of components
that use other licenses. Those *deviations* have to be listed on the licensing page (see
:ref:`external-contributions`). Maintaining that list by hand is error prone: entries get
duplicated, forgotten, or go stale as files are added or removed.

Instead of a hand-written list, every deviation is described once, as a standard `REUSE`_
annotation, and this extension turns those annotations into the documentation page.

The licensing/copyright facts are parsed and resolved with the ``reuse`` Python library itself (the
very same tool used to check REUSE compliance in CI), so the license and copyright shown for each
component are guaranteed to match what the tooling sees.

Metadata format
***************

The licensing/copyright facts stay 100% standard REUSE/SPDX. Each deviation is a single
``[[annotations]]`` block in ``REUSE.toml`` that uses the standard keys plus, at most, three small
Zephyr-specific keys to *explain* the deviation (the `REUSE`_ tool ignores unknown keys):

.. REUSE-IgnoreStart

.. code-block:: toml

    [[annotations]]
    path = "subsys/net/lib/wireguard/crypto/**"
    SPDX-License-Identifier = "BSD-3-Clause"
    SPDX-FileCopyrightText = "Copyright (c) 2021 Daniel Hope (www.floorsense.nz)"
    SPDX-FileComment = "Only compiled into the firmware when the feature is enabled."
    Zephyr-Description = "WireGuard VPN"
    Zephyr-Origin = "wireguard-lwip <https://github.com/smartalock/wireguard-lwip>"
    Zephyr-Kconfig-Condition = "CONFIG_WIREGUARD"

.. REUSE-IgnoreEnd

Standard keys (the legally meaningful metadata):

``SPDX-License-Identifier``
    The license of the component. This is what is rendered as the component's license and is
    validated by the ``reuse`` tool.
``SPDX-FileCopyrightText``
    Copyright notice(s).
``SPDX-FileComment``
    Free-text justification explaining *why* the deviation is acceptable (rendered as the "Impact"
    of the component). This is a standard SPDX tag.

``reuse`` interprets none of the following keys but preserves them on the annotation's
``custom_properties``. They are all Zephyr-specific: SPDX has no non-deprecated *file*-level field
for an upstream URL (the ``ArtifactOf*`` file tags were deprecated in favour of relationships, which
REUSE.toml cannot express), and its ``Package*`` fields are not valid on files.

``Zephyr-Description``
    Short human description of the component, used as its section title on the page. Its presence
    is what turns a plain REUSE annotation into a documented licensing exception.
``Zephyr-Origin``
    URL of the upstream project the files come from, rendered as the component's "Origin" link.
``Zephyr-Kconfig-Condition``
    Space/comma separated Kconfig option(s) that must be enabled for the files to end up in a build.
    Rendered with a :rst:role:`kconfig:option` cross reference.

The list of files for each component is expanded from the ``path`` globs against the actual work
tree, so it is always accurate and never needs to be kept in sync by hand.

Usage
*****

Add the directive to a document (it takes no options)::

    .. zephyr-licensing-exceptions::

"""

from __future__ import annotations

import re
import sys
from functools import lru_cache
from pathlib import Path
from typing import Any, Final

from docutils import nodes
from docutils.statemachine import StringList
from reuse.global_licensing import ReuseTOML
from reuse.project import Project
from sphinx.application import Sphinx
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective

__version__ = "0.1.0"

ZEPHYR_BASE: Final[Path] = Path(__file__).parents[3]
REUSE_TOML: Final[Path] = ZEPHYR_BASE / "REUSE.toml"

# Reuse the licensing-policy helpers shared with the CI compliance check: the marker key that turns
# a REUSE annotation into a documented exception, and license resolution through the reuse library.
sys.path.append(str(ZEPHYR_BASE / "scripts"))
from list_undocumented_licenses import MARKER_KEY, resolved_licenses  # noqa: E402

logger = logging.getLogger(__name__)


@lru_cache(maxsize=1)
def _project() -> Project:
    """The reuse project, used to resolve the *actual* license of a file."""
    return Project.from_directory(ZEPHYR_BASE)


def _spdx_url(license_expr: str) -> str | None:
    """Return the canonical SPDX page for a *simple* license id, else ``None``.

    License *expressions* (containing ``WITH``/``OR``/``AND`` or parentheses)
    have no single page, so they are rendered as plain text.
    """
    if re.fullmatch(r"[A-Za-z0-9.\-+]+", license_expr):
        return f"https://spdx.org/licenses/{license_expr}.html"
    return None


class LicensingException:
    """A single documented licensing deviation, resolved against the tree."""

    def __init__(self, annotation: Any):
        extra = annotation.custom_properties
        self.title: str = extra[MARKER_KEY]
        self.licenses: set[str] = {str(e) for e in annotation.spdx_expressions}
        self.license: str = " / ".join(sorted(self.licenses))
        self.impact: str = extra.get("SPDX-FileComment", "")
        self.origin: str = extra.get("Zephyr-Origin", "")
        self.condition: str = extra.get("Zephyr-Kconfig-Condition", "")

        self._annotation = annotation
        self.files: list[str] = self._resolve_files(annotation.paths)

    def _resolve_files(self, patterns: set[str]) -> list[str]:
        found: set[str] = set()
        for pattern in patterns:
            matches = [p for p in ZEPHYR_BASE.glob(pattern) if p.is_file()]
            if not matches:
                logger.warning(
                    "licensing: pattern '%s' for component '%s' matches no files",
                    pattern,
                    self.title,
                    type="licensing",
                )
            for match in matches:
                found.add(match.relative_to(ZEPHYR_BASE).as_posix())
        return sorted(found)

    def validate(self) -> None:
        """Warn if the declared license is not what ``reuse`` resolves.

        This cross-checks the documented license against the license the
        ``reuse`` tool actually computes for each file (from its SPDX header
        and/or ``REUSE.toml``), catching silent drift.
        """
        for rel in self.files:
            resolved = resolved_licenses(_project(), ZEPHYR_BASE / rel)
            # Only flag when reuse resolved *something* that shares no license expression with what
            # is documented (empty resolution is reported separately by the LicenseAndCopyrightCheck
            # compliance test).
            if resolved and not self.licenses & resolved:
                logger.warning(
                    "licensing: '%s' is documented as '%s' but reuse resolves it to %s",
                    rel,
                    self.license,
                    ", ".join(sorted(resolved)),
                    type="licensing",
                )


def load_exceptions() -> list[LicensingException]:
    """Parse ``REUSE.toml`` and return the documented licensing exceptions.

    An annotation is a documented exception when it has an explicitly set
    ``Zephyr-Description`` and ``SPDX-FileComment`` justification.
    """
    reuse_toml = ReuseTOML.from_file(REUSE_TOML)
    exceptions = [
        LicensingException(annotation)
        for annotation in reuse_toml.annotations
        if MARKER_KEY in annotation.custom_properties
    ]
    exceptions.sort(key=lambda e: e.title.lower())
    return exceptions


class ZephyrLicensingExceptions(SphinxDirective):
    """Render the licensing exceptions table from ``REUSE.toml``."""

    has_content = False

    def _body_rst(self, exc: LicensingException) -> list[str]:
        """reStructuredText for a component's body (everything but its title).

        Emitting RST for the body (rather than raw doctree nodes) lets the existing
        ``:zephyr_file:`` and ``:kconfig:option:`` roles do the heavy lifting, so the page reuses
        the theme's styling and needs no new CSS.
        """
        lines: list[str] = []

        # Metadata as a reST field list (rendered as a compact definition table
        # by the RTD theme, no custom CSS required).
        if exc.origin:
            lines.append(f":Origin: {_format_origin(exc.origin)}")

        url = _spdx_url(exc.license)
        license_field = f"`{exc.license} <{url}>`__" if url else f"``{exc.license}``"
        lines.append(f":License: {license_field}")

        if exc.condition:
            options = re.split(r"[,\s]+", exc.condition.strip())
            refs = ", ".join(f":kconfig:option:`{opt}`" for opt in options if opt)
            lines.append(f":Enabled by: {refs}")
        lines.append("")

        if exc.impact:
            lines.append(exc.impact)
            lines.append("")

        lines.append(f"Files ({len(exc.files)}):")
        lines.append("")
        for rel in exc.files:
            lines.append(f"* :zephyr_file:`{rel}`")
        lines.append("")

        return lines

    def _build_section(self, exc: LicensingException) -> nodes.section:
        section = nodes.section()
        section += nodes.title(text=exc.title)
        section["ids"] = [nodes.make_id(exc.title)]
        self.state.document.note_implicit_target(section, section)

        content = StringList(self._body_rst(exc), source="zephyr-licensing-exceptions")
        self.state.nested_parse(content, self.content_offset, section)
        return section

    def run(self) -> list[nodes.Node]:
        try:
            exceptions = load_exceptions()
        except (OSError, ValueError, KeyError) as err:
            logger.warning("licensing: could not read REUSE.toml: %s", err, type="licensing")
            return [nodes.paragraph(text=f"Unable to generate licensing page: {err}")]

        if not exceptions:
            return [nodes.paragraph(text="No licensing exceptions are currently declared.")]

        result: list[nodes.Node] = []
        for exc in exceptions:
            exc.validate()
            result.append(self._build_section(exc))
        return result


def _format_origin(url: str) -> str:
    """Render an origin URL as a reST link labelled with its host and path."""
    label = re.sub(r"^https?://", "", url).rstrip("/")
    return f"`{label} <{url}>`__"


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive("zephyr-licensing-exceptions", ZephyrLicensingExceptions)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
