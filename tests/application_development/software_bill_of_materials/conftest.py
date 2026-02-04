# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Pytest configuration for SPDX content validation tests."""

import os

import pytest
from packaging import version
from spdx_tools.spdx.parser.parse_anything import parse_file


def pytest_addoption(parser):
    """Add command-line options for pytest."""
    parser.addoption(
        "--build-dir",
        action="store",
        required=True,
        help="Path to the build directory containing SPDX files",
    )
    parser.addoption(
        "--spdx-version",
        action="store",
        required=True,
        help="Expected SPDX version (e.g., '2.2' or '2.3')",
    )
    parser.addoption(
        "--source-dir",
        action="store",
        required=True,
        help="Path to the test source directory containing src/main.c",
    )


def pytest_configure(config):
    """Register custom markers."""
    config.addinivalue_line(
        "markers",
        "min_spdx_version(version): skip test if SPDX version is less than specified",
    )


def pytest_runtest_setup(item):
    """Skip tests based on min_spdx_version marker."""
    marker = item.get_closest_marker("min_spdx_version")
    if marker is not None:
        min_version = version.parse(marker.args[0])
        current_version = version.parse(item.config.getoption("--spdx-version"))
        if current_version < min_version:
            pytest.skip(f"Requires SPDX version >= {min_version}, got {current_version}")


@pytest.fixture(scope="session")
def build_dir(request):
    """Fixture providing the build directory path."""
    return request.config.getoption("--build-dir")


@pytest.fixture(scope="session")
def spdx_version(request):
    """Fixture providing the expected SPDX version."""
    return request.config.getoption("--spdx-version")


@pytest.fixture(scope="session")
def source_dir(request):
    """Fixture providing the test source directory path."""
    return request.config.getoption("--source-dir")


@pytest.fixture(scope="session")
def spdx_dir(build_dir):
    """Fixture providing the SPDX directory path."""
    return os.path.join(build_dir, "spdx")


@pytest.fixture(scope="session")
def app_doc(spdx_dir):
    """Fixture providing the parsed app.spdx document."""
    return parse_file(os.path.join(spdx_dir, "app.spdx"))


@pytest.fixture(scope="session")
def zephyr_doc(spdx_dir):
    """Fixture providing the parsed zephyr.spdx document."""
    return parse_file(os.path.join(spdx_dir, "zephyr.spdx"))


@pytest.fixture(scope="session")
def build_doc(spdx_dir):
    """Fixture providing the parsed build.spdx document."""
    return parse_file(os.path.join(spdx_dir, "build.spdx"))


@pytest.fixture(scope="session")
def modules_doc(spdx_dir):
    """Fixture providing the parsed modules-deps.spdx document."""
    return parse_file(os.path.join(spdx_dir, "modules-deps.spdx"))
