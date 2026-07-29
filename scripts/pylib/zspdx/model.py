# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
from dataclasses import dataclass, field
from enum import StrEnum
from typing import Any, TypedDict

# SPDX sentinel used when a value is intentionally left unasserted.
NOASSERTION = "NOASSERTION"


class ComponentPurpose(StrEnum):
    """Format-agnostic component purpose types.

    These values intentionally match SPDX package purpose strings so serializers can map them
    directly when the target format supports the same vocabulary.
    """

    APPLICATION = "APPLICATION"
    LIBRARY = "LIBRARY"
    SOURCE = "SOURCE"
    FILE = "FILE"


class RelationshipType(StrEnum):
    """Format-agnostic relationship types used by the SBOM model.

    These values intentionally match SPDX relationship strings for relationship types currently
    emitted by zspdx.
    """

    CONTAINS = "CONTAINS"
    DEPENDENCY_OF = "DEPENDENCY_OF"
    DESCRIBES = "DESCRIBES"
    GENERATED_FROM = "GENERATED_FROM"
    HAS_PREREQUISITE = "HAS_PREREQUISITE"
    STATIC_LINK = "STATIC_LINK"

    @classmethod
    def from_value(cls, value: RelationshipType | str) -> RelationshipType:
        """Return a relationship type from either an enum value or raw SPDX string.

        Args:
            value: Existing enum value or raw relationship string.

        Returns:
            The matching relationship type.

        Raises:
            ValueError: If ``value`` is a string that does not match a known relationship type.
        """
        if isinstance(value, cls):
            return value
        return cls(value)


class ExternalReferenceType(StrEnum):
    """External reference kinds supported by the SBOM model."""

    CPE23 = "cpe23Type"
    PURL = "purl"
    OTHER = "other"


@dataclass
class ExternalReference:
    """Format-agnostic external reference attached to a component.

    Attributes:
        locator: External reference locator, such as a CPE 2.3 string or package URL.
        reference_type: Classified external reference type.
    """

    locator: str
    reference_type: ExternalReferenceType

    def __post_init__(self) -> None:
        if not self.locator:
            raise ValueError("external reference locator is required")

    @classmethod
    def from_locator(cls, locator: str) -> ExternalReference:
        """Classify a raw external reference locator.

        Args:
            locator: Raw external reference string from module metadata or generated data.

        Returns:
            An external reference with ``reference_type`` inferred from the locator prefix.

        Raises:
            ValueError: If ``locator`` is empty.
        """
        if locator.startswith("cpe:2.3:"):
            return cls(locator, ExternalReferenceType.CPE23)
        if locator.startswith("pkg:"):
            return cls(locator, ExternalReferenceType.PURL)
        return cls(locator, ExternalReferenceType.OTHER)


@dataclass
class SBOMFile:
    """Format-agnostic representation of a file in the SBOM graph.

    Attributes:
        path: Absolute file path on disk.
        relative_path: File path relative to the owning component's base directory.
        hashes: File hashes keyed by hash algorithm name.
        concluded_license: Concluded license expression for the file.
        license_info_in_file: License identifiers detected in the file.
        copyright_text: Copyright text detected for the file.
        relationships: Relationships where this file is the source element.
        component: Component that owns this file. Set by ``SBOMGraph.add_file()``.
        metadata: Additional data not represented by the common model fields.
    """

    path: str
    relative_path: str = ""
    hashes: dict[str, str] = field(default_factory=dict)
    concluded_license: str = NOASSERTION
    license_info_in_file: list[str] = field(default_factory=list)
    copyright_text: str = NOASSERTION
    relationships: list[SBOMRelationship] = field(default_factory=list)
    component: SBOMComponent | None = None
    metadata: dict[str, Any] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if not self.path:
            raise ValueError("file path is required")


