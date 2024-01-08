#!/usr/bin/env python3
# Copyright (c) 2023 Google LLC
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for runner.py classes
"""

import mock
import os
import pytest
import sys
import yaml

from typing import List

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.runner import ProjectBuilder


@pytest.fixture
def mocked_instance(tmp_path):
    instance = mock.Mock()
    testsuite = mock.Mock()
    testsuite.source_dir: str = ''
    instance.testsuite = testsuite
    platform = mock.Mock()
    platform.binaries: List[str] = []
    instance.platform = platform
    build_dir = tmp_path / 'build_dir'
    os.makedirs(build_dir)
    instance.build_dir: str = str(build_dir)
    return instance


@pytest.fixture
def mocked_env():
    env = mock.Mock()
    options = mock.Mock()
    env.options = options
    return env


@pytest.fixture
def mocked_jobserver():
    jobserver = mock.Mock()
    return jobserver


@pytest.fixture
def project_builder(mocked_instance, mocked_env, mocked_jobserver) -> ProjectBuilder:
    project_builder = ProjectBuilder(mocked_instance, mocked_env, mocked_jobserver)
    return project_builder


@pytest.fixture
def runners(project_builder: ProjectBuilder) -> dict:
    """
    Create runners.yaml file in build_dir/zephyr directory and return file
    content as dict.
    """
    build_dir_zephyr_path = os.path.join(project_builder.instance.build_dir, 'zephyr')
    os.makedirs(build_dir_zephyr_path)
    runners_file_path = os.path.join(build_dir_zephyr_path, 'runners.yaml')
    runners_content: dict = {
        'config': {
            'elf_file': 'zephyr.elf',
            'hex_file': os.path.join(build_dir_zephyr_path, 'zephyr.elf'),
            'bin_file': 'zephyr.bin',
        }
    }
    with open(runners_file_path, 'w') as file:
        yaml.dump(runners_content, file)

    return runners_content


@mock.patch("os.path.exists")
def test_projectbuilder_cmake_assemble_args(m):
    # Causes the additional_overlay_path to be appended
    m.return_value = True

    class MockHandler:
        pass

    handler = MockHandler()
    handler.args = ["handler_arg1", "handler_arg2"]
    handler.ready = True

    assert(ProjectBuilder.cmake_assemble_args(
        ["basearg1"],
        handler,
        ["a.conf;b.conf", "c.conf"],
        ["extra_overlay.conf"],
        ["x.overlay;y.overlay", "z.overlay"],
        ["cmake1=foo", "cmake2=bar"],
        "/builddir/",
    ) == [
        "-Dcmake1=foo", "-Dcmake2=bar",
        "-Dbasearg1",
        "-Dhandler_arg1", "-Dhandler_arg2",
        "-DCONF_FILE=a.conf;b.conf;c.conf",
        "-DDTC_OVERLAY_FILE=x.overlay;y.overlay;z.overlay",
        "-DOVERLAY_CONFIG=extra_overlay.conf "
        "/builddir/twister/testsuite_extra.conf",
    ])


def test_if_default_binaries_are_taken_properly(project_builder: ProjectBuilder):
    default_binaries = [
        os.path.join('zephyr', 'zephyr.hex'),
        os.path.join('zephyr', 'zephyr.bin'),
        os.path.join('zephyr', 'zephyr.elf'),
        os.path.join('zephyr', 'zephyr.exe'),
    ]
    binaries = project_builder._get_binaries()
    assert sorted(binaries) == sorted(default_binaries)


def test_if_binaries_from_platform_are_taken_properly(project_builder: ProjectBuilder):
    platform_binaries = ['spi_image.bin']
    project_builder.platform.binaries = platform_binaries
    platform_binaries_expected = [os.path.join('zephyr', bin) for bin in platform_binaries]
    binaries = project_builder._get_binaries()
    assert sorted(binaries) == sorted(platform_binaries_expected)


def test_if_binaries_from_runners_are_taken_properly(runners, project_builder: ProjectBuilder):
    runners_binaries = list(runners['config'].values())
    runners_binaries_expected = [bin if os.path.isabs(bin) else os.path.join('zephyr', bin) for bin in runners_binaries]
    binaries = project_builder._get_binaries_from_runners()
    assert sorted(binaries) == sorted(runners_binaries_expected)


def test_if_runners_file_is_sanitized_properly(runners, project_builder: ProjectBuilder):
    runners_file_path = os.path.join(project_builder.instance.build_dir, 'zephyr', 'runners.yaml')
    with open(runners_file_path, 'r') as file:
        unsanitized_runners_content = yaml.safe_load(file)
    unsanitized_runners_binaries = list(unsanitized_runners_content['config'].values())
    abs_paths = [bin for bin in unsanitized_runners_binaries if os.path.isabs(bin)]
    assert len(abs_paths) > 0

    project_builder._sanitize_runners_file()

    with open(runners_file_path, 'r') as file:
        sanitized_runners_content = yaml.safe_load(file)
    sanitized_runners_binaries = list(sanitized_runners_content['config'].values())
    abs_paths = [bin for bin in sanitized_runners_binaries if os.path.isabs(bin)]
    assert len(abs_paths) == 0


def test_if_zephyr_base_is_sanitized_properly(project_builder: ProjectBuilder):
    sanitized_path_expected = os.path.join('sanitized', 'path')
    path_to_sanitize = os.path.join(os.path.realpath(ZEPHYR_BASE), sanitized_path_expected)
    cmakecache_file_path = os.path.join(project_builder.instance.build_dir, 'CMakeCache.txt')
    with open(cmakecache_file_path, 'w') as file:
        file.write(path_to_sanitize)

    project_builder._sanitize_zephyr_base_from_files()

    with open(cmakecache_file_path, 'r') as file:
        sanitized_path = file.read()
    assert sanitized_path == sanitized_path_expected
