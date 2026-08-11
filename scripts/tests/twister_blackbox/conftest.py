#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Shared fixtures and helpers for twister blackbox tests."""

import contextlib
import logging
import os
import shutil
import sys
from unittest import mock

import pytest

ZEPHYR_BASE = os.getenv('ZEPHYR_BASE')
# Resolve any symlink so paths stored by twister (which also resolves) match.
TEST_DATA = os.path.realpath(
    os.path.join(ZEPHYR_BASE, 'scripts', 'tests', 'twister_blackbox', 'test_data')
)

sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts/pylib/twister'))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts'))

# Twister test YAML files in test_data/ are named test_data.yaml.
# Override the scan filename to find them.
TEST_FILENAME_MOCK = mock.PropertyMock(return_value=['test_data.yaml'])


# The 'fast', 'build' and 'run' markers are declared in pytest.ini.

# ---------------------------------------------------------------------------
# External tool requirements
# ---------------------------------------------------------------------------


def requires_tool(name):
    """Return a skip marker for tests that need ``name`` on PATH.

    Twister aborts outright when an option's backing executable is missing,
    so such tests must be skipped rather than fail on hosts without it.
    """
    return pytest.mark.skipif(
        shutil.which(name) is None, reason=f'{name} executable not found in PATH'
    )


# ---------------------------------------------------------------------------
# Log isolation
# ---------------------------------------------------------------------------


def _reset_logging():
    """Remove all handlers from every known logger.

    The twister logger accumulates file handlers across repeated calls inside
    the same process; clearing them prevents pytest from reporting spurious
    'already closed' errors and keeps captured output clean.
    """
    root = logging.getLogger()
    manager_loggers = list(logging.Logger.manager.loggerDict.values())
    named_loggers = [logging.getLogger(n) for n in logging.root.manager.loggerDict]
    for logger in [root] + manager_loggers + named_loggers:
        for handler in getattr(logger, 'handlers', [])[:]:
            logger.removeHandler(handler)
            with contextlib.suppress(Exception):
                handler.close()


@pytest.fixture(autouse=True)
def reset_logging_before_test():
    """Isolate logging state for every test."""
    _reset_logging()
    yield
    _reset_logging()


# ---------------------------------------------------------------------------
# Output directory
# ---------------------------------------------------------------------------


@pytest.fixture()
def out_path(tmp_path):
    """Provide a fresh output directory path; clean up after the test."""
    container = tmp_path / 'bb-out-container'
    container.mkdir()
    path = str(container / 'bb-out')
    yield path
    _reset_logging()
    shutil.rmtree(container, ignore_errors=True)


# ---------------------------------------------------------------------------
# Common path fixtures
# ---------------------------------------------------------------------------


@pytest.fixture(scope='session')
def zephyr_base():
    return ZEPHYR_BASE


@pytest.fixture(scope='session')
def test_data():
    return TEST_DATA


# ---------------------------------------------------------------------------
# Convenience helpers used by tests
# ---------------------------------------------------------------------------


def read_testplan(out_path):
    """Return the parsed testplan.json from a twister output directory."""
    import json

    with open(os.path.join(out_path, 'testplan.json')) as fh:
        return json.load(fh)


def active_testcases(plan):
    """Return (platform, suite_name, tc_identifier) for non-filtered test cases."""
    return [
        (ts['platform'], ts['name'], tc['identifier'])
        for ts in plan['testsuites']
        for tc in ts['testcases']
        if 'reason' not in tc
    ]


def active_testsuites(plan):
    """Return testsuite dicts that are not filtered/skipped."""
    return [ts for ts in plan['testsuites'] if ts.get('status') != 'skipped']


# ---------------------------------------------------------------------------
# Session-scoped pre-built ELF cache
# ---------------------------------------------------------------------------

_SESSION_ELF_CACHE: dict[str, str | None] = {}


@pytest.fixture(scope='session')
def hello_world_elf(tmp_path_factory):
    """Build hello_world for qemu_x86 once per session and return the ELF path.

    Returns ``None`` when the build fails so callers can skip rather than fail.
    The output directory is kept for the lifetime of the session.
    """
    cache_key = 'hello_world_qemu_x86'
    if cache_key in _SESSION_ELF_CACHE:
        return _SESSION_ELF_CACHE[cache_key]

    from twisterlib.testplan import TestPlan
    from twisterlib.twister_main import main as twister_main

    build_dir = str(tmp_path_factory.mktemp('hello_world_session'))
    hello_world = os.path.join(TEST_DATA, 'samples', 'hello_world')

    _reset_logging()
    with mock.patch.object(
        TestPlan, 'TEST_DEFINITION_FILENAME', mock.PropertyMock(return_value=['test_data.yaml'])
    ):
        result = twister_main(['-i', '--outdir', build_dir, '-T', hello_world, '-p', 'qemu_x86'])
    _reset_logging()

    elf_path = None
    if result == 0:
        for root, _, files in os.walk(build_dir):
            for fname in files:
                if fname == 'zephyr.elf':
                    elf_path = os.path.join(root, fname)
                    break
            if elf_path:
                break

    _SESSION_ELF_CACHE[cache_key] = elf_path
    return elf_path
