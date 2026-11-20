#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Tests for the document-level output every generated SPDX document must carry."""

import pytest
from conftest import NAMESPACE, ORGANIZATION, TOOL_NAME, TOOL_VERSION
from spdx_tools.spdx.model.actor import ActorType
from spdx_tools.spdx.model.relationship import RelationshipType
from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document
from zspdx.walker import ZEPHYR_ORGANIZATION, Walker, WalkerConfig

CREATOR_EMAIL = "sbom@example.com"
CREATOR_URL = "https://example.com/sbom"


@pytest.fixture
def graph(walker_graph):
    """A graph spanning more than one document, so every document is checked."""
    return walker_graph([{"name": "mymodule", "remote": "https://github.com/vendor/mymodule"}])


@pytest.fixture
def graph_with_contact(graph):
    """The same graph, with contact details for the SBOM creator."""
    graph.metadata.update({"creator_email": CREATOR_EMAIL, "creator_url": CREATOR_URL})
    return graph


class TestCreatorMetadata:
    """Tests for the SBOM creator the walker records in the graph metadata."""

    @pytest.mark.parametrize(
        ("name", "expected"),
        [
            pytest.param("", ZEPHYR_ORGANIZATION, id="default"),
            pytest.param("Example Manufacturer", "Example Manufacturer", id="override"),
        ],
    )
    def test_creator(self, tmp_path, name, expected):
        config = WalkerConfig()
        config.namespace_prefix = NAMESPACE
        config.build_dir = str(tmp_path)
        config.creator_name = name
        config.creator_email = CREATOR_EMAIL
        config.creator_url = CREATOR_URL
        walker = Walker(config)
        walker._set_creation_metadata(None)
        metadata = walker.sbom_graph.metadata
        assert (
            metadata["creator_organization"],
            metadata["creator_email"],
            metadata["creator_url"],
        ) == (expected, CREATOR_EMAIL, CREATOR_URL)


