#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for the SOURCE_DATE_EPOCH handling that makes the SPDX output reproducible."""

from datetime import UTC, datetime

import pytest
from conftest import DEPS_DOCUMENT
from zspdx.model import SBOMFile
from zspdx.sbom import DEFAULT_NAMESPACE_BASE, default_namespace_prefix
from zspdx.serializers.spdx2 import SPDX2Serializer
from zspdx.serializers.spdx3 import SPDX3Serializer
from zspdx.util import source_date_epoch
from zspdx.version import SPDX_VERSION_2_3, SPDX_VERSION_3_0

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


@pytest.fixture
def scanned_graph(graph):
    """The graph with files carrying the hashes the scanner records."""
    component = graph.components["mymodule-deps"]
    for name, digest in (("a.c", "a"), ("b.c", "b")):
        file = SBOMFile(
            path=f"/src/{name}",
            relative_path=name,
            hashes={"SHA1": digest * 40, "SHA256": digest * 64},
        )
        graph.add_file(file, component)
    return graph


class TestDefaultNamespacePrefix:
    """The default namespace prefix is derived from a fixed creation time."""

    def test_random_without_creation_time(self, scanned_graph):
        prefixes = {default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3) for _ in range(2)}
        assert len(prefixes) == 2
        assert all(p.startswith(f"{DEFAULT_NAMESPACE_BASE}-") for p in prefixes)

    def test_derived_from_creation_time(self, scanned_graph):
        prefix = default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, CREATED)
        assert prefix.startswith(f"{DEFAULT_NAMESPACE_BASE}-")
        assert prefix == default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, CREATED)

    def test_depends_on_creation_time(self, scanned_graph):
        later = CREATED.replace(second=21)
        assert default_namespace_prefix(
            scanned_graph, SPDX_VERSION_2_3, CREATED
        ) != default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, later)

    def test_depends_on_version(self, scanned_graph):
        assert default_namespace_prefix(
            scanned_graph, SPDX_VERSION_2_3, CREATED
        ) != default_namespace_prefix(scanned_graph, SPDX_VERSION_3_0, CREATED)

    def test_depends_on_file_contents(self, scanned_graph):
        prefix = default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, CREATED)
        scanned_graph.files["/src/a.c"].hashes["SHA256"] = "c" * 64
        assert prefix != default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, CREATED)


@pytest.mark.parametrize(
    "serializer, version",
    [(SPDX2Serializer, SPDX_VERSION_2_3), (SPDX3Serializer, SPDX_VERSION_3_0)],
    ids=["2.3", "3.0"],
)
def test_output_is_reproducible(scanned_graph, tmp_path, serializer, version):
    """Serializing the same graph twice gives byte-identical documents."""
    scanned_graph.namespace_prefix = default_namespace_prefix(scanned_graph, version, CREATED)
    outputs = []
    for run in ("first", "second"):
        output_dir = tmp_path / run
        output_dir.mkdir()
        assert serializer(scanned_graph, version).serialize(str(output_dir))
        outputs.append({p.name: p.read_bytes() for p in output_dir.iterdir()})
    assert outputs[0]
    assert outputs[0] == outputs[1]
