# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(flash_util.c)

zephyr_syscall_header_ifdef(
  CONFIG_FLASH_SIMULATOR
  ${ZEPHYR_BASE}/include/zephyr/drivers/flash/flash_simulator.h
)

zephyr_syscall_header_ifdef(
  CONFIG_NORDIC_QSPI_NOR
  ${ZEPHYR_BASE}/include/zephyr/drivers/flash/nrf_qspi_nor.h
)

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/flash.h)

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_FLASH_JESD216 jesd216.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_PAGE_LAYOUT flash_page_layout.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_SHELL flash_shell.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE flash_handlers.c)
# zephyr-keep-sorted-stop

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_FLASH_AMBIQ flash_ambiq.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_ANDES_QSPI flash_andes_qspi.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_CAD_QSPI_NOR flash_cadence_qspi_nor.c flash_cadence_qspi_nor_ll.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_CDNS_NAND flash_cadence_nand.c flash_cadence_nand_ll.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_INFINEON_CAT1 flash_ifx_cat1.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_HYPERFLASH flash_mcux_flexspi_hyperflash.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_MX25UM51345G flash_mcux_flexspi_mx25um51345g.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_NOR flash_mcux_flexspi_nor.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MSPI_ATXP032 flash_mspi_atxp032.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MSPI_EMUL_DEVICE flash_mspi_emul_device.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_NPCX_FIU_NOR flash_npcx_fiu_nor.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_NPCX_FIU_QSPI flash_npcx_fiu_qspi.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_RPI_PICO flash_rpi_pico.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_STM32_OSPI flash_stm32_ospi.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_STM32_QSPI flash_stm32_qspi.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_STM32_XSPI flash_stm32_xspi.c)
zephyr_library_sources_ifdef(CONFIG_INFINEON_CAT1_QSPI_FLASH flash_ifx_cat1_qspi.c)
zephyr_library_sources_ifdef(CONFIG_NORDIC_QSPI_NOR nrf_qspi_nor.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_CC13XX_CC26XX soc_flash_cc13xx_cc26xx.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_CC23X0 soc_flash_cc23x0.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_ESP32 flash_esp32.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_GECKO flash_gecko.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_ITE_IT8XXX2 flash_ite_it8xxx2.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_LPC soc_flash_lpc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_MAX32 flash_max32.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_MCUX soc_flash_mcux.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NIOS2_QSPI soc_flash_nios2_qspi.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF soc_flash_nrf.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF_MRAM soc_flash_nrf_mram.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF_RRAM soc_flash_nrf_rram.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NUMAKER soc_flash_numaker.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NUMAKER_RMC soc_flash_numaker_rmc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_RV32M1 soc_flash_rv32m1.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SAM flash_sam.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SAM0 flash_sam0.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SI32 flash_si32.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SILABS_SIWX91X soc_flash_silabs_siwx91x.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SMARTBOND flash_smartbond.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_TELINK_B91 soc_flash_b91.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_XMC4XXX soc_flash_xmc4xxx.c)
zephyr_library_sources_ifdef(CONFIG_SPI_FLASH_AT45 spi_flash_at45.c)
zephyr_library_sources_ifdef(CONFIG_SPI_NOR spi_nor.c)
# zephyr-keep-sorted-stop

if(CONFIG_FLASH_SIMULATOR)
  zephyr_library_sources(flash_simulator.c)
  if(CONFIG_NATIVE_LIBRARY)
    target_sources(native_simulator INTERFACE flash_simulator_native.c)
  elseif(CONFIG_ARCH_POSIX)
    zephyr_library_sources(flash_simulator_native.c)
  endif()
endif()

