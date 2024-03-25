# Zephyr documentation build configuration file.
# Reference: https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
import os
from pathlib import Path
import re
import textwrap

from sphinx.cmd.build import get_parser

args = get_parser().parse_args()
ZEPHYR_BASE = Path(__file__).resolve().parents[1]
ZEPHYR_BUILD = Path(args.outputdir).resolve()

# Add the '_extensions' directory to sys.path, to enable finding Sphinx
# extensions within.
sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))

# Add the '_scripts' directory to sys.path, to enable finding utility
# modules.
sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_scripts"))

# Add the directory which contains the runners package as well,
# for autodoc directives on runners.xyz.
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "west_commands"))

# Add the directory which contains the pytest-twister-pytest
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "pylib" / "pytest-twister-harness" / "src"))

import redirects

try:
    import west as west_found
except ImportError:
    west_found = False

# -- Project --------------------------------------------------------------

project = "Zephyr Project"
copyright = "2015-2024 Zephyr Project members and individual contributors"
author = "The Zephyr Project Contributors"

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

release = version

# parse SDK version from 'SDK_VERSION' file
with open(ZEPHYR_BASE / "SDK_VERSION") as f:
    sdk_version = f.read().strip()

# -- General configuration ------------------------------------------------

extensions = [
    "breathe",
    "sphinx_rtd_theme",
    "sphinx.ext.todo",
    "sphinx.ext.extlinks",
    "sphinx.ext.autodoc",
    "sphinx.ext.graphviz",
    "sphinxcontrib.jquery",
    "zephyr.application",
    "zephyr.html_redirects",
    "zephyr.kconfig",
    "zephyr.dtcompatible-role",
    "zephyr.link-roles",
    "sphinx_tabs.tabs",
    "zephyr.warnings_filter",
    "zephyr.doxyrunner",
    "zephyr.gh_utils",
    "zephyr.manifest_projects_table",
    "notfound.extension",
    "sphinx_copybutton",
    "sphinx_togglebutton",
    "zephyr.external_content",
    "zephyr.domain",
    "zephyr.api_overview",
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

pygments_style = "sphinx"
highlight_language = "none"

todo_include_todos = False

nitpick_ignore = [
    # ignore C standard identifiers (they are not defined in Zephyr docs)
    ("c:identifier", "FILE"),
    ("c:identifier", "int8_t"),
    ("c:identifier", "int16_t"),
    ("c:identifier", "int32_t"),
    ("c:identifier", "int64_t"),
    ("c:identifier", "intptr_t"),
    ("c:identifier", "off_t"),
    ("c:identifier", "size_t"),
    ("c:identifier", "ssize_t"),
    ("c:identifier", "time_t"),
    ("c:identifier", "uint8_t"),
    ("c:identifier", "uint16_t"),
    ("c:identifier", "uint32_t"),
    ("c:identifier", "uint64_t"),
    ("c:identifier", "uintptr_t"),
    ("c:identifier", "va_list"),
]

SDK_URL_BASE="https://github.com/zephyrproject-rtos/sdk-ng/releases/download"

rst_epilog = f"""
.. include:: /substitutions.txt

.. |sdk-version-literal| replace:: ``{sdk_version}``
.. |sdk-version-trim| unicode:: {sdk_version}
   :trim:
.. |sdk-version-ltrim| unicode:: {sdk_version}
   :ltrim:
.. _Zephyr SDK bundle: https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v{sdk_version}
.. |sdk-url-linux| replace:: `{SDK_URL_BASE}/v{sdk_version}/zephyr-sdk-{sdk_version}_linux-x86_64.tar.xz`
.. |sdk-url-linux-sha| replace:: `{SDK_URL_BASE}/v{sdk_version}/sha256.sum`
.. |sdk-url-macos| replace:: `{SDK_URL_BASE}/v{sdk_version}/zephyr-sdk-{sdk_version}_macos-x86_64.tar.xz`
.. |sdk-url-macos-sha| replace:: `{SDK_URL_BASE}/v{sdk_version}/sha256.sum`
.. |sdk-url-windows| replace:: `{SDK_URL_BASE}/v{sdk_version}/zephyr-sdk-{sdk_version}_windows-x86_64.7z`
"""

# -- Options for HTML output ----------------------------------------------

html_theme = "sphinx_rtd_theme"
html_theme_options = {
    "logo_only": True,
    "prev_next_buttons_location": None
}
html_baseurl = "https://docs.zephyrproject.org/latest/"
html_title = "Zephyr Project Documentation"
html_logo = str(ZEPHYR_BASE / "doc" / "_static" / "images" / "logo.svg")
html_favicon = str(ZEPHYR_BASE / "doc" / "_static" / "images" / "favicon.png")
html_static_path = [str(ZEPHYR_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_domain_indices = False
html_split_index = True
html_show_sourcelink = False
html_show_sphinx = False
html_search_scorer = str(ZEPHYR_BASE / "doc" / "_static" / "js" / "scorer.js")
html_additional_pages = {
    "gsearch": "gsearch.html"
}

is_release = tags.has("release")  # pylint: disable=undefined-variable
reference_prefix = ""
if tags.has("publish"):  # pylint: disable=undefined-variable
    reference_prefix = f"/{version}" if is_release else "/latest"
docs_title = "Docs / {}".format(version if is_release else "Latest")
html_context = {
    "show_license": True,
    "docs_title": docs_title,
    "is_release": is_release,
    "current_version": version,
    "versions": (
        ("latest", "/"),
        ("3.6.0", "/3.6.0/"),
        ("3.5.0", "/3.5.0/"),
        ("2.7.6 (LTS)", "/2.7.6/"),
    ),
    "display_gh_links": True,
    "reference_links": {
        "API": f"{reference_prefix}/doxygen/html/index.html",
        "Kconfig Options": f"{reference_prefix}/kconfig.html",
        "Devicetree Bindings": f"{reference_prefix}/build/dts/api/bindings.html",
        "West Projects": f"{reference_prefix}/develop/manifest/index.html",
    },
    # Set google_searchengine_id to your Search Engine ID to replace built-in search
    # engine with Google's Programmable Search Engine.
    # See https://programmablesearchengine.google.com/ for details.
    "google_searchengine_id": "746031aa0d56d4912",
}

# -- Options for LaTeX output ---------------------------------------------

latex_elements = {
    "papersize": "a4paper",
    "maketitle": open(ZEPHYR_BASE / "doc" / "_static" / "latex" / "title.tex").read(),
    "preamble": open(ZEPHYR_BASE / "doc" / "_static" / "latex" / "preamble.tex").read(),
    "makeindex": r"\usepackage[columns=1]{idxlayout}\makeindex",
    "fontpkg": textwrap.dedent(r"""
                                    \usepackage{noto}
                                    \usepackage{inconsolata-nerd-font}
                                    \usepackage[T1]{fontenc}
                                """),
    "sphinxsetup": ",".join(
        (
            # NOTE: colors match those found in light.css stylesheet
            "verbatimwithframe=false",
            "VerbatimColor={HTML}{f0f2f4}",
            "InnerLinkColor={HTML}{2980b9}",
            "warningBgColor={HTML}{e9a499}",
            "warningborder=0pt",
            r"HeaderFamily=\rmfamily\bfseries",
        )
    ),
}
latex_logo = str(ZEPHYR_BASE / "doc" / "_static" / "images" / "logo-latex.pdf")
latex_documents = [
    ("index-tex", "zephyr.tex", "Zephyr Project Documentation", author, "manual"),
]
latex_engine = "xelatex"

# -- Options for zephyr.doxyrunner plugin ---------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = ZEPHYR_BASE / "doc" / "zephyr.doxyfile.in"
doxyrunner_outdir = ZEPHYR_BUILD / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {"ZEPHYR_BASE": str(ZEPHYR_BASE), "ZEPHYR_VERSION": version}
doxyrunner_outdir_var = "DOXY_OUT"

# -- Options for Breathe plugin -------------------------------------------

breathe_projects = {"Zephyr": str(doxyrunner_outdir / "xml")}
breathe_default_project = "Zephyr"
breathe_domain_by_extension = {
    "h": "c",
    "c": "c",
}
breathe_show_enumvalue_initializer = True
breathe_default_members = ("members", )

cpp_id_attributes = [
    "__syscall",
    "__syscall_always_inline",
    "__deprecated",
    "__may_alias",
    "__used",
    "__unused",
    "__weak",
    "__attribute_const__",
    "__DEPRECATED_MACRO",
    "FUNC_NORETURN",
    "__subsystem",
    "ALWAYS_INLINE",
]
c_id_attributes = cpp_id_attributes

# -- Options for html_redirect plugin -------------------------------------

html_redirect_pages = redirects.REDIRECTS

# -- Options for zephyr.warnings_filter -----------------------------------

warnings_filter_config = str(ZEPHYR_BASE / "doc" / "known-warnings.txt")

# -- Options for zephyr.link-roles ----------------------------------------

link_roles_manifest_project = "zephyr"
link_roles_manifest_baseurl = "https://github.com/zephyrproject-rtos/zephyr"

# -- Options for notfound.extension ---------------------------------------

notfound_urls_prefix = f"/{version}/" if is_release else "/latest/"

# -- Options for zephyr.gh_utils ------------------------------------------

gh_link_version = f"v{version}" if is_release else "main"
gh_link_base_url = f"https://github.com/zephyrproject-rtos/zephyr"
gh_link_prefixes = {
    "samples/.*": "",
    "boards/.*": "",
    "snippets/.*": "",
    ".*": "doc",
}
gh_link_exclude = [
    "reference/kconfig.*",
    "build/dts/api/bindings.*",
    "build/dts/api/compatibles.*",
]

# -- Options for zephyr.kconfig -------------------------------------------

kconfig_generate_db = True
kconfig_ext_paths = [ZEPHYR_BASE]

# -- Options for zephyr.external_content ----------------------------------

external_content_contents = [
    (ZEPHYR_BASE / "doc", "[!_]*"),
    (ZEPHYR_BASE, "boards/**/*.rst"),
    (ZEPHYR_BASE, "boards/**/doc"),
    (ZEPHYR_BASE, "samples/**/*.html"),
    (ZEPHYR_BASE, "samples/**/*.rst"),
    (ZEPHYR_BASE, "samples/**/doc"),
    (ZEPHYR_BASE, "snippets/**/*.rst"),
    (ZEPHYR_BASE, "snippets/**/doc"),
]
external_content_keep = [
    "reference/kconfig/*",
    "develop/manifest/index.rst",
    "build/dts/api/bindings.rst",
    "build/dts/api/bindings/**/*",
    "build/dts/api/compatibles/**/*",
]

# -- Options for zephyr.domain --------------------------------------------

zephyr_breathe_insert_related_samples = True

# -- Options for sphinx.ext.graphviz --------------------------------------

graphviz_dot = os.environ.get("DOT_EXECUTABLE", "dot")
graphviz_output_format = "svg"
graphviz_dot_args = [
    "-Gbgcolor=transparent",
    "-Nstyle=filled",
    "-Nfillcolor=white",
    "-Ncolor=gray60",
    "-Nfontcolor=gray25",
    "-Ecolor=gray60",
]

# -- Options for sphinx_copybutton ----------------------------------------

copybutton_prompt_text = r"\$ |uart:~\$ "
copybutton_prompt_is_regexp = True

# -- Linkcheck options ----------------------------------------------------

linkcheck_ignore = [
    r"https://github.com/zephyrproject-rtos/zephyr/issues/.*"
]

extlinks = {
    "github": ("https://github.com/zephyrproject-rtos/zephyr/issues/%s", "GitHub #%s"),
}

linkcheck_timeout = 30
linkcheck_workers = 10
linkcheck_anchors = False

# -- Options for zephyr.api_overview --------------------------------------

api_overview_doxygen_base_url = "../../doxygen/html"

def setup(app):
    # theme customizations
    app.add_css_file("css/custom.css")
    app.add_js_file("js/custom.js")
    app.add_js_file("js/dark-mode-toggle.min.mjs", type="module")
