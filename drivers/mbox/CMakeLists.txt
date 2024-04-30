# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/mbox.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE   mbox_handlers.c)
zephyr_library_sources_ifdef(CONFIG_MBOX_NRFX_IPC   mbox_nrfx_ipc.c)
zephyr_library_sources_ifdef(CONFIG_MBOX_NXP_S32_MRU   mbox_nxp_s32_mru.c)
zephyr_library_sources_ifdef(CONFIG_MBOX_NXP_IMX_MU   mbox_nxp_imx_mu.c)
zephyr_library_sources_ifdef(CONFIG_MBOX_NXP_MAILBOX   mbox_nxp_mailbox.c)
zephyr_library_sources_ifdef(CONFIG_MBOX_ANDES_PLIC_SW   mbox_andes_plic_sw.c)
