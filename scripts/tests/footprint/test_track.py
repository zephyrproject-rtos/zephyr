#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the footprint tracking script (scripts/footprint/track.py)."""

import os
import sys
from unittest.mock import MagicMock

from git import GitCommandError

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "footprint"))

from track import get_version  # noqa: E402


def test_get_version_falls_back_when_no_tags():
    fake_hash = "deadbeefcafe"
    g = MagicMock()
    g.describe.side_effect = GitCommandError(
        "describe", 128, "fatal: No names found, cannot describe anything."
    )
    g.rev_parse.return_value = fake_hash

    assert get_version(g) == fake_hash
    g.rev_parse.assert_called_once_with("--short=12", "HEAD")


def test_get_version_uses_describe_when_available():
    fake_version = "v1.2.3-4-gabcdef012345"
    g = MagicMock()
    g.describe.return_value = fake_version

    assert get_version(g) == fake_version
    g.rev_parse.assert_not_called()
