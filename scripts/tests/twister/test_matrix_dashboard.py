#!/usr/bin/env python3
# Copyright (c) 2025 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the standalone scripts/gen_test_matrix_dashboard.py
"""

import importlib.util
import json
import pathlib
import re
import sys
from unittest import mock

_SCRIPT = pathlib.Path(__file__).parents[3] / "scripts" / "gen_test_matrix_dashboard.py"


def _load():
    spec = importlib.util.spec_from_file_location("gen_test_matrix_dashboard", _SCRIPT)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def test_render_dashboard_embeds_matrix():
    mod = _load()
    matrix = {
        "by_line": {"foo.c": {"10": ["scn.test_a", "scn.test_b"]}},
        "by_test": {"scn.test_a": {"foo.c": [10]},
                    "scn.test_b": {"foo.c": [10]}},
    }
    html = mod.render_dashboard(matrix)

    # Placeholder fully substituted and the matrix round-trips through JSON.
    assert "__MATRIX_DATA__" not in html
    embedded = re.search(r'application/json">(.*?)</script>', html, re.S).group(1)
    assert json.loads(embedded.replace("<\\/", "</")) == matrix
    # Structural anchors the dashboard's JS relies on.
    for token in ('id="testTable"', 'id="fileView"', "Per-test coverage matrix"):
        assert token in html


def test_main_writes_html(tmp_path):
    mod = _load()
    matrix = {"by_line": {}, "by_test": {}}
    inp = tmp_path / "test_matrix.json"
    inp.write_text(json.dumps(matrix))
    out = tmp_path / "dashboard.html"

    with mock.patch.object(sys, "argv", ["prog", "-i", str(inp), "-o", str(out)]):
        mod.main()

    assert out.exists()
    assert "Per-test coverage matrix" in out.read_text()


def test_main_default_output(tmp_path):
    mod = _load()
    inp = tmp_path / "test_matrix.json"
    inp.write_text(json.dumps({"by_line": {}, "by_test": {}}))

    with mock.patch.object(sys, "argv", ["prog", "-i", str(inp)]):
        mod.main()

    # Output defaults to the input path with a .html suffix.
    assert (tmp_path / "test_matrix.html").exists()
