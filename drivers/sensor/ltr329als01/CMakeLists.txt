#
# Copyright (c) 2018 Jan Vaishali Pathak vaishali@electronut.in
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LTR329ALS01 ltr329als01.c)
