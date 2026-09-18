# Copyright (c) 2026 Fabien Proriol
# SPDX-License-Identifier: Apache-2.0

# knx_l1.h is in include/zephyr/knx/ — on the default include path.
# Neither driver reaches into subsys/knx/.
zephyr_sources_ifdef(CONFIG_NCN5130    ncn5130-uart.c)
zephyr_sources_ifdef(CONFIG_KNX_LOOPBACK knx_loopback.c)
