# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

add_library(tfm_app_rot_partition_se STATIC)

target_include_directories(tfm_app_rot_partition_se
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_BINARY_DIR}/se_over_i2c
)

# Add partition integration source generate from partition YAML file
target_sources(tfm_app_rot_partition_se
    PRIVATE
        ${CMAKE_BINARY_DIR}/se_over_i2c/auto_generated/intermedia_tfm_se_over_i2c.c
)

target_link_libraries(tfm_app_rot_partition_se
    PRIVATE
        platform_s
        tfm_sprt
)

target_sources(tfm_app_rot_partition_se
    PRIVATE
        se_over_i2c.c
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/se_over_i2c
)

# Add partition integration source generate from partition YAML file
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/se_over_i2c/auto_generated/load_info_tfm_se_over_i2c.c
)

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_se
)


target_link_libraries(tfm_spm
    PRIVATE
        tfm_app_rot_partition_se
)
