#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for what the zspdx serializers emit for a build target and its artifact."""

import pytest
from conftest import NAMESPACE
from zspdx.model import SBOMComponent, SBOMDocument, SBOMFile, SBOMGraph

BUILD_DOCUMENT = "build"


@pytest.fixture
def graph():
    """A build document with one target producing an artifact, and one producing none."""
    graph = SBOMGraph(namespace_prefix=NAMESPACE)
    graph.add_document(SBOMDocument(name=BUILD_DOCUMENT, namespace=f"{NAMESPACE}/build"))
    document = graph.get_document(BUILD_DOCUMENT)

    target = SBOMComponent(name="app", base_dir="/build")
    graph.add_component(target, BUILD_DOCUMENT)
    artifact = SBOMFile(path="/build/app/libapp.a", relative_path="app/libapp.a")
    graph.add_file(artifact, target)
    target.target_build_file = artifact
    document.add_described_component(target)

    no_artifact = SBOMComponent(name="offsets", base_dir="/build")
    graph.add_component(no_artifact, BUILD_DOCUMENT)
    document.add_described_component(no_artifact)
    return graph


class TestSPDX3Serialization:
    """Tests for the SPDX 3.x JSON-LD output, at every supported version."""

    @staticmethod
    def _distribution_artifacts(spdx, objects):
        """Map each package name to the names of its distribution artifacts."""
        names = {o._id: o.name for o in objects if isinstance(o, spdx.Element)}
        artifacts = {}
        for rel in objects:
            if (
                isinstance(rel, spdx.Relationship)
                and rel.relationshipType == spdx.RelationshipType.hasDistributionArtifact
            ):
                artifacts.setdefault(names[rel.from_._id], []).extend(names[t._id] for t in rel.to)
        return artifacts

    def test_target_has_its_artifact(self, graph, spdx3_documents):
        """Only a target that produced an artifact gets one, and it is that artifact."""
        spdx, documents = spdx3_documents(graph)
        artifacts = self._distribution_artifacts(spdx, documents[BUILD_DOCUMENT].objects)
        assert artifacts == {"app": ["app/libapp.a"]}
