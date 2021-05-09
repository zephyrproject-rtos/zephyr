# Zephyr documentation build configuration file.
# Reference: https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
import os
from pathlib import Path
import re

import sphinx_rtd_theme


ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    raise ValueError("ZEPHYR_BASE environment variable undefined")
ZEPHYR_BASE = Path(ZEPHYR_BASE)

ZEPHYR_BUILD = os.environ.get("ZEPHYR_BUILD")
if not ZEPHYR_BUILD:
    raise ValueError("ZEPHYR_BUILD environment variable undefined")
ZEPHYR_BUILD = Path(ZEPHYR_BUILD)

# Add the '_extensions' directory to sys.path, to enable finding Sphinx
# extensions within.
sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))

# Add the '_scripts' directory to sys.path, to enable finding utility
# modules.
sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_scripts"))

# Add the directory which contains the runners package as well,
# for autodoc directives on runners.xyz.
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "west_commands"))

import redirects

try:
    import west as west_found
except ImportError:
    west_found = False

# -- Project --------------------------------------------------------------

project = "Zephyr Project"
copyright = "2015-2021 Zephyr Project members and individual contributors"
author = "The Zephyr Project"

# parse version from 'VERSION' file
with open(ZEPHYR_BASE / "VERSION") as f:
    m = re.match(
        (
            r"^VERSION_MAJOR\s*=\s*(\d+)$\n"
            + r"^VERSION_MINOR\s*=\s*(\d+)$\n"
            + r"^PATCHLEVEL\s*=\s*(\d+)$\n"
            + r"^VERSION_TWEAK\s*=\s*\d+$\n"
            + r"^EXTRAVERSION\s*=\s*(.*)$"
        ),
        f.read(),
        re.MULTILINE,
    )

    if not m:
        sys.stderr.write("Warning: Could not extract kernel version\n")
        version = "Unknown"
    else:
        major, minor, patch, extra = m.groups(1)
        version = ".".join((major, minor, patch))
        if extra:
            version += "-" + extra

# -- General configuration ------------------------------------------------

extensions = [
    "breathe",
    "sphinx.ext.todo",
    "sphinx.ext.extlinks",
    "sphinx.ext.autodoc",
    "zephyr.application",
    "zephyr.html_redirects",
    "only.eager_only",
    "zephyr.dtcompatible-role",
    "zephyr.link-roles",
    "sphinx_tabs.tabs",
    "zephyr.warnings_filter",
    "zephyr.doxyrunner",
]

# Only use SVG converter when it is really needed, e.g. LaTeX.
if tags.has("svgconvert"):  # pylint: disable=undefined-variable
    extensions.append("sphinxcontrib.rsvgconverter")

templates_path = ["_templates"]

exclude_patterns = ["_build"]

if not west_found:
    exclude_patterns.append("**/*west-apis*")
else:
    exclude_patterns.append("**/*west-not-found*")

# This change will allow us to use bare back-tick notation to let
# Sphinx hunt for a reference, starting with normal "document"
# references such as :ref:, but also including :c: and :cpp: domains
# (potentially) helping with API (doxygen) references simply by using
# `name`
default_role = "any"

pygments_style = "sphinx"

todo_include_todos = False

rst_epilog = """
.. include:: /substitutions.txt
"""

# -- Options for HTML output ----------------------------------------------

html_theme = "sphinx_rtd_theme"
html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
html_theme_options = {"prev_next_buttons_location": None}
html_title = "Zephyr Project Documentation"
html_logo = "_static/images/logo.svg"
html_favicon = "images/zp_favicon.png"
html_static_path = [str(ZEPHYR_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_domain_indices = False
html_split_index = True
html_show_sourcelink = False
html_show_sphinx = False
html_search_scorer = "_static/js/scorer.js"

is_release = tags.has("release")  # pylint: disable=undefined-variable
docs_title = "Docs / {}".format(version if is_release else "Latest")
html_context = {
    "show_license": True,
    "docs_title": docs_title,
    "is_release": is_release,
    "theme_logo_only": False,
    "current_version": version,
    "versions": (
        ("latest", "/"),
        ("2.5.0", "/2.5.0/"),
        ("2.4.0", "/2.4.0/"),
        ("2.3.0", "/2.3.0/"),
        ("2.2.0", "/2.2.0/"),
        ("1.14.1", "/1.14.1/"),
    ),
}

# -- Options for LaTeX output ---------------------------------------------

latex_elements = {
    "preamble": r"\setcounter{tocdepth}{2}",
}

latex_documents = [
    ("index", "zephyr.tex", "Zephyr Project Documentation", "many", "manual"),
]

# -- Options for zephyr.doxyrunner plugin ---------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = ZEPHYR_BASE / "doc" / "zephyr.doxyfile.in"
doxyrunner_outdir = ZEPHYR_BUILD / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {"ZEPHYR_BASE": str(ZEPHYR_BASE)}

# -- Options for Breathe plugin -------------------------------------------

breathe_projects = {
    "Zephyr": str(doxyrunner_outdir / "xml"),
    "doc-examples": str(doxyrunner_outdir / "xml"),
}
breathe_default_project = "Zephyr"
breathe_domain_by_extension = {
    "h": "c",
    "c": "c",
}
breathe_separate_member_pages = True
breathe_show_enumvalue_initializer = True

cpp_id_attributes = [
    "__syscall",
    "__deprecated",
    "__may_alias",
    "__used",
    "__unused",
    "__weak",
    "__attribute_const__",
    "__DEPRECATED_MACRO",
    "FUNC_NORETURN",
    "__subsystem",
]
c_id_attributes = cpp_id_attributes

# -- Options for html_redirect plugin -------------------------------------

html_redirect_pages = redirects.REDIRECTS

# -- Options for zephyr.warnings_filter -----------------------------------

warnings_filter_config = str(ZEPHYR_BASE / "doc" / "known-warnings.txt")
warnings_filter_silent = False

# -- Linkcheck options ----------------------------------------------------

extlinks = {
    "jira": ("https://jira.zephyrproject.org/browse/%s", ""),
    "github": ("https://github.com/zephyrproject-rtos/zephyr/issues/%s", ""),
}

linkcheck_timeout = 30
linkcheck_workers = 10
linkcheck_anchors = False


def setup(app):
    # theme customizations
    app.add_css_file("css/custom.css")
    app.add_js_file("js/custom.js")
    app.add_js_file("js/dark-mode-toggle.min.mjs", type="module")

    app.add_js_file("https://www.googletagmanager.com/gtag/js?id=UA-831873-47")
    app.add_js_file("js/ga-tracker.js")
