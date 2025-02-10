# Copyright (c) 2021 Nordic Semiconductor ASA
# Copyright (c) 2024 Siemens AG
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DISK_DRIVERS)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_FLASH flashdisk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_RAM ramdisk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_LOOPBACK loopback_disk.c)

zephyr_library_sources_ifdef(CONFIG_SDMMC_STM32 sdmmc_stm32.c)
zephyr_library_sources_ifdef(CONFIG_SDMMC_SUBSYS sdmmc_subsys.c)
zephyr_library_sources_ifdef(CONFIG_MMC_SUBSYS mmc_subsys.c)

add_subdirectory_ifdef(CONFIG_NVME nvme)

set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated/ramdisk)
set(generated_file ${gen_inc_dir}/ramdisk_diskimages.inc)

zephyr_include_directories(${gen_inc_dir})

# Scan RAM disk device nodes in the DTS to identify the ones that are enabled and
# specify disk image paths.
dt_comp_path(ramdisk_devs COMPATIBLE "zephyr,ram-disk")
foreach(ramdisk_dev ${ramdisk_devs})
  dt_node_has_status(dt_status_okay PATH ${ramdisk_dev} STATUS "okay")
  dt_prop(dt_diskimage_path PATH ${ramdisk_dev} PROPERTY "zephyr,disk-image-path")
  if(dt_status_okay AND DEFINED dt_diskimage_path)
    list(APPEND ramdisk_dev_paths ${ramdisk_dev})

    dt_prop(disk_sector_size PATH ${ramdisk_dev} PROPERTY "sector-size")
    list(APPEND ramdisk_sector_sizes ${disk_sector_size})

    cmake_path(IS_ABSOLUTE dt_diskimage_path is_diskimg_path_absolute)
    if(${is_diskimg_path_absolute})
      list(APPEND ramdisk_diskimg_paths ${dt_diskimage_path})
    else()
      # Relative paths are assumed to be relative to application source directory.
      # Adjust the path accordingly.
      list(APPEND ramdisk_diskimg_paths ${APPLICATION_SOURCE_DIR}/${dt_diskimage_path})
    endif()

    # If GNU extensions are not available for use, disk image binary data needs to be converted into
    # hex characters form so that it can be included as array data.
    if(NOT CONFIG_TOOLCHAIN_SUPPORTS_GNU_EXTENSIONS)
      string(TOLOWER ${ramdisk_dev} ramdisk_dev)
      string(REPLACE "/" "_S_" _ramdisk_dev ${ramdisk_dev})
      string(REGEX REPLACE "[-,.@/+]" "_" ramdisk_dev_path_id ${_ramdisk_dev})
      string(PREPEND ramdisk_dev_path_id "N")
      list(APPEND ramdisk_diskimg_data_incs ${gen_inc_dir}/${ramdisk_dev_path_id}.inc)
    endif()
  endif()
endforeach()

# If any RAM disk devices are found in the DTS, that use disk images as backing store,
# add appropriate commands to include the disk images binary data in the build.
if (DEFINED ramdisk_diskimg_paths)
  set(generated_files ${generated_file})

  if(CONFIG_TOOLCHAIN_SUPPORTS_GNU_EXTENSIONS)
    list(APPEND generated_files ${generated_file}.S)
    zephyr_library_sources(${generated_file}.S)
  endif()

  add_custom_command(
    OUTPUT ${generated_files}
    COMMAND ${CMAKE_COMMAND}
      -DCONFIG_TOOLCHAIN_SUPPORTS_GNU_EXTENSIONS=${CONFIG_TOOLCHAIN_SUPPORTS_GNU_EXTENSIONS}
      -DGENERATED_FILE=${generated_file}
      "-DRAMDISK_DEV_PATHS=${ramdisk_dev_paths}"
      "-DRAMDISK_SECTOR_SIZES=${ramdisk_sector_sizes}"
      "-DRAMDISK_DISKIMG_PATHS=${ramdisk_diskimg_paths}"
      -P ${CMAKE_CURRENT_LIST_DIR}/gen_ramdisk_diskimages_inc.cmake
    DEPENDS ${ramdisk_diskimg_paths} ${ZEPHYR_BINARY_DIR}/include/generated/zephyr/devicetree_generated.h
    VERBATIM
  )

  add_custom_target(gen_ramdisk_diskimages_inc DEPENDS ${generated_files})

  if(NOT CONFIG_TOOLCHAIN_SUPPORTS_GNU_EXTENSIONS)
    foreach(diskimg_path diskimg_data_inc IN ZIP_LISTS ramdisk_diskimg_paths ramdisk_diskimg_data_incs)
      generate_inc_file_for_target(
        gen_ramdisk_diskimages_inc
        ${diskimg_path}
        ${diskimg_data_inc}
      )
    endforeach()
  endif()

  zephyr_library_add_dependencies(gen_ramdisk_diskimages_inc)
else()
  file(WRITE ${generated_file} "/* No RAM disk device instances found that use a disk image as backing store. */\n")
endif()
endif()
