"""
Doxygen Tooltip Extension
#########################

Copyright (c) 2024 The Linux Foundation
SPDX-License-Identifier: Apache-2.0

A simple Sphinx extension that adds JS and CSS resources to the app
to enable tooltips for C domain links.
"""

from pathlib import Path
from typing import Any

from sphinx.application import Sphinx
from sphinx.util import logging

logger = logging.getLogger(__name__)

RESOURCES_DIR = Path(__file__).parent / "static"

def setup(app: Sphinx) -> dict[str, Any]:
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())

    app.add_js_file("tippy/popper.min.js")
    app.add_js_file("tippy/tippy-bundle.umd.min.js")

    app.add_js_file("doxytooltip.js")
    app.add_css_file("doxytooltip.css")

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
