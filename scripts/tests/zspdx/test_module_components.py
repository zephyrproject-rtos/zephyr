#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for the module dependency components the walker builds from module metadata."""

import pytest
from conftest import DEPS_DOCUMENT
from spdx_tools.spdx.model.relationship import RelationshipType

MODULE = {
    "name": "mymodule",
    "remote": "https://github.com/vendor/mymodule",
    "revision": "1" * 40,
}

PURL_ZEPHYR_PREFIX = "pkg:github/zephyrproject-rtos/zephyr"
PURL_MODULE_PREFIX = "pkg:github/vendor/mymodule"


@pytest.fixture
def graph(walker_graph):
    return walker_graph([MODULE])


def find_package(document, component_name):
    return next((p for p in document.packages if p.name == component_name), None)


def has_relationship(document, from_id, relationship_type, to_id):
    return any(
        r.spdx_element_id == from_id
        and r.relationship_type == relationship_type
        and str(r.related_spdx_element_id) == to_id
        for r in document.relationships
    )


def doc_ref_id(document, namespace):
    return next(
        (
            ref.document_ref_id
            for ref in document.creation_info.external_document_refs
            if ref.document_uri == namespace
        ),
        None,
    )


class TestPackageComments:
    """Tests that each dependency package explains its own role."""

    @pytest.mark.parametrize("package_name", ["zephyr-deps", "mymodule-deps"])
    def test_deps_package_is_marked_reference_only(self, graph, spdx2_documents, package_name):
        _, documents = spdx2_documents(graph)
        package = find_package(documents[DEPS_DOCUMENT], package_name)
        assert package is not None, f"{package_name} package not found"
        assert package.comment and "Reference-only" in str(package.comment), (
            f"{package_name} comment should mention 'Reference-only', got '{package.comment}'"
        )


class TestPackageProvenance:
    """Tests for the supplier and purl derived from a component's remote URL.

    Both branches of the supplier mapping are covered: a repository under the Zephyr
    GitHub namespace is supplied by the Zephyr Project, anything else by its own
    namespace.
    """

    @pytest.mark.parametrize(
        ("package_name", "supplier", "purl_prefix"),
        [
            pytest.param("zephyr-deps", "The Zephyr Project", PURL_ZEPHYR_PREFIX, id="zephyr"),
            pytest.param("mymodule-deps", "vendor", PURL_MODULE_PREFIX, id="module"),
        ],
    )
    def test_supplier_and_purl(self, graph, spdx2_documents, package_name, supplier, purl_prefix):
        _, documents = spdx2_documents(graph)
        package = find_package(documents[DEPS_DOCUMENT], package_name)
        assert package is not None, f"{package_name} package not found"
        assert str(package.supplier) == f"Organization: {supplier}", (
            f"{package_name} supplier is '{package.supplier}'"
        )
        purls = [r.locator for r in package.external_references if r.reference_type == "purl"]
        assert any(p.startswith(purl_prefix) for p in purls), (
            f"{package_name} missing purl prefix '{purl_prefix}', got {purls}"
        )

    def test_module_purl_carries_the_revision(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        package = find_package(documents[DEPS_DOCUMENT], "mymodule-deps")
        purls = [r.locator for r in package.external_references if r.reference_type == "purl"]
        assert any(p.endswith(f"@{MODULE['revision']}") for p in purls), (
            f"mymodule-deps purl should include the revision, got {purls}"
        )


class TestModuleRelationships:
    """Tests for the VARIANT_OF and DEPENDENCY_OF edges between sources and dependencies."""

    def test_module_deps_dependency_of_zephyr_deps(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        document = documents[DEPS_DOCUMENT]
        module_deps = find_package(document, "mymodule-deps")
        zephyr_deps = find_package(document, "zephyr-deps")
        assert has_relationship(
            document, module_deps.spdx_id, RelationshipType.DEPENDENCY_OF, zephyr_deps.spdx_id
        ), f"expected {module_deps.spdx_id} DEPENDENCY_OF {zephyr_deps.spdx_id}"

    @pytest.mark.parametrize(
        ("sources_name", "deps_name"),
        [
            pytest.param("zephyr-sources", "zephyr-deps", id="zephyr"),
            pytest.param("mymodule-sources", "mymodule-deps", id="module"),
        ],
    )
    def test_sources_variant_of_deps(self, graph, spdx2_documents, sources_name, deps_name):
        """The checked-out sources are Zephyr's variant of the upstream dependency."""
        _, documents = spdx2_documents(graph)
        zephyr_doc, deps_doc = documents["zephyr"], documents[DEPS_DOCUMENT]

        sources = find_package(zephyr_doc, sources_name)
        assert sources is not None, f"{sources_name} package not found"
        deps = find_package(deps_doc, deps_name)
        assert deps is not None, f"{deps_name} package not found"

        ref = doc_ref_id(zephyr_doc, deps_doc.creation_info.document_namespace)
        assert ref is not None, "zephyr document has no external reference to modules-deps"
        target = f"{ref}:{deps.spdx_id}"
        assert has_relationship(zephyr_doc, sources.spdx_id, RelationshipType.VARIANT_OF, target), (
            f"expected {sources.spdx_id} VARIANT_OF {target}"
        )
