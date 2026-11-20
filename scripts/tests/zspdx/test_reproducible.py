#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for the SOURCE_DATE_EPOCH handling that makes the SPDX output reproducible."""

import re
from datetime import UTC, datetime

import pytest
from conftest import DEPS_DOCUMENT, NAMESPACE, SPDX3_VERSIONS
from zspdx import sbom
from zspdx.model import SBOMFile
from zspdx.sbom import DEFAULT_NAMESPACE_BASE, SBOMConfig, default_namespace_prefix, make_spdx
from zspdx.util import source_date_epoch
from zspdx.version import SPDX_VERSION_2_2, SPDX_VERSION_2_3, SPDX_VERSION_3_0

EPOCH = "1700000000"
CREATED = datetime(2023, 11, 14, 22, 13, 20, tzinfo=UTC)
MODULE = {
    "name": "mymodule",
    "remote": "https://github.com/vendor/mymodule",
    "revision": "1" * 40,
}
VERSIONS = [
    pytest.param(SPDX_VERSION_2_2, id="2.2"),
    pytest.param(SPDX_VERSION_2_3, id="2.3"),
    *SPDX3_VERSIONS,
]


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
    """A reproducible prefix is derived from the documents it is used in."""

    def test_random_when_not_reproducible(self, scanned_graph):
        prefixes = {default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3) for _ in range(2)}
        assert len(prefixes) == 2
        assert all(p.startswith(f"{DEFAULT_NAMESPACE_BASE}-") for p in prefixes)

    def test_derived_when_reproducible(self, scanned_graph):
        prefix = default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, True)
        assert prefix.startswith(f"{DEFAULT_NAMESPACE_BASE}-")
        assert prefix == default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, True)
        assert scanned_graph.namespace_prefix == NAMESPACE

    def test_depends_on_creation_time(self, scanned_graph):
        prefix = default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, True)
        scanned_graph.created = CREATED.replace(second=21)
        assert prefix != default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, True)

    def test_depends_on_version(self, scanned_graph):
        assert default_namespace_prefix(
            scanned_graph, SPDX_VERSION_2_3, True
        ) != default_namespace_prefix(scanned_graph, SPDX_VERSION_3_0, True)

    def test_depends_on_file_contents(self, scanned_graph):
        prefix = default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, True)
        scanned_graph.files["/src/a.c"].hashes["SHA256"] = "c" * 64
        assert prefix != default_namespace_prefix(scanned_graph, SPDX_VERSION_2_3, True)

    def test_depends_on_module_revision(self, walker_graph):
        """Builds of the same files with other module metadata do not share a prefix."""
        prefixes = set()
        for revision in ("1" * 40, "2" * 40):
            graph = walker_graph([{**MODULE, "revision": revision}], namespace="")
            graph.created = CREATED
            prefixes.add(default_namespace_prefix(graph, SPDX_VERSION_2_3, True))
        assert len(prefixes) == 2


@pytest.fixture
def run_make_spdx(monkeypatch, walker_graph, tmp_path):
    """Factory running make_spdx over a freshly walked graph, without scanning files.

    Returns the documents written, as ``{file name: bytes}``.
    """

    class _Walker:
        def __init__(self, config):
            self.config = config

        def collect_sbom_graph(self):
            return walker_graph([MODULE], namespace=self.config.namespace_prefix)

    monkeypatch.setattr(sbom, "Walker", _Walker)
    monkeypatch.setattr(sbom, "scan_sbom_graph", lambda config, graph: None)

    def _run(run, version, created=CREATED, namespace_prefix=""):
        output_dir = tmp_path / run
        output_dir.mkdir()
        config = SBOMConfig(
            namespace_prefix=namespace_prefix,
            spdx_dir=str(output_dir),
            spdx_version=version,
            created=created,
        )
        assert make_spdx(config)
        return {p.name: p.read_bytes() for p in output_dir.iterdir()}

    return _run


@pytest.mark.parametrize("version", VERSIONS)
class TestMakeSpdx:
    """Documents written by two runs over the same build."""

    def test_identical_with_creation_time(self, run_make_spdx, version):
        first = run_make_spdx("first", version)
        assert len(first) > 1
        assert first == run_make_spdx("second", version)

    def test_different_without_creation_time(self, run_make_spdx, version):
        assert run_make_spdx("first", version, None) != run_make_spdx("second", version, None)

    def test_creation_time_is_recorded(self, run_make_spdx, version):
        for name, data in run_make_spdx("run", version).items():
            assert b"2023-11-14T22:13:20Z" in data, name

    def test_derived_prefix_is_used(self, run_make_spdx, version):
        text = b"".join(run_make_spdx("run", version).values()).decode()
        assert re.search(rf"{re.escape(DEFAULT_NAMESPACE_BASE)}-[0-9a-f-]{{36}}/", text)
        assert f"{DEFAULT_NAMESPACE_BASE}/" not in text

    def test_explicit_prefix_is_kept(self, run_make_spdx, version):
        text = b"".join(run_make_spdx("run", version, namespace_prefix=NAMESPACE).values())
        assert f"{NAMESPACE}/".encode() in text
        assert f"{DEFAULT_NAMESPACE_BASE}-".encode() not in text


@pytest.mark.parametrize("version", VERSIONS[:2])
def test_external_document_refs_use_derived_namespaces(run_make_spdx, version):
    """References between documents point at the namespaces the documents declare."""
    texts = [data.decode() for data in run_make_spdx("run", version).values()]
    declared = {m for t in texts for m in re.findall(r"^DocumentNamespace: (\S+)$", t, re.M)}
    referenced = {
        m for t in texts for m in re.findall(r"^ExternalDocumentRef: \S+ (\S+) ", t, re.M)
    }
    assert referenced
    assert referenced <= declared
    assert all(n.startswith(f"{DEFAULT_NAMESPACE_BASE}-") for n in declared)
