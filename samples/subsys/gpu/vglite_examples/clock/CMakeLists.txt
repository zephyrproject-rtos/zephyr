cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(clock)

target_compile_definitions(app PUBLIC __ZEPHYR__)

# Define path to vglite_window folder
set(VGLITE_WINDOW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vglite_window)

# Define path to vglite_layer folder
set(VGLITE_LAYER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vglite_layer)

# Define path to qoi folder
set(QOI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../qoi)

# Add sources
target_sources(app PRIVATE
    src/main.c
    ${VGLITE_WINDOW_DIR}/vglite_window.c
    ${VGLITE_LAYER_DIR}/vglite_layer.c
)

# Include directories
target_include_directories(app PRIVATE ${VGLITE_WINDOW_DIR})
target_include_directories(app PRIVATE ${VGLITE_LAYER_DIR})
target_include_directories(app PRIVATE ${QOI_DIR})

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
zephyr_include_directories(${ZEPHYR_BASE}/include)
zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr)

# Add board directory 
if(DEFINED BOARD_DIR AND EXISTS ${BOARD_DIR})
  zephyr_include_directories(${BOARD_DIR})
  message(STATUS "Added board include directory: ${BOARD_DIR}")
endif()