# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import hashlib
import os

from west import log

from spdx.cmakefileapi import TargetType
from spdx.cmakefileapijson import parseReply
from spdx.builder import BuilderDocumentConfig, BuilderPackageConfig, convertToSPDXIDSafe, makeSPDX
from spdx.relationships import outputSPDXRelationships

def getCmakeRelationships(cm):
    """
    Extracts details from Cmake API about which built files derive from
    which sources. Looks at all targets within the first configuration
    in the CodeModel.

    Arguments:
        - cm: CodeModel
    Returns: list of tuples with relationships: [(filepathA, is_buildA, rln, filepathB, is_buildB), ...]
    """
    # get relative path: os.path.relpath(filename, cfg.scandir)
    rlns = []
    # walk through targets
    for cfgTarget in cm.configurations[0].configTargets:
        target = cfgTarget.target
        # FIXME currently only handles static / object libraries
        # for static / object libraries or executables, gather source files
        if target.type in [TargetType.EXECUTABLE, TargetType.STATIC_LIBRARY, TargetType.OBJECT_LIBRARY]:
            # FIXME currently only handles one artifact in list
            if len(target.artifacts) != 1:
                log.wrn(f"For target {target.name}, expected 1 artifact, got {len(target.artifacts)}; not generating relationships")
                continue
            artifactPath = target.artifacts[0]
            for src in target.sources:
                # FIXME this assumes that isGenerated tells us whether the file
                # FIXME is in build or sources; may not always be correct
                newRln = (os.path.join(".", artifactPath), True, "GENERATED_FROM", src.path, src.isGenerated)
                rlns.append(newRln)
            # also, if any dependencies of static libraries or executables created
            # artifacts, include STATIC_LINK relationships for those
            if target.type in [TargetType.EXECUTABLE, TargetType.STATIC_LIBRARY]:
                for dep in target.dependencies:
                    # now we need to find the target with this dep's ID
                    for depCfgTarget in cm.configurations[0].configTargets:
                        depTarget = depCfgTarget.target
                        if depTarget.id != dep.id:
                            continue
                        # now we've got the right one; check the dep types
                        # only link in library dependencies, not utility or executable
                        if depTarget.type in [TargetType.STATIC_LIBRARY, TargetType.OBJECT_LIBRARY]:
                            if len(depTarget.artifacts) != 1:
                                log.wrn(f"For dependency {depTarget.name}, expected 1 artifact, got {len(depTarget.artifacts)}; not generating linking relationship")
                                continue
                            depArtifactPath = depTarget.artifacts[0]
                            # FIXME this assumes that artifacts are always statically linking to something
                            # FIXME that was in the build directory; may not always be correct
                            newDepRln = (os.path.join(".", artifactPath), True, "STATIC_LINK",
                                         os.path.join(".", depArtifactPath), True)
                            rlns.append(newDepRln)
                            break
    return rlns

def makeCmakeSpdx(cm, srcRootDirs, spdxOutputDir, spdxNamespacePrefix):
    """
    Parse Cmake data and scan source / build directories, and create a
    corresponding SPDX tag-value document.

    Arguments:
        - cm: Cmake codemodel parsed by parseReply()
        - srcRootDirs: mapping of package SPDX ID (without "SPDXRef-") =>
                       sources root dir
        - spdxOutputDir: output directory where SPDX documents will be written
        - spdxNamespacePrefix: prefix for SPDX Document Namespace (will have
            "sources" and "build" appended); see Document Creation Info
            section in SPDX spec for more information
    Returns: True on success, False on failure; note that failure may still
             produce one or more partial SPDX documents
    """
    # create SPDX file for sources
    srcSpdxPath = os.path.join(spdxOutputDir, "sources.spdx")
    srcDocCfg = BuilderDocumentConfig()
    srcDocCfg.documentName = "sources"
    srcDocCfg.documentNamespace = os.path.join(spdxNamespacePrefix, "sources")
    for pkgID, pkgRootDir in srcRootDirs.items():
        srcPkgCfg = BuilderPackageConfig()
        srcPkgCfg.packageName = pkgID + " sources"
        srcPkgCfg.spdxID = "SPDXRef-" + pkgID
        srcPkgCfg.doSHA256 = True
        srcPkgCfg.scandir = pkgRootDir
        # FIXME is this correct as-is, or needs adjustment / resolve relative?
        srcPkgCfg.excludeDirs.append(cm.paths_build)
        srcDocCfg.packageConfigs[pkgRootDir] = srcPkgCfg

    srcDoc = makeSPDX(srcDocCfg, srcSpdxPath)
    if srcDoc:
        log.inf(f"Saved sources SPDX to {srcSpdxPath}")
    else:
        log.err(f"Couldn't generate sources SPDX file")
        return False

    # get hash of sources SPDX file, to use for build doc's extRef
    hSHA256 = hashlib.sha256()
    with open(srcSpdxPath, 'rb') as f:
        buf = f.read()
        hSHA256.update(buf)
    srcSHA256 = hSHA256.hexdigest()

    # get auto-generated relationships between filenames
    fileRlns = getCmakeRelationships(cm)

    # create SPDX file for build
    buildSpdxPath = os.path.join(spdxOutputDir, "build.spdx")
    buildDocCfg = BuilderDocumentConfig()
    buildDocCfg.documentName = "build"
    buildDocCfg.documentNamespace = os.path.join(spdxNamespacePrefix, "build")

    buildPkgCfg = BuilderPackageConfig()
    buildPkgCfg.packageName = "build"
    buildPkgCfg.spdxID = "SPDXRef-build"
    buildPkgCfg.doSHA256 = True
    buildPkgCfg.scandir = cm.paths_build
    buildDocCfg.packageConfigs[cm.paths_build] = buildPkgCfg

    # add external document ref to sources SPDX file
    buildDocCfg.extRefs = [("DocumentRef-sources", srcDocCfg.documentNamespace, "SHA256", srcSHA256)]

    # exclude CMake file-based API responses -- presume only used for this
    # SPDX generation scan, not for actual build artifact
    buildExcludeDir = os.path.join(cm.paths_build, ".cmake", "api")
    buildPkgCfg.excludeDirs.append(buildExcludeDir)
    # also exclude the generated SPDX documents themselves
    buildPkgCfg.excludeDirs.append("spdx")

    buildDoc = makeSPDX(buildDocCfg, buildSpdxPath)
    if buildDoc:
        log.inf(f"Saved build SPDX to {buildSpdxPath}")
    else:
        log.err(f"Couldn't generate build SPDX file")
        return False

    # and output relationships to build file also
    retval = outputSPDXRelationships(cm.paths_source, cm.paths_build, srcDoc, buildDoc, fileRlns, buildSpdxPath)
    if retval:
        log.inf(f"Added relationships to {buildSpdxPath}")
    else:
        log.err(f"Couldn't add relationships to build SPDX file")
        return False

    return True

