#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for domains.py classes."""

import os
import sys
from contextlib import nullcontext
from unittest import mock

import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/build_helpers"))

import domains  # noqa: E402

SAMPLE_TOP_BUILD_DIR = os.path.abspath(
    os.path.join(os.sep, "tmp", "pkg", "sample.ipc.static_vrings")
)

TESTDATA_FROM_FILE = [
    (
        "",
        "domains.yaml",
        False,
        1,
        ["domains.yaml file not found: domains.yaml"],
        None,
        None,
        None,
    ),
    (
        f"""
default: static_vrings
build_dir: {SAMPLE_TOP_BUILD_DIR}
domains:
- name: static_vrings
  build_dir: {os.path.join(SAMPLE_TOP_BUILD_DIR, "static_vrings")}
- name: remote
  build_dir: {os.path.join(SAMPLE_TOP_BUILD_DIR, "remote")}
flash_order:
- static_vrings
- remote
""",
        os.path.join(SAMPLE_TOP_BUILD_DIR, "domains.yaml"),
        True,
        None,
        [],
        SAMPLE_TOP_BUILD_DIR,
        {
            "static_vrings": os.path.join(SAMPLE_TOP_BUILD_DIR, "static_vrings"),
            "remote": os.path.join(SAMPLE_TOP_BUILD_DIR, "remote"),
        },
        "static_vrings",
    ),
    (
        """
default: static_vrings
build_dir: /old/build/dir/sample.ipc.static_vrings
domains:
- name: static_vrings
  build_dir: /old/build/dir/sample.ipc.static_vrings/static_vrings
- name: remote
  build_dir: /old/build/dir/sample.ipc.static_vrings/remote
flash_order:
- static_vrings
- remote
""",
        os.path.join(SAMPLE_TOP_BUILD_DIR, "domains.yaml"),
        True,
        None,
        [
            "Rebasing domain build directories from "
            "'/old/build/dir/sample.ipc.static_vrings' "
            f"to '{SAMPLE_TOP_BUILD_DIR}'"
        ],
        SAMPLE_TOP_BUILD_DIR,
        {
            "static_vrings": os.path.join(SAMPLE_TOP_BUILD_DIR, "static_vrings"),
            "remote": os.path.join(SAMPLE_TOP_BUILD_DIR, "remote"),
        },
        "static_vrings",
    ),
]


@pytest.mark.parametrize(
    (
        "f_contents, file_path, f_exists, exit_code, "
        "expected_logs, expected_build_dir, "
        "expected_domain_build_dirs, expected_default"
    ),
    TESTDATA_FROM_FILE,
    ids=["no file", "matching build_dir", "relocated artifacts"],
)
def test_from_file(
    caplog,
    f_contents,
    file_path,
    f_exists,
    exit_code,
    expected_logs,
    expected_build_dir,
    expected_domain_build_dirs,
    expected_default,
):
    def mock_open(*args, **kwargs):
        if f_exists:
            return mock.mock_open(read_data=f_contents)(args, kwargs)
        raise FileNotFoundError("domains.yaml not found.")

    with (
        mock.patch("builtins.open", mock_open),
        pytest.raises(SystemExit) if exit_code else nullcontext() as s_exit,
    ):
        result = domains.Domains.from_file(file_path)

    if exit_code:
        assert str(s_exit.value) == str(exit_code)
    else:
        assert result is not None
        assert result.get_top_build_dir() == expected_build_dir
        assert result.get_default_domain().name == expected_default
        for name, build_dir in expected_domain_build_dirs.items():
            assert result.get_domain(name).build_dir == build_dir

    assert all(log in caplog.text for log in expected_logs)


TESTDATA_FROM_YAML = [
    (
        """
default: some default
build_dir: my/dir
domains:
- name: some default
  build_dir: dir/2
- name: another
  build_dir: dir/3
flash_order: I don't think this is correct
""",
        1,
        None,
        None,
        None,
        None,
    ),
    (
        """
build_dir: build/dir
domains:
- name: a domain
  build_dir: dir/1
- name: default_domain
  build_dir: dir/2
default: default_domain
flash_order:
- default_domain
- a domain
""",
        None,
        "build/dir",
        [("default_domain", "dir/2"), ("a domain", "dir/1")],
        ("default_domain", "dir/2"),
        {
            "a domain": ("a domain", "dir/1"),
            "default_domain": ("default_domain", "dir/2"),
        },
    ),
    (
        f"""
build_dir: build/dir
domains:
- name: relative
  build_dir: nested/dir
- name: absolute
  build_dir: {os.path.join(os.sep, "abs", "domain")}
default: relative
flash_order:
- absolute
- relative
""",
        None,
        "build/dir",
        [
            ("absolute", os.path.join(os.sep, "abs", "domain")),
            ("relative", "nested/dir"),
        ],
        ("relative", "nested/dir"),
        {
            "relative": ("relative", "nested/dir"),
            "absolute": (
                "absolute",
                os.path.join(os.sep, "abs", "domain"),
            ),
        },
    ),
]


