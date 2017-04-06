# Makefile - Gecko SDK
#
# Copyright (c) 2017, Christian Taedcke
#
# SPDX-License-Identifier: Apache-2.0
#

# Translate the SoC name and part number into the gecko device and cpu name
# respectively.
SILABS_GECKO_DEVICE = $(shell echo $(CONFIG_SOC_SERIES) | tr '[:lower:]' '[:upper:]')
SILABS_GECKO_PART_NUMBER = $(subst $(DQUOTE),,$(CONFIG_SOC_PART_NUMBER))

ifdef CONFIG_HAS_SILABS_GECKO
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/silabs/gecko/Device/SiliconLabs/$(SILABS_GECKO_DEVICE)/Include
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/silabs/gecko/emlib/inc

# The gecko SDK uses the cpu name to include the matching device header.
# See Device/SiliconLabs/$(SILABS_GECKO_DEVICE)/Include/em_device.h for an example.
KBUILD_CFLAGS += -D$(SILABS_GECKO_PART_NUMBER)

endif
