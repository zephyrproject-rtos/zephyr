# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
from dataclasses import dataclass

from packaging.version import Version

from zspdx.scanner import ScannerConfig, scan_sbom_graph
from zspdx.version import SPDX_VERSION_2_3
from zspdx.walker import Walker, WalkerConfig

_logger = logging.getLogger(__name__)


# SBOMConfig contains settings that will be passed along to the various
# SBOM maker subcomponents.
@dataclass(eq=True)
class SBOMConfig:
    # prefix for Document namespaces; should not end with "/"
    namespace_prefix: str = ""

    # location of build directory
    build_dir: str = ""

    # location of SPDX document output directory
    spdx_dir: str = ""

    # SPDX specification version to use
    spdx_version: Version = SPDX_VERSION_2_3

    # should also analyze for included header files?
    analyze_includes: bool = False

    # should also add an SPDX document for the SDK?
    include_sdk: bool = False

    # --analyze-elf=snippets: generate a snippets add-on document from the
    # final image's DWARF debug info?
    generate_snippets: bool = False

    # ELF file to analyze for --analyze-elf; when empty, defaults to
    # <build_dir>/zephyr/zephyr.elf
    elf_file: str = ""


# create Cmake file-based API directories and query file
# Arguments:
#   1) build_dir: build directory
def setup_cmake_query(build_dir):
    # check that query dir exists as a directory, or else create it
    cmake_api_dir_path = os.path.join(build_dir, ".cmake", "api", "v1", "query")
    if os.path.exists(cmake_api_dir_path):
        if not os.path.isdir(cmake_api_dir_path):
            _logger.error(
                "cmake api query directory %s exists and is not a directory", cmake_api_dir_path
            )
            return False
        # directory exists, we're good
    else:
        # create the directory
        os.makedirs(cmake_api_dir_path, exist_ok=False)

    # request the codemodel (targets/sources) and toolchains-v1 (compiler ids and versions, used by
    # the SPDX 3.0 Build profile) file-based API objects by creating an empty query file for each
    for query_object in ("codemodel-v2", "toolchains-v1"):
        query_file_path = os.path.join(cmake_api_dir_path, query_object)
        if os.path.exists(query_file_path):
            if not os.path.isfile(query_file_path):
                _logger.error("cmake api query file %s exists and is not a file", query_file_path)
                return False
            # file exists, we're good
            continue
        # file doesn't exist, let's create an empty file
        with open(query_file_path, "w"):
            pass

    return True


# main entry point for SBOM maker
# Arguments:
#   1) cfg: SBOMConfig
def make_spdx(cfg):
    # report any odd configuration settings
    if cfg.analyze_includes and not cfg.include_sdk:
        _logger.warning(
            "config: requested to analyze includes but not to generate SDK SPDX document;"
        )
        _logger.warning(
            "config: will proceed but will discard detected includes for SDK header files"
        )

    # set up walker configuration
    walker_cfg = WalkerConfig()
    walker_cfg.namespace_prefix = cfg.namespace_prefix
    walker_cfg.build_dir = cfg.build_dir
    walker_cfg.analyze_includes = cfg.analyze_includes
    walker_cfg.include_sdk = cfg.include_sdk

    # make and run the walker
    w = Walker(walker_cfg)
    sbom_graph = w.collect_sbom_graph()
    if not sbom_graph:
        _logger.error("SPDX walker failed; bailing")
        return False

    # set up scanner configuration
    scanner_cfg = ScannerConfig()

    # scan SBOM graph
    scan_sbom_graph(scanner_cfg, sbom_graph)

    # optional: analyze the final image's DWARF debug info
    if cfg.generate_snippets:
        _analyze_elf(cfg, sbom_graph)

    # route to appropriate serializer based on version
    if cfg.spdx_version.major == 2:
        # Use SPDX 2.x serializer
        from zspdx.serializers.spdx2 import SPDX2Serializer

        serializer = SPDX2Serializer(sbom_graph, cfg.spdx_version)
        return serializer.serialize(cfg.spdx_dir)
    elif cfg.spdx_version.major == 3:
        # Use SPDX 3.0 serializer
        from zspdx.serializers.spdx3 import SPDX3Serializer

        serializer = SPDX3Serializer(sbom_graph, cfg.spdx_version)
        return serializer.serialize(cfg.spdx_dir)
    else:
        _logger.error("Unsupported SPDX version: %s", cfg.spdx_version)
        return False


def _analyze_elf(cfg: SBOMConfig, sbom_graph) -> None:
    """Record the final image's used source ranges as SPDX snippets."""
    from zspdx.dwarf import collect_used_lines

    elf_path = cfg.elf_file or os.path.join(cfg.build_dir, "zephyr", "zephyr.elf")
    if not os.path.isfile(elf_path):
        _logger.error(
            "ELF file not found for --analyze-elf: %s "
            "(build first, or pass --elf-file=<path>)",
            elf_path,
        )
        return

    file_lines = collect_used_lines(elf_path)
    _extract_snippets(sbom_graph, elf_path, file_lines)


def _extract_snippets(sbom_graph, elf_path: str, file_lines: dict[str, set[int]]) -> None:
    """Populate ``sbom_graph.snippets`` from the collected DWARF line map."""
    from zspdx.dwarf import ranges_from_line_map
    from zspdx.model import SBOMSnippet

    # Restrict to tracked files before folding into ranges so we only read the
    # sources we actually emit snippets for.
    known_paths = set(sbom_graph.files.keys())
    ranges_by_file = ranges_from_line_map(
        {path: lines for path, lines in file_lines.items() if path in known_paths}
    )

    # Record the image the snippets came from so serializers can emit the
    # source-to-binary provenance relationship. The graph may key the file by a
    # different (non-realpath) path, so fall back to a realpath comparison.
    sbom_graph.snippet_binary = sbom_graph.get_file(elf_path)
    if sbom_graph.snippet_binary is None:
        real_elf = os.path.realpath(elf_path)
        for file_path, spdx_file in sbom_graph.files.items():
            if os.path.realpath(file_path) == real_elf:
                sbom_graph.snippet_binary = spdx_file
                break

    for path, ranges in ranges_by_file.items():
        spdx_file = sbom_graph.get_file(path)
        if spdx_file is None:
            continue
        for r in ranges:
            snippet = SBOMSnippet(
                spdx_file=spdx_file,
                byte_range=(r.start_byte, r.end_byte),
                line_range=(r.start_line, r.end_line),
                concluded_license=spdx_file.concluded_license,
                copyright_text=spdx_file.copyright_text,
            )
            sbom_graph.snippets.append(snippet)

    _logger.info(
        "Extracted %d snippet(s) from %d source file(s)",
        len(sbom_graph.snippets),
        len(ranges_by_file),
    )
