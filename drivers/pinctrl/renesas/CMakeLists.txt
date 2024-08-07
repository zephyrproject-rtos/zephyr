# Copyright (c) 2024 EPAM Systems
# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_PINCTRL_RENESAS_RA_PFS ra/pinctrl_ra.c)
zephyr_library_sources_ifdef(CONFIG_PINCTRL_RZT2M rz/pinctrl_rzt2m.c)
zephyr_library_sources_ifdef(CONFIG_PINCTRL_SMARTBOND smartbond/pinctrl_smartbond.c)

add_subdirectory_ifdef(CONFIG_PINCTRL_RCAR_PFC rcar)
