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

if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_MODULE)

  if(NOT CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD)
    make_available(its/transform.h)
  endif()

endif() # CONFIG_SECURE_STORAGE_ITS_TRANSFORM_MODULE

if(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_CUSTOM)
  make_available(its/store.h)
endif()

if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD)

  # Make the aead_get.h header available whenever none of the Zephyr-provided
  # implementations are in use. In that case either the custom or an additional
  # option added downstream is used, and it needs that header file.

  if((NOT CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_AES_GCM
      AND NOT CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_CHACHA20_POLY1305)
  OR (NOT CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_DEVICE_ID_HASH
      AND NOT CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_ENTRY_UID_HASH)
  OR (NOT CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_PROVIDER_DEFAULT))
    make_available(its/transform/aead_get.h)
  endif()

endif() # CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD

if(CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_CUSTOM)
  make_available(its/store/settings_get.h)
endif()
