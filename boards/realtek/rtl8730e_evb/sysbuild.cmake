# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

# merge_bin.py handle_amebasmart folds the optional KM4/KM0 image2 outputs into the
# CA32 app.bin flashed next to the vendor boot.bin. So the KM4/KM0 images must build first.

# Resolve an app source dir that may be relative to APP_DIR.
function(rtl8730e_resolve_app_dir out_var src)
  if(IS_ABSOLUTE "${src}")
    set(${out_var} "${src}" PARENT_SCOPE)
  else()
    get_filename_component(_abs "${APP_DIR}/${src}" ABSOLUTE)
    set(${out_var} "${_abs}" PARENT_SCOPE)
  endif()
endfunction()

# Optional KM4 image
if(SB_CONFIG_RTL8730E_BUILD_KM4_APP AND NOT TARGET rtl8730e_km4)
  rtl8730e_resolve_app_dir(_km4_src "${SB_CONFIG_RTL8730E_KM4_APP_SOURCE_DIR}")
  ExternalZephyrProject_Add(
    APPLICATION rtl8730e_km4
    SOURCE_DIR  ${_km4_src}
    BOARD       rtl8730e_evb/rtl8730e/km4
  )
endif()

# Optional KM0 image
if(SB_CONFIG_RTL8730E_BUILD_KM0_APP AND NOT TARGET rtl8730e_km0)
  rtl8730e_resolve_app_dir(_km0_src "${SB_CONFIG_RTL8730E_KM0_APP_SOURCE_DIR}")
  ExternalZephyrProject_Add(
    APPLICATION rtl8730e_km0
    SOURCE_DIR  ${_km0_src}
    BOARD       rtl8730e_evb/rtl8730e/km0
  )
endif()

# KM4/KM0 images are not flashed on their own. They are configured and built
# before the CA32 image that merges them.
if(NOT SB_CONFIG_BOOTLOADER_MCUBOOT AND DEFINED DEFAULT_IMAGE)
  foreach(core_img rtl8730e_km4 rtl8730e_km0)
    if(TARGET ${core_img})
      set_target_properties(${core_img} PROPERTIES BUILD_ONLY True)
      add_dependencies(${DEFAULT_IMAGE} ${core_img})
      sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${core_img})
    endif()
  endforeach()
endif()
