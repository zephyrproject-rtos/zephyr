# Makefile - MCUXpresso SDK
#
# Copyright (c) 2016, Freescale Semiconductor, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Translate the SoC name and part number into the mcux device and cpu name
# respectively.
MCUX_DEVICE = $(shell echo $(CONFIG_SOC) | tr '[:lower:]' '[:upper:]')
MCUX_CPU = CPU_$(subst $(DQUOTE),,$(CONFIG_SOC_PART_NUMBER))

ifdef CONFIG_HAS_MCUX
ifdef CONFIG_ETH_MCUX
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/nxp/mcux/components/phyksz8081
endif
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/nxp/mcux/devices/$(MCUX_DEVICE)
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/nxp/mcux/drivers

# The mcux uses the cpu name to expose SoC-specific features of a given
# peripheral. For example, the UART peripheral may be instantiated with/without
# a hardware FIFO, and the size of that FIFO may be different for each instance
# in a given SoC. See fsl_device_registers.h and $(MCUX_DEVICE)_features.h
KBUILD_CFLAGS += -D$(MCUX_CPU)

# Build mcux device-specific objects. Although it is not normal Kbuild
# practice, drilling down like this avoids the need for repetitive Makefiles
# for every mcux device.
obj-y += devices/$(MCUX_DEVICE)/fsl_clock.o

# Build mcux drivers that can be used for multiple SoC's.
obj-y += components/
obj-y += drivers/

endif
