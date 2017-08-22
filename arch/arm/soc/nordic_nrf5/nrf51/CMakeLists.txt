target_include_directories(zephyr PUBLIC
  ${PROJECT_SOURCE_DIR}/arch/arm/soc/nordic_nrf5/include
)

if(CONFIG_SOC_SERIES_NRF51X)
  target_compile_definitions(zephyr PUBLIC
    NRF51
    NRF51822
    )
endif()

target_sources(zephyr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/soc.c
  )
