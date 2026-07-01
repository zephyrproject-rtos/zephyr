#!/usr/bin/env python3
# Copyright (c) 2025 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the per-test coverage helpers in twisterlib.coverage
"""

import json
import os

import re

from twisterlib.coverage import (
    build_test_matrix,
    build_test_matrix_html,
    discover_per_test_semihost,
    materialize_canonical_gcda,
    retrieve_tagged_gcov_data,
    sanitize_coverage_name,
)

TESTDATA_SANITIZE = [
    ("framework_tests.test_empty", "framework_tests.test_empty"),
    ("scenario/name-with.dots", "scenario_name-with.dots".replace("-", "_")),
    ("a b\tc", "a_b_c"),
]


def test_sanitize_coverage_name():
    assert sanitize_coverage_name("keep.dots_1") == "keep.dots_1"
    # Every character outside [A-Za-z0-9_.] becomes an underscore.
    assert sanitize_coverage_name("a/b-c d") == "a_b_c_d"


def test_discover_per_test_semihost(tmp_path):
    obj_dir = tmp_path / "CMakeFiles" / "app.dir"
    obj_dir.mkdir(parents=True)
    canonical = obj_dir / "main.c.gcda"
    # Two tags for the same object, plus an unrelated / malformed file.
    (obj_dir / "main.c.gcda@@suite.test_a").write_bytes(b"a")
    (obj_dir / "main.c.gcda@@suite.test_b").write_bytes(b"b")
    (obj_dir / "main.c.gcno").write_bytes(b"x")  # no @@, ignored
    (obj_dir / "notgcda.txt@@suite.test_a").write_bytes(b"y")  # not .gcda, ignored

    tags = discover_per_test_semihost(str(tmp_path))

    assert set(tags) == {"suite.test_a", "suite.test_b"}
    assert tags["suite.test_a"][str(canonical)] == (
        "file", str(obj_dir / "main.c.gcda@@suite.test_a"))
    assert list(tags["suite.test_b"]) == [str(canonical)]


def test_retrieve_tagged_gcov_data(tmp_path):
    log = tmp_path / "handler.log"
    log.write_text(
        "boot noise\n"
        "GCOV_COVERAGE_DUMP_START suite.test_a\n"
        "*/work/foo.gcda<0a0b\n"
        "GCOV_COVERAGE_DUMP_END\n"
        # An untagged block must be ignored (no per-test attribution).
        "GCOV_COVERAGE_DUMP_START\n"
        "*/work/foo.gcda<ffff\n"
        "GCOV_COVERAGE_DUMP_END\n"
        "GCOV_COVERAGE_DUMP_START suite.test_b\n"
        "*/work/foo.gcda<0c0d\n"
        "*/work/bar.gcda<0e0f\n"
        "GCOV_COVERAGE_DUMP_END\n"
    )

    tags = retrieve_tagged_gcov_data(str(log))

    assert set(tags) == {"suite.test_a", "suite.test_b"}
    assert tags["suite.test_a"]["/work/foo.gcda"] == [bytes.fromhex("0a0b")]
    assert tags["suite.test_b"]["/work/bar.gcda"] == [bytes.fromhex("0e0f")]


def test_materialize_canonical_gcda(tmp_path):
    # From raw bytes.
    dst = tmp_path / "sub" / "out.gcda"
    materialize_canonical_gcda(str(dst), ("bytes", b"\x01\x02"))
    assert dst.read_bytes() == b"\x01\x02"

    # From an existing file.
    src = tmp_path / "src.gcda"
    src.write_bytes(b"\x03\x04")
    dst2 = tmp_path / "copy.gcda"
    materialize_canonical_gcda(str(dst2), ("file", str(src)))
    assert dst2.read_bytes() == b"\x03\x04"


def _write_info(path, tn, records):
    lines = [f"TN:{tn}"]
    for sf, das in records.items():
        lines.append(f"SF:{sf}")
        lines += [f"DA:{ln},{hits}" for ln, hits in das]
        lines.append("end_of_record")
    path.write_text("\n".join(lines) + "\n")


def test_build_test_matrix(tmp_path):
    base = tmp_path / "work"
    base.mkdir()
    info_a = tmp_path / "a.info"
    info_b = tmp_path / "b.info"
    # test_a hits foo.c:10; foo.c:11 is present but uncovered (0 hits).
    # bar.c lives under tests/ and must be filtered out of the matrix.
    _write_info(info_a, "scn.test_a", {
        str(base / "foo.c"): [(10, 1), (11, 0)],
        str(base / "tests" / "bar.c"): [(5, 1)],
    })
    _write_info(info_b, "scn.test_b", {
        str(base / "foo.c"): [(10, 1), (12, 1)],
    })

    out = tmp_path / "matrix.json"
    returned = build_test_matrix(
        [str(info_a), str(info_b)], str(out), base_dir=str(base))
    matrix = json.loads(out.read_text())
    # The builder both writes the JSON and returns the matrix for reuse.
    assert returned == matrix

    by_line = matrix["by_line"]
    assert set(by_line) == {"foo.c"}  # tests/bar.c filtered out
    assert by_line["foo.c"]["10"] == ["scn.test_a", "scn.test_b"]
    assert by_line["foo.c"]["12"] == ["scn.test_b"]
    assert "11" not in by_line["foo.c"]  # uncovered line excluded

    by_test = matrix["by_test"]
    assert by_test["scn.test_a"] == {"foo.c": [10]}
    assert by_test["scn.test_b"] == {"foo.c": [10, 12]}
    # bar.c under tests/ is absent from every view.
    assert all("bar.c" not in os.path.basename(k) for k in by_line)


def test_build_test_matrix_html(tmp_path):
    matrix = {
        "by_line": {"foo.c": {"10": ["scn.test_a", "scn.test_b"]}},
        "by_test": {"scn.test_a": {"foo.c": [10]},
                    "scn.test_b": {"foo.c": [10]}},
    }
    out = tmp_path / "matrix.html"
    build_test_matrix_html(matrix, str(out))
    html = out.read_text()

    # Placeholder fully substituted and the matrix is embedded as parseable JSON.
    assert "__MATRIX_DATA__" not in html
    embedded = re.search(r'application/json">(.*?)</script>', html, re.S).group(1)
    assert json.loads(embedded.replace("<\\/", "</")) == matrix
    # A few structural anchors the dashboard relies on.
    for token in ('id="testTable"', 'id="fileView"', "Per-test coverage matrix"):
        assert token in html
