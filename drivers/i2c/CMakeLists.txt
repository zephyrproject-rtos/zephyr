# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/i2c.h)

zephyr_library()

zephyr_library_sources(i2c_common.c)

zephyr_library_sources_ifdef(CONFIG_I2C_RTIO
	i2c_rtio.c
	i2c_rtio_default.c
)
zephyr_library_sources_ifdef(CONFIG_I2C_SHELL		i2c_shell.c)
zephyr_library_sources_ifdef(CONFIG_I2C_BITBANG		i2c_bitbang.c)
zephyr_library_sources_ifdef(CONFIG_I2C_TELINK_B91		i2c_b91.c)
zephyr_library_sources_ifdef(CONFIG_I2C_IPROC i2c_bcm_iproc.c)
zephyr_library_sources_ifdef(CONFIG_I2C_CC13XX_CC26XX		i2c_cc13xx_cc26xx.c)
zephyr_library_sources_ifdef(CONFIG_I2C_CC32XX		i2c_cc32xx.c)
zephyr_library_sources_ifdef(CONFIG_I2C_ESP32		i2c_esp32.c)
zephyr_library_sources_ifdef(CONFIG_I2C_GPIO		i2c_gpio.c)
zephyr_library_sources_ifdef(CONFIG_I2C_ITE_IT8XXX2		i2c_ite_it8xxx2.c)
zephyr_library_sources_ifdef(CONFIG_I2C_ITE_ENHANCE		i2c_ite_enhance.c)
zephyr_library_sources_ifdef(CONFIG_I2C_IMX		i2c_imx.c)
zephyr_library_sources_ifdef(CONFIG_I2C_LPC11U6X        i2c_lpc11u6x.c)
zephyr_library_sources_ifdef(CONFIG_I2C_XEC		i2c_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_I2C_MCUX		i2c_mcux.c)
zephyr_library_sources_ifdef(CONFIG_I2C_MCUX_FLEXCOMM	i2c_mcux_flexcomm.c)

if(CONFIG_I2C_RTIO)
  zephyr_library_sources_ifdef(CONFIG_I2C_MCUX_LPI2C	i2c_mcux_lpi2c_rtio.c)
else()
  zephyr_library_sources_ifdef(CONFIG_I2C_MCUX_LPI2C	i2c_mcux_lpi2c.c)
endif()

zephyr_library_sources_ifdef(CONFIG_I2C_EMUL		i2c_emul.c)

if(CONFIG_I2C_RTIO)
  zephyr_library_sources_ifdef(CONFIG_I2C_NRFX_TWI
	i2c_nrfx_twi_rtio.c
	i2c_nrfx_twi_common.c
	)
else()
  zephyr_library_sources_ifdef(CONFIG_I2C_NRFX_TWI
	i2c_nrfx_twi.c
	i2c_nrfx_twi_common.c
	)
endif()

zephyr_library_sources_ifdef(CONFIG_I2C_NRFX_TWIM	i2c_nrfx_twim.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SAM_TWI		i2c_sam_twi.c)

if(CONFIG_I2C_RTIO)
  zephyr_library_sources_ifdef(CONFIG_I2C_SAM_TWIHS    i2c_sam_twihs_rtio.c)
else()
  zephyr_library_sources_ifdef(CONFIG_I2C_SAM_TWIHS    i2c_sam_twihs.c)
endif()

zephyr_library_sources_ifdef(CONFIG_I2C_SAM_TWIM	i2c_sam4l_twim.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SBCON		i2c_sbcon.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SIFIVE		i2c_sifive.c)
zephyr_library_sources_ifdef(CONFIG_I2C_NIOS2		i2c_nios2.c)
zephyr_library_sources_ifdef(CONFIG_I2C_GECKO		i2c_gecko.c)
zephyr_library_sources_ifdef(CONFIG_I2C_RV32M1_LPI2C	i2c_rv32m1_lpi2c.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SAM0		i2c_sam0.c)
zephyr_library_sources_ifdef(CONFIG_I2C_LITEX		i2c_litex.c)
zephyr_library_sources_ifdef(CONFIG_I2C_NPCX		i2c_npcx_controller.c)
zephyr_library_sources_ifdef(CONFIG_I2C_NPCX		i2c_npcx_port.c)
zephyr_library_sources_ifdef(CONFIG_I2C_DW		i2c_dw.c)
zephyr_library_sources_ifdef(CONFIG_I2C_RCAR		i2c_rcar.c)
zephyr_library_sources_ifdef(CONFIG_I2C_TCA954X		i2c_tca954x.c)
zephyr_library_sources_ifdef(CONFIG_I2C_XEC_V2		i2c_mchp_xec_v2.c)
zephyr_library_sources_ifdef(CONFIG_I2C_GD32		i2c_gd32.c)
zephyr_library_sources_ifdef(CONFIG_I2C_INFINEON_CAT1	i2c_ifx_cat1.c)
zephyr_library_sources_ifdef(CONFIG_I2C_INFINEON_XMC4	i2c_ifx_xmc4.c)
zephyr_library_sources_ifdef(CONFIG_I2C_ANDES_ATCIIC100		i2c_andes_atciic100.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SC18IM704	i2c_sc18im704.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SMARTBOND	i2c_smartbond.c)
zephyr_library_sources_ifdef(CONFIG_I2C_XILINX_AXI	i2c_xilinx_axi.c)
zephyr_library_sources_ifdef(CONFIG_I2C_MCHP_MSS		i2c_mchp_mss.c)
zephyr_library_sources_ifdef(CONFIG_I2C_SEDI		i2c_sedi.c)
zephyr_library_sources_ifdef(CONFIG_I2C_AMBIQ		i2c_ambiq.c)
zephyr_library_sources_ifdef(CONFIG_I2C_ENE_KB1200	i2c_ene_kb1200.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_I2C_SWITCH	gpio_i2c_switch.c)
zephyr_library_sources_ifdef(CONFIG_I2C_NUMAKER		i2c_numaker.c)
zephyr_library_sources_ifdef(CONFIG_I2C_MAX32		i2c_max32.c)

zephyr_library_sources_ifdef(CONFIG_I2C_STM32_V1
	i2c_ll_stm32_v1.c
	i2c_ll_stm32.c
	)
zephyr_library_sources_ifdef(CONFIG_I2C_STM32_V2
	i2c_ll_stm32_v2.c
	i2c_ll_stm32.c
	)

zephyr_library_sources_ifdef(CONFIG_RENESAS_RA_I2C_IIC i2c_renesas_ra_iic.c)

zephyr_library_sources_ifdef(CONFIG_I2C_TEST		i2c_test.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE		i2c_handlers.c)

add_subdirectory_ifdef(CONFIG_I2C_TARGET target)