@dataclass
class SBOMComponent:
    """Format-agnostic representation of a component in the SBOM graph.

    Attributes:
        name: Component name and graph-wide component identifier.
        purpose: Component purpose, or ``None`` when no purpose applies.
        version: Component version, if known.
        revision: Source revision, such as a git commit hash.
        url: Source repository or download URL.
        base_dir: Base directory used to calculate relative paths for owned files.
        files: Files owned by this component, keyed by absolute path.
        relationships: Relationships where this component is the source element.
        concluded_license: Concluded license expression for the component.
        declared_license: Declared license expression for the component.
        license_info_from_files: License identifiers detected in the component's files.
        copyright_text: Copyright text for the component.
        external_references: Structured external references such as CPEs and package URLs.
        supplier: Supplier or vendor name.
        target_build_file: Main build artifact when the component represents a build target.
        metadata: Additional data not represented by the common model fields.
    """

    name: str
    purpose: ComponentPurpose | None = None
    version: str = ""
    revision: str = ""
    url: str = ""
    base_dir: str = ""
    files: dict[str, SBOMFile] = field(default_factory=dict)
    relationships: list[SBOMRelationship] = field(default_factory=list)
    concluded_license: str = NOASSERTION
    declared_license: str = NOASSERTION
    license_info_from_files: list[str] = field(default_factory=list)
    copyright_text: str = NOASSERTION
    external_references: list[ExternalReference] = field(default_factory=list)
    supplier: str = ""
    target_build_file: SBOMFile | None = None
    metadata: dict[str, Any] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if not self.name:
            raise ValueError("component name is required")

    def add_external_reference(self, reference: ExternalReference | str) -> ExternalReference:
        """Add an external reference.

        Args:
            reference: Structured external reference or raw locator string. Raw strings are
                       classified with ``ExternalReference.from_locator()``.

        Returns:
            The structured external reference added to this component.

        Raises:
            TypeError: If ``reference`` is neither a string nor ``ExternalReference``.
            ValueError: If the resulting external reference has an empty locator.
        """
        if isinstance(reference, str):
            reference = ExternalReference.from_locator(reference)
        if not isinstance(reference, ExternalReference):
            raise TypeError("external reference must be a string or ExternalReference")
        self.external_references.append(reference)
        return reference


type SBOMElement = SBOMComponent | SBOMFile


@dataclass
class SBOMRelationship:
    """Format-agnostic relationship between SBOM graph elements.

    Attributes:
        from_element: Source component or file.
        to_elements: Target components or files.
        relationship_type: Type of relationship from source to targets.
        metadata: Additional data not represented by the common model fields.
    """

    from_element: SBOMElement
    to_elements: list[SBOMElement]
    relationship_type: RelationshipType
    metadata: dict[str, Any] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if not self.to_elements:
            raise ValueError("relationship must have at least one target element")


@dataclass
class SBOMDocument:
    """Format-agnostic representation of an SBOM document.

    Attributes:
        name: Document name and graph-wide document identifier. Also used to derive the output
              filename, namespace, and cross-document reference ID.
        title: Human-readable document name emitted as the SPDX ``DocumentName``. Falls back to
               ``name`` when empty, allowing the displayed name to differ from the identifier.
        namespace: Document namespace URI.
        components: Components contained in this document, keyed by component name.
        described_components: Names of the components that are the primary subject(s) of this
            document, emitted as SPDX ``DESCRIBES`` relationships. When empty, the document
            describes none of its components explicitly.
        relationships: Document-level relationships.
        external_documents: Documents referenced by cross-document relationships, keyed by name.
        custom_license_ids: Custom license IDs that need to be declared in this document.
        metadata: Additional data not represented by the common model fields.
    """

    name: str
    title: str = ""
    namespace: str = ""
    components: dict[str, SBOMComponent] = field(default_factory=dict)
    described_components: list[str] = field(default_factory=list)
    relationships: list[SBOMRelationship] = field(default_factory=list)
    external_documents: dict[str, SBOMDocument] = field(default_factory=dict)
    custom_license_ids: set[str] = field(default_factory=set)
    metadata: dict[str, Any] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if not self.name:
            raise ValueError("document name is required")

    def add_component(self, component: SBOMComponent) -> None:
        """Add a component to this document.

        Args:
            component: Component to add.

        Raises:
            ValueError: If this document already contains a component with the same name.
        """
        if component.name in self.components:
            raise ValueError(f"component {component.name} already exists in document {self.name}")
        self.components[component.name] = component

    def add_described_component(self, component: SBOMComponent | str) -> None:
        """Mark a component as a primary subject (SPDX ``DESCRIBES``) of this document.

        Args:
            component: Component object or component name to describe.
        """
        name = component if isinstance(component, str) else component.name
        if name not in self.described_components:
            self.described_components.append(name)

    def get_component(self, name: str) -> SBOMComponent | None:
        """Get a component by name from this document.

        Args:
            name: Component name.

        Returns:
            Matching component, or ``None`` if this document does not contain it.
        """
        return self.components.get(name)

    def add_external_document(self, doc: SBOMDocument) -> None:
        """Register an external document reference.

        Args:
            doc: Referenced document.
        """
        if doc.name and doc.name != self.name:
            self.external_documents[doc.name] = doc

    def get_all_files(self) -> dict[str, SBOMFile]:
        """Get all files from all components in this document.

        Returns:
            Files contained by this document's components, keyed by absolute path.
        """
        all_files = {}
        for component in self.components.values():
            all_files.update(component.files)
        return all_files


