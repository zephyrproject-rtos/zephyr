#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for how the zspdx serializers describe the final image among the build artifacts."""

import pytest
from conftest import NAMESPACE
from zspdx.model import ComponentPurpose, SBOMComponent, SBOMDocument, SBOMFile, SBOMGraph

BUILD_DOCUMENT = "build"


@pytest.fixture
def graph():
    """A build document with the final image target and a library target."""
    graph = SBOMGraph(namespace_prefix=NAMESPACE)
    graph.add_document(SBOMDocument(name=BUILD_DOCUMENT, namespace=f"{NAMESPACE}/build"))
    document = graph.get_document(BUILD_DOCUMENT)

    for name, purpose, artifact in (
        ("zephyr_final", ComponentPurpose.APPLICATION, "zephyr/zephyr.elf"),
        ("app", ComponentPurpose.LIBRARY, "app/libapp.a"),
    ):
        target = SBOMComponent(name=name, purpose=purpose, base_dir="/build")
        graph.add_component(target, BUILD_DOCUMENT)
        build_file = SBOMFile(path=f"/build/{artifact}", relative_path=artifact)
        graph.add_file(build_file, target)
        target.target_build_file = build_file
        document.add_described_component(target)
    return graph


class TestSPDX3Serialization:
    """Tests for the SPDX 3.x JSON-LD output, at every supported version."""

    def test_artifact_purposes(self, graph, spdx3_documents):
        """Only the final image is an executable used as firmware."""
        spdx, documents = spdx3_documents(graph)
        purposes = {
            o.name: (o.software_primaryPurpose, list(o.software_additionalPurpose))
            for o in documents[BUILD_DOCUMENT].objects
            if isinstance(o, spdx.software_File)
        }
        assert purposes == {
            "zephyr/zephyr.elf": (
                spdx.software_SoftwarePurpose.executable,
                [spdx.software_SoftwarePurpose.firmware],
            ),
            "app/libapp.a": (spdx.software_SoftwarePurpose.file, []),
        }
