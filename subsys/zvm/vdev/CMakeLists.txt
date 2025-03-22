# Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
# Copyright 2024-2025 openEuler SIG-Zephyr
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
    CONFIG_ZVM
    vpsci.c
    vserial.c
    vpl011.c
)

zephyr_sources_ifdef(
    CONFIG_ARM64
    vgic_common.c
    vgic_v3.c
)