class BuildInfo(TypedDict, total=False):
    """Compiler, linker and CMake details stored on ``SBOMBuild.metadata``"""

    # compiler binaries (``cmake_compiler`` is the generic/C compiler path)
    cmake_compiler: str
    cmake_c_compiler: str
    cmake_cxx_compiler: str
    cmake_asm_compiler: str
    # compiler versions and ids, per language
    c_compiler_version: str
    cxx_compiler_version: str
    asm_compiler_version: str
    c_compiler_id: str
    cxx_compiler_id: str
    asm_compiler_id: str
    # linker and archiver binaries and versions
    cmake_linker: str
    cmake_ar: str
    linker_version: str
    ar_version: str
    # build type and target system
    cmake_build_type: str
    cmake_system_name: str
    cmake_system_processor: str
    # CMake generator and version
    cmake_generator: str
    cmake_version: str
    # build environment variables, keyed by name (e.g. BOARD, ARCH)
    environment: dict[str, str]


@dataclass
class SBOMBuild:
    """Format-agnostic identity of the build that produced the graph's artifacts."""

    id: str = ""
    build_type: str = ""
    started_at: str = ""
    finished_at: str = ""
    metadata: BuildInfo = field(default_factory=dict)


@dataclass
class SBOMGraph:
    """Format-agnostic SBOM graph and consistency boundary.

    ``SBOMGraph`` owns the global indexes for documents, components, files, and
    relationships. Public mutation helpers keep those indexes in sync with
    per-document and per-component ownership.

    Attributes:
        namespace_prefix: Prefix used by serializers when a document has no explicit namespace.
        build_dir: Build directory used to collect the SBOM graph.
        documents: Documents in the graph, keyed by document name.
        components: Components in the graph, keyed by component name.
        files: Files in the graph, keyed by absolute path.
        relationships: Relationships in the graph.
        custom_license_ids: Custom license IDs that need to be declared by serializers.
        build: Build that produced the graph's artifacts, or ``None`` if it carries no build.
        metadata: Additional data not represented by the common model fields.
    """

    namespace_prefix: str = ""
    build_dir: str = ""
    documents: dict[str, SBOMDocument] = field(default_factory=dict)
    components: dict[str, SBOMComponent] = field(default_factory=dict)
    files: dict[str, SBOMFile] = field(default_factory=dict)
    relationships: list[SBOMRelationship] = field(default_factory=list)
    custom_license_ids: set[str] = field(default_factory=set)
    build: SBOMBuild | None = None
    metadata: dict[str, Any] = field(default_factory=dict)

    def add_document(self, document: SBOMDocument) -> None:
        """Add a document to the graph.

        Args:
            document: Document to add.

        Raises:
            ValueError: If a document with the same name already exists.
        """
        if document.name in self.documents:
            raise ValueError(f"document {document.name} already exists")
        self.documents[document.name] = document

    def get_document(self, name: str) -> SBOMDocument | None:
        """Get a document by name.

        Args:
            name: Document name.

        Returns:
            Matching document, or ``None`` if the graph does not contain it.
        """
        return self.documents.get(name)

    def add_component(self, component: SBOMComponent, document_name: str) -> None:
        """Add a component to the graph and to an existing document.

        Args:
            component: Component to add.
            document_name: Name of the document that owns the component.

        Raises:
            ValueError: If the component already exists or the document is missing.
        """
        if component.name in self.components:
            raise ValueError(f"component {component.name} already exists")
        document = self.get_document(document_name)
        if document is None:
            raise ValueError(f"document {document_name} does not exist")
        document.add_component(component)
        self.components[component.name] = component

    def add_file(self, file: SBOMFile, component: SBOMComponent) -> None:
        """Add a file to the graph and attach it to a registered component.

        Args:
            file: File to add.
            component: Registered component that owns the file.

        Raises:
            ValueError: If the file already exists, the component is missing, or the component
                        object is not the instance registered in the graph.
        """
        if file.path in self.files:
            raise ValueError(f"file {file.path} already exists")
        registered_component = self.components.get(component.name)
        if registered_component is None:
            raise ValueError(f"component {component.name} does not exist")
        if registered_component is not component:
            raise ValueError(f"component {component.name} is not the registered instance")
        if file.path in component.files:
            raise ValueError(f"file {file.path} already exists in component {component.name}")

        file.component = component
        if not file.relative_path and component.base_dir:
            file.relative_path = os.path.relpath(file.path, component.base_dir)

        component.files[file.path] = file
        self.files[file.path] = file

    def add_relationship(
        self,
        from_element: SBOMElement,
        to_elements: SBOMElement | list[SBOMElement],
        relationship_type: RelationshipType | str,
    ) -> SBOMRelationship:
        """Add a relationship and attach it to the source element.

        Cross-document relationships automatically register the target document
        as an external document reference on the source document.

        Args:
            from_element: Registered component or file that owns the relationship.
            to_elements: Registered target component, file, or list of targets.
            relationship_type: Relationship type enum or raw relationship string.

        Returns:
            Relationship added to the graph.

        Raises:
            TypeError: If any element is not an SBOM component or file.
            ValueError: If any element is not registered in this graph, if no target elements are
                        provided, or if the relationship type is unknown.
        """
        if not isinstance(to_elements, list):
            to_elements = [to_elements]
        self._require_registered_element(from_element)
        for to_element in to_elements:
            self._require_registered_element(to_element)

        relationship = SBOMRelationship(
            from_element=from_element,
            to_elements=to_elements,
            relationship_type=RelationshipType.from_value(relationship_type),
        )
        from_element.relationships.append(relationship)
        self.relationships.append(relationship)

        from_doc = self.get_document_for_element(from_element)
        for to_element in to_elements:
            to_doc = self.get_document_for_element(to_element)
            if from_doc and to_doc and from_doc.name != to_doc.name:
                from_doc.add_external_document(to_doc)

        return relationship

    def get_component(self, name: str) -> SBOMComponent | None:
        """Get a component by name.

        Args:
            name: Component name.

        Returns:
            Matching component, or ``None`` if the graph does not contain it.
        """
        return self.components.get(name)

    def get_file(self, path: str) -> SBOMFile | None:
        """Get a file by absolute path.

        Args:
            path: Absolute file path.

        Returns:
            Matching file, or ``None`` if the graph does not contain it.
        """
        return self.files.get(path)

    def get_document_for_component(self, component: SBOMComponent | str) -> SBOMDocument | None:
        """Get the document containing the given component.

        Args:
            component: Component object or component name.

        Returns:
            Owning document, or ``None`` if no document contains the component.
        """
        component_name = component if isinstance(component, str) else component.name
        for document in self.documents.values():
            if component_name in document.components:
                return document
        return None

    def get_document_for_file(self, file: SBOMFile | str) -> SBOMDocument | None:
        """Get the document containing the given file.

        Args:
            file: File object or absolute file path.

        Returns:
            Owning document, or ``None`` if the file is missing or has no component owner.
        """
        file_obj = self.get_file(file) if isinstance(file, str) else file
        if file_obj and file_obj.component:
            return self.get_document_for_component(file_obj.component)
        return None

    def get_document_for_element(self, element: SBOMElement) -> SBOMDocument | None:
        """Get the document containing the given SBOM element.

        Args:
            element: Component or file.

        Returns:
            Owning document, or ``None`` if the element is not in a document.
        """
        if isinstance(element, SBOMComponent):
            return self.get_document_for_component(element)
        if isinstance(element, SBOMFile):
            return self.get_document_for_file(element)
        return None

    def _require_registered_element(self, element: SBOMElement) -> None:
        """Raise if an element is not registered in this graph."""
        if isinstance(element, SBOMComponent):
            registered_component = self.components.get(element.name)
            if registered_component is not element:
                raise ValueError(f"component {element.name} is not registered")
            return
        if isinstance(element, SBOMFile):
            registered_file = self.files.get(element.path)
            if registered_file is not element:
                raise ValueError(f"file {element.path} is not registered")
            return
        raise TypeError("relationship elements must be SBOMComponent or SBOMFile")
