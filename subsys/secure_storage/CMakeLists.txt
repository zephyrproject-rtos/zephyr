# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_library_include_directories(include/internal) # secure_storage headers
add_subdirectory(src)

# Make the subsystem's PSA Secure Storage API headers available only when it's enabled.
zephyr_include_directories(
  include
)

# Make the secure_storage headers available to the application only when it's implementing the relevant APIs.
function(make_available header)
  if(NOT header STREQUAL "common.h")
    make_available(common.h)
  endif()
  if((header MATCHES "^its") AND NOT (header STREQUAL "its/common.h"))
    make_available(its/common.h)
  endif()
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

if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_CUSTOM
    OR (CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_CUSTOM
        AND CONFIG_SECURE_STORAGE_ITS_TRANSFORM_MODULE))
  make_available(its/transform.h)
endif()

if(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_CUSTOM)
  make_available(its/store.h)
endif()

if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_CUSTOM
 OR CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_CUSTOM
 OR CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_PROVIDER_CUSTOM)
  make_available(its/transform/aead_get.h)
endif()

if(CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_CUSTOM)
  make_available(its/store/settings_get.h)
endif()
