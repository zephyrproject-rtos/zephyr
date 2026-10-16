# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

add_library(tfm_app_rot_partition_st_secure_i2c STATIC)

target_include_directories(tfm_app_rot_partition_st_secure_i2c
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_BINARY_DIR}/st_secure_i2c_partition
)
target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/st_secure_i2c_partition
)

target_sources(tfm_app_rot_partition_st_secure_i2c
    PRIVATE
        st_secure_i2c_partition.c
)

# The generated sources
target_sources(tfm_app_rot_partition_st_secure_i2c
    PRIVATE
        ${CMAKE_BINARY_DIR}/st_secure_i2c_partition/auto_generated/intermedia_tfm_st_secure_i2c_partition.c
)
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/st_secure_i2c_partition/auto_generated/load_info_tfm_st_secure_i2c_partition.c
)

target_link_libraries(tfm_app_rot_partition_st_secure_i2c
    PRIVATE
        platform_s
        tfm_sprt
)

target_link_libraries(tfm_spm
    PRIVATE
        tfm_app_rot_partition_st_secure_i2c
)


target_sources(tfm_s PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/tfm_gtzc_config.c
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_st_secure_i2c
)
