"""
Git/GitHub utilities for Sphinx
###############################

Copyright (c) 2021 Nordic Semiconductor ASA
Copyright (c) 2023 The Linux Foundation

SPDX-License-Identifier: Apache-2.0

Introduction
============

This Sphinx extension can be used to obtain various Git and GitHub related metadata for a page.
This is useful, for example, when adding features like "Open on GitHub" on top
of pages, direct links to open a GitHub issue regarding a page, or date of the most recent commit
to a page.

The extension installs the following Jinja filter:

* ``gh_link_get_blob_url``: Returns a URL to the source of a page on GitHub.
* ``gh_link_get_edit_url``: Returns a URL to edit the given page on GitHub.
* ``gh_link_get_open_issue_url``: Returns a URL to open a new issue regarding the given page.
* ``git_info``: Returns the date and SHA1 of the last commit made to a page (if this page is
    managed by Git).

Configuration options
=====================

- ``gh_link_version``: GitHub version to use in the URL (e.g. "main")
- ``gh_link_base_url``: Base URL used as a prefix for generated URLs.
- ``gh_link_prefixes``: Mapping of pages (regex) <> GitHub prefix.
- ``gh_link_exclude``: List of pages (regex) that will not report a URL. Useful
  for, e.g., auto-generated pages not in Git.
"""

import os
import re
import subprocess
import sys
from datetime import datetime
from functools import partial
from pathlib import Path
from textwrap import dedent
from typing import Final
from urllib.parse import quote

from sphinx.application import Sphinx
from sphinx.util.i18n import format_date

sys.path.insert(0, str(Path(__file__).parents[3] / "scripts"))

from get_maintainer import Maintainers

ZEPHYR_BASE : Final[str] = Path(__file__).parents[3]
MAINTAINERS : Final[Maintainers] = Maintainers(filename=f"{ZEPHYR_BASE}/MAINTAINERS.yml")


__version__ = "0.1.0"


def get_page_prefix(app: Sphinx, pagename: str) -> str:
    """Return the prefix that needs to be added to the page path to get its location in the
    repository.

    If pagename refers to a page that is automatically generated by Sphinx or if it matches one of
    the patterns in ``gh_link_exclude`` configuration option, return None.

    Args:
        app: Sphinx instance.
        pagename: Page name (path).

    Returns:
        Prefix if applicable, None otherwise.
    """

    if not os.path.isfile(app.env.doc2path(pagename)):
        return None

    for exclude in app.config.gh_link_exclude:
        if re.match(exclude, pagename):
            return None

    found_prefix = ""
    for pattern, prefix in app.config.gh_link_prefixes.items():
        if re.match(pattern, pagename):
            found_prefix = prefix
            break

    return found_prefix


def gh_link_get_url(app: Sphinx, pagename: str, mode: str = "blob") -> str | None:
    """Obtain GitHub URL for the given page.

    Args:
        app: Sphinx instance.
        mode: Typically "edit", or "blob".
        pagename: Page name (path).

    Returns:
        GitHub URL if applicable, None otherwise.
    """

    page_prefix = get_page_prefix(app, pagename)
    if page_prefix is None:
        return None

    return "/".join(
        [
            app.config.gh_link_base_url,
            mode,
            app.config.gh_link_version,
            page_prefix,
            str(app.env.doc2path(pagename, False)),
        ]
    )


def gh_link_get_open_issue_url(app: Sphinx, pagename: str, sha1: str) -> str | None:
    """Link to open a new Github issue regarding "pagename" with title, body, and
    labels already pre-filled with useful information.

    Args:
        app: Sphinx instance.
        pagename: Page name (path).

    Returns:
        URL to open a new issue if applicable, None otherwise.
    """

    page_prefix = get_page_prefix(app, pagename)
    if page_prefix is None:
        return None

    rel_path = os.path.join(
        os.path.relpath(ZEPHYR_BASE),
        page_prefix,
        app.env.doc2path(pagename, False),
    )

    title = quote(f"doc: Documentation issue in '{pagename}'")
    labels = quote("area: Documentation")
    areas = MAINTAINERS.path2areas(rel_path)
    if areas:
        labels += "," + ",".join([label for area in areas for label in area.labels])
    body = quote(
        dedent(
            f"""\
    **Describe the bug**

    << Please describe the issue here >>
    << You may also want to update the automatically generated issue title above. >>

    **Environment**

    * Page: `{pagename}`
    * Version: {app.config.gh_link_version}
    * SHA-1: {sha1}
    """
        )
    )

    return f"{app.config.gh_link_base_url}/issues/new?title={title}&labels={labels}&body={body}"


def git_info_filter(app: Sphinx, pagename) -> tuple[str, str] | None:
    """Return a tuple with the date and SHA1 of the last commit made to a page.

    Arguments:
        app {Sphinx} -- Sphinx application object
        pagename {str} -- Page name

    Returns:
        Optional[Tuple[str, str]] -- Tuple with the date and SHA1 of the last commit made to the
        page, or None if the page is not in the repo (generated file, or manually authored file not
        yet tracked by git).
    """

    page_prefix = get_page_prefix(app, pagename)
    if page_prefix is None:
        return None

    orig_path = os.path.join(
        ZEPHYR_BASE,
        page_prefix,
        app.env.doc2path(pagename, False),
    )

    # Check if the file is tracked by git
    try:
        subprocess.check_output(
            ["git", "ls-files", "--error-unmatch", orig_path],
            stderr=subprocess.STDOUT,
        )
    except subprocess.CalledProcessError:
        return None

    try:
        date_and_sha1 = (
            subprocess.check_output(
                [
                    "git",
                    "log",
                    "-1",
                    "--format=%ad %H",
                    "--date=unix",
                    orig_path,
                ],
                stderr=subprocess.STDOUT,
            )
            .decode("utf-8")
            .strip()
        )
        if not date_and_sha1: # added but not committed
            return None
        date, sha1 = date_and_sha1.split(" ", 1)
        date_object = datetime.fromtimestamp(int(date))
        last_update_fmt = app.config.html_last_updated_fmt
        if last_update_fmt is not None:
            date = format_date(last_update_fmt, date=date_object, language=app.config.language)

        return (date, sha1)
    except subprocess.CalledProcessError:
        return None

def add_jinja_filter(app: Sphinx):
    if app.builder.format != "html":
        return

    app.builder.templates.environment.filters["gh_link_get_blob_url"] = partial(
        gh_link_get_url, app, mode="blob"
    )

    app.builder.templates.environment.filters["gh_link_get_edit_url"] = partial(
        gh_link_get_url, app, mode="edit"
    )

    app.builder.templates.environment.filters["gh_link_get_open_issue_url"] = partial(
        gh_link_get_open_issue_url, app
    )

    app.builder.templates.environment.filters["git_info"] = partial(git_info_filter, app)


def setup(app: Sphinx):
    app.add_config_value("gh_link_version", "", "")
    app.add_config_value("gh_link_base_url", "", "")
    app.add_config_value("gh_link_prefixes", {}, "")
    app.add_config_value("gh_link_exclude", [], "")

    app.connect("builder-inited", add_jinja_filter)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
