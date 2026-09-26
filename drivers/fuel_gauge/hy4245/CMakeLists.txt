# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(hy4245.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_HY4245 emul_hy4245.c)
