#
#  Copyright (c) 2021 Murali Tejeshwar Janaswami
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SI1133 si1133.c)
