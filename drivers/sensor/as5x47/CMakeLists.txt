# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(as5x47.c lib/AS5047P/src/AS5047P.cpp lib/AS5047P/src/spi/AS5047P_SPI_Zephyr.cpp lib/AS5047P/src/types/AS5047P_Types.cpp lib/as5047p_c_interface.cpp)
zephyr_library_include_directories(lib)
zephyr_library_compile_definitions(-DAS5047P_OP_MODE_Zephyr)