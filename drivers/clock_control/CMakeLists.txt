zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_CLOCK_CONTROL_BEETLE}   beetle_clock_control.c
            IFDEF:${CONFIG_CLOCK_CONTROL_MCUX_CCM} clock_control_mcux_ccm.c
            IFDEF:${CONFIG_CLOCK_CONTROL_MCUX_SIM} clock_control_mcux_sim.c
            IFDEF:${CONFIG_CLOCK_CONTROL_NRF5}     nrf5_power_clock.c
            IFDEF:${CONFIG_CLOCK_CONTROL_QUARK_SE} quark_se_clock_control.c
)

if(CONFIG_CLOCK_CONTROL_STM32_CUBE)
  zephyr_list(SOURCES APPEND
              OUTPUT PRIVATE_SOURCES
              stm32_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F0X} stm32f0x_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F1X} stm32f1x_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F2X} stm32f2x_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F3X} stm32f3x_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F4X} stm32f4x_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F7X} stm32f7x_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32L0X} stm32l0x_ll_clock.c
              IFDEF:${CONFIG_SOC_SERIES_STM32L4X} stm32l4x_ll_clock.c
  )
endif()

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
