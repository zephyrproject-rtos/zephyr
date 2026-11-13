# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

if(DEFINED CMAKE_PRESET)
  file(READ ${APP_DIR}/CMakePresets.json presets)

  if(DEFINED CMAKE_PRESET_SYSBUILD_FIELD)
    # Preset defines a specific field to use for sysbuild, let's honor that.
    string(JSON variables GET "${presets}" ${CMAKE_PRESET_SYSBUILD_FIELD})
  else()
    # Look for the field `vendor;sysbuild;cacheVariables` in the specified presets field.
    set(cache_field vendor sysbuild cacheVariables)
    string(JSON presets GET "${presets}" configurePresets)
    string(JSON length LENGTH "${presets}")
    math(EXPR end "${length} - 1")
    foreach(i RANGE ${end})
      string(JSON preset_name GET "${presets}" ${i} name)
      if(preset_name STREQUAL "${CMAKE_PRESET}")
        string(JSON variables ERROR_VARIABLE _ GET "${presets}" ${i} ${cache_field})
        break()
      endif()
    endforeach()
  endif()

  if(variables)
    string(JSON length LENGTH "${variables}")
    math(EXPR end "${length} - 1")
    foreach(i RANGE ${end})
      string(JSON var MEMBER "${variables}" ${i})
      string(JSON val GET "${variables}" ${var})
      set(${var} "${val}" CACHE INTERNAL "Sysbuild preset value")
    endforeach()
  endif()
endif()
