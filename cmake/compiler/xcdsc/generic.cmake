set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})
# Find and validate the xc-dsc-gcc compiler binary
find_program(CMAKE_C_COMPILER xc-dsc-gcc PATHS ${XCDSC_TOOLCHAIN_PATH}/bin/ NO_DEFAULT_PATH REQUIRED )
set(CMAKE_C_FLAGS -D__XC_DSC__)
# Get compiler version
execute_process(
	COMMAND ${CMAKE_C_COMPILER} --version
	OUTPUT_VARIABLE XCDSC_VERSION_STR
	ERROR_VARIABLE XCDSC_VERSION_ERR
	OUTPUT_STRIP_TRAILING_WHITESPACE )
# Verify that the installed version is v3.30 or higher
if("${XCDSC_VERSION_STR}" MATCHES ".*v([0-9]+)\\.([0-9]+).*")
	string(REGEX REPLACE ".*v([0-9]+)\\.([0-9]+).*" "\\1\\2" __XCDSC_VERSION__ "${XCDSC_VERSION_STR}")
	math(EXPR XCDSC_VERSION_INT "${__XCDSC_VERSION__}")
	if(XCDSC_VERSION_INT LESS 330)
	message(FATAL_ERROR "XC-DSC compiler v3.30 or newer is required. Found version: ${XCDSC_VERSION_STR}")
	endif()
else()
	message(FATAL_ERROR "Unable to detect XC-DSC compiler version from: '${XCDSC_VERSION_STR}'")
endif()