zephyr_library_include_directories_ifdef(CONFIG_MSPI ${ZEPHYR_BASE}/drivers/mspi)
if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
  dt_chosen(chosen_flash PROPERTY "zephyr,flash")
  dt_prop(compat_flash PATH ${chosen_flash} PROPERTY compatible)
  if(compat_flash MATCHES "nxp,imx-flexspi-nor")
	  zephyr_code_relocate(FILES flash_mcux_flexspi_nor.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
	  zephyr_code_relocate(FILES jesd216.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  elseif(compat_flash MATCHES "nxp,imx-flexspi-mx25um51345g")
	  zephyr_code_relocate(FILES flash_mcux_flexspi_mx25um51345g.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  elseif(compat_flash MATCHES "nxp,imx-flexspi-hyperflash")
    zephyr_code_relocate(FILES flash_mcux_flexspi_hyperflash.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  endif()
endif()

if(CONFIG_SOC_FLASH_STM32)
  zephyr_library_sources_ifdef(CONFIG_FLASH_EX_OP_ENABLED flash_stm32_ex_op.c)
  if(CONFIG_SOC_SERIES_STM32H7X)
    zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32H7_FLASH_CONTROLLER_ENABLED flash_stm32h7x.c)
  elseif(CONFIG_SOC_SERIES_STM32H7RSX)
    zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32H7_FLASH_CONTROLLER_ENABLED flash_stm32h7x.c)
  elseif(CONFIG_SOC_SERIES_STM32WBAX)
    if(CONFIG_BT_STM32WBA)
      # BLE is enabled. Use implementation over Flash Manager for coexistence with RF activities
      zephyr_library_sources(flash_stm32wba_fm.c)
    else()
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32_FLASH_CONTROLLER_ENABLED flash_stm32.c flash_stm32wbax.c)
    endif()
  elseif(CONFIG_SOC_SERIES_STM32WB0X)
    zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32WB0_FLASH_CONTROLLER_ENABLED flash_stm32wb0x.c)
  else()
    if(CONFIG_DT_HAS_ST_STM32_FLASH_CONTROLLER_ENABLED)
      zephyr_library_sources(flash_stm32.c)

# zephyr-keep-sorted-start
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32F1_FLASH_CONTROLLER_ENABLED flash_stm32f1x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32F2_FLASH_CONTROLLER_ENABLED flash_stm32f2x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32F4_FLASH_CONTROLLER_ENABLED flash_stm32f4x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32F7_FLASH_CONTROLLER_ENABLED flash_stm32f7x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32G0_FLASH_CONTROLLER_ENABLED flash_stm32g0x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32G4_FLASH_CONTROLLER_ENABLED flash_stm32g4x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32L4_FLASH_CONTROLLER_ENABLED flash_stm32l4x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32L5_FLASH_CONTROLLER_ENABLED flash_stm32l5x.c)
      zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_STM32WB_FLASH_CONTROLLER_ENABLED flash_stm32wbx.c)
# zephyr-keep-sorted-stop
    endif()
  endif()
endif()

if(CONFIG_SOC_FLASH_GD32)
  zephyr_library_sources(flash_gd32.c)

# zephyr-keep-sorted-start
  zephyr_library_sources_ifdef(CONFIG_GD32_NV_FLASH_V1 flash_gd32_v1.c)
  zephyr_library_sources_ifdef(CONFIG_GD32_NV_FLASH_V2 flash_gd32_v2.c)
  zephyr_library_sources_ifdef(CONFIG_GD32_NV_FLASH_V3 flash_gd32_v3.c)
# zephyr-keep-sorted-stop
endif()

zephyr_library_include_directories_ifdef(
  CONFIG_FLASH_MCUX_FLEXSPI_NOR
  ${ZEPHYR_BASE}/drivers/memc
)

zephyr_library_include_directories_ifdef(
  CONFIG_FLASH_MCUX_FLEXSPI_MX25UM51345G
  ${ZEPHYR_BASE}/drivers/memc
)

zephyr_library_include_directories_ifdef(
  CONFIG_FLASH_MCUX_FLEXSPI_HYPERFLASH
  ${ZEPHYR_BASE}/drivers/memc
)

zephyr_library_sources_ifdef(CONFIG_FLASH_NXP_S32_QSPI_NOR flash_nxp_s32_qspi_nor.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_NXP_S32_QSPI_HYPERFLASH flash_nxp_s32_qspi_hyperflash.c)
if(CONFIG_FLASH_NXP_S32_QSPI_NOR OR CONFIG_FLASH_NXP_S32_QSPI_HYPERFLASH)
  zephyr_library_sources(flash_nxp_s32_qspi.c)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/memc)
endif()

if(CONFIG_RA_FLASH_HP)
  zephyr_library_sources(flash_hp_ra.c)
  zephyr_library_sources_ifdef(CONFIG_FLASH_EX_OP_ENABLED flash_hp_ra_ex_op.c)
endif()
