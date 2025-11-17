cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(decompress_etc2)

target_compile_definitions(app PUBLIC __ZEPHYR__)

# Define path to vglite_window folder
set(VGLITE_WINDOW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vglite_window)

# Add sources
target_sources(app PRIVATE
    src/main.c
    ${VGLITE_WINDOW_DIR}/vglite_window.c
)

# Include directories
target_include_directories(app PRIVATE ${VGLITE_WINDOW_DIR})

# Path to gpu-vglite repo: modules/gpu-vglite
set(GPU_VGLITE_DIR ${ZEPHYR_BASE}/../modules/gpu-vglite)

# Add gpu-vglite so its CMakeLists.txt is processed
add_subdirectory(${GPU_VGLITE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/gpu-vglite)

# Link libraries
target_link_libraries(app PRIVATE vglite)

# Explicitly link kernel library after vglite
target_link_libraries(app PRIVATE ${CMAKE_BINARY_DIR}/zephyr/kernel/libkernel.a)

target_link_libraries(app PRIVATE zephyr_interface)

# Zephyr-specific include directories
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
zephyr_include_directories(${ZEPHYR_BASE}/../modules/gpu-vglite)
zephyr_include_directories(${ZEPHYR_BASE}/../modules/gpu-vglite/inc)
zephyr_include_directories(${ZEPHYR_BASE}/include)
zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr)

if(${CONFIG_SOC_PART_NUMBER_MIMXRT1176DVMAA})
  zephyr_include_directories(${ZEPHYR_BASE}/boards/nxp/mimxrt1170_evk)

elseif(${CONFIG_SOC_PART_NUMBER_MIMXRT595SFFOC})
  zephyr_include_directories(${ZEPHYR_BASE}/boards/nxp/mimxrt595_evk)

elseif(${CONFIG_SOC_PART_NUMBER_MIMXRT798SGFOB})
  zephyr_include_directories(${ZEPHYR_BASE}/boards/nxp/mimxrt700_evk)

elseif(${CONFIG_SOC_PART_NUMBER_MIMXRT1166DVM6A})
  zephyr_include_directories(${ZEPHYR_BASE}/boards/nxp/mimxrt1160_evk)

else()
  message(WARNING "Unknown SOC part number — no board include directory set.")
endif()
