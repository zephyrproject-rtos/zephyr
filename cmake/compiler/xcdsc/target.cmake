# Set default C compiler if not already set
find_program(CMAKE_C_COMPILER xc-dsc-gcc PATHS ${XCDSC_TOOLCHAIN_PATH}/bin/ NO_DEFAULT_PATH )
# XC-DSC toolchain does not support C++, so set CXX to fallback to gcc (or a dummy) # This prevents CMake errors if C++ is requested (even if unused)
find_program(CMAKE_CXX_COMPILER xc-dsc-gcc PATHS ${XCDSC_TOOLCHAIN_PATH}/bin/ NO_DEFAULT_PATH )
# Set assembler explicitly
find_program(CMAKE_ASM_COMPILER xc-dsc-gcc PATHS ${XCDSC_TOOLCHAIN_PATH}/bin/ NO_DEFAULT_PATH )
# Target CPU (must match user platform)
list(APPEND TOOLCHAIN_C_FLAGS "-mcpu=${TARGET_CPU}")
# Picolibc and standard includes(if supported in toolchain)
list(APPEND TOOLCHAIN_C_FLAGS
	-I${XCDSC_TOOLCHAIN_PATH}/include/picolibc
	-isystem${XCDSC_TOOLCHAIN_PATH}/include/picolibc/gcc
	-isystem${XCDSC_TOOLCHAIN_PATH}/include/picolibc/bits
	-isystem${XCDSC_TOOLCHAIN_PATH}/include
	-isystem${ZEPHYR_BASE}/include/zephyr)
