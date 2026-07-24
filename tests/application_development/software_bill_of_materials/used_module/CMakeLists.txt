# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(src/answer.c)
# no_header.c has no SPDX tags of its own; its license/copyright come from
# REUSE.toml. Compiling it lets the SBOM tests verify REUSE.toml support.
zephyr_library_sources(src/no_header.c)
zephyr_include_directories(include)

# Fake vendor blobs, linked into the final image the two ways HAL modules link
# their real ones: as an imported library (like hal_stm32) and as -l/-L linker
# flags (like hal_espressif). The SBOM tests verify both end up in the SBOM,
# attributed to this module and carrying the blob metadata from module.yml.
# An empty ar archive is just its 8-byte magic, so the "blobs" can be
# generated here instead of committing binaries to the tree.
set(SBOM_BLOBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs)
file(WRITE ${SBOM_BLOBS_DIR}/libsbom_blob_imported.a "!<arch>\n")
file(WRITE ${SBOM_BLOBS_DIR}/libsbom_blob_flag.a "!<arch>\n")

add_library(sbom_blob_imported STATIC IMPORTED GLOBAL)
set_target_properties(sbom_blob_imported
  PROPERTIES IMPORTED_LOCATION ${SBOM_BLOBS_DIR}/libsbom_blob_imported.a
)
zephyr_link_libraries(sbom_blob_imported)

zephyr_link_libraries(sbom_blob_flag -L${SBOM_BLOBS_DIR})