@pytest.mark.parametrize(
    (
        "data, exit_code, expected_build_dir, expected_flash_order, "
        "expected_default, expected_domains"
    ),
    TESTDATA_FROM_YAML,
    ids=["invalid", "valid relative", "valid mixed"],
)
def test_from_yaml(
    caplog,
    data,
    exit_code,
    expected_build_dir,
    expected_flash_order,
    expected_default,
    expected_domains,
):
    def mock_domain(name, build_dir, *args, **kwargs):
        return name, build_dir

    with (
        mock.patch("domains.Domain", side_effect=mock_domain),
        pytest.raises(SystemExit) if exit_code else nullcontext() as exit_st,
    ):
        doms = domains.Domains.from_yaml(data)

    if exit_code:
        assert str(exit_st.value) == str(exit_code)
        return

    assert doms.get_default_domain() == expected_default
    assert doms.get_top_build_dir() == expected_build_dir
    assert doms._domains == expected_domains
    assert all(domain in expected_flash_order for domain in doms._flash_order)
    assert all(domain in doms._flash_order for domain in expected_flash_order)


TESTDATA_GET_DOMAINS = [
    (
        None,
        True,
        [
            ("some", os.path.join("dir", "2")),
            ("order", os.path.join("dir", "1")),
        ],
    ),
    (
        None,
        False,
        [
            ("order", os.path.join("dir", "1")),
            ("some", os.path.join("dir", "2")),
        ],
    ),
    (["some"], False, [("some", os.path.join("dir", "2"))]),
]


@pytest.mark.parametrize(
    "names, default_flash_order, expected_result",
    TESTDATA_GET_DOMAINS,
    ids=["order only", "no parameters", "valid"],
)
def test_get_domains(caplog, names, default_flash_order, expected_result):
    doms = domains.Domains.from_yaml(
        """
domains:
- name: dummy
  build_dir: dummy
default: dummy
build_dir: dummy
"""
    )
    doms._flash_order = [
        ("some", os.path.join("dir", "2")),
        ("order", os.path.join("dir", "1")),
    ]
    doms._domains = {
        "order": ("order", os.path.join("dir", "1")),
        "some": ("some", os.path.join("dir", "2")),
    }

    result = doms.get_domains(names, default_flash_order)

    assert result == expected_result


TESTDATA_GET_DOMAIN = [
    (
        "other",
        1,
        ['domain "other" not found, valid domains are: order, some'],
        None,
    ),
    ("some", None, [], ("some", os.path.join("dir", "2"))),
]


@pytest.mark.parametrize(
    "name, exit_code, expected_logs, expected_result",
    TESTDATA_GET_DOMAIN,
    ids=["domain not found", "valid"],
)
def test_get_domain(caplog, name, exit_code, expected_logs, expected_result):
    doms = domains.Domains.from_yaml(
        """
domains:
- name: dummy
  build_dir: dummy
default: dummy
build_dir: dummy
"""
    )
    doms._flash_order = [
        ("some", os.path.join("dir", "2")),
        ("order", os.path.join("dir", "1")),
    ]
    doms._domains = {
        "order": ("order", os.path.join("dir", "1")),
        "some": ("some", os.path.join("dir", "2")),
    }

    with pytest.raises(SystemExit) if exit_code else nullcontext() as s_exit:
        result = doms.get_domain(name)

    assert all(log in caplog.text for log in expected_logs)

    if exit_code:
        assert str(s_exit.value) == str(exit_code)
    else:
        assert result == expected_result


def test_domain():
    name = "Domain Name"
    build_dir = "build/dir"

    domain = domains.Domain(name, build_dir)

    assert domain.name == name
    assert domain.build_dir == build_dir

    domain.name = "New Name"
    domain.build_dir = "new/dir"

    assert domain.name == "New Name"
    assert domain.build_dir == "new/dir"
