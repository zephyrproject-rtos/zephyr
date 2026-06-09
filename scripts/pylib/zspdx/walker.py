# Copyright (c) 2020-2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import re
from dataclasses import dataclass

import yaml
from west.util import WestNotFound, west_topdir

from zspdx.cmakecache import parseCMakeCacheFile
from zspdx.cmakefileapijson import parseReply
from zspdx.getincludes import getCIncludes
from zspdx.model import (
    ComponentPurpose,
    RelationshipType,
    SBOMComponent,
    SBOMDocument,
    SBOMFile,
    SBOMGraph,
)

_logger = logging.getLogger(__name__)


# WalkerConfig contains configuration data for the Walker.
@dataclass(eq=True)
class WalkerConfig:
    # prefix for Document namespaces; should not end with "/"
    namespacePrefix: str = ""

    # location of build directory
    buildDir: str = ""

    # should also analyze for included header files?
    analyzeIncludes: bool = False

    # should also add an SPDX document for the SDK?
    includeSDK: bool = False


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
        self.sbom_graph = SBOMGraph(namespace_prefix=cfg.namespacePrefix, build_dir=cfg.buildDir)

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
        self.pendingSources = []

        # queue of pending relationship data to create, process and assign
        # Format: (from_type, from_identifier, to_type, to_identifier, relationship_type)
        # Types: "component", "file"
        self.pendingRelationships = []

        # parsed CMake codemodel
        self.cm = None

        # parsed CMake cache dict
        self.cmakeCache = {}

        # C compiler path from parsed CMake cache
        self.compilerPath = ""

        # SDK install path from parsed CMake cache
        self.sdkPath = ""

        # Meta file path
        self.metaFile = ""

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
    def collectSBOMGraph(self):
        """
        Collect the SBOM graph from CMake codemodel and build artifacts.
        Returns SBOMGraph object containing all collected information.
        """
        # parse CMake cache file and get compiler path
        _logger.info("parsing CMake Cache file")
        self.getCacheFile()

        # check if meta file is generated
        if not self.metaFile:
            _logger.error(
                "CONFIG_BUILD_OUTPUT_META must be enabled to generate spdx files; bailing"
            )
            return None

        # parse codemodel from CMake file-based API
        _logger.info("parsing CMake file-based API codemodel")
        self.cm = self.getCodemodel()
        if not self.cm:
            _logger.error("could not parse codemodel from CMake API reply; bailing")
            return None

        # set up components
        _logger.info("setting up SBOM components")
        retval = self.setupComponents()
        if not retval:
            return None

        # walk through targets in codemodel to gather information
        _logger.info("walking through targets")
        self.walkTargets()

        # walk through pending sources and create corresponding files
        _logger.info("walking through pending sources files")
        self.walkPendingSources()

        # walk through pending relationship data and create relationships
        _logger.info("walking through pending relationships")
        self.walkRelationships()

        return self.sbom_graph

    # parse cache file and pull out relevant data
    def getCacheFile(self):
        cacheFilePath = os.path.join(self.cfg.buildDir, "CMakeCache.txt")
        self.cmakeCache = parseCMakeCacheFile(cacheFilePath)
        if self.cmakeCache:
            self.compilerPath = self.cmakeCache.get("CMAKE_C_COMPILER", "")
            self.sdkPath = self.cmakeCache.get("ZEPHYR_SDK_INSTALL_DIR", "")
            self.metaFile = self.cmakeCache.get("KERNEL_META_PATH", "")

    # determine path from build dir to CMake file-based API index file, then
    # parse it and return the Codemodel
    def getCodemodel(self):
        _logger.debug("getting codemodel from CMake API reply files")

        # make sure the reply directory exists
        cmakeReplyDirPath = os.path.join(self.cfg.buildDir, ".cmake", "api", "v1", "reply")
        if not os.path.exists(cmakeReplyDirPath):
            _logger.error(f'cmake api reply directory {cmakeReplyDirPath} does not exist')
            _logger.error('was query directory created before cmake build ran?')
            return None
        if not os.path.isdir(cmakeReplyDirPath):
            _logger.error(
                f'cmake api reply directory {cmakeReplyDirPath} exists but is not a directory'
            )
            return None

        # find file with "index" prefix; there should only be one
        indexFilePath = ""
        for f in os.listdir(cmakeReplyDirPath):
            if f.startswith("index"):
                indexFilePath = os.path.join(cmakeReplyDirPath, f)
                break
        if indexFilePath == "":
            # didn't find it
            _logger.error(f'cmake api reply index file not found in {cmakeReplyDirPath}')
            return None

        # parse it
        return parseReply(indexFilePath)

    def _create_document(self, name: str, title: str = "") -> SBOMDocument:
        """Create a document with the given name and register it with SBOM data.

        ``name`` is the identifier used for the output filename, namespace and
        cross-document reference ID; ``title`` is the human-readable SPDX
        ``DocumentName`` and defaults to ``name`` when not given.
        """
        doc = SBOMDocument(name=name, title=title, namespace=f"{self.cfg.namespacePrefix}/{name}")
        self.sbom_graph.add_document(doc)
        return doc

    def setupDocuments(self):
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
        if self.cfg.includeSDK:
            self.doc_sdk = self._create_document("sdk")

    def setupComponents(self):
        """Set up all SBOM components from meta file and configuration."""
        _logger.debug("setting up SBOM components")

        # First set up documents
        self.setupDocuments()

        try:
            with open(self.metaFile) as file:
                content = yaml.load(file.read(), yaml.SafeLoader)
                if not self.setupZephyrComponent(content["zephyr"], content["modules"]):
                    return False
        except (FileNotFoundError, yaml.YAMLError):
            _logger.error("cannot find a valid zephyr.meta required for SPDX generation; bailing")
            return False

        self.setupAppComponent()

        if self.cfg.includeSDK:
            self.setupSDKComponent()

        self.setupModulesDepsComponent(content["modules"], content["zephyr"])

        return True

    def setupAppComponent(self):
        """Set up app sources component."""
        component = SBOMComponent(
            name="app-sources",
            purpose=ComponentPurpose.SOURCE,
            base_dir=self.cm.paths_source,
        )

        self.sbom_graph.add_component(component, "app")
        self.doc_app.add_described_component(component)
        self.component_app = component

    def setupZephyrComponent(self, zephyr, modules):
        """Set up zephyr sources component and module components."""
        # relativeBaseDir is Zephyr sources topdir
        try:
            relativeBaseDir = west_topdir(self.cm.paths_source)
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
            base_dir=relativeBaseDir,
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

    def setupSDKComponent(self):
        """Set up SDK sources component."""
        component = SBOMComponent(
            name="sdk-sources",
            purpose=ComponentPurpose.SOURCE,
            base_dir=self.sdkPath,
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

    def setupModulesDepsComponent(self, modules, zephyr=None):
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
                self.pendingRelationships.append(
                    ("component", component.name, "component", zephyr_deps.name, "DEPENDENCY_OF")
                )

        return True

    # walk through targets and gather information
    def walkTargets(self):
        _logger.debug("walking targets from codemodel")

        # assuming just one configuration; consider whether this is incorrect
        cfgTargets = self.cm.configurations[0].configTargets
        for cfgTarget in cfgTargets:
            # build the Component for this target
            component = self.initConfigTargetComponent(cfgTarget)

            # see whether this target has any build artifacts at all
            if len(cfgTarget.target.artifacts) > 0:
                # add its build file
                bf = self.addBuildFile(cfgTarget, component)
                if component.name == "zephyr_final":
                    component.purpose = ComponentPurpose.APPLICATION
                    # the build document's primary subject is the final image
                    self.doc_build.add_described_component(component)
                else:
                    component.purpose = ComponentPurpose.LIBRARY

                # get its source files if build file is found
                if bf:
                    self.collectPendingSourceFiles(cfgTarget, component, bf)
            else:
                _logger.debug(f"  - target {cfgTarget.name} has no build artifacts")

            # get its target dependencies
            self.collectTargetDependencies(cfgTargets, cfgTarget, component)

    # build a Component for the given ConfigTarget
    def initConfigTargetComponent(self, cfgTarget):
        _logger.debug(f"  - initializing Component for target: {cfgTarget.name}")

        # create target Component
        component = SBOMComponent(
            name=cfgTarget.name,
            base_dir=self.cm.paths_build,
        )

        # add Component to SBOM data and build document
        self.sbom_graph.add_component(component, "build")
        self.component_build_targets[cfgTarget.name] = component
        return component

    # create a target's build product File and add it to its Component
    # call with:
    #   1) ConfigTarget
    #   2) Component for that target
    # returns: SBOMFile
    def addBuildFile(self, cfgTarget, component):
        # assumes only one artifact in each target
        artifactPath = os.path.join(component.base_dir, cfgTarget.target.artifacts[0])
        _logger.debug(f"  - adding File {artifactPath}")
        _logger.debug(f"    - base_dir: {component.base_dir}")
        _logger.debug(f"    - artifacts[0]: {cfgTarget.target.artifacts[0]}")

        # don't create build File if artifact path points to nonexistent file
        if not os.path.exists(artifactPath):
            _logger.debug(
                f"  - target {cfgTarget.name} lists build artifact {artifactPath} "
                "but file not found after build; skipping"
            )
            return None

        # create build File
        bf = SBOMFile(path=artifactPath, relative_path=cfgTarget.target.artifacts[0])
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
    def collectPendingSourceFiles(self, cfgTarget, component, bf):
        _logger.debug("  - collecting source files and adding to pending queue")

        targetIncludesSet = set()

        # walk through target's sources
        for src in cfgTarget.target.sources:
            _logger.debug(f"    - add pending source file and relationship for {src.path}")
            # get absolute, normalized path if we don't have it
            srcAbspath = src.path
            if not os.path.isabs(src.path):
                srcAbspath = os.path.normpath(os.path.join(self.cm.paths_source, src.path))

            # check whether it even exists
            if not (os.path.exists(srcAbspath) and os.path.isfile(srcAbspath)):
                _logger.debug(
                    f"  - {srcAbspath} does not exist but is referenced in sources for "
                    f"target {component.name}; skipping"
                )
                continue

            # add it to pending source files queue, remembering the build
            # target that referenced it (used to assign generated/build-dir
            # files to the correct build component)
            self.pendingSources.append((srcAbspath, component))

            # create relationship data: build file GENERATED_FROM source file
            self.pendingRelationships.append(
                ("file", bf.path, "file", srcAbspath, "GENERATED_FROM")
            )

            # collect this source file's includes
            if self.cfg.analyzeIncludes and self.compilerPath:
                includes = self.collectIncludes(cfgTarget, component, bf, src)
                for inc in includes:
                    targetIncludesSet.add(inc)

        # make relationships for the overall included files,
        # avoiding duplicates for multiple source files including
        # the same headers
        targetIncludesList = list(targetIncludesSet)
        targetIncludesList.sort()
        for inc in targetIncludesList:
            # add it to pending source files queue, remembering the build
            # target that referenced it
            self.pendingSources.append((inc, component))

            # create relationship data: build file GENERATED_FROM include file
            self.pendingRelationships.append(("file", bf.path, "file", inc, "GENERATED_FROM"))

    # collect the include files corresponding to this source file
    # call with:
    #   1) ConfigTarget
    #   2) Component for this target
    #   3) build File for this target
    #   4) TargetSource entry for this source file
    # returns: sorted list of include files for this source file
    def collectIncludes(self, cfgTarget, component, bf, src):
        # get the right compile group for this source file
        if len(cfgTarget.target.compileGroups) < (src.compileGroupIndex + 1):
            _logger.debug(
                f"    - {cfgTarget.target.name} has compileGroupIndex {src.compileGroupIndex} "
                f"but only {len(cfgTarget.target.compileGroups)} found; "
                "skipping included files search"
            )
            return []
        cg = cfgTarget.target.compileGroups[src.compileGroupIndex]

        # currently only doing C includes
        if cg.language != "C":
            _logger.debug(
                f"    - {cfgTarget.target.name} has compile group language {cg.language} "
                "but currently only searching includes for C files; "
                "skipping included files search"
            )
            return []

        srcAbspath = src.path
        if src.path[0] != "/":
            srcAbspath = os.path.join(self.cm.paths_source, src.path)
        return getCIncludes(self.compilerPath, srcAbspath, cg)

    # collect relationships for dependencies of this target Component
    # call with:
    #   1) all ConfigTargets from CodeModel
    #   2) this particular ConfigTarget
    #   3) Component for this Target
    def collectTargetDependencies(self, cfgTargets, cfgTarget, component):
        _logger.debug(f"  - collecting target dependencies for {component.name}")

        # walk through target's dependencies
        for dep in cfgTarget.target.dependencies:
            # extract dep name from its id
            depFragments = dep.id.split(":")
            depName = depFragments[0]
            _logger.debug(f"    - adding pending relationship for {depName}")

            # create relationship data between dependency components
            self.pendingRelationships.append(
                ("component", component.name, "component", depName, "HAS_PREREQUISITE")
            )

            # if this is a target with any build artifacts (e.g. non-UTILITY),
            # also create STATIC_LINK relationship for dependency build files,
            # together with this Component's own target build file
            if len(cfgTarget.target.artifacts) == 0:
                continue

            # find the filename for the dependency's build product, using the
            # codemodel (since we might not have created this dependency's
            # Component or File yet)
            depAbspath = ""
            for ct in cfgTargets:
                if ct.name == depName:
                    # skip utility targets
                    if len(ct.target.artifacts) == 0:
                        continue
                    # all targets use the same base_dir, so this works
                    depAbspath = os.path.join(component.base_dir, ct.target.artifacts[0])
                    break
            if depAbspath == "":
                continue

            # create relationship data between build files
            if component.target_build_file:
                self.pendingRelationships.append(
                    ("file", component.target_build_file.path, "file", depAbspath, "STATIC_LINK")
                )

    # walk through pending sources and create corresponding files,
    # assigning them to the appropriate Component
    def walkPendingSources(self):
        _logger.debug("walking pending sources")

        for srcAbspath, collecting_component in self.pendingSources:
            # check whether we've already seen it
            if srcAbspath in self.sbom_graph.files:
                _logger.debug(f"  - {srcAbspath}: already seen")
                continue

            # not yet assigned; figure out which component should own it
            owning_component = self.findOwningComponent(srcAbspath, collecting_component)
            if not owning_component:
                _logger.debug(
                    f"  - {srcAbspath}: can't determine which component should own; skipping"
                )
                continue

            # create File and assign it to the Component
            self.sbom_graph.add_file(SBOMFile(path=srcAbspath), owning_component)

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

    def findOwningComponent(self, srcAbspath, collecting_component=None):
        # Generated files live under the build directory. Every build-target
        # component shares the build directory as its base_dir, so a plain path
        # search cannot tell them apart and would assign every such file to whichever
        # build target happens to be first (e.g. "app"). Prefer the build target that
        # actually referenced this file as a source.
        if (
            collecting_component is not None
            and collecting_component.name in self.component_build_targets
            and self._is_within(srcAbspath, collecting_component.base_dir)
        ):
            return collecting_component

        # Check build targets first (most specific)
        for component in self.component_build_targets.values():
            if self._is_within(srcAbspath, component.base_dir):
                return component

        # Check SDK
        if (
            self.cfg.includeSDK
            and self.component_sdk
            and self._is_within(srcAbspath, self.component_sdk.base_dir)
        ):
            return self.component_sdk

        # Check app
        if self.component_app and self._is_within(srcAbspath, self.component_app.base_dir):
            return self.component_app

        # Check zephyr sources and module sources. A module path is nested under
        # the zephyr west topdir, so prefer the deepest matching base_dir to attribute
        # a file to its owning module rather than to the top-level zephyr component.
        zephyr_candidates = list(self.component_zephyr_modules.values())
        if self.component_zephyr:
            zephyr_candidates.append(self.component_zephyr)

        best_match = None
        for component in zephyr_candidates:
            if self._is_within(srcAbspath, component.base_dir) and (
                best_match is None or len(component.base_dir) > len(best_match.base_dir)
            ):
                best_match = component

        return best_match

    # walk through pending relationship data and create relationships
    def walkRelationships(self):
        _logger.debug("walking pending relationships")

        for from_type, from_id, to_type, to_id, rln_type in self.pendingRelationships:
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
