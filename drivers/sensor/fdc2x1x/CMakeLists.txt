#
#  Copyright (c) 2020 arithmetics.io
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(fdc2x1x.c)
zephyr_library_sources_ifdef(CONFIG_FDC2X1X_TRIGGER fdc2x1x_trigger.c)
