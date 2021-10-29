# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(as5x47.c lib/as5x47/AS5047P.cpp lib/as5x47/types/AS5047P_Types.cpp lib/as5x47/spi/AS5047P_SPI_Zephyr.cpp)
zephyr_library_include_directories(lib)
