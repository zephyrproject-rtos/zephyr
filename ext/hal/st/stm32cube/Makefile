# Makefile - STM32Cube SDK
#
# Copyright (c) 2016 Linaro Limited
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

ifdef CONFIG_HAS_STM32CUBE

STM32CUBE_CPU = $(shell echo $(CONFIG_SOC) | tr '[a-wyz]' '[A-WYZ]')

# STM32Cub uses the CPU name to expose SOC-specific attributes of a specific
# peripheral.
# It also requires USE_HAL_DRIVER to be define in order to benefit from
# STM32Cube HAL and LL APIs

KBUILD_CFLAGS += -D$(STM32CUBE_CPU) -DUSE_HAL_DRIVER

ifdef CONFIG_SOC_SERIES_STM32F1X
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f1xx/soc/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f1xx/drivers/include/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f1xx/drivers/include/Legacy/
endif

ifdef CONFIG_SOC_SERIES_STM32F3X
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f3xx/soc/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f3xx/drivers/include/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f3xx/drivers/include/Legacy/
endif

ifdef CONFIG_SOC_SERIES_STM32F4X
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f4xx/soc/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f4xx/drivers/include/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f4xx/drivers/include/Legacy/
endif

ifdef CONFIG_SOC_SERIES_STM32F7X
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f7xx/soc/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f7xx/drivers/include/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32f7xx/drivers/include/Legacy/
endif

ifdef CONFIG_SOC_SERIES_STM32L4X
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32l4xx/soc/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32l4xx/drivers/include/
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/st/stm32cube/stm32l4xx/drivers/include/Legacy/
endif

endif
