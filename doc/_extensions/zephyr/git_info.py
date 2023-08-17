"""
Git Info Extension
##################

Copyright (c) 2023 The Linux Foundation
SPDX-License-Identifier: Apache-2.0

Introduction
============

This extension adds a new Jinja filter, ``git_info``, that returns the date and SHA1 of the last
commit made to a page if this page is managed by Git.
"""

import os
import re
import subprocess
from datetime import datetime
from functools import partial
from pathlib import Path
from typing import Optional
from typing import Tuple

from sphinx.application import Sphinx
from sphinx.util.i18n import format_date

__version__ = "0.1.0"


def git_info_filter(app: Sphinx, pagename) -> Optional[Tuple[str, str]]:
    """Return a tuple with the date and SHA1 of the last commit made to a page.

    Arguments:
        app {Sphinx} -- Sphinx application object
        pagename {str} -- Page name

    Returns:
        Optional[Tuple[str, str]] -- Tuple with the date and SHA1 of the last commit made to the
        page, or None if the page is not in the repo.
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

    orig_path = os.path.join(
        app.config.ZEPHYR_BASE,
        found_prefix,
        app.env.project.doc2path(pagename, basedir=False),
    )

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
        date, sha1 = date_and_sha1.split(" ", 1)
        date_object = datetime.fromtimestamp(int(date))
        last_update_fmt = app.config.html_last_updated_fmt
        if last_update_fmt is not None:
            date = format_date(last_update_fmt, date=date_object, language=app.config.language)

        return (date, sha1)
    except subprocess.CalledProcessError:
        return None


def add_jinja_filter(app: Sphinx):
    if app.builder.name != "html":
        return

    app.builder.templates.environment.filters["git_info"] = partial(git_info_filter, app)


def setup(app: Sphinx):
    app.add_config_value("ZEPHYR_BASE", Path(__file__).resolve().parents[3], "html")
    app.connect("builder-inited", add_jinja_filter)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
