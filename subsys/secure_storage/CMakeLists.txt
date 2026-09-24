# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD AND CONFIG_TF_PSA_CRYPTO_BUILTIN)
  zephyr_library_link_libraries(tfpsacrypto)
  # Inherit tfpsacrypto's private include directories for access to psa_crypto_driver_wrappers.h.
  zephyr_library_include_directories($<TARGET_PROPERTY:tfpsacrypto,INCLUDE_DIRECTORIES>)
endif()

zephyr_library_include_directories(include/internal) # secure_storage headers
add_subdirectory(src)

# Make the subsystem's PSA Secure Storage API headers available only when it's enabled.
zephyr_include_directories(
  include
)

# Outputs, in `deps_var`, the subsystem headers included by `header`, if any.
function(get_common_header header deps_var)
  if(header STREQUAL "its.h" OR header STREQUAL "ps.h")
    set(deps "common.h" "uid.h")
  elseif((header MATCHES "^its") AND NOT (header STREQUAL "its/common.h"))
    set(deps "common.h" "its/common.h" "uid.h")
  else()
    set(deps "")
  endif()
  set(${deps_var} "${deps}" PARENT_SCOPE)
endfunction()

# Make the secure_storage headers available to the application only when it's implementing the relevant APIs.
function(make_available header)
  get_common_header(${header} deps)
  foreach(dep IN LISTS deps)
    make_available(${dep})
  endforeach()
  configure_file(include/internal/zephyr/secure_storage/${header}
                 ${CMAKE_BINARY_DIR}/zephyr/include/generated/zephyr/secure_storage/${header}
                 COPYONLY)
endfunction()

if(CONFIG_SECURE_STORAGE_ITS_IMPLEMENTATION_CUSTOM)
  make_available(its.h)
endif()

if(CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_CUSTOM)
  make_available(ps.h)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/its.cmake)
