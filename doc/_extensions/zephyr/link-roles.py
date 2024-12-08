# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# based on http://protips.readthedocs.io/link-roles.html

import re
import subprocess
from collections.abc import Sequence
from pathlib import Path
from typing import Any, Final

from docutils import nodes
from sphinx.util import logging

ZEPHYR_BASE: Final[str] = Path(__file__).parents[3]

try:
    import west.manifest

    try:
        west_manifest = west.manifest.Manifest.from_file()
    except west.util.WestNotFound:
        west_manifest = None
except ImportError:
    west_manifest = None


logger = logging.getLogger(__name__)


def get_github_rev():
    try:
        output = subprocess.check_output(
            "git describe --exact-match", shell=True, stderr=subprocess.DEVNULL
        )
    except subprocess.CalledProcessError:
        return "main"

    return output.strip().decode("utf-8")


def setup(app):
    app.add_role("zephyr_file", modulelink("zephyr"))
    app.add_role("zephyr_raw", modulelink("zephyr", format="raw"))
    app.add_role("module_file", modulelink())

    app.add_config_value("link_roles_manifest_baseurl", None, "env")
    app.add_config_value("link_roles_manifest_project", None, "env")
    app.add_config_value("link_roles_manifest_project_broken_links_ignore_globs", [], "env")

    # The role just creates new nodes based on information in the
    # arguments; its behavior doesn't depend on any other documents.
    return {
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }


def modulelink(default_module=None, format="blob"):
    def role(
        name: str,
        rawtext: str,
        text: str,
        lineno: int,
        inliner,
        options: dict[str, Any] | None = None,
        content: Sequence[str] = (),
    ):
        if options is None:
            options = {}
        module = default_module
        rev = get_github_rev()
        config = inliner.document.settings.env.app.config
        baseurl = config.link_roles_manifest_baseurl
        source, line = inliner.reporter.get_source_and_line(lineno)
        trace = f"at '{source}:{line}'"

        m = re.search(r"(.*)\s*<(.*)>", text)
        if m:
            link_text = m.group(1)
            link = m.group(2)
        else:
            link_text = text
            link = text

        module_match = re.search(r"(.+?):\s*(.+)", link)
        if module_match:
            module = module_match.group(1).strip()
            link = module_match.group(2).strip()

        # Try to get a module repository's GitHub URL from the manifest.
        #
        # This allows e.g. building the docs in downstream Zephyr-based
        # software with forks of the zephyr repository, and getting
        # :zephyr_file: / :zephyr_raw: output that links to the fork,
        # instead of mainline zephyr.
        projects = [p.name for p in west_manifest.projects] if west_manifest else []
        if module in projects:
            project = west_manifest.get_projects([module])[0]
            baseurl = project.url
            rev = project.revision
        # No module provided
        elif module is None:
            raise ValueError(
                f"Role 'module_file' must take a module as an argument\n\t{trace}"
            )
        # Invalid module provided
        elif module != config.link_roles_manifest_project:
            logger.debug(f"Module {module} not found in the west manifest")
        # Baseurl for manifest project not set
        elif baseurl is None:
            raise ValueError(
                f"Configuration value `link_roles_manifest_baseurl` not set\n\t{trace}"
            )

        if module == config.link_roles_manifest_project:
            p = Path(source).relative_to(inliner.document.settings.env.srcdir)
            if not any(
                p.match(glob)
                for glob in config.link_roles_manifest_project_broken_links_ignore_globs
            ) and not Path(ZEPHYR_BASE, link).exists():
                logger.warning(
                    f"{link} not found in {config.link_roles_manifest_project} {trace}"
                )

        url = f"{baseurl}/{format}/{rev}/{link}"
        node = nodes.reference(rawtext, link_text, refuri=url, **options)
        return [node], []

    return role