def makeSpdxFromCmakeReply(replyIndexPath, spdxOutputDir, spdxNamespacePrefix):
    """
    Parse Cmake data to determine source / build directories, and call
    makeCmakeSpdx to create the corresponding SPDX tag-value document.

    Arguments:
        - replyIndexPath: path to index file from Cmake API reply JSON file
        - spdxOutputDir: output directory where SPDX documents will be written
        - spdxNamespacePrefix: prefix for SPDX Document Namespace (will have
            "sources" and "build" appended); see Document Creation Info
            section in SPDX spec for more information
    Returns: True on success, False on failure; note that failure may still
             produce one or more partial SPDX documents
    """
    # get CMake info from build
    cm = parseReply(replyIndexPath)

    # determine source packages and directory mappings
    srcRootDirs = {}
    for prj in cm.configurations[0].projects:
        # go through the directories and determine top directory for this package
        seen_first_dir = False
        is_prj_relative = False
        should_skip = False
        dirs_seen = []
        for dt in prj.directories:
            # make sure each project has either just relative or just absolute paths
            # FIXME this is probably not the best way to do this
            is_dir_relative = not(dt.source.startswith("/") or dt.source.startswith("\\"))
            if not seen_first_dir:
                is_prj_relative = is_dir_relative
                seen_first_dir = True
            else:
                if is_prj_relative != is_dir_relative:
                    log.wrn(f"directories for project {prj.name} contain both absolute and relative paths; skipping")
                    should_skip = True
                    break
            dirs_seen.append(dt.source)
        if should_skip:
            break
        srcRootDir = os.path.commonpath(dirs_seen)
        if is_prj_relative:
            srcRootDir = os.path.join(cm.paths_source, srcRootDir)

        # make SPDX ID for package
        # FIXME this needs to also ensure there isn't overlap in IDs
        pkgID = convertToSPDXIDSafe(prj.name)

        # add it to map
        srcRootDirs[pkgID] = srcRootDir

    # scan and create SPDX document
    return makeCmakeSpdx(cm, srcRootDirs, spdxOutputDir, spdxNamespacePrefix)

def setupCmakeQuery(buildDir):
    """
    Ensures that the query file (buildDir/.cmake/api/v1/query/codemodel-v2)
    exists, and creates it if it doesn't.

    Arguments:
        - buildDir: build directory
    Returns: True on success, False on failure
    """
    # check that query dir exists as a directory, or else create it
    cmakeApiDirPath = os.path.join(buildDir, ".cmake", "api", "v1", "query")
    if os.path.exists(cmakeApiDirPath):
        if not os.path.isdir(cmakeApiDirPath):
            log.err(f'cmake api query directory {cmakeApiDirPath} exists and is not a directory')
            return False
        # directory exists, we're good
    else:
        # create the directory
        os.makedirs(cmakeApiDirPath, exist_ok=False)

    # check that codemodel-v2 exists as a file, or else create it
    queryFilePath = os.path.join(cmakeApiDirPath, "codemodel-v2")
    if os.path.exists(queryFilePath):
        if not os.path.isfile(queryFilePath):
            log.err(f'cmake api query file {queryFilePath} exists and is not a directory')
            return False
        # file exists, we're good
        return True
    else:
        # file doesn't exist, let's create it
        os.mknod(queryFilePath)
        return True

def findIndexFile(buildDir):
    """
    Find the path to the index file from the cmake API response.

    Arguments:
        - buildDir: build directory
    Returns: path to index file in API reply directory or None if not found
    """
    # make sure the reply directory exists
    cmakeReplyDirPath = os.path.join(buildDir, ".cmake", "api", "v1", "reply")
    if not os.path.exists(cmakeReplyDirPath):
        log.err(f'cmake api reply directory {cmakeReplyDirPath} does not exist')
        log.err('was query directory created before cmake build ran?')
        return None
    if not os.path.isdir(cmakeReplyDirPath):
        log.err(f'cmake api reply directory {cmakeReplyDirPath} exists but is not a directory')
        return None

    # find file with "index" prefix; there should only be one
    for f in os.listdir(cmakeReplyDirPath):
        if f.startswith("index"):
            return os.path.join(cmakeReplyDirPath, f)

    # didn't find it
    log.err(f'cmake api reply index file not found in {cmakeReplyDirPath}')
    return None
