# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/drivers/cmis.h
  ${ZEPHYR_BASE}/include/zephyr/drivers/sfp.h
)

zephyr_library_sources_ifdef(CONFIG_USERSPACE sfp_handlers.c)
zephyr_library_sources_ifdef(CONFIG_SFP_SFF8472 sfp_sff8472.c)
zephyr_library_sources_ifdef(CONFIG_SFP_SHELL sfp_shell.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_SFF8472 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_SFF8472 emul_sff8472.c)

zephyr_library_sources_ifdef(CONFIG_SFP_CMIS cmis_module.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE cmis_handlers.c)
zephyr_library_sources_ifdef(CONFIG_SFP_CMIS_SHELL cmis_shell.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_CMIS .)
zephyr_library_sources_ifdef(CONFIG_EMUL_CMIS emul_cmis.c)
