# SPDX-License-Identifier: Apache-2.0

# if(CONFIG_LSM9DS1)
#   # Add hello_world_driver.h to the set of global include paths.
#   zephyr_include_directories(.)

#   zephyr_library()
#   zephyr_library_sources(
#     lsm9ds1.c
#     )
# endif()

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LSM9DS1 lsm9ds1.c)
zephyr_library_sources_ifdef(CONFIG_LSM9DS1 lsm9ds1.h)
