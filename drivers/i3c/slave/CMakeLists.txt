# Copyright (c) 2021 ASPEED Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_I3C_SLAVE_MQUEUE	i3c_slave_mqueue.c)
