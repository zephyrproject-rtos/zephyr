#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for what the zspdx serializers emit for a module dependency component."""

import pytest
from conftest import DEPS_DOCUMENT
from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document
from zspdx.model import ExternalReference, ExternalReferenceType

CPE = "cpe:2.3:a:vendor:product:1.2.3:*:*:*:*:*:*:*"
PURL = "pkg:github/vendor/product@1.2.3"


class TestExternalReference:
    """Tests for locator classification in the format-agnostic model."""

    @pytest.mark.parametrize(
        ("locator", "expected"),
        [
            pytest.param(CPE, ExternalReferenceType.CPE23, id="cpe"),
            pytest.param(PURL, ExternalReferenceType.PURL, id="purl"),
            pytest.param("https://example.com/product", ExternalReferenceType.OTHER, id="other"),
        ],
    )
    def test_from_locator(self, locator, expected):
        assert ExternalReference.from_locator(locator).reference_type is expected

    def test_empty_locator_rejected(self):
        with pytest.raises(ValueError):
            ExternalReference.from_locator("")


@pytest.fixture
def graph(make_graph, module_component):
    """A graph with one component carrying external references, and one without."""
    return make_graph(
        module_component("mymodule-deps", [CPE, PURL]),
        module_component("other-deps"),
    )


class TestSPDX2Serialization:
    """Tests for the SPDX 2.x tag-value output."""

    def test_document_is_valid(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        assert validate_full_spdx_document(documents[DEPS_DOCUMENT]) == []

    def test_every_component_becomes_a_package(self, graph, spdx2_documents):
        """A component keeps its own name, whether or not it carries a CPE."""
        _, documents = spdx2_documents(graph)
        assert {p.name for p in documents[DEPS_DOCUMENT].packages} == {
            "mymodule-deps",
            "other-deps",
        }

    def test_external_references(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        package = next(p for p in documents[DEPS_DOCUMENT].packages if p.name == "mymodule-deps")
        emitted = {
            (str(r.category.name), r.reference_type, r.locator) for r in package.external_references
        }
        assert emitted == {("SECURITY", "cpe23Type", CPE), ("PACKAGE_MANAGER", "purl", PURL)}

    def test_component_without_references(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        package = next(p for p in documents[DEPS_DOCUMENT].packages if p.name == "other-deps")
        assert package.external_references == []


class TestSPDX3Serialization:
    """Tests for the SPDX 3.x JSON-LD output, at every supported version."""

    def test_every_component_becomes_a_package(self, graph, spdx3_documents):
        spdx, documents = spdx3_documents(graph)
        packages = [
            o for o in documents[DEPS_DOCUMENT].objects if isinstance(o, spdx.software_Package)
        ]
        assert sorted(p.name for p in packages) == ["mymodule-deps", "other-deps"]

    def test_external_identifiers(self, graph, spdx3_documents):
        spdx, documents = spdx3_documents(graph)
        package = next(
            o
            for o in documents[DEPS_DOCUMENT].objects
            if isinstance(o, spdx.software_Package) and o.name == "mymodule-deps"
        )
        emitted = {(i.externalIdentifierType, i.identifier) for i in package.externalIdentifier}
        assert emitted == {
            (spdx.ExternalIdentifierType.cpe23, CPE),
            (spdx.ExternalIdentifierType.packageUrl, PURL),
        }

    def test_component_without_references(self, graph, spdx3_documents):
        spdx, documents = spdx3_documents(graph)
        package = next(
            o
            for o in documents[DEPS_DOCUMENT].objects
            if isinstance(o, spdx.software_Package) and o.name == "other-deps"
        )
        assert package.externalIdentifier == []
