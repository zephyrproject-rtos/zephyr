# Copyright 2024 CISPA Helmholtz Center for Information Security gGmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_sources()

zephyr_include_directories_ifdef(CONFIG_SOC_FAMILY_OPENHWGROUP_CVA6 ".")
