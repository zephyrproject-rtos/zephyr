# Copyright (c) 2020-2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import re
import subprocess
from dataclasses import dataclass

import yaml
from west.util import WestNotFound, west_topdir

from zspdx.cmakecache import parse_cmake_cache_file
from zspdx.cmakefileapijson import parse_reply, parse_toolchains_and_info
from zspdx.getincludes import get_c_includes
from zspdx.model import (
    BuildInfo,
    ComponentPurpose,
    RelationshipType,
    SBOMBuild,
    SBOMComponent,
    SBOMDocument,
    SBOMFile,
    SBOMGraph,
)

_logger = logging.getLogger(__name__)


def get_tool_version(tool_path):
    """Get a tool's version by running it with ``--version``.

    Used for the linker and archiver, which the CMake toolchains-v1 reply does
    not describe. Returns "" when the tool is missing or no version can be parsed.
    """
    if not tool_path or not os.path.isfile(tool_path):
        return ""

    try:
        result = subprocess.run(
            [tool_path, "--version"],
            capture_output=True,
            text=True,
            timeout=5,  # avoid hanging on a misbehaving tool
        )
        output = result.stdout or result.stderr
        if not output:
            return ""

        # parse the version from the first line of output, e.g.
        # "GNU ld (Zephyr SDK 0.17.4) 2.38" -> "2.38",
        # "cmake version 3.28.1" -> "3.28.1"
        first_line = output.strip().split('\n')[0]
        for pattern in (
            r'version\s+(\d+\.\d+(?:\.\d+)?)',
            r'\b(\d+\.\d+(?:\.\d+)?)\s*$',
            r'\b(\d+\.\d+(?:\.\d+)?)\b',
        ):
            match = re.search(pattern, first_line, re.IGNORECASE)
            if match:
                return match.group(1)
        return ""
    except (subprocess.SubprocessError, OSError) as e:
        _logger.debug(f"Could not get version for {tool_path}: {e}")
        return ""


# WalkerConfig contains configuration data for the Walker.
@dataclass(eq=True)
class WalkerConfig:
    # prefix for Document namespaces; should not end with "/"
    namespace_prefix: str = ""

    # location of build directory
    build_dir: str = ""

    # should also analyze for included header files?
    analyze_includes: bool = False

    # should also add an SPDX document for the SDK?
    include_sdk: bool = False


