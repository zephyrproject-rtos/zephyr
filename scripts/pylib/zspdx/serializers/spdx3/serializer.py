# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import re
from datetime import UTC, datetime

from spdx_python_model import v3_0_1 as spdx

from zspdx.model import (
    NOASSERTION,
    ComponentPurpose,
    ExternalReferenceType,
    RelationshipType,
    SBOMComponent,
    SBOMDocument,
    SBOMFile,
    SBOMGraph,
    SBOMRelationship,
)
from zspdx.serializers.helpers import (
    CPE23TYPE_REGEX,
    PURL_REGEX,
    generate_download_url,
    get_standard_licenses,
    normalize_spdx_name,
)
from zspdx.spdxids import get_unique_file_id

_logger = logging.getLogger(__name__)


class SPDX3Serializer:
    """Serializer that converts SBOMGraph to SPDX 3.0 format (JSON-LD)."""

    # Human-readable document name per SBOMDocument.
    _DOCUMENT_NAMES = {
        "app": "Zephyr Application",
        "zephyr": "Zephyr RTOS",
        "build": "Zephyr Build Artifacts",
        "modules-deps": "Zephyr Module Dependencies",
        "sdk": "Zephyr SDK",
    }

    # Name of the SBOMDocument that hosts the Build profile (build targets).
    _BUILD_DOCUMENT = "build"

    # build_buildType is mandatory and must be a URI; used when the SBOMBuild
    # does not provide an absolute URI of its own.
    _DEFAULT_BUILD_TYPE = "urn:spdx.dev:zephyr-cmake"

    def __init__(self, sbom_graph: SBOMGraph, spdx_version=None):
        self.sbom_data = sbom_graph
        self.spdx_version = spdx_version  # Not used for SPDX 3.0, but kept for API consistency

        # Track SPDX 3.0 elements
        self.elements = []  # All SPDX3 elements (packages, files, relationships, etc.)
        self.component_elements = {}  # component_name -> software_Package
        self.file_elements = {}  # file_path -> software_File
        self.relationship_elements = []  # List of Relationship objects

        # Track original from_id for relationships (before reversal)
        # This is used to assign cross-document relationships to the correct document
        # Key: relationship._id, Value: original from_id
        self.relationship_original_from = {}

        # Shared objects
        self.tool = None  # Tool element for createdUsing
        self.creator_agent = None  # SoftwareAgent for createdBy
        self.creation_info = None
        self.documents = {}  # doc_name -> SpdxDocument

        # SPDX 3.0 Build profile state
        self.build = None  # overall build_Build element
        self.target_builds = {}  # component name -> per-target build_Build element
        self.build_tools = {}  # tool key -> Tool element
        self.build_info = sbom_graph.build.metadata if sbom_graph.build else {}

        # Track file IDs for uniqueness
        self.filename_counts = {}

        # Namespace prefixes for shortened IDs
        self.namespace_prefixes = {}
        if self.sbom_data.namespace_prefix:
            prefix = "zephyr"
            uri = self.sbom_data.namespace_prefix.rstrip("/") + "/"
            self.namespace_prefixes[uri] = prefix

    def _shorten_id(self, full_uri: str) -> str:
        """Shorten a URI-based ID using known namespace prefixes."""
        for ns_uri, prefix in self.namespace_prefixes.items():
            if full_uri.startswith(ns_uri):
                return full_uri.replace(ns_uri, f"{prefix}:", 1)
        return full_uri

    def _generate_package_id(self, component_name: str) -> str:
        """Generate URI-based ID for a package."""
        normalized = normalize_spdx_name(component_name)
        namespace = self.sbom_data.namespace_prefix.rstrip("/")
        full_uri = f"{namespace}/packages/{normalized}"
        return self._shorten_id(full_uri)

    def _generate_file_id(self, file_path: str) -> str:
        """Generate URI-based ID for a file."""
        filename_only = os.path.basename(file_path)
        unique_id = get_unique_file_id(filename_only, self.filename_counts)
        # Remove "SPDXRef-" prefix and normalize
        normalized_id = unique_id.replace("SPDXRef-", "").replace("_", "-")
        namespace = self.sbom_data.namespace_prefix.rstrip("/")
        full_uri = f"{namespace}/files/{normalized_id}"
        return self._shorten_id(full_uri)

    def _generate_relationship_id(self, index: int) -> str:
        """Generate URI-based ID for a relationship."""
        namespace = self.sbom_data.namespace_prefix.rstrip("/")
        full_uri = f"{namespace}/relationships/{index}"
        return self._shorten_id(full_uri)

    def _purpose_to_spdx3(self, purpose: ComponentPurpose) -> spdx.software_SoftwarePurpose:
        """Convert ComponentPurpose enum to SPDX 3.0 SoftwarePurpose."""
        purpose_map = {
            ComponentPurpose.APPLICATION: spdx.software_SoftwarePurpose.application,
            ComponentPurpose.LIBRARY: spdx.software_SoftwarePurpose.library,
            ComponentPurpose.SOURCE: spdx.software_SoftwarePurpose.source,
            ComponentPurpose.FILE: spdx.software_SoftwarePurpose.file,
        }
        return purpose_map.get(purpose, spdx.software_SoftwarePurpose.library)

    def _initialize_shared_objects(self):
        """Initialize shared Tool and CreationInfo objects.

        Per SPDX 3.0.1 spec:
        - CreationInfo.createdBy takes Agent (who created the SPDX data) - REQUIRED
        - CreationInfo.createdUsing takes Tool (what tool created the SPDX data) - optional
        """
        namespace = self.sbom_data.namespace_prefix.rstrip("/")

        # Create a SoftwareAgent representing the automated SBOM generation process
        # This satisfies the required createdBy field
        if self.tool is None:
            # Create the creator agent (required by createdBy)
            self.creator_agent = spdx.SoftwareAgent()
            self.creator_agent._id = self._shorten_id(f"{namespace}/agents/west-spdx-agent")
            self.creator_agent.name = "West SPDX Generator"
            self.elements.append(self.creator_agent)

            # Create the tool (for createdUsing)
            self.tool = spdx.Tool()
            self.tool._id = self._shorten_id(f"{namespace}/tools/west-spdx")
            self.tool.name = "West SPDX Tool"
            self.elements.append(self.tool)

        if self.creation_info is None:
            self.creation_info = spdx.CreationInfo()
            self.creation_info._id = self._shorten_id(f"{namespace}/creationinfo")
            self.creation_info.created = datetime.now(UTC)
            # createdBy references the Agent that created this SPDX document (REQUIRED)
            self.creation_info.createdBy.append(self.creator_agent._id)
            # createdUsing references the Tool that created this SPDX document (optional)
            self.creation_info.createdUsing.append(self.tool._id)
            self.creation_info.specVersion = "3.0.1"
            self.elements.append(self.creation_info)

            # Now set the tool's and agent's creationInfo
            self.tool.creationInfo = self.creation_info._id
            self.creator_agent.creationInfo = self.creation_info._id

        # Build profile elements; no-ops when no build information was collected.
        self._create_build_tools()
        self._create_build_object()

    # ---- SPDX 3.0 Build profile -------------------------------------------------

    def _create_tool(
        self, key: str, name: str, path: str = "", version: str = "", identifiers=None
    ) -> spdx.Tool:
        """Create and register a build Tool element keyed by ``key``.

        Tool identity lives on the Tool itself: ``path`` is recorded as an ``other`` external
        identifier and ``version`` as a ``packageUrl`` (``pkg:generic/<tool>@<version>``), since
        SPDX 3.0 has no Tool version field. Extra ``(ExternalIdentifierType, value)`` tuples may be
        passed in ``identifiers``.
        """
        namespace = self.sbom_data.namespace_prefix.rstrip("/")
        tool = spdx.Tool()
        tool._id = self._shorten_id(f"{namespace}/tools/{key}")
        tool.name = name
        tool.creationInfo = self.creation_info._id

        ext_ids = []
        if path:
            ext_ids.append((spdx.ExternalIdentifierType.other, path))
        if version:
            pkg = os.path.basename(path) if path else key
            ext_ids.append((spdx.ExternalIdentifierType.packageUrl, f"pkg:generic/{pkg}@{version}"))
        ext_ids += identifiers or []

        for id_type, id_value in ext_ids:
            if not id_value:
                continue
            ext_id = spdx.ExternalIdentifier()
            ext_id.externalIdentifierType = id_type
            ext_id.identifier = id_value
            tool.externalIdentifier.append(ext_id)

        self.elements.append(tool)
        self.build_tools[key] = tool
        return tool

    def _create_build_tools(self):
        """Create a Tool element for each build tool we collected information for."""
        if self.build_tools or not self.build_info:
            return

        info = self.build_info
        self._create_cmake_tool(info)

        # the C compiler also records the target architecture
        c_path = info.get("cmake_compiler") or info.get("cmake_c_compiler", "")
        if c_path:
            version = info.get("c_compiler_version", "")
            self._create_tool(
                "c-compiler",
                self._compiler_tool_name("C Compiler", c_path, version),
                path=c_path,
                version=version,
                identifiers=self._compiler_arch_identifiers(info),
            )

        tool_specs = [
            ("cxx-compiler", "C++ Compiler", "cmake_cxx_compiler", "cxx_compiler_version"),
            ("asm-compiler", "Assembler", "cmake_asm_compiler", "asm_compiler_version"),
            ("linker", "Linker", "cmake_linker", "linker_version"),
            ("archiver", "Archiver", "cmake_ar", "ar_version"),
        ]
        for key, label, path_key, version_key in tool_specs:
            path = info.get(path_key, "")
            # skip missing tools and the C++ compiler when it is the same binary as the C compiler
            if not path or (key == "cxx-compiler" and path == c_path):
                continue
            version = info.get(version_key, "")
            self._create_tool(
                key,
                self._compiler_tool_name(label, path, version),
                path=path,
                version=version,
            )

    def _create_cmake_tool(self, info: dict):
        """Create the CMake build Tool from the collected build info, when present."""
        if not (info.get("cmake_version") or info.get("cmake_generator")):
            return
        name = "CMake"
        if info.get("cmake_version"):
            name += f" {info['cmake_version']}"
        if info.get("cmake_generator"):
            name += f" ({info['cmake_generator']})"
        self._create_tool("cmake", name, version=info.get("cmake_version", ""))

    @staticmethod
    def _compiler_arch_identifiers(info: dict):
        """Target-architecture external identifier for a compiler, or ``None`` if unknown."""
        processor = info.get("cmake_system_processor")
        if not processor:
            return None
        return [(spdx.ExternalIdentifierType.other, f"target-arch:{processor}")]

    @staticmethod
    def _compiler_tool_name(label: str, path: str, version: str) -> str:
        """Build a tool display name like ``C Compiler (gcc 12.2.0)``."""
        basename = os.path.basename(path)
        if version:
            return f"{label} ({basename} {version})"
        return f"{label} ({basename})"

    def _create_build_object(self):
        """Create the SPDX 3.0 ``build_Build`` element."""
        if self.build is not None:
            return
        if not self.build_info and not self.sbom_data.build:
            return

        namespace = self.sbom_data.namespace_prefix.rstrip("/")
        self.build = spdx.build_Build()
        self.build._id = self._shorten_id(f"{namespace}/builds/default")
        self.build.creationInfo = self.creation_info._id
        self.build.build_buildType = self._DEFAULT_BUILD_TYPE

        sbom_build = self.sbom_data.build
        if sbom_build:
            if sbom_build.id:
                self.build.build_buildId = sbom_build.id
            if sbom_build.build_type:
                build_type = sbom_build.build_type
                if not build_type.startswith("http"):
                    build_type = f"https://zephyrproject.org/build-types/{build_type}"
                self.build.build_buildType = build_type
            if sbom_build.started_at:
                self.build.build_buildStartTime = sbom_build.started_at
            if sbom_build.finished_at:
                self.build.build_buildEndTime = sbom_build.finished_at

        self._add_build_parameters()
        self._add_build_environment()
        self.elements.append(self.build)

    def _add_build_environment(self):
        """Populate ``build_environment`` from the collected environment variables."""
        if not self.build:
            return
        for key, value in sorted(self.build_info.get("environment", {}).items()):
            if not value:
                continue
            entry = spdx.DictionaryEntry()
            entry.key = key
            entry.value = str(value)
            self.build.build_environment.append(entry)

    def _add_build_parameters(self):
        """Populate ``build_parameter`` with the global build configuration."""
        info = self.build_info
        if not self.build or not info:
            return

        for key, info_key in (
            ("cmake:generator", "cmake_generator"),
            ("cmake:buildType", "cmake_build_type"),
            ("target:system", "cmake_system_name"),
            ("target:processor", "cmake_system_processor"),
        ):
            value = info.get(info_key, "")
            if value:
                entry = spdx.DictionaryEntry()
                entry.key = key
                entry.value = str(value)
                self.build.build_parameter.append(entry)

    @staticmethod
    def _is_build_target_component(component: SBOMComponent) -> bool:
        """Whether a component is a build target (produces an artifact).

        Source aggregation and ``*-deps`` components are inputs, not outputs.
        """
        if component.name in {"app-sources", "zephyr-sources", "sdk-sources"}:
            return False
        return not component.name.endswith("-deps")

    def _new_build_relationship(
        self,
        rel_type: spdx.RelationshipType,
        from_id: str,
        to_ids: list[str],
        description: str = "",
    ) -> spdx.LifecycleScopedRelationship:
        """Create and register a relationship scoped to the ``build`` lifecycle."""
        rel = spdx.LifecycleScopedRelationship()
        rel._id = self._generate_relationship_id(len(self.relationship_elements))
        rel.relationshipType = rel_type
        rel.from_ = from_id
        rel.to = list(to_ids)
        rel.scope = spdx.LifecycleScopeType.build
        rel.creationInfo = self.creation_info._id
        if description:
            rel.description = description
        self.elements.append(rel)
        self.relationship_elements.append(rel)
        return rel

    def _create_build_relationships(self):
        """Create the Build profile's build-scoped relationships.

        The overall build ``usesTool`` every build tool, ``hasInput`` the source/dependency packages
        and ``hasOutput`` the final image(s). Each other build target gets its own sub-build
        recording the exact sources and tools that produced its artifact. Every relationship's
        ``from`` is a Build, so they all stay in the build document.
        """
        if not self.build:
            return

        for tool in self.build_tools.values():
            self._new_build_relationship(spdx.RelationshipType.usesTool, self.build._id, [tool._id])

        self._create_build_output_relationships()

        input_ids = self._build_input_ids()
        if input_ids:
            self._new_build_relationship(spdx.RelationshipType.hasInput, self.build._id, input_ids)

    def _create_build_output_relationships(self):
        """Emit ``hasOutput`` for the final image(s) and a sub-build for every other target."""
        for component in self.sbom_data.components.values():
            if not self._is_build_target_component(component):
                continue
            if not component.target_build_file:
                continue
            build_file = self.file_elements.get(component.target_build_file.path)
            if not build_file:
                continue
            if component.metadata.get("target_type") == "EXECUTABLE":
                # final images are outputs of the overall build; their compiled
                # sources/headers are inputs of that same build
                self._new_build_relationship(
                    spdx.RelationshipType.hasOutput, self.build._id, [build_file._id]
                )
                input_ids = self._artifact_input_ids(component)
                if input_ids:
                    self._new_build_relationship(
                        spdx.RelationshipType.hasInput, self.build._id, input_ids
                    )
            else:
                self._create_target_build(component, build_file)

    def _artifact_input_ids(self, component: SBOMComponent) -> list[str]:
        """Element IDs of the files that fed a target's artifact.

        These are the artifact's ``GENERATED_FROM`` endpoints: the compiled sources and, when
        include analysis is enabled, the headers they pulled in. They become the Build's
        ``hasInput`` for the target.
        """
        input_ids = []
        for rel in component.target_build_file.relationships:
            if rel.relationship_type != RelationshipType.GENERATED_FROM:
                continue
            for source in rel.to_elements:
                source_id = self._get_element_id(source)
                if source_id and source_id not in input_ids:
                    input_ids.append(source_id)
        return input_ids

    def _create_target_build(self, component: SBOMComponent, build_file) -> None:
        """Create a per-target sub-build for one artifact-producing target.

        The sub-build ``hasOutput`` the target's artifact, ``hasInput`` its compiled sources/headers
        and ``usesTool`` the specific compiler(s) and archiver/linker that produced it. Sub-builds
        connect to the overall build through the artifacts they share, which carry ``hasStaticLink``
        edges to the final image.
        """
        namespace = self.sbom_data.namespace_prefix.rstrip("/")
        target_build = spdx.build_Build()
        target_build._id = self._shorten_id(
            f"{namespace}/builds/{normalize_spdx_name(component.name)}"
        )
        target_build.creationInfo = self.creation_info._id
        target_build.build_buildType = self.build.build_buildType
        # per-language compile flags and defines used to produce this artifact
        for kind in ("flags", "defines"):
            for lang, value in sorted(component.metadata.get(f"compile_{kind}", {}).items()):
                entry = spdx.DictionaryEntry()
                entry.key = f"compile:{kind}:{lang}"
                entry.value = value
                target_build.build_parameter.append(entry)
        self.elements.append(target_build)
        self.target_builds[component.name] = target_build

        self._new_build_relationship(
            spdx.RelationshipType.hasOutput, target_build._id, [build_file._id]
        )

        # the target's compiled sources/headers are the artifact's GENERATED_FROM endpoints
        input_ids = self._artifact_input_ids(component)
        if input_ids:
            self._new_build_relationship(
                spdx.RelationshipType.hasInput, target_build._id, input_ids
            )

        for tool_id in self._target_tool_ids(component):
            self._new_build_relationship(
                spdx.RelationshipType.usesTool, target_build._id, [tool_id]
            )

    def _target_tool_ids(self, component: SBOMComponent) -> list[str]:
        """Tool IDs that built ``component``: compiler(s) per language plus the archiver (static
        libraries) or linker (shared/module libraries).
        """
        language_to_tool = {
            "C": "c-compiler",
            "CXX": "cxx-compiler",
            "ASM": "asm-compiler",
            "ASM-ATT": "asm-compiler",
        }
        tool_ids = []
        for lang in component.metadata.get("compile_languages", []):
            tool = self.build_tools.get(language_to_tool.get(lang, ""))
            if tool and tool._id not in tool_ids:
                tool_ids.append(tool._id)

        target_type = component.metadata.get("target_type", "")
        if target_type == "STATIC_LIBRARY":
            extra = self.build_tools.get("archiver")
        elif target_type in ("SHARED_LIBRARY", "MODULE_LIBRARY"):
            extra = self.build_tools.get("linker")
        else:
            extra = None
        if extra and extra._id not in tool_ids:
            tool_ids.append(extra._id)
        return tool_ids

    def _build_input_ids(self) -> list[str]:
        """Package IDs of the build's inputs (the source/dependency roots).

        The described (root) components of every non-build document, falling back to all of a
        document's components when it declares no root.
        """
        input_ids = []
        seen = set()
        for sbom_doc in self.sbom_data.documents.values():
            if sbom_doc.name == self._BUILD_DOCUMENT:
                continue
            names = sbom_doc.described_components or list(sbom_doc.components.keys())
            for name in names:
                package = self.component_elements.get(name)
                if package and package._id not in seen:
                    seen.add(package._id)
                    input_ids.append(package._id)
        return input_ids

    def _create_software_package(self, component: SBOMComponent) -> spdx.software_Package:
        """Convert SBOMComponent to SPDX 3.0 software_Package."""
        package = spdx.software_Package()
        package._id = self._generate_package_id(component.name)
        package.name = component.name
        package.creationInfo = self.creation_info._id
        package.software_primaryPurpose = self._purpose_to_spdx3(component.purpose)

        # Version
        if component.version:
            package.software_packageVersion = component.version
        elif component.revision:
            package.software_packageVersion = component.revision

        # Download location
        if component.url:
            package.software_downloadLocation = generate_download_url(
                component.url, component.revision
            )
        else:
            package.software_downloadLocation = NOASSERTION

        # Copyright
        package.software_copyrightText = component.copyright_text or NOASSERTION

        # License information will be added via relationships after package creation

        # External references (CPE, PURL)
        for ref in component.external_references:
            if not ref or not ref.locator:
                _logger.warning(f"Invalid external reference: {ref}")
                continue
            if ref.reference_type == ExternalReferenceType.CPE23 and re.fullmatch(
                CPE23TYPE_REGEX, ref.locator
            ):
                ext_id = spdx.ExternalIdentifier()
                ext_id.externalIdentifierType = spdx.ExternalIdentifierType.cpe23
                ext_id.identifier = ref.locator
                package.externalIdentifier.append(ext_id)
            elif ref.reference_type == ExternalReferenceType.PURL and re.fullmatch(
                PURL_REGEX, ref.locator
            ):
                ext_id = spdx.ExternalIdentifier()
                ext_id.externalIdentifierType = spdx.ExternalIdentifierType.packageUrl
                ext_id.identifier = ref.locator
                package.externalIdentifier.append(ext_id)
            else:
                _logger.warning(f"Unknown external reference format: {ref.locator}")

        self.elements.append(package)
        self.component_elements[component.name] = package
        return package

    def _create_software_file(self, file_obj: SBOMFile) -> spdx.software_File:
        """Convert SBOMFile to SPDX 3.0 software_File."""
        file_element = spdx.software_File()
        file_element._id = self._generate_file_id(file_obj.path)
        file_element.name = file_obj.relative_path or os.path.basename(file_obj.path)
        file_element.creationInfo = self.creation_info._id
        file_element.software_fileKind = spdx.software_FileKindType.file

        # File purpose (default to file)
        file_element.software_primaryPurpose = spdx.software_SoftwarePurpose.file

        # Copyright
        file_element.software_copyrightText = file_obj.copyright_text or NOASSERTION

        # Hashes - SPDX 3.0 uses verifiedUsing with Hash (which is a type of IntegrityMethod)
        for hash_type, hash_value in file_obj.hashes.items():
            if hash_value:
                hash_obj = spdx.Hash()
                if hash_type == "SHA1":
                    hash_obj.algorithm = spdx.HashAlgorithm.sha1
                elif hash_type == "SHA256":
                    hash_obj.algorithm = spdx.HashAlgorithm.sha256
                elif hash_type == "MD5":
                    hash_obj.algorithm = spdx.HashAlgorithm.md5
                else:
                    _logger.warning(f"Unknown hash algorithm: {hash_type}")
                    continue
                hash_obj.hashValue = hash_value
                file_element.verifiedUsing.append(hash_obj)

        # License information will be added via relationships after file creation

        self.elements.append(file_element)
        self.file_elements[file_obj.path] = file_element
        return file_element

    def _map_relationship_type(self, rel_type: str) -> tuple[spdx.RelationshipType, bool]:
        """Map relationship type string to SPDX 3.0 RelationshipType enum.
        Returns a tuple of (RelationshipType, reversed).
        """
        # Map SPDX 2.x relationship types to SPDX 3.0 RelationshipType.
        # GENERATED_FROM is intentionally absent: it is handled by the Build profile
        # (see _create_relationships and _artifact_input_ids).
        type_map = {
            "HAS_PREREQUISITE": (spdx.RelationshipType.dependsOn, False),
            "STATIC_LINK": (spdx.RelationshipType.hasStaticLink, False),
            "CONTAINS": (spdx.RelationshipType.contains, False),
            "DESCRIBES": (spdx.RelationshipType.describes, False),
            "DEPENDS_ON": (spdx.RelationshipType.dependsOn, False),
            "DYNAMIC_LINK": (spdx.RelationshipType.hasDynamicLink, False),
            "BUILD_TOOL_OF": (spdx.RelationshipType.usesTool, True),
            "DEV_TOOL_OF": (spdx.RelationshipType.usesTool, True),
            "TEST_TOOL_OF": (spdx.RelationshipType.usesTool, True),
            "OTHER": (spdx.RelationshipType.other, False),
        }
        return type_map.get(rel_type, (spdx.RelationshipType.other, False))

    def _get_element_id(self, element):
        """Get SPDX 3.0 element ID from various element types."""
        if isinstance(element, SBOMComponent):
            if element.name in self.component_elements:
                return self.component_elements[element.name]._id
        elif isinstance(element, SBOMFile):
            if element.path in self.file_elements:
                return self.file_elements[element.path]._id
        elif isinstance(element, str):
            if element == NOASSERTION:
                return spdx.IndividualElement.NoAssertionElement
            # Try to resolve as component name first, then file path
            if element in self.component_elements:
                return self.component_elements[element]._id
            elif element in self.file_elements:
                return self.file_elements[element]._id
        return None

    def _create_relationship(self, rel: SBOMRelationship) -> spdx.Relationship | None:
        """Convert SBOMRelationship to SPDX 3.0 Relationship."""
        # Get from_element ID
        from_id = self._get_element_id(rel.from_element)
        if not from_id:
            _logger.warning(
                f"Could not resolve from_element for relationship: {rel.relationship_type}"
            )
            return None

        # Get to_elements IDs
        to_elements = rel.to_elements if isinstance(rel.to_elements, list) else [rel.to_elements]
        to_ids = []
        for to_elem in to_elements:
            to_id = self._get_element_id(to_elem)
            if to_id:
                to_ids.append(to_id)
            else:
                _logger.warning(
                    f"Could not resolve to_element for relationship: {rel.relationship_type}"
                )

        if not to_ids:
            _logger.warning(f"No valid to_elements found for relationship: {rel.relationship_type}")
            return None

        rel_type, is_reversed = self._map_relationship_type(rel.relationship_type)

        relationship = spdx.Relationship()
        relationship._id = self._generate_relationship_id(len(self.relationship_elements))
        relationship.relationshipType = rel_type
        if is_reversed:
            # Swap from/to for reversed relationships (e.g., A GENERATED_FROM B -> B generates A)
            if len(to_ids) > 1:
                _logger.warning(
                    f"Reversed relationship {rel.relationship_type} with multiple to_elements "
                    "is not well-defined for swapping. Using first element."
                )
            relationship.from_ = to_ids[0]
            relationship.to = [from_id]
        else:
            relationship.from_ = from_id
            relationship.to = to_ids
        relationship.creationInfo = self.creation_info._id

        # Store original from_id for document assignment (before reversal)
        # This ensures cross-document relationships are placed in the document
        # of the original "from" element, matching SPDX 2.x behavior
        self.relationship_original_from[relationship._id] = from_id

        self.elements.append(relationship)
        self.relationship_elements.append(relationship)
        return relationship

    def _create_license_expression(
        self, license_str: str
    ) -> spdx.simplelicensing_LicenseExpression | None:
        """Create a license expression object and add it to elements."""
        if not license_str or license_str == NOASSERTION:
            return None

        license_expr = spdx.simplelicensing_LicenseExpression()
        standard_licenses = get_standard_licenses()

        # Check if it's a standard license ID
        if license_str in standard_licenses:
            license_expr._id = f"https://spdx.org/licenses/{license_str}"
        else:
            # Custom license - use a namespace-based ID
            namespace = self.sbom_data.namespace_prefix.rstrip("/")
            # Normalize the license string for use in URI
            normalized = normalize_spdx_name(
                license_str.replace(" ", "-").replace("(", "").replace(")", "")
            )
            license_expr._id = self._shorten_id(f"{namespace}/licenses/{normalized}")

        license_expr.simplelicensing_licenseExpression = license_str
        license_expr.creationInfo = self.creation_info._id

        # Add to elements if not already present
        existing_ids = {elem._id for elem in self.elements if hasattr(elem, '_id')}
        if license_expr._id not in existing_ids:
            self.elements.append(license_expr)

        return license_expr

    def _get_license_target_id(self, license_str: str) -> str | None:
        """Return the SPDX 3 target ID for a license value."""
        if not license_str or license_str == NOASSERTION:
            return spdx.expandedlicensing_IndividualLicensingInfo.NoAssertionLicense

        license_expr = self._create_license_expression(license_str)
        return license_expr._id if license_expr else None

    def _create_license_relationship(
        self,
        from_id: str,
        license_str: str,
        relationship_type: spdx.RelationshipType,
    ) -> spdx.Relationship | None:
        """Create a relationship from an element to a license target.

        A NOASSERTION license still produces an explicit relationship to NoAssertionLicense rather
        than being skipped: per the SPDX 3.0 Licensing profile a "known unknown"
        (NoAssertionLicense) is deliberately distinct from a missing relationship, and this mirrors
        what the SPDX 2.x output records (``LicenseConcluded: NOASSERTION``).
        """
        license_id = self._get_license_target_id(license_str)
        if not license_id:
            return None

        rel = spdx.Relationship()
        rel._id = self._generate_relationship_id(len(self.relationship_elements))
        rel.relationshipType = relationship_type
        rel.from_ = from_id
        rel.to = [license_id]
        rel.creationInfo = self.creation_info._id
        self.elements.append(rel)
        self.relationship_elements.append(rel)
        return rel

    def _create_document(self, sbom_doc: SBOMDocument) -> spdx.SpdxDocument:
        """Create an SPDX 3.0 document for a specific SBOMDocument."""
        self._initialize_shared_objects()

        document = self._new_spdx_document(sbom_doc)
        components = sbom_doc.components.values()

        element_ids = self._collect_document_element_ids(sbom_doc, components)
        self._populate_document(document, element_ids, components, sbom_doc)

        self.elements.append(document)
        self.documents[sbom_doc.name] = document
        return document

    def _new_spdx_document(self, sbom_doc: SBOMDocument) -> spdx.SpdxDocument:
        """Build the SpdxDocument shell: identity, namespaces, name and licensing."""
        document = spdx.SpdxDocument()

        # Use the document's own namespace when set, otherwise the global prefix.
        namespace = (
            sbom_doc.namespace.rstrip("/")
            if sbom_doc.namespace
            else self.sbom_data.namespace_prefix.rstrip("/")
        )
        document._id = self._shorten_id(f"{namespace}/documents/{sbom_doc.name}")

        for uri, prefix in self.namespace_prefixes.items():
            ns_map = spdx.NamespaceMap()
            ns_map.prefix = prefix
            ns_map.namespace = uri
            document.namespaceMap.append(ns_map)

        document.name = self._DOCUMENT_NAMES.get(
            sbom_doc.name, f"Zephyr {sbom_doc.name.capitalize()}"
        )
        document.creationInfo = self.creation_info

        data_license = self._create_license_expression("CC0-1.0")
        if data_license:
            document.dataLicense = data_license._id

        document.profileConformance.append(spdx.ProfileIdentifierType.core)
        document.profileConformance.append(spdx.ProfileIdentifierType.software)
        document.profileConformance.append(spdx.ProfileIdentifierType.simpleLicensing)
        # Only the build document conforms to the Build profile.
        if self.build and sbom_doc.name == self._BUILD_DOCUMENT:
            document.profileConformance.append(spdx.ProfileIdentifierType.build)
        return document

    def _collect_document_element_ids(self, sbom_doc: SBOMDocument, components) -> set:
        """Gather the IDs of every element that belongs to this document.

        This spans the document's packages and files, the licenses they
        reference, the relationships connecting them, and the shared
        tool/agent/data-license elements.
        """
        element_ids = self._package_and_file_ids(components)
        element_ids.update(self._referenced_license_ids(components))

        # Custom licenses recorded on the document must exist as elements even
        # if no package or file references them directly.
        self._register_custom_licenses(sbom_doc)

        # Seed the build document so its build-scoped relationships are collected.
        self._seed_build_element_ids(sbom_doc, element_ids)

        self._collect_relationship_ids(element_ids)

        if self.tool:
            element_ids.add(self.tool._id)
        if self.creator_agent:
            element_ids.add(self.creator_agent._id)
        data_license = self._create_license_expression("CC0-1.0")
        if data_license:
            element_ids.add(data_license._id)

        return element_ids

    def _package_and_file_ids(self, components) -> set:
        """IDs of the package and file elements contained in this document."""
        element_ids = set()
        for component in components:
            package = self.component_elements.get(component.name)
            if package:
                element_ids.add(package._id)
            for file_obj in component.files.values():
                file_element = self.file_elements.get(file_obj.path)
                if file_element:
                    element_ids.add(file_element._id)
        return element_ids

    def _referenced_license_ids(self, components) -> set:
        """License-expression IDs referenced by the document's packages and files."""
        license_ids = set()
        for component in components:
            license_ids.add(self._get_license_target_id(component.concluded_license))
            license_ids.add(self._get_license_target_id(component.declared_license))
            for file_obj in component.files.values():
                license_ids.add(self._get_license_target_id(file_obj.concluded_license))
                for lic in file_obj.license_info_in_file:
                    if lic != "NONE":
                        license_ids.add(self._get_license_target_id(lic))
        license_ids.discard(None)
        return license_ids

    def _register_custom_licenses(self, sbom_doc: SBOMDocument):
        """Ensure custom licenses stored on the document exist as elements."""
        for lic in sbom_doc.custom_license_ids:
            self._create_license_expression(lic)

    def _seed_build_element_ids(self, sbom_doc: SBOMDocument, element_ids: set):
        """Add the Build elements and build tool IDs to the build document's set.

        No-op for any other document, or when no Build element was produced.
        """
        if not self.build or sbom_doc.name != self._BUILD_DOCUMENT:
            return
        element_ids.add(self.build._id)
        for target_build in self.target_builds.values():
            element_ids.add(target_build._id)
        for tool in self.build_tools.values():
            element_ids.add(tool._id)

    def _collect_relationship_ids(self, element_ids: set):
        """Add this document's relationships and their endpoints to ``element_ids``.

        A relationship belongs to the document that owns its original
        (pre-reversal) "from" element. Ownership is tested against a snapshot
        of the document's own elements so that pulling in endpoints does not
        draw in unrelated relationships.
        """
        owned = set(element_ids)
        relationship_ids = set()
        endpoint_ids = set()
        for rel in self.relationship_elements:
            from_id = getattr(rel, 'from_', None)
            original_from_id = self.relationship_original_from.get(rel._id, from_id)
            if original_from_id in owned:
                relationship_ids.add(rel._id)
                if from_id:
                    endpoint_ids.add(from_id)
                endpoint_ids.update(rel.to)
        element_ids.update(relationship_ids)
        element_ids.update(endpoint_ids)

    def _populate_document(
        self,
        document: spdx.SpdxDocument,
        element_ids: set,
        components,
        sbom_doc: SBOMDocument,
    ):
        """Attach the selected elements and root components to the document."""
        for element in self.elements:
            if self._belongs_in_document(element, document._id, element_ids):
                document.element.append(element)

        for component in components:
            package = self.component_elements.get(component.name)
            if package:
                document.rootElement.append(package)

        # The Build element is a root of the build document.
        if self.build and sbom_doc.name == self._BUILD_DOCUMENT:
            document.rootElement.append(self.build)

    @staticmethod
    def _belongs_in_document(element, document_id: str, element_ids: set) -> bool:
        """Whether ``element`` should be listed in the document's element set.

        Only proper SPDX ``Element`` objects (never the document itself) that
        were selected into ``element_ids`` qualify.
        """
        if not isinstance(element, spdx.Element) or isinstance(element, spdx.SpdxDocument):
            return False
        element_id = getattr(element, '_id', None)
        return bool(element_id) and element_id != document_id and element_id in element_ids

    def serialize(self, output_dir: str) -> bool:
        """Serialize SBOMData to SPDX 3.0 format (JSON-LD)."""
        try:
            if not self._validate_inputs(output_dir):
                return False

            self._initialize_shared_objects()
            self._create_packages()
            self._create_files()
            self._create_relationships()
            self._create_contains_relationships()
            self._create_build_relationships()
            self._create_license_relationships()
            self._create_documents()
            self._write_documents(output_dir)

            _logger.info(f"SPDX 3.0 documents written to {output_dir}")
            return True

        except Exception:
            _logger.exception("Failed to serialize SPDX 3.0 document")
            return False

    def _validate_inputs(self, output_dir: str) -> bool:
        """Check that the serializer has everything it needs to run."""
        if not self.sbom_data:
            _logger.error("SBOMGraph is None or empty")
            return False
        if not self.sbom_data.namespace_prefix:
            _logger.error("Namespace prefix is required for SPDX 3.0")
            return False
        if not os.path.exists(output_dir):
            _logger.error(f"Output directory does not exist: {output_dir}")
            return False
        return True

    def _create_packages(self):
        """Create software_Package elements from all named components."""
        if not self.sbom_data.components:
            _logger.warning("No components found in SBOM data")
            return
        for component in self.sbom_data.components.values():
            if not component.name:
                _logger.warning("Skipping component with empty name")
                continue
            self._create_software_package(component)

    def _create_files(self):
        """Create software_File elements from all valid files."""
        if not self.sbom_data.files:
            _logger.warning("No files found in SBOM data")
            return
        for file_obj in self.sbom_data.files.values():
            if not file_obj.path:
                _logger.warning("Skipping file with empty path")
                continue
            self._create_software_file(file_obj)

    def _create_relationships(self):
        """Create the relationships declared by components and files.

        ``GENERATED_FROM`` edges are skipped: build provenance is carried by the Build profile
        (the Build's ``hasInput``/``hasOutput``), not by file-level ``generates`` edges like it
        would for SPDX 2.x.
        """
        for component in self.sbom_data.components.values():
            self._create_owner_relationships(component.relationships, "component", component.name)
        for file_obj in self.sbom_data.files.values():
            self._create_owner_relationships(file_obj.relationships, "file", file_obj.path)

    def _create_owner_relationships(self, relationships, owner_kind, owner_id):
        """Create each declared relationship, skipping ``GENERATED_FROM`` (see above)."""
        for rel in relationships:
            if rel.relationship_type == RelationshipType.GENERATED_FROM:
                continue
            if not self._create_relationship(rel):
                _logger.warning(f"Failed to create relationship from {owner_kind} {owner_id}")

    def _create_contains_relationships(self):
        """Create a CONTAINS relationship from each package to each of its files."""
        for component in self.sbom_data.components.values():
            package = self.component_elements.get(component.name)
            if not package:
                continue
            for file_obj in component.files.values():
                file_element = self.file_elements.get(file_obj.path)
                if file_element:
                    self._add_contains_relationship(package, file_element)

    def _add_contains_relationship(self, package, file_element):
        """Append a single package-contains-file relationship."""
        contains_rel = spdx.Relationship()
        contains_rel._id = self._generate_relationship_id(len(self.relationship_elements))
        contains_rel.relationshipType = spdx.RelationshipType.contains
        contains_rel.from_ = package._id
        contains_rel.to = [file_element._id]
        contains_rel.creationInfo = self.creation_info._id
        self.elements.append(contains_rel)
        self.relationship_elements.append(contains_rel)

    def _create_license_relationships(self):
        """Create concluded/declared license relationships for packages and files."""
        for component in self.sbom_data.components.values():
            package = self.component_elements.get(component.name)
            if not package:
                continue
            self._create_license_relationship(
                package._id,
                component.concluded_license,
                spdx.RelationshipType.hasConcludedLicense,
            )
            self._create_license_relationship(
                package._id,
                component.declared_license,
                spdx.RelationshipType.hasDeclaredLicense,
            )

        for file_obj in self.sbom_data.files.values():
            file_element = self.file_elements.get(file_obj.path)
            if not file_element:
                continue
            self._create_license_relationship(
                file_element._id,
                file_obj.concluded_license,
                spdx.RelationshipType.hasConcludedLicense,
            )
            for lic in file_obj.license_info_in_file or []:
                if lic != "NONE":
                    self._create_license_relationship(
                        file_element._id,
                        lic,
                        spdx.RelationshipType.hasDeclaredLicense,
                    )

    def _create_documents(self):
        """Create one SPDX document per non-empty SBOMDocument group."""
        for sbom_doc in self.sbom_data.documents.values():
            if sbom_doc.components:
                self._create_document(sbom_doc)

    def _write_documents(self, output_dir: str):
        """Write each created document to its own JSON-LD file."""
        for doc_name, document in self.documents.items():
            jsonld_path = os.path.join(output_dir, f"{doc_name}.jsonld")
            self._serialize_document_to_jsonld(document, jsonld_path)

    def _serialize_document_to_jsonld(self, document: spdx.SpdxDocument, output_path: str):
        """Serialize a single document to JSON-LD format."""
        object_set = spdx.SHACLObjectSet()
        for element in self._gather_elements_to_serialize(document):
            object_set.add(element)

        with open(output_path, "wb") as f:
            spdx.JSONLDSerializer().write(object_set, f, force_at_graph=True, indent=2)

        _logger.info(f"Written SPDX 3.0 JSON-LD to {output_path}")

    def _gather_elements_to_serialize(self, document: spdx.SpdxDocument) -> list:
        """Collect the document plus every element it references.

        The document, its creation info and the tool are always included; the
        remaining elements are looked up by the IDs listed in ``document.element``.
        """
        elements = [document]
        if self.creation_info:
            elements.append(self.creation_info)
        if self.tool:
            elements.append(self.tool)

        seen_ids = {getattr(elem, '_id', None) for elem in elements}

        referenced_ids = {
            elem._id for elem in getattr(document, 'element', []) if getattr(elem, '_id', None)
        }

        elements_by_id = {}
        for elem in self.elements:
            elem_id = getattr(elem, '_id', None)
            if elem_id and elem_id not in elements_by_id:
                elements_by_id[elem_id] = elem

        for elem_id in referenced_ids:
            if elem_id in seen_ids:
                continue
            elem = elements_by_id.get(elem_id)
            if elem is not None:
                elements.append(elem)
                seen_ids.add(elem_id)

        return elements
