#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Shared fixtures for the zspdx unit tests.

These fixtures build an SBOM graph by hand and run the serializers over it, so the suite
covers everything that follows from the graph: the model's rules, and how each SPDX
version renders them. No Zephyr build is involved, so anything that depends on real build
artifacts -- file hashes, detected licences and copyrights, CMake targets, build
provenance -- belongs in tests/application_development/software_bill_of_materials
instead.
"""

import importlib
import os
import sys

import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib"))

from spdx_tools.spdx.parser.tagvalue import tagvalue_parser  # noqa: E402
from zspdx.model import ComponentPurpose, SBOMComponent, SBOMDocument, SBOMGraph  # noqa: E402
from zspdx.serializers.spdx2 import SPDX2Serializer  # noqa: E402
from zspdx.serializers.spdx3 import SPDX3Serializer  # noqa: E402
from zspdx.version import (  # noqa: E402
    SPDX_VERSION_2_2,
    SPDX_VERSION_2_3,
    SPDX_VERSION_3_0,
    SPDX_VERSION_3_1,
)
from zspdx.walker import SOURCES_COMMENT, Walker, WalkerConfig  # noqa: E402

NAMESPACE = "https://example.com/sbom"

# The document module components live in, and so the one most fixtures serialize.
DEPS_DOCUMENT = "modules-deps"

# Deliberately fictional: these only have to travel from the graph's metadata into a
# document's creators, so real Zephyr values would suggest the identity is what is
# being asserted.
ORGANIZATION = "Example Organization"
TOOL_NAME = "Example SBOM Builder"
TOOL_VERSION = "0.0.1"

# Stands in for the 'zephyr' entry of a build's metadata file.
ZEPHYR_META = {
    "remote": "https://github.com/zephyrproject-rtos/zephyr",
    "revision": "0" * 40,
    "tags": ["v4.3.0"],
}

# Bindings module per SPDX 3.x version, mirroring the serializer's own selection.
SPDX3_BINDINGS = {SPDX_VERSION_3_0: "v3_0_1", SPDX_VERSION_3_1: "v3_1"}

# SPDX 3.1 support is experimental and no released spdx-python-model carries its
# bindings yet, so that leg skips until one does.
SPDX3_VERSIONS = [
    pytest.param(
        version,
        id=str(version),
        marks=pytest.mark.skipif(
            not hasattr(importlib.import_module("spdx_python_model"), bindings),
            reason=f"spdx-python-model has no {bindings} bindings",
        ),
    )
    for version, bindings in SPDX3_BINDINGS.items()
]


@pytest.fixture
def module_component():
    """Factory for the reference-only component the walker builds for a module."""

    def _component(name, external_references=()):
        component = SBOMComponent(name=name, comment="Module dependency; no files")
        for reference in external_references:
            component.add_external_reference(reference)
        return component

    return _component


@pytest.fixture
def make_graph():
    """Factory turning components into a described modules-deps graph."""

    def _graph(*components):
        graph = SBOMGraph(namespace_prefix=NAMESPACE)
        graph.metadata.update(
            {
                "creator_organization": ORGANIZATION,
                "tool_name": TOOL_NAME,
                "tool_version": TOOL_VERSION,
            }
        )
        graph.add_document(
            SBOMDocument(name=DEPS_DOCUMENT, namespace=f"{NAMESPACE}/{DEPS_DOCUMENT}")
        )
        document = graph.get_document(DEPS_DOCUMENT)
        for component in components:
            graph.add_component(component, DEPS_DOCUMENT)
            document.add_described_component(component)
        return graph

    return _graph


@pytest.fixture(params=[SPDX_VERSION_2_2, SPDX_VERSION_2_3], ids=["2.2", "2.3"])
def spdx2_documents(request, tmp_path):
    """Factory serializing a graph to SPDX 2.x tag-value and parsing every document back.

    Returns ``(spec version, {document name: parsed document})``. Parametrized over every
    supported SPDX 2.x version.
    """
    version = request.param

    def _documents(graph):
        assert SPDX2Serializer(graph, version).serialize(str(tmp_path))
        return version, {
            name: tagvalue_parser.parse_from_file(str(tmp_path / f"{name}.spdx"))
            for name, document in graph.documents.items()
            if document.components
        }

    return _documents


@pytest.fixture(params=SPDX3_VERSIONS)
def spdx3_documents(request, tmp_path):
    """Factory serializing a graph at one SPDX 3.x version.

    Returns ``(bindings module, {document name: object set})`` so a test can use the same
    classes the serializer emitted. Parametrized over every supported SPDX 3.x version.
    """
    version = request.param
    spdx = getattr(importlib.import_module("spdx_python_model"), SPDX3_BINDINGS[version])

    def _documents(graph):
        assert SPDX3Serializer(graph, version).serialize(str(tmp_path))
        object_sets = {}
        for name, document in graph.documents.items():
            if not document.components:
                continue
            object_set = spdx.SHACLObjectSet()
            with open(tmp_path / f"{name}.jsonld", "rb") as f:
                spdx.JSONLDDeserializer().read(f, object_set)
            object_sets[name] = object_set
        return spdx, object_sets

    return _documents


@pytest.fixture
def walker_graph(tmp_path):
    """Factory building the module half of the graph the way the walker does.

    Takes the module entries a build's metadata file would carry. The '-sources'
    components are added directly rather than through the walker, whose
    setup_zephyr_component() needs a west workspace and the CMake file-API; everything
    the module dependency half contributes comes from the walker itself.
    """

    def _graph(modules, zephyr=None):
        config = WalkerConfig()
        config.namespace_prefix = NAMESPACE
        config.build_dir = str(tmp_path)
        walker = Walker(config)
        walker.setup_documents()
        walker.sbom_graph.metadata.update(
            {
                "creator_organization": ORGANIZATION,
                "tool_name": TOOL_NAME,
                "tool_version": TOOL_VERSION,
            }
        )

        for name in ["zephyr"] + [module["name"] for module in modules]:
            sources = SBOMComponent(
                name=f"{name}-sources",
                purpose=ComponentPurpose.SOURCE,
                comment=SOURCES_COMMENT,
            )
            walker.sbom_graph.add_component(sources, "zephyr")
            walker.doc_zephyr.add_described_component(sources)

        assert walker.setup_modules_deps_component(modules, zephyr or ZEPHYR_META)
        walker.walk_relationships()
        return walker.sbom_graph

    return _graph
