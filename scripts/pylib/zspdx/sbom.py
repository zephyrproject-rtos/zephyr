# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
from dataclasses import dataclass

from packaging.version import Version

from zspdx.scanner import ScannerConfig, scanSBOMGraph
from zspdx.version import SPDX_VERSION_2_3
from zspdx.walker import Walker, WalkerConfig

_logger = logging.getLogger(__name__)


# SBOMConfig contains settings that will be passed along to the various
# SBOM maker subcomponents.
@dataclass(eq=True)
class SBOMConfig:
    # prefix for Document namespaces; should not end with "/"
    namespacePrefix: str = ""

    # location of build directory
    buildDir: str = ""

    # location of SPDX document output directory
    spdxDir: str = ""

    # SPDX specification version to use
    spdxVersion: Version = SPDX_VERSION_2_3

    # should also analyze for included header files?
    analyzeIncludes: bool = False

    # should also add an SPDX document for the SDK?
    includeSDK: bool = False


# create Cmake file-based API directories and query file
# Arguments:
#   1) build_dir: build directory
def setupCmakeQuery(build_dir):
    # check that query dir exists as a directory, or else create it
    cmakeApiDirPath = os.path.join(build_dir, ".cmake", "api", "v1", "query")
    if os.path.exists(cmakeApiDirPath):
        if not os.path.isdir(cmakeApiDirPath):
            _logger.error(
                "cmake api query directory %s exists and is not a directory", cmakeApiDirPath
            )
            return False
        # directory exists, we're good
    else:
        # create the directory
        os.makedirs(cmakeApiDirPath, exist_ok=False)

    # check that codemodel-v2 exists as a file, or else create it
    queryFilePath = os.path.join(cmakeApiDirPath, "codemodel-v2")
    if os.path.exists(queryFilePath):
        if not os.path.isfile(queryFilePath):
            _logger.error("cmake api query file %s exists and is not a directory", queryFilePath)
            return False
        # file exists, we're good
        return True
    else:
        # file doesn't exist, let's create an empty file
        with open(queryFilePath, "w"):
            pass
        return True


# main entry point for SBOM maker
# Arguments:
#   1) cfg: SBOMConfig
def makeSPDX(cfg):
    # report any odd configuration settings
    if cfg.analyzeIncludes and not cfg.includeSDK:
        _logger.warning(
            "config: requested to analyze includes but not to generate SDK SPDX document;"
        )
        _logger.warning(
            "config: will proceed but will discard detected includes for SDK header files"
        )

    # set up walker configuration
    walkerCfg = WalkerConfig()
    walkerCfg.namespacePrefix = cfg.namespacePrefix
    walkerCfg.buildDir = cfg.buildDir
    walkerCfg.analyzeIncludes = cfg.analyzeIncludes
    walkerCfg.includeSDK = cfg.includeSDK

    # make and run the walker
    w = Walker(walkerCfg)
    sbom_graph = w.collectSBOMGraph()
    if not sbom_graph:
        _logger.error("SPDX walker failed; bailing")
        return False

    # set up scanner configuration
    scannerCfg = ScannerConfig()

    # scan SBOM graph
    scanSBOMGraph(scannerCfg, sbom_graph)

    # route to appropriate serializer based on version
    if cfg.spdxVersion.major == 2:
        # Use SPDX 2.x serializer
        from zspdx.serializers.spdx2 import SPDX2Serializer

        serializer = SPDX2Serializer(sbom_graph, cfg.spdxVersion)
        return serializer.serialize(cfg.spdxDir)
    else:
        _logger.error("Unsupported SPDX version: %s", cfg.spdxVersion)
        return False
