#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for the SOURCE_DATE_EPOCH handling that makes the SPDX output reproducible."""

from datetime import UTC, datetime

import pytest
from conftest import DEPS_DOCUMENT
from zspdx.util import source_date_epoch

EPOCH = "1700000000"
CREATED = datetime(2023, 11, 14, 22, 13, 20, tzinfo=UTC)


class TestSourceDateEpoch:
    """Parsing of the SOURCE_DATE_EPOCH environment variable."""

    def test_unset(self, monkeypatch):
        monkeypatch.delenv("SOURCE_DATE_EPOCH", raising=False)
        assert source_date_epoch() is None

    def test_empty(self, monkeypatch):
        monkeypatch.setenv("SOURCE_DATE_EPOCH", "")
        assert source_date_epoch() is None

    def test_valid(self, monkeypatch):
        monkeypatch.setenv("SOURCE_DATE_EPOCH", EPOCH)
        assert source_date_epoch() == CREATED

    @pytest.mark.parametrize("value", ["1.5", "-1", " 1", "1\n", "abc", "١٢"])
    def test_malformed(self, monkeypatch, value):
        monkeypatch.setenv("SOURCE_DATE_EPOCH", value)
        with pytest.raises(ValueError, match="SOURCE_DATE_EPOCH"):
            source_date_epoch()

    def test_out_of_range(self, monkeypatch):
        monkeypatch.setenv("SOURCE_DATE_EPOCH", "9" * 30)
        with pytest.raises(ValueError, match="SOURCE_DATE_EPOCH"):
            source_date_epoch()


@pytest.fixture
def graph(make_graph, module_component):
    graph = make_graph(module_component("mymodule-deps"))
    graph.created = CREATED
    return graph


class TestCreated:
    """The graph's creation time is the one every document records."""

    def test_spdx2(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        # the tag-value parser returns a naive UTC datetime
        assert documents[DEPS_DOCUMENT].creation_info.created == CREATED.replace(tzinfo=None)

    def test_spdx3(self, graph, spdx3_documents):
        spdx, documents = spdx3_documents(graph)
        creation_infos = [
            o for o in documents[DEPS_DOCUMENT].objects if isinstance(o, spdx.CreationInfo)
        ]
        assert creation_infos
        assert all(c.created == CREATED for c in creation_infos)
