#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Fixtures for the scripts/tracing tests."""

import shutil

import pytest
from trace_builder import TSDL_METADATA, parse_metadata


@pytest.fixture
def metadata_file(tmp_path):
    """The tracing subsystem's TSDL metadata, in a directory of its own."""
    path = tmp_path / "metadata"
    shutil.copyfile(TSDL_METADATA, path)
    return path


@pytest.fixture
def event_defs(metadata_file):
    """The {event id: EventDef} table the metadata describes."""
    return parse_metadata(str(metadata_file))
