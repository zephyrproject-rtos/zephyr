# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import os

from west import log

def resolveRelationshipID(relpathSrcDir, relpathBuildDir, srcDoc, buildDoc, filepath, is_build):
    """
    Determines the corresponding SPDX ID for filepath, depending on whether
    it is a source file or build file.

    Arguments:
        - relpathSrcDir: root directory of sources location for relative paths
        - relpathBuildDir: root directory of build location for relative paths
        - srcPkg: source SPDX Package section data
        - buildPkg: build SPDX Package section data
        - filepath: path to file being resolved; might be absolute or relative
        - is_build: should this file ID be in the build paths or the sources paths?
    Returns: resolved ID or None if not resolvable
    """
    # figure out relative path we're searching for
    # FIXME this is probably not the best way to do this
    is_relative = not(filepath.startswith("/") or filepath.startswith("\\"))

    # figure out if maybe it's a build file based on its absolute path,
    # even though we earlier concluded it was probably a sources file based on
    # its relation to the target type
    # FIXME this is really hackish -- we should be able to tell earlier in
    # FIXME the process whether it's a sources or build file, but isGenerated
    # FIXME doesn't always seem to be reliable
    if not is_relative and (os.path.commonpath([relpathBuildDir, filepath]) == relpathBuildDir):
        is_build = True

    # exclude files that have a relative path above its root dir
    # FIXME this is also probably not the best way to do this
    if filepath.startswith("../") or filepath.startswith("..\\"):
        # points to somewhere outside our sources root dir;
        # we won't be able to create this relationship
        log.wrn(f"{filepath} contains \"..\", can't create relationship")
        return None

    # is this a file from the build results?
    if is_build:
        if is_relative:
            searchPath = filepath
        else:
            searchPath = os.path.join(".", os.path.relpath(filepath, relpathBuildDir))
        # only one package in the build doc; get the "first" one
        for p in buildDoc.packages.values():
            pkg = p
            break

        # search through files for the one with this filename
        for f in pkg.files:
            if os.path.normpath(f.name) == os.path.normpath(searchPath):
                return f.spdxID

        log.wrn(f"{filepath} not found in build document, can't create relationship")
        return None

    # if we get here, it's a sources file
    # search the sources doc's packages for which one has this file
    # FIXME note that this may pick the wrong sources package / file path
    # FIXME   if two sources packages have the exact same file path
    # FIXME currently it will just go through in order and pick the first one
    # FIXME   that matches
    for srcRootDir, srcPkg in srcDoc.packages.items():
        if is_relative:
            searchPath = filepath
        else:
            searchPath = os.path.join(".", os.path.relpath(filepath, srcRootDir))

        # search through files for the one with this filename
        for f in srcPkg.files:
            if os.path.normpath(f.name) == os.path.normpath(searchPath):
                return f.spdxID

    # if we get here, we checked all the source packages and couldn't find it
    log.wrn(f"{filepath} not found in sources document, can't create relationship")
    return None

def outputSPDXRelationships(relpathSrcDir, relpathBuildDir, srcDoc, buildDoc, rlns, spdxPath):
    """
    Create and append SPDX relationships to the end of the previously-created
    SPDX build document.

    Arguments:
        - relpathSrcDir: root directory of sources location for relative paths
        - relpathBuildDir: root directory of build location for relative paths
        - srcDoc: source SPDX Document data
        - buildDoc: build SPDX Document data
        - rlns: Cmake relationship data from call to getCmakeRelationships()
        - spdxPath: path to previously-started SPDX build document
    Returns: True on success, False on error.
    """
    try:
        with open(spdxPath, "a") as f:
            for rln in rlns:
                pathA = rln[0]
                is_buildA = rln[1]
                rln_type = rln[2]
                pathB = rln[3]
                is_buildB = rln[4]

                rlnIDA = resolveRelationshipID(relpathSrcDir, relpathBuildDir, srcDoc, buildDoc, pathA, is_buildA)
                rlnIDB = resolveRelationshipID(relpathSrcDir, relpathBuildDir, srcDoc, buildDoc, pathB, is_buildB)
                if not rlnIDA or not rlnIDB:
                    continue

                # add DocumentRef- prefix for sources files
                if not is_buildA:
                    rlnIDA = "DocumentRef-sources:" + rlnIDA
                if not is_buildB:
                    rlnIDB = "DocumentRef-sources:" + rlnIDB

                f.write(f"Relationship: {rlnIDA} {rln_type} {rlnIDB}\n")
            return True

    except OSError as e:
        log.err(f"Error: Unable to append to {spdxPath}: {str(e)}")
        return False
