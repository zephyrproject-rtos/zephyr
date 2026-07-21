# SPDX-License-Identifier: Apache-2.0
zephyr_sources_ifdef(CONFIG_BT_BAS bas.c)
zephyr_sources_ifdef(CONFIG_BT_BAS_BLS bas_bls.c)
zephyr_sources_ifdef(CONFIG_BT_BAS_BCS bas_bcs.c)
