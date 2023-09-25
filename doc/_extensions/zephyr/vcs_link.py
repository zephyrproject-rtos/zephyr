"""
VCS Link
########

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0

Introduction
============

This Sphinx extension can be used to obtain various VCS URLs for a given Sphinx page.
This is useful, for example, when adding features like "Open on GitHub" on top
of pages.
The extension installs the following Jinja filter:

* ``vcs_link_get_blob_url``: Returns a URL to the source of a page in the VCS.
* ``vcs_link_get_edit_url``: Returns a URL to edit the given page in the VCS.
* ``vcs_link_get_open_issue_url``: Returns a URL to open a new issue regarding the given page.

Configuration options
=====================

- ``vcs_link_version``: VCS version to use in the URL (e.g. "main")
- ``vcs_link_base_url``: Base URL used as a prefix for generated URLs.
- ``vcs_link_prefixes``: Mapping of pages (regex) <> VCS prefix.
- ``vcs_link_exclude``: List of pages (regex) that will not report a URL. Useful
  for, e.g., auto-generated pages not in VCS.
"""

from functools import partial
import os
import re
from textwrap import dedent
from typing import Optional
from urllib.parse import quote

from sphinx.application import Sphinx


__version__ = "0.1.0"


def vcs_link_get_url(app: Sphinx, pagename: str, mode: str = "blob") -> Optional[str]:
    """Obtain VCS URL for the given page.

    Args:
        app: Sphinx instance.
        mode: Typically "edit", or "blob".
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
            mode,
            app.config.vcs_link_version,
            found_prefix,
            app.env.project.doc2path(pagename, basedir=False),
        ]
    )


def vcs_link_get_open_issue_url(app: Sphinx, pagename: str, sha1: str) -> Optional[str]:
    """Link to open a new Github issue regarding "pagename" with title, body, and
    labels already pre-filled with useful information.

    Args:
        app: Sphinx instance.
        pagename: Page name (path).

    Returns:
        URL to open a new issue if applicable, None otherwise.
    """

    if not os.path.isfile(app.env.project.doc2path(pagename)):
        return None

    title = quote(f"[doc] Issue with {pagename}")
    labels = quote("area: Documentation")
    body = quote(
        dedent(
            f"""\
    << Please describe the issue here >>

    **Environment**

    * Page: `{pagename}`
    * Version: {app.config.vcs_link_version}
    * SHA-1: {sha1}
    """
        )
    )

    return f"{app.config.vcs_link_base_url}/issues/new?title={title}&labels={labels}&body={body}"


def add_jinja_filter(app: Sphinx):
    if app.builder.name != "html":
        return

    app.builder.templates.environment.filters["vcs_link_get_blob_url"] = partial(
        vcs_link_get_url, app, mode="blob"
    )

    app.builder.templates.environment.filters["vcs_link_get_edit_url"] = partial(
        vcs_link_get_url, app, mode="edit"
    )

    app.builder.templates.environment.filters["vcs_link_get_open_issue_url"] = partial(
        vcs_link_get_open_issue_url, app
    )


def setup(app: Sphinx):
    app.add_config_value("vcs_link_version", "", "")
    app.add_config_value("vcs_link_base_url", "", "")
    app.add_config_value("vcs_link_prefixes", {}, "")
    app.add_config_value("vcs_link_exclude", [], "")

    app.connect("builder-inited", add_jinja_filter)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
