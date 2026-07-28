# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_sources(common/backend.c common/poller.c)
zephyr_library_sources_ifdef(CONFIG_NFC_NFCA protocol/nfca.c)
zephyr_library_sources_ifdef(CONFIG_NFC_ISO_DEP protocol/iso_dep.c)
zephyr_library_sources_ifdef(CONFIG_NFC_NDEF ndef/ndef.c)
zephyr_library_sources_ifdef(CONFIG_NFC_T2T tag/t2t.c)
zephyr_library_sources_ifdef(CONFIG_NFC_T4T tag/t4t.c)
zephyr_library_sources_ifdef(CONFIG_NFC_TAG tag/tag.c)
