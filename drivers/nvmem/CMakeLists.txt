# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NVMEM_NXP_OCOTP nvmem_nxp_ocotp.c)
zephyr_library_sources_ifdef(CONFIG_NVMEM_LPC11U6X nvmem_lpc11u6x.c)
zephyr_library_sources_ifdef(CONFIG_NVMEM_FM25XXX nvmem_fm25xxx.c)

# Emit a configure-time warning if OCOTP programming is enabled.
if(CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE)
    message(WARNING "OCOTP fuse programming is destructive and irreversible;
            enable only for provisioning in a controlled environment.
            DO NOT enable for production builds.")
endif()
