#!/usr/bin/env python3
# Copyright (c) 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for package.py classes."""

import json
import os
from unittest import mock

import pytest
from twisterlib.package import Artifacts
from twisterlib.statuses import TwisterStatus


def _make_artifacts(outdir):
    env = mock.Mock()
    env.options = mock.Mock()
    env.options.outdir = str(outdir)
    env.options.package_artifacts = str(outdir / "PACKAGE")
    return Artifacts(env)


def test_get_testsuite_artifact_dir_prefers_path_based_build_dir(tmp_path):
    artifacts = _make_artifacts(tmp_path)
    build_dir = (
        tmp_path
        / "qemu_x86_atom"
        / "zephyr_gnu"
        / "samples"
        / "pytest"
        / "shell"
        / "sample.pytest.shell"
    )
    build_dir.mkdir(parents=True)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "path": os.path.join("samples", "pytest", "shell"),
        "name": "sample.pytest.shell",
    }

    assert artifacts.get_testsuite_artifact_dir(testsuite) == str(build_dir)


def test_get_testsuite_artifact_dir_falls_back_to_name_only_build_dir(tmp_path):
    artifacts = _make_artifacts(tmp_path)
    build_dir = tmp_path / "qemu_x86_atom" / "zephyr_gnu" / "sample.basic.helloworld"
    build_dir.mkdir(parents=True)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "path": os.path.join("samples", "hello_world"),
        "name": "sample.basic.helloworld",
    }

    assert artifacts.get_testsuite_artifact_dir(testsuite) == str(build_dir)


def test_get_testsuite_artifact_dir_ignores_escaping_path(tmp_path):
    artifacts = _make_artifacts(tmp_path)
    build_dir = tmp_path / "qemu_x86_atom" / "zephyr_gnu" / "sample.basic.helloworld"
    build_dir.mkdir(parents=True)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "path": os.path.join("..", "..", "outside"),
        "name": "sample.basic.helloworld",
    }

    assert artifacts.get_testsuite_artifact_dir(testsuite) == str(build_dir)


def test_get_testsuite_artifact_dir_rejects_escaping_name(tmp_path):
    artifacts = _make_artifacts(tmp_path)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "name": os.path.join("..", "..", "outside"),
    }

    with pytest.raises(ValueError, match="Invalid testsuite artifact name"):
        artifacts.get_testsuite_artifact_dir(testsuite)


def test_get_testsuite_artifact_dir_rejects_missing_artifact_dir(tmp_path):
    artifacts = _make_artifacts(tmp_path)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "path": os.path.join("samples", "hello_world"),
        "name": "sample.basic.helloworld",
    }

    with (
        mock.patch.object(
            artifacts,
            "_get_existing_build_dirs",
            return_value=set(),
        ),
        pytest.raises(
            ValueError,
            match="Could not find testsuite artifact directory",
        ),
    ):
        artifacts.get_testsuite_artifact_dir(testsuite)


def test_package_uses_existing_testsuite_artifact_dirs(tmp_path):
    artifacts = _make_artifacts(tmp_path)

    nested_build_dir = (
        tmp_path
        / "qemu_x86_atom"
        / "zephyr_gnu"
        / "samples"
        / "pytest"
        / "shell"
        / "sample.pytest.shell"
    )
    nested_build_dir.mkdir(parents=True)
    detailed_id_build_dir = tmp_path / "qemu_x86_atom" / "zephyr_gnu" / "sample.basic.helloworld"
    detailed_id_build_dir.mkdir(parents=True)

    twister_json = {
        "testsuites": [
            {
                "platform": "qemu_x86/atom",
                "toolchain": "zephyr/gnu",
                "path": os.path.join("samples", "pytest", "shell"),
                "name": "sample.pytest.shell",
                "status": TwisterStatus.NOTRUN,
            },
            {
                "platform": "qemu_x86/atom",
                "toolchain": "zephyr/gnu",
                "path": os.path.join("samples", "hello_world"),
                "name": "sample.basic.helloworld",
                "status": TwisterStatus.NOTRUN,
            },
            {
                "platform": "qemu_x86/atom",
                "toolchain": "zephyr/gnu",
                "path": os.path.join("samples", "filtered"),
                "name": "sample.filtered",
                "status": TwisterStatus.FILTER,
            },
        ]
    }

    (tmp_path / "twister.json").write_text(json.dumps(twister_json), encoding="utf-8")
    (tmp_path / "testplan.json").write_text("{}", encoding="utf-8")

    with mock.patch.object(artifacts, "make_tarfile") as make_tarfile:
        artifacts.package()

    make_tarfile.assert_called_once()
    output_filename, source_dirs = make_tarfile.call_args.args

    assert output_filename == str(tmp_path / "PACKAGE")
    assert source_dirs == [
        str(nested_build_dir),
        str(detailed_id_build_dir),
        str(tmp_path / "twister.json"),
        str(tmp_path / "testplan.json"),
    ]