# Walker is the main analysis class: it walks through the CMake codemodel,
# build files, and corresponding source and SDK files, and gathers the
# information needed to build the SBOM data.
class Walker:
    # initialize with WalkerConfig
    def __init__(self, cfg):
        super().__init__()

        # configuration - WalkerConfig
        self.cfg = cfg

        # SBOM graph container
        self.sbom_graph = SBOMGraph(namespace_prefix=cfg.namespace_prefix, build_dir=cfg.build_dir)

        # Component references for easy access
        self.component_app = None
        self.component_zephyr = None
        self.component_sdk = None
        self.component_build_targets = {}  # target_name -> SBOMComponent
        self.component_zephyr_modules = {}  # module_name -> SBOMComponent
        self.component_modules_deps = {}  # module_name -> SBOMComponent

        # Document references for easy access
        self.doc_app = None
        self.doc_zephyr = None
        self.doc_sdk = None
        self.doc_build = None
        self.doc_modules_deps = None

        # queue of pending source file paths to create, process and assign
        self.pending_sources = []

        # queue of pending relationship data to create, process and assign
        # Format: (from_type, from_identifier, to_type, to_identifier, relationship_type)
        # Types: "component", "file"
        self.pending_relationships = []

        # parsed CMake codemodel
        self.cm = None

        # parsed CMake toolchains-v1 reply (compiler ids and versions)
        self.toolchains = None

        # parsed CMake info (generator and version) from the file API index
        self.cmake_info = None

        # parsed CMake cache dict
        self.cmake_cache = {}

        # C compiler path from parsed CMake cache
        self.compiler_path = ""

        # SDK install path from parsed CMake cache
        self.sdk_path = ""

        # Meta file path
        self.meta_file = ""

    def _build_purl(self, url, version=None):
        if not url:
            return None

        purl = None
        # This is designed to match repository with the following url pattern:
        # '<protocol><type>/<namespace>/<package>
        COMMON_GIT_URL_REGEX = (
            r'((git@|http(s)?:\/\/)(?P<type>[\w\.@]+)(\.\w+)(\/|:))'
            r'(?P<namespace>[\w,\-,\_\/]+)\/(?P<package>[\w,\-,\_]+)(.git){0,1}((\/){0,1})$'
        )

        match = re.fullmatch(COMMON_GIT_URL_REGEX, url)
        if match:
            purl = f'pkg:{match.group("type")}/{match.group("namespace")}/{match.group("package")}'

        if purl and version:
            purl += f'@{version}'

        return purl

    # primary entry point
    def collect_sbom_graph(self):
        """
        Collect the SBOM graph from CMake codemodel and build artifacts.
        Returns SBOMGraph object containing all collected information.
        """
        # parse CMake cache file and get compiler path
        _logger.info("parsing CMake Cache file")
        self.get_cache_file()

        # check if meta file is generated
        if not self.meta_file:
            _logger.error(
                "CONFIG_BUILD_OUTPUT_META must be enabled to generate spdx files; bailing"
            )
            return None

        # parse codemodel from CMake file-based API
        _logger.info("parsing CMake file-based API codemodel")
        self.cm = self.get_codemodel()
        if not self.cm:
            _logger.error("could not parse codemodel from CMake API reply; bailing")
            return None

        # extract Build profile info; non-fatal, the profile is omitted if absent
        _logger.info("extracting build information from CMake file-based API")
        self.get_toolchains_and_info()
        self.extract_build_info()

        # set up components
        _logger.info("setting up SBOM components")
        retval = self.setup_components()
        if not retval:
            return None

        # walk through targets in codemodel to gather information
        _logger.info("walking through targets")
        self.walk_targets()

        # walk through pending sources and create corresponding files
        _logger.info("walking through pending sources files")
        self.walk_pending_sources()

        # walk through pending relationship data and create relationships
        _logger.info("walking through pending relationships")
        self.walk_relationships()

        return self.sbom_graph

    # parse cache file and pull out relevant data
    def get_cache_file(self):
        cache_file_path = os.path.join(self.cfg.build_dir, "CMakeCache.txt")
        self.cmake_cache = parse_cmake_cache_file(cache_file_path)
        if self.cmake_cache:
            self.compiler_path = self.cmake_cache.get("CMAKE_C_COMPILER", "")
            self.sdk_path = self.cmake_cache.get("ZEPHYR_SDK_INSTALL_DIR", "")
            self.meta_file = self.cmake_cache.get("KERNEL_META_PATH", "")

    # locate the CMake file-based API reply index file within the build dir
    def get_reply_index_path(self):
        cmake_reply_dir_path = os.path.join(self.cfg.build_dir, ".cmake", "api", "v1", "reply")
        if not os.path.exists(cmake_reply_dir_path):
            _logger.error(f'cmake api reply directory {cmake_reply_dir_path} does not exist')
            _logger.error('was query directory created before cmake build ran?')
            return None
        if not os.path.isdir(cmake_reply_dir_path):
            _logger.error(
                f'cmake api reply directory {cmake_reply_dir_path} exists but is not a directory'
            )
            return None

        # find file with "index" prefix; there should only be one
        for f in os.listdir(cmake_reply_dir_path):
            if f.startswith("index"):
                return os.path.join(cmake_reply_dir_path, f)

        _logger.error(f'cmake api reply index file not found in {cmake_reply_dir_path}')
        return None

    # determine path from build dir to CMake file-based API index file, then
    # parse it and return the Codemodel
    def get_codemodel(self):
        _logger.debug("getting codemodel from CMake API reply files")

        index_file_path = self.get_reply_index_path()
        if not index_file_path:
            return None

        # parse it
        return parse_reply(index_file_path)

    # parse the toolchains-v1 reply and CMake info from the file-based API index
    def get_toolchains_and_info(self):
        _logger.debug("getting toolchains and CMake info from CMake API reply files")

        index_file_path = self.get_reply_index_path()
        if not index_file_path:
            return

        self.cmake_info, self.toolchains = parse_toolchains_and_info(index_file_path)

    def extract_build_info(self):
        """Collect global build information for the SPDX 3.0 Build profile.

        Stores the details on the graph's ``SBOMBuild`` (its ``id``, ``build_type`` and
        the detailed ``metadata`` mapping); serializers without a build vocabulary ignore
        them.
        """
        if not self.cmake_cache:
            _logger.debug("no CMake cache parsed; skipping build info extraction")
            return

        build_info: BuildInfo = {}

        # compiler paths, ids and versions: prefer toolchains-v1, fall back to cache
        if self.toolchains and self.toolchains.by_language:
            for lang, key in (("C", "c"), ("CXX", "cxx"), ("ASM", "asm")):
                build_info[f"cmake_{key}_compiler"] = self.toolchains.get_compiler_path(lang)
                build_info[f"{key}_compiler_version"] = self.toolchains.get_compiler_version(lang)
                build_info[f"{key}_compiler_id"] = self.toolchains.get_compiler_id(lang)
            # generic compiler-path key, set to the C compiler
            build_info["cmake_compiler"] = build_info.get("cmake_c_compiler", "")
        else:
            build_info["cmake_compiler"] = self.cmake_cache.get("CMAKE_C_COMPILER", "")
            build_info["cmake_cxx_compiler"] = self.cmake_cache.get("CMAKE_CXX_COMPILER", "")
            build_info["cmake_asm_compiler"] = self.cmake_cache.get("CMAKE_ASM_COMPILER", "")

        # linker, archiver, build type and target system always come from the cache
        build_info["cmake_linker"] = self.cmake_cache.get("CMAKE_LINKER", "")
        build_info["cmake_ar"] = self.cmake_cache.get("CMAKE_AR", "")
        build_info["cmake_build_type"] = self.cmake_cache.get("CMAKE_BUILD_TYPE", "")
        build_info["cmake_system_name"] = self.cmake_cache.get("CMAKE_SYSTEM_NAME", "")
        build_info["cmake_system_processor"] = self.cmake_cache.get("CMAKE_SYSTEM_PROCESSOR", "")

        # CMake generator and version from the file-API index
        if self.cmake_info:
            build_info["cmake_generator"] = self.cmake_info.generator_name
            build_info["cmake_version"] = self.cmake_info.version_string

        # build environment: Zephyr config inputs surfaced as build_environment
        environment = {}
        for var in (
            "BOARD",
            "ARCH",
            "ZEPHYR_TOOLCHAIN_VARIANT",
            "ZEPHYR_SDK_INSTALL_DIR",
            "CMAKE_BUILD_TYPE",
        ):
            value = self.cmake_cache.get(var, "")
            if value:
                environment[var] = value
        build_info["environment"] = environment

        # linker and archiver versions are not in toolchains-v1; query the tools
        for version_key, path in (
            ("linker_version", build_info["cmake_linker"]),
            ("ar_version", build_info["cmake_ar"]),
        ):
            version = get_tool_version(path)
            if version:
                build_info[version_key] = version

        # drop empty entries to keep the build_parameter output tidy
        build_info = {k: v for k, v in build_info.items() if v}
        if not build_info:
            _logger.debug("no build information available; skipping Build profile inputs")
            return

        # summarise as an SBOMBuild; build timestamps are omitted to keep builds reproducible
        self.sbom_graph.build = SBOMBuild(
            build_type=build_info.get("cmake_build_type", ""),
            metadata=build_info,
        )

    def _create_document(self, name: str, title: str = "") -> SBOMDocument:
        """Create a document with the given name and register it with SBOM data.

        ``name`` is the identifier used for the output filename, namespace and
        cross-document reference ID; ``title`` is the human-readable SPDX
        ``DocumentName`` and defaults to ``name`` when not given.
        """
        doc = SBOMDocument(name=name, title=title, namespace=f"{self.cfg.namespace_prefix}/{name}")
        self.sbom_graph.add_document(doc)
        return doc

    def setup_documents(self):
        """Set up all SBOM documents."""
        _logger.debug("setting up SBOM documents")

        # Create core documents. The app and zephyr documents historically carry a
        # "-sources" suffix in their DocumentName while keeping the unsuffixed
        # identifier for filename/namespace/reference purposes.
        self.doc_app = self._create_document("app", "app-sources")
        self.doc_zephyr = self._create_document("zephyr", "zephyr-sources")
        self.doc_build = self._create_document("build")
        self.doc_modules_deps = self._create_document("modules-deps")

        # SDK document is optional
        if self.cfg.include_sdk:
            self.doc_sdk = self._create_document("sdk")

    def setup_components(self):
        """Set up all SBOM components from meta file and configuration."""
        _logger.debug("setting up SBOM components")

        # First set up documents
        self.setup_documents()

        try:
            with open(self.meta_file) as file:
                content = yaml.load(file.read(), yaml.SafeLoader)
                if not self.setup_zephyr_component(content["zephyr"], content["modules"]):
                    return False
        except (FileNotFoundError, yaml.YAMLError):
            _logger.error("cannot find a valid zephyr.meta required for SPDX generation; bailing")
            return False

        self.setup_app_component()

        if self.cfg.include_sdk:
            self.setup_sdk_component()

        self.setup_modules_deps_component(content["modules"], content["zephyr"])

        return True

    def setup_app_component(self):
        """Set up app sources component."""
        component = SBOMComponent(
            name="app-sources",
            purpose=ComponentPurpose.SOURCE,
            base_dir=self.cm.paths_source,
        )

        self.sbom_graph.add_component(component, "app")
        self.doc_app.add_described_component(component)
        self.component_app = component

    def setup_zephyr_component(self, zephyr, modules):
        """Set up zephyr sources component and module components."""
        # relativeBaseDir is Zephyr sources topdir
        try:
            relative_base_dir = west_topdir(self.cm.paths_source)
        except WestNotFound:
            _logger.error(
                "cannot find west_topdir for CMake Codemodel sources path "
                f"{self.cm.paths_source}; bailing"
            )
            return False

        # set up zephyr sources component
        component = SBOMComponent(
            name="zephyr-sources",
            purpose=ComponentPurpose.SOURCE,
            base_dir=relative_base_dir,
        )

        zephyr_url = zephyr.get("remote", "")
        if zephyr_url:
            component.url = zephyr_url

        if zephyr.get("revision"):
            component.revision = zephyr.get("revision")

        purl = None
        zephyr_tags = zephyr.get("tags", "")
        if zephyr_tags:
            # Find tag vX.Y.Z
            for tag in zephyr_tags:
                version = re.fullmatch(r'^v(?P<version>\d+\.\d+\.\d+)$', tag)
                purl = self._build_purl(zephyr_url, tag)

                if purl:
                    component.add_external_reference(purl)

                # Extract version from tag once
                if component.version == "" and version:
                    component.version = version.group('version')

        if len(component.version) > 0:
            cpe = f'cpe:2.3:o:zephyrproject:zephyr:{component.version}:-:*:*:*:*:*:*'
            component.add_external_reference(cpe)

        self.sbom_graph.add_component(component, "zephyr")
        self.doc_zephyr.add_described_component(component)
        self.component_zephyr = component

        # set up a source component for each module, living in the zephyr document
        for module in modules:
            module_name = module.get("name", None)
            module_path = module.get("path", None)
            module_url = module.get("remote", None)
            module_revision = module.get("revision", None)

            if not module_name:
                _logger.error("cannot find module name in meta file; bailing")
                return False

            module_component = SBOMComponent(
                name=module_name + "-sources",
                purpose=ComponentPurpose.SOURCE,
                base_dir=module_path,
            )

            if module_revision:
                module_component.revision = module_revision

            if module_url:
                module_component.url = module_url

            self.sbom_graph.add_component(module_component, "zephyr")
            self.doc_zephyr.add_described_component(module_component)
            self.component_zephyr_modules[module_name] = module_component

        return True

    def setup_sdk_component(self):
        """Set up SDK sources component."""
        component = SBOMComponent(
            name="sdk-sources",
            purpose=ComponentPurpose.SOURCE,
            base_dir=self.sdk_path,
        )

        self.sbom_graph.add_component(component, "sdk")
        self.doc_sdk.add_described_component(component)
        self.component_sdk = component

    def _setup_zephyr_deps_component(self, zephyr):
        """Set up the zephyr dependency component in the modules-deps document.

        Returns the created component, or ``None`` when no zephyr metadata is
        available.
        """
        if zephyr is None:
            return None

        # no PrimaryPackagePurpose: this is a reference-only dependency package with no files
        component = SBOMComponent(name="zephyr-deps")
        component.url = zephyr.get("remote", "")
        component.revision = zephyr.get("revision", "")

        purl = None
        zephyr_tags = zephyr.get("tags", None)
        if zephyr_tags:
            # Find tag vX.Y.Z
            for tag in zephyr_tags:
                version = re.fullmatch(r'^v(?P<version>\d+\.\d+\.\d+)$', tag)
                purl = self._build_purl(component.url, tag)

                if purl:
                    component.add_external_reference(purl)

                # Extract version from tag once
                if component.version == "" and version:
                    component.version = version.group('version')
        else:
            if zephyr.get("revision"):
                purl = self._build_purl(component.url, zephyr.get("revision"))
            if purl:
                component.add_external_reference(purl)

        if len(component.version) > 0:
            cpe = f'cpe:2.3:o:zephyrproject:zephyr:{component.version}:-:*:*:*:*:*:*'
            component.add_external_reference(cpe)

        if component.version == "" and zephyr.get("revision"):
            component.version = zephyr.get("revision")

        self.sbom_graph.add_component(component, "modules-deps")
        self.doc_modules_deps.add_described_component(component)
        return component

    def setup_modules_deps_component(self, modules, zephyr=None):
        """Set up module dependency components."""
        # the zephyr dependency component, which every module depends on
        zephyr_deps = self._setup_zephyr_deps_component(zephyr)

        for module in modules:
            module_name = module.get("name", None)
            module_security = module.get("security", None)

            if not module_name:
                _logger.error("cannot find module name in meta file; bailing")
                return False

            module_ext_ref = []
            if module_security:
                module_ext_ref = module_security.get("external-references", [])

            # set up module deps component (reference-only, no files; no purpose)
            component = SBOMComponent(name=module_name + "-deps")

            for ref in module_ext_ref:
                component.add_external_reference(ref)

            self.sbom_graph.add_component(component, "modules-deps")
            self.component_modules_deps[module_name] = component

            # each module is a dependency of the zephyr dependency component
            if zephyr_deps is not None:
                self.pending_relationships.append(
                    ("component", component.name, "component", zephyr_deps.name, "DEPENDENCY_OF")
                )

        return True

    # walk through targets and gather information
    def walk_targets(self):
        _logger.debug("walking targets from codemodel")

        # assuming just one configuration; consider whether this is incorrect
        cfg_targets = self.cm.configurations[0].config_targets
        for cfg_target in cfg_targets:
            # build the Component for this target
            component = self.init_config_target_component(cfg_target)

            # see whether this target has any build artifacts at all
            if len(cfg_target.target.artifacts) > 0:
                # add its build file
                bf = self.add_build_file(cfg_target, component)
                if component.name == "zephyr_final":
                    component.purpose = ComponentPurpose.APPLICATION
                    # the build document's primary subject is the final image
                    self.doc_build.add_described_component(component)
                else:
                    component.purpose = ComponentPurpose.LIBRARY

                self.capture_build_metadata(cfg_target, component)

                # get its source files if build file is found
                if bf:
                    self.collect_pending_source_files(cfg_target, component, bf)
            else:
                _logger.debug(f"  - target {cfg_target.name} has no build artifacts")

            # get its target dependencies
            self.collect_target_dependencies(cfg_targets, cfg_target, component)

    # capture the per-target metadata the SPDX 3.0 Build profile needs to attribute a compiler/
    # archiver to each artifact: target type, compiled languages and per-language compile flags
    def capture_build_metadata(self, cfg_target, component):
        target = cfg_target.target
        component.metadata["target_type"] = target.type.name

        languages = set()
        compile_flags = {}
        compile_defines = {}
        for cg in target.compile_groups:
            if not cg.language:
                continue
            languages.add(cg.language)
            flags = [frag for frag in (cg.compile_command_fragments or []) if frag]
            if flags:
                compile_flags.setdefault(cg.language, []).extend(flags)
            defines = [f"-D{d.define}" for d in (cg.defines or []) if d.define]
            if defines:
                compile_defines.setdefault(cg.language, []).extend(defines)

        if languages:
            component.metadata["compile_languages"] = sorted(languages)
        if compile_flags:
            component.metadata["compile_flags"] = {
                lang: " ".join(flags) for lang, flags in compile_flags.items()
            }
        if compile_defines:
            component.metadata["compile_defines"] = {
                lang: " ".join(defs) for lang, defs in compile_defines.items()
            }

    # build a Component for the given ConfigTarget
    def init_config_target_component(self, cfg_target):
        _logger.debug(f"  - initializing Component for target: {cfg_target.name}")

        # create target Component
        component = SBOMComponent(
            name=cfg_target.name,
            base_dir=self.cm.paths_build,
        )

        # add Component to SBOM data and build document
        self.sbom_graph.add_component(component, "build")
        self.component_build_targets[cfg_target.name] = component
        return component

    # create a target's build product File and add it to its Component
    # call with:
    #   1) ConfigTarget
    #   2) Component for that target
    # returns: SBOMFile
    def add_build_file(self, cfg_target, component):
        # assumes only one artifact in each target
        artifact_path = os.path.join(component.base_dir, cfg_target.target.artifacts[0])
        _logger.debug(f"  - adding File {artifact_path}")
        _logger.debug(f"    - base_dir: {component.base_dir}")
        _logger.debug(f"    - artifacts[0]: {cfg_target.target.artifacts[0]}")

        # don't create build File if artifact path points to nonexistent file
        if not os.path.exists(artifact_path):
            _logger.debug(
                f"  - target {cfg_target.name} lists build artifact {artifact_path} "
                "but file not found after build; skipping"
            )
            return None

        # create build File
        bf = SBOMFile(path=artifact_path, relative_path=cfg_target.target.artifacts[0])
        self.sbom_graph.add_file(bf, component)

        # also set this file as the target component's build product file
        component.target_build_file = bf

        return bf

    # collect a target's source files, add to pending sources queue, and
    # create pending relationship data entry
    # call with:
    #   1) ConfigTarget
    #   2) Component for that target
    #   3) build File for that target
    def collect_pending_source_files(self, cfg_target, component, bf):
        _logger.debug("  - collecting source files and adding to pending queue")

        target_includes_set = set()

        # walk through target's sources
        for src in cfg_target.target.sources:
            _logger.debug(f"    - add pending source file and relationship for {src.path}")
            # get absolute, normalized path if we don't have it
            src_abspath = src.path
            if not os.path.isabs(src.path):
                src_abspath = os.path.normpath(os.path.join(self.cm.paths_source, src.path))

            # check whether it even exists
            if not (os.path.exists(src_abspath) and os.path.isfile(src_abspath)):
                _logger.debug(
                    f"  - {src_abspath} does not exist but is referenced in sources for "
                    f"target {component.name}; skipping"
                )
                continue

            # add it to pending source files queue, remembering the build
            # target that referenced it (used to assign generated/build-dir
            # files to the correct build component)
            self.pending_sources.append((src_abspath, component))

            # create relationship data: build file GENERATED_FROM source file
            self.pending_relationships.append(
                ("file", bf.path, "file", src_abspath, "GENERATED_FROM")
            )

            # collect this source file's includes
            if self.cfg.analyze_includes and self.compiler_path:
                includes = self.collect_includes(cfg_target, component, bf, src)
                for inc in includes:
                    target_includes_set.add(inc)

        # make relationships for the overall included files,
        # avoiding duplicates for multiple source files including
        # the same headers
        target_includes_list = list(target_includes_set)
        target_includes_list.sort()
        for inc in target_includes_list:
            # add it to pending source files queue, remembering the build
            # target that referenced it
            self.pending_sources.append((inc, component))

            # create relationship data: build file GENERATED_FROM include file
            self.pending_relationships.append(("file", bf.path, "file", inc, "GENERATED_FROM"))

    # collect the include files corresponding to this source file
    # call with:
    #   1) ConfigTarget
    #   2) Component for this target
    #   3) build File for this target
    #   4) TargetSource entry for this source file
    # returns: sorted list of include files for this source file
    def collect_includes(self, cfg_target, component, bf, src):
        # get the right compile group for this source file
        if len(cfg_target.target.compile_groups) < (src.compile_group_index + 1):
            _logger.debug(
                f"    - {cfg_target.target.name} has compile_group_index {src.compile_group_index} "
                f"but only {len(cfg_target.target.compile_groups)} found; "
                "skipping included files search"
            )
            return []
        cg = cfg_target.target.compile_groups[src.compile_group_index]

        # currently only doing C includes
        if cg.language != "C":
            _logger.debug(
                f"    - {cfg_target.target.name} has compile group language {cg.language} "
                "but currently only searching includes for C files; "
                "skipping included files search"
            )
            return []

        src_abspath = src.path
        if src.path[0] != "/":
            src_abspath = os.path.join(self.cm.paths_source, src.path)
        return get_c_includes(self.compiler_path, src_abspath, cg)

    # collect relationships for dependencies of this target Component
    # call with:
    #   1) all ConfigTargets from CodeModel
    #   2) this particular ConfigTarget
    #   3) Component for this Target
    def collect_target_dependencies(self, cfg_targets, cfg_target, component):
        _logger.debug(f"  - collecting target dependencies for {component.name}")

        # walk through target's dependencies
        for dep in cfg_target.target.dependencies:
            # extract dep name from its id
            dep_fragments = dep.id.split(":")
            dep_name = dep_fragments[0]
            _logger.debug(f"    - adding pending relationship for {dep_name}")

            # create relationship data between dependency components
            self.pending_relationships.append(
                ("component", component.name, "component", dep_name, "HAS_PREREQUISITE")
            )

            # if this is a target with any build artifacts (e.g. non-UTILITY),
            # also create STATIC_LINK relationship for dependency build files,
            # together with this Component's own target build file
            if len(cfg_target.target.artifacts) == 0:
                continue

            # find the filename for the dependency's build product, using the
            # codemodel (since we might not have created this dependency's
            # Component or File yet)
            dep_abspath = ""
            for ct in cfg_targets:
                if ct.name == dep_name:
                    # skip utility targets
                    if len(ct.target.artifacts) == 0:
                        continue
                    # all targets use the same base_dir, so this works
                    dep_abspath = os.path.join(component.base_dir, ct.target.artifacts[0])
                    break
            if dep_abspath == "":
                continue

            # create relationship data between build files
            if component.target_build_file:
                self.pending_relationships.append(
                    ("file", component.target_build_file.path, "file", dep_abspath, "STATIC_LINK")
                )

    # walk through pending sources and create corresponding files,
    # assigning them to the appropriate Component
    def walk_pending_sources(self):
        _logger.debug("walking pending sources")

        for src_abspath, collecting_component in self.pending_sources:
            # check whether we've already seen it
            if src_abspath in self.sbom_graph.files:
                _logger.debug(f"  - {src_abspath}: already seen")
                continue

            # not yet assigned; figure out which component should own it
            owning_component = self.find_owning_component(src_abspath, collecting_component)
            if not owning_component:
                _logger.debug(
                    f"  - {src_abspath}: can't determine which component should own; skipping"
                )
                continue

            # create File and assign it to the Component
            self.sbom_graph.add_file(SBOMFile(path=src_abspath), owning_component)

    # figure out which Component should own the given file based on path
    def _is_within(self, src_abspath, base_dir):
        """True if src_abspath lives under base_dir (same common ancestor)."""
        if not base_dir:
            return False
        try:
            return os.path.commonpath([src_abspath, base_dir]) == base_dir
        except ValueError:
            # Paths don't share a common ancestor (e.g. different drives)
            return False

    def find_owning_component(self, src_abspath, collecting_component=None):
        # Generated files live under the build directory. Every build-target
        # component shares the build directory as its base_dir, so a plain path
        # search cannot tell them apart and would assign every such file to whichever
        # build target happens to be first (e.g. "app"). Prefer the build target that
        # actually referenced this file as a source.
        if (
            collecting_component is not None
            and collecting_component.name in self.component_build_targets
            and self._is_within(src_abspath, collecting_component.base_dir)
        ):
            return collecting_component

        # Check build targets first (most specific)
        for component in self.component_build_targets.values():
            if self._is_within(src_abspath, component.base_dir):
                return component

        # Check SDK
        if (
            self.cfg.include_sdk
            and self.component_sdk
            and self._is_within(src_abspath, self.component_sdk.base_dir)
        ):
            return self.component_sdk

        # Check app
        if self.component_app and self._is_within(src_abspath, self.component_app.base_dir):
            return self.component_app

        # Check zephyr sources and module sources. A module path is nested under
        # the zephyr west topdir, so prefer the deepest matching base_dir to attribute
        # a file to its owning module rather than to the top-level zephyr component.
        zephyr_candidates = list(self.component_zephyr_modules.values())
        if self.component_zephyr:
            zephyr_candidates.append(self.component_zephyr)

        best_match = None
        for component in zephyr_candidates:
            if self._is_within(src_abspath, component.base_dir) and (
                best_match is None or len(component.base_dir) > len(best_match.base_dir)
            ):
                best_match = component

        return best_match

    # walk through pending relationship data and create relationships
    def walk_relationships(self):
        _logger.debug("walking pending relationships")

        for from_type, from_id, to_type, to_id, rln_type in self.pending_relationships:
            # Get from element
            from_elem = None
            if from_type == "component":
                from_elem = self.sbom_graph.get_component(from_id)
            elif from_type == "file":
                from_elem = self.sbom_graph.get_file(from_id)

            if not from_elem:
                _logger.debug(f"  - skipping relationship: {from_type}:{from_id} not found")
                continue

            # Get to element(s)
            to_elem = None
            if to_type == "component":
                to_elem = self.sbom_graph.get_component(to_id)
            elif to_type == "file":
                to_elem = self.sbom_graph.get_file(to_id)

            if not to_elem:
                _logger.debug(f"  - skipping relationship: {to_type}:{to_id} not found")
                continue

            self.sbom_graph.add_relationship(
                from_elem, to_elem, RelationshipType.from_value(rln_type)
            )

            _logger.debug(
                f"  - added relationship: {from_type}:{from_id} {rln_type} {to_type}:{to_id}"
            )
