# Add MPU configuration file if MPU is enabled
if(CONFIG_ARM_MPU)
  zephyr_sources(mpu_regions.c)
endif()
