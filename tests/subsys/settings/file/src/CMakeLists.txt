# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Nordic Semiconductor ASA

FILE(GLOB fssources *.c )
target_sources(app PRIVATE ${fssources})
