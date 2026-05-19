#!/usr/bin/env python3

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

"""Validate the dashboard output of against a test reference."""

import os
import sys

from lxml import html

REFERENCE_OUTPUT_FILES = [
    "index.html",
    "dts.html",
    "elfstats.html",
    "kconfig.html",
    "memoryreport.html",
    "sysinit.html",
    "static/css/bootstrap-chop.css",
    "static/css/dashboard.css",
    "static/css/icons.css",
    "static/css/light.css",
    "static/css/pygments.css",
    "static/css/treetable.css",
    "static/font/icons.woff2",
    "static/img/favicon.png",
    "static/img/logo.svg",
    "static/js/bootstrap-chop.js",
    "static/js/kconfig.js",
    "static/js/treetable.js",
    "static/js/memoryreport.js",
]

if len(sys.argv) != 2:
    print(f"usage: {sys.argv[0]} <DASHBOARD_DIR>")
    sys.exit(1)

parser = html.HTMLParser()

for output in REFERENCE_OUTPUT_FILES:
    # Check for file existence.
    fname = os.path.join(sys.argv[1], output)
    if not os.path.isfile(fname):
        print(f"Missing expected output file {fname}")
        sys.exit(1)

    # If this is HTML content, do simple syntax validation.
    if fname.endswith('.html'):
        html_tree = html.parse(fname, parser=parser)
        if parser.error_log:
            print(f"HTML validation failed for {fname}:")
            for error in parser.error_log:
                print(error)

print("TEST PASSED")
sys.exit(0)
