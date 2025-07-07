/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SOC_SERIES_IMXRT118X)
__VECTOR_TABLE = CONFIG_ROM_START_OFFSET;
__Vectors = __VECTOR_TABLE;
#endif

#if defined(CONFIG_BOOT_XSPI_NOR)
. += CONFIG_XSPI_CONFIG_BLOCK_OFFSET - (. - __rom_start_address);
#else
. += CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET - (. - __rom_start_address);
#endif
#if defined(CONFIG_SOC_SERIES_IMXRT11XX) || defined(CONFIG_SOC_SERIES_IMXRT10XX) || defined(CONFIG_SOC_SERIES_IMXRT118X)
KEEP(*(.boot_hdr.conf))
#endif
#if defined(CONFIG_SOC_SERIES_IMXRT6XX) || defined(CONFIG_SOC_SERIES_IMXRT5XX) || defined(CONFIG_SOC_SERIES_IMXRT7XX)
KEEP(*(.flash_conf))
#endif
#if defined(CONFIG_SOC_SERIES_IMXRT118X)
#ifdef CONFIG_EXTERNAL_MEM_CONFIG_DATA
	. += CONFIG_EXTERNAL_MEM_CONFIG_OFFSET - (. - __rom_start_address);
	KEEP(*(.boot_hdr.xmcd_data))
#endif
. += CONFIG_IMAGE_CONTAINER_OFFSET - (. - __rom_start_address);
KEEP(*(.boot_hdr.container))
#else
. += CONFIG_IMAGE_VECTOR_TABLE_OFFSET - (. - __rom_start_address);
KEEP(*(.boot_hdr.ivt))
#endif
#if defined(CONFIG_SOC_SERIES_IMXRT11XX) || defined(CONFIG_SOC_SERIES_IMXRT10XX)
KEEP(*(.boot_hdr.data))
#ifdef CONFIG_DEVICE_CONFIGURATION_DATA
	KEEP(*(.boot_hdr.dcd_data))
#endif /* CONFIG_DEVICE_CONFIGURATION_DATA */
#ifdef CONFIG_EXTERNAL_MEM_CONFIG_DATA
	. += CONFIG_EXTERNAL_MEM_CONFIG_OFFSET - (. - __rom_start_address);
	KEEP(*(.boot_hdr.xmcd_data))
#endif
#endif /* CONFIG_SOC_SERIES_IMXRT10XX || CONFIG_SOC_SERIES_IMXRT11XX */
