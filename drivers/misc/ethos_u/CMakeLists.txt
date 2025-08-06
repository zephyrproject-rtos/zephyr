# Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(ethos_u_common.c)
zephyr_library_sources_ifdef(CONFIG_ETHOS_U_ARM ethos_u_arm.c)
zephyr_library_sources_ifdef(CONFIG_ETHOS_U_NUMAKER ethos_u_numaker.c)
