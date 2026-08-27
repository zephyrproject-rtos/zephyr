# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/biometrics.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BIOMETRICS_SHELL biometrics_shell.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_BIOMETRICS_EMUL biometrics_emul.c)
zephyr_library_sources_ifdef(CONFIG_BIOMETRICS_GT5X biometrics_gt5x.c)
zephyr_library_sources_ifdef(CONFIG_BIOMETRICS_ZFM_X0 biometrics_zfm_x0.c)
# zephyr-keep-sorted-stop