class TestSPDX2Documents:
    """Tests for the SPDX 2.x tag-value document header and DESCRIBES relationships."""

    def test_spdx_version(self, graph, spdx2_documents):
        version, documents = spdx2_documents(graph)
        for name, document in documents.items():
            assert document.creation_info.spdx_version == f"SPDX-{version}", (
                f"{name}: spdx_version is '{document.creation_info.spdx_version}', "
                f"expected 'SPDX-{version}'"
            )

    def test_documents_are_valid(self, graph, spdx2_documents):
        """Covers the data licence and the rest of what the spec mandates."""
        _, documents = spdx2_documents(graph)
        for name, document in documents.items():
            messages = [m.validation_message for m in validate_full_spdx_document(document)]
            assert messages == [], f"{name}: {messages}"

    def test_document_namespace_and_name(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        for name, document in documents.items():
            assert document.creation_info.document_namespace, f"{name}: namespace is empty"
            assert document.creation_info.name, f"{name}: document name is empty"

    def test_creators(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        for name, document in documents.items():
            creators = [str(c) for c in document.creation_info.creators]
            assert f"Organization: {ORGANIZATION}" in creators, f"{name}: got {creators}"
            assert f"Tool: {TOOL_NAME}-{TOOL_VERSION}" in creators, f"{name}: got {creators}"

    def test_creator_contact(self, graph_with_contact, spdx2_documents):
        """The e-mail address rides on the creator; SPDX 2.x has no room for the URL."""
        _, documents = spdx2_documents(graph_with_contact)
        for name, document in documents.items():
            organizations = [
                (c.name, c.email)
                for c in document.creation_info.creators
                if c.actor_type == ActorType.ORGANIZATION
            ]
            assert organizations == [(ORGANIZATION, CREATOR_EMAIL)], name
            messages = [m.validation_message for m in validate_full_spdx_document(document)]
            assert messages == [], f"{name}: {messages}"

    def test_describes_relationship(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        for name, document in documents.items():
            describes = [
                r
                for r in document.relationships
                if r.relationship_type == RelationshipType.DESCRIBES
            ]
            assert describes, f"{name}: no DESCRIBES relationship found"

    def test_packages_have_valid_spdx_ids(self, graph, spdx2_documents):
        _, documents = spdx2_documents(graph)
        for name, document in documents.items():
            for package in document.packages:
                assert package.spdx_id.startswith("SPDXRef-"), (
                    f"{name}: package '{package.name}' has invalid spdx_id '{package.spdx_id}'"
                )


class TestSPDX3Documents:
    """Tests for the SPDX 3.x document element and its roots."""

    def test_document_has_a_name_and_namespace(self, graph, spdx3_documents):
        spdx, documents = spdx3_documents(graph)
        for name, objects in documents.items():
            document = next(o for o in objects.objects if isinstance(o, spdx.SpdxDocument))
            assert document.name, f"{name}: document name is empty"
            assert document._id.startswith(NAMESPACE), f"{name}: ID is '{document._id}'"

    def test_builds_share_no_element_ids(self, walker_graph, spdx3_documents):
        """Documents of different builds can be loaded into one graph.

        Two builds of the same modules, each with its own namespace prefix, must not
        define any element under the same IRI, or their elements merge into one.
        """
        modules = [{"name": "mymodule", "remote": "https://github.com/vendor/mymodule"}]
        build_ids = []
        for build in ("build-a", "build-b"):
            _, documents = spdx3_documents(walker_graph(modules, namespace=f"{NAMESPACE}/{build}"))
            build_ids.append(
                {
                    o._id
                    for objects in documents.values()
                    for o in objects.objects
                    if o._id and not o._id.startswith("_:")
                }
            )
        assert all(build_ids), "no element IDs found"
        assert build_ids[0].isdisjoint(build_ids[1]), sorted(build_ids[0] & build_ids[1])

    def test_imports_resolve_to_sibling_documents(self, graph, spdx3_documents):
        """Every imported element is defined by the document its locationHint names.

        An import a consumer cannot resolve, such as a License List license that is
        neither defined here nor served as SPDX 3 by spdx.org, is reported as undefined.
        """
        spdx, documents = spdx3_documents(graph)
        defined = {
            f"./{name}.jsonld": {o._id for o in objects.objects if o._id}
            for name, objects in documents.items()
        }
        imports = 0
        for name, objects in documents.items():
            document = next(o for o in objects.objects if isinstance(o, spdx.SpdxDocument))
            for external in document.import_:
                imports += 1
                assert external.externalSpdxId in defined.get(external.locationHint, ()), (
                    f"{name}: {external.externalSpdxId} not defined at {external.locationHint}"
                )
        assert imports, "no cross-document import to check"

    def test_document_declares_root_elements(self, graph, spdx3_documents):
        """The document's only root is a build Sbom, which holds the root packages."""
        spdx, documents = spdx3_documents(graph)
        for name, objects in documents.items():
            document = next(o for o in objects.objects if isinstance(o, spdx.SpdxDocument))
            roots = list(document.rootElement)
            assert len(roots) == 1, f"{name}: expected one root, got {roots}"
            sbom = roots[0]
            assert isinstance(sbom, spdx.software_Sbom), f"{name}: root is {type(sbom)}"
            assert sbom in objects.objects, f"{name}: root Sbom not defined in the document"
            assert list(sbom.software_sbomType) == [spdx.software_SbomType.build], name
            assert list(sbom.rootElement), f"{name}: Sbom declares no root element"

    def test_creators(self, graph, spdx3_documents):
        spdx, documents = spdx3_documents(graph)
        for name, objects in documents.items():
            agents = [o.name for o in objects.objects if isinstance(o, spdx.Agent)]
            assert ORGANIZATION in agents, f"{name}: expected '{ORGANIZATION}' agent, got {agents}"

            tools = [o for o in objects.objects if isinstance(o, spdx.Tool)]
            assert TOOL_NAME in [t.name for t in tools], f"{name}: got {[t.name for t in tools]}"
            # SPDX 3.x has no Tool version field, so the version rides on a purl.
            identifiers = [i.identifier for t in tools for i in t.externalIdentifier]
            assert any(i.endswith(f"@{TOOL_VERSION}") for i in identifiers), (
                f"{name}: no tool identifier carrying version {TOOL_VERSION}, got {identifiers}"
            )

    def test_creator_contact(self, graph_with_contact, spdx3_documents):
        """The creator organization carries its e-mail address and URL."""
        spdx, documents = spdx3_documents(graph_with_contact)
        for name, objects in documents.items():
            document = next(o for o in objects.objects if isinstance(o, spdx.SpdxDocument))
            organizations = [
                a for a in document.creationInfo.createdBy if isinstance(a, spdx.Organization)
            ]
            assert [o.name for o in organizations] == [ORGANIZATION], name
            identifiers = organizations[0].externalIdentifier
            emitted = {(i.externalIdentifierType, i.identifier) for i in identifiers}
            assert emitted == {
                (spdx.ExternalIdentifierType.email, CREATOR_EMAIL),
                (spdx.ExternalIdentifierType.urlScheme, CREATOR_URL),
            }, name
