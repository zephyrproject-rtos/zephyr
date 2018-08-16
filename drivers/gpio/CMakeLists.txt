zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_GPIO_ALTERA_NIOS2}  gpio_altera_nios2.c
            IFDEF:${CONFIG_GPIO_ATMEL_SAM3}    gpio_atmel_sam3.c
            IFDEF:${CONFIG_GPIO_CC2650}        gpio_cc2650.c
            IFDEF:${CONFIG_GPIO_CC32XX}        gpio_cc32xx.c
            IFDEF:${CONFIG_GPIO_CMSDK_AHB}     gpio_cmsdk_ahb.c
            IFDEF:${CONFIG_GPIO_DW}            gpio_dw.c
            IFDEF:${CONFIG_GPIO_ESP32}         gpio_esp32.c
            IFDEF:${CONFIG_GPIO_SIFIVE}        gpio_sifive.c
            IFDEF:${CONFIG_GPIO_GECKO}         gpio_gecko.c
            IFDEF:${CONFIG_GPIO_IMX}           gpio_imx.c
            IFDEF:${CONFIG_GPIO_MCUX}          gpio_mcux.c
            IFDEF:${CONFIG_GPIO_MCUX_IGPIO}    gpio_mcux_igpio.c
            IFDEF:${CONFIG_GPIO_MCUX_LPC}      gpio_mcux_lpc.c
            IFDEF:${CONFIG_GPIO_MMIO32}        gpio_mmio32.c
            IFDEF:${CONFIG_GPIO_NRFX}          gpio_nrfx.c
            IFDEF:${CONFIG_GPIO_PCAL9535A}     gpio_pcal9535a.c
            IFDEF:${CONFIG_GPIO_PULPINO}       gpio_pulpino.c
            IFDEF:${CONFIG_GPIO_QMSI}          gpio_qmsi.c
            IFDEF:${CONFIG_GPIO_QMSI_SS}       gpio_qmsi_ss.c
            IFDEF:${CONFIG_GPIO_SCH}           gpio_sch.c
            IFDEF:${CONFIG_GPIO_STM32}         gpio_stm32.c
            IFDEF:${CONFIG_GPIO_SAM0}          gpio_sam0.c
            IFDEF:${CONFIG_GPIO_SAM}           gpio_sam.c
            IFDEF:${CONFIG_GPIO_SX1509B}       gpio_sx1509b.c
            IFDEF:${CONFIG_GPIO_INTEL_APL}     gpio_intel_apl.c
            IFDEF:${CONFIG_GPIO_SHELL}         gpio_shell.c
            IFDEF:${CONFIG_USERSPACE}          gpio_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
