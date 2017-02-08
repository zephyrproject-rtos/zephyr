target_include_directories(zephyr PUBLIC
  ${PROJECT_SOURCE_DIR}/arch/arm/soc/nordic_nrf5/include
)

if(CONFIG_SOC_NRF52832)
  target_compile_definitions(zephyr PUBLIC -DNRF52832_XXAA)
endif()

if(CONFIG_SOC_NRF52840)
  target_compile_definitions(zephyr PUBLIC -DNRF52840_XXAA)
endif()

target_sources(zephyr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/soc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mpu_regions.c
  )
