# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import os

from west import log

from zspdx.walker import WalkerConfig, Walker
from zspdx.scanner import ScannerConfig, scanDocument
from zspdx.writer import writeSPDX


# SBOMConfig contains settings that will be passed along to the various
# SBOM maker subcomponents.
class SBOMConfig:
    def __init__(self):
        super(SBOMConfig, self).__init__()

        # prefix for Document namespaces; should not end with "/"
        self.namespacePrefix = ""

        # location of build directory
        self.buildDir = ""

        # location of SPDX document output directory
        self.spdxDir = ""

        # should also analyze for included header files?
        self.analyzeIncludes = False

        # should also add an SPDX document for the SDK?
        self.includeSDK = False


# create Cmake file-based API directories and query file
# Arguments:
#   1) build_dir: build directory
def setupCmakeQuery(build_dir):
    # check that query dir exists as a directory, or else create it
    cmakeApiDirPath = os.path.join(build_dir, ".cmake", "api", "v1", "query")
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
        # file doesn't exist, let's create an empty file
        cm_fd = open(queryFilePath, "w")
        cm_fd.close()
        return True


# main entry point for SBOM maker
# Arguments:
#   1) cfg: SBOMConfig
def makeSPDX(cfg):
    # report any odd configuration settings
    if cfg.analyzeIncludes and not cfg.includeSDK:
        log.wrn(f"config: requested to analyze includes but not to generate SDK SPDX document;")
        log.wrn(f"config: will proceed but will discard detected includes for SDK header files")

    # set up walker configuration
    walkerCfg = WalkerConfig()
    walkerCfg.namespacePrefix = cfg.namespacePrefix
    walkerCfg.buildDir = cfg.buildDir
    walkerCfg.analyzeIncludes = cfg.analyzeIncludes
    walkerCfg.includeSDK = cfg.includeSDK

    # make and run the walker
    w = Walker(walkerCfg)
    retval = w.makeDocuments()
    if not retval:
        log.err("SPDX walker failed; bailing")
        return False

    # set up scanner configuration
    scannerCfg = ScannerConfig()

    # scan each document from walker
    if cfg.includeSDK:
        scanDocument(scannerCfg, w.docSDK)
    scanDocument(scannerCfg, w.docApp)
    scanDocument(scannerCfg, w.docZephyr)
    scanDocument(scannerCfg, w.docBuild)

    # write each document, in this particular order so that the
    # hashes for external references are calculated

    # write SDK document, if we made one
    if cfg.includeSDK:
        retval = writeSPDX(os.path.join(cfg.spdxDir, "sdk.spdx"), w.docSDK)
        if not retval:
            log.err("SPDX writer failed for SDK document; bailing")
            return False

    # write app document
    retval = writeSPDX(os.path.join(cfg.spdxDir, "app.spdx"), w.docApp)
    if not retval:
        log.err("SPDX writer failed for app document; bailing")
        return False

    # write zephyr document
    writeSPDX(os.path.join(cfg.spdxDir, "zephyr.spdx"), w.docZephyr)
    if not retval:
        log.err("SPDX writer failed for zephyr document; bailing")
        return False

    # write build document
    writeSPDX(os.path.join(cfg.spdxDir, "build.spdx"), w.docBuild)
    if not retval:
        log.err("SPDX writer failed for build document; bailing")
        return False

    # write modules document
    writeSPDX(os.path.join(cfg.spdxDir, "modules-deps.spdx"), w.docModulesExtRefs)
    if not retval:
        log.err("SPDX writer failed for modules-deps document; bailing")
        return False

    return True
