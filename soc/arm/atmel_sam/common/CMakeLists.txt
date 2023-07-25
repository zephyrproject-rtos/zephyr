# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_library_sources_ifndef(CONFIG_SOC_SERIES_SAM4L soc_pmc.c)
zephyr_library_sources_ifndef(CONFIG_SOC_SERIES_SAM4L soc_gpio.c)

zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_SAM4L soc_sam4l_pm.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_SAM4L soc_sam4l_gpio.c)
