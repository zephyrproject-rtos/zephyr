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


def _make_artifacts(outdir, detailed_test_id=False):
    env = mock.Mock()
    env.options = mock.Mock()
    env.options.outdir = str(outdir)
    env.options.package_artifacts = str(outdir / "PACKAGE")
    env.options.detailed_test_id = detailed_test_id
    return Artifacts(env)


def test_get_testsuite_artifact_dir_uses_path_when_not_detailed_id(tmp_path):
    artifacts = _make_artifacts(tmp_path, detailed_test_id=False)
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


def test_get_testsuite_artifact_dir_uses_name_when_detailed_id(tmp_path):
    artifacts = _make_artifacts(tmp_path, detailed_test_id=True)
    build_dir = tmp_path / "qemu_x86_atom" / "zephyr_gnu" / "sample.basic.helloworld"
    build_dir.mkdir(parents=True)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "path": os.path.join("samples", "hello_world"),
        "name": "sample.basic.helloworld",
    }

    assert artifacts.get_testsuite_artifact_dir(testsuite) == str(build_dir)


def test_get_testsuite_artifact_dir_rejects_escaping_path(tmp_path):
    artifacts = _make_artifacts(tmp_path, detailed_test_id=False)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "path": os.path.join("..", "..", "outside"),
        "name": "sample.basic.helloworld",
    }

    with pytest.raises(ValueError, match="Invalid testsuite artifact path"):
        artifacts.get_testsuite_artifact_dir(testsuite)


def test_get_testsuite_artifact_dir_rejects_escaping_name(tmp_path):
    artifacts = _make_artifacts(tmp_path, detailed_test_id=True)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "name": os.path.join("..", "..", "outside"),
    }

    with pytest.raises(ValueError, match="Invalid testsuite artifact name"):
        artifacts.get_testsuite_artifact_dir(testsuite)


def test_get_testsuite_artifact_dir_rejects_missing_path(tmp_path):
    artifacts = _make_artifacts(tmp_path, detailed_test_id=False)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "name": "sample.basic.helloworld",
    }

    with pytest.raises(ValueError, match="Missing testsuite artifact path"):
        artifacts.get_testsuite_artifact_dir(testsuite)


def test_get_testsuite_artifact_dir_rejects_missing_artifact_dir(tmp_path):
    artifacts = _make_artifacts(tmp_path, detailed_test_id=False)

    testsuite = {
        "platform": "qemu_x86/atom",
        "toolchain": "zephyr/gnu",
        "path": os.path.join("samples", "hello_world"),
        "name": "sample.basic.helloworld",
    }

    with pytest.raises(
        ValueError,
        match="Could not find testsuite artifact directory",
    ):
        artifacts.get_testsuite_artifact_dir(testsuite)


def test_get_safe_artifact_dir_rejects_outside_build_root(tmp_path):
    build_root = tmp_path / "qemu_x86_atom" / "zephyr_gnu"
    build_root.mkdir(parents=True)

    artifact_dir = Artifacts._get_safe_artifact_dir(
        str(build_root),
        os.path.join("..", "outside"),
    )

    assert artifact_dir is None


@pytest.mark.parametrize(
    ("detailed_test_id", "first_build_dir", "second_build_dir"),
    [
        (
            False,
            os.path.join("samples", "pytest", "shell", "sample.pytest.shell"),
            os.path.join("samples", "hello_world", "sample.basic.helloworld"),
        ),
        (
            True,
            "sample.pytest.shell",
            "sample.basic.helloworld",
        ),
    ],
)
def test_package_uses_existing_testsuite_artifact_dirs(
    tmp_path,
    detailed_test_id,
    first_build_dir,
    second_build_dir,
):
    artifacts = _make_artifacts(tmp_path, detailed_test_id=detailed_test_id)

    first_build_dir = tmp_path / "qemu_x86_atom" / "zephyr_gnu" / first_build_dir
    first_build_dir.mkdir(parents=True)
    second_build_dir = tmp_path / "qemu_x86_atom" / "zephyr_gnu" / second_build_dir
    second_build_dir.mkdir(parents=True)

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
        str(first_build_dir),
        str(second_build_dir),
        str(tmp_path / "twister.json"),
        str(tmp_path / "testplan.json"),
    ]
