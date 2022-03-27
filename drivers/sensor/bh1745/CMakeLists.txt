#
#  Copyright (c) 2019 Nordic Semiconductor ASA
#  Copyright (c) 2022 Grzegorz Blach
#
#  SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(bh1745.c)
zephyr_library_sources_ifdef(CONFIG_BH1745_TRIGGER bh1745_trigger.c)
