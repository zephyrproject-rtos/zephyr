"""
VCS Link
########

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0

Introduction
============

This Sphinx extension can be used to obtain the VCS URL for a given Sphinx page.
This is useful, for example, when adding features like "Open on GitHub" on top
of pages. The extension installs a Jinja filter which can be used on the
template to obtain VCS page URLs.

Configuration options
=====================

- ``vcs_link_base_url``: Base URL used as a prefix for generated URLs.
- ``vcs_link_prefixes``: Mapping of pages (regex) <> VCS prefix.
- ``vcs_link_exclude``: List of pages (regex) that will not report a URL. Useful
  for, e.g., auto-generated pages not in VCS.
"""

from functools import partial
import os
import re
from typing import Optional

from sphinx.application import Sphinx


__version__ = "0.1.0"


def vcs_link_get_url(app: Sphinx, pagename: str) -> Optional[str]:
    """Obtain VCS URL for the given page.

    Args:
        app: Sphinx instance.
        pagename: Page name (path).

    Returns:
        VCS URL if applicable, None otherwise.
    """

    if not os.path.isfile(app.env.project.doc2path(pagename)):
        return None

    for exclude in app.config.vcs_link_exclude:
        if re.match(exclude, pagename):
            return None

    found_prefix = ""
    for pattern, prefix in app.config.vcs_link_prefixes.items():
        if re.match(pattern, pagename):
            found_prefix = prefix
            break

    return "/".join(
        [
            app.config.vcs_link_base_url,
            found_prefix,
            app.env.project.doc2path(pagename, basedir=False),
        ]
    )


def add_jinja_filter(app: Sphinx):
    if app.builder.name != "html":
        return

    app.builder.templates.environment.filters["vcs_link_get_url"] = partial(
        vcs_link_get_url, app
    )


def setup(app: Sphinx):
    app.add_config_value("vcs_link_base_url", "", "")
    app.add_config_value("vcs_link_prefixes", {}, "")
    app.add_config_value("vcs_link_exclude", [], "")

    app.connect("builder-inited", add_jinja_filter)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
