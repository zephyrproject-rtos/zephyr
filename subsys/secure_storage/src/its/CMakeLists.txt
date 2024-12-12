# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_IMPLEMENTATION_ZEPHYR
  implementation.c
)

zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD
  transform/aead.c
  transform/aead_get.c
)
if (NOT CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NO_INSECURE_KEY_WARNING)
  if (CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_DEVICE_ID_HASH)
    message(WARNING "
      The PSA ITS encryption key provider in use generates keys by hashing the device ID
      retrieved through the HW info API. This is not necessarily secure as the device ID may be
      easily readable by an attacker, not unique, and/or guessable, depending on the device.
      This means that the data and keys stored via the PSA APIs may not be secure at rest.")
  elseif(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_ENTRY_UID_HASH)
    message(WARNING "
      The PSA ITS encryption key provider in use is not secure.
      It's only intended for functional support.
      This means that the data and keys stored via the PSA APIs will not be secure at rest.
      Use a secure key provider if possible.")
  endif()
endif()

zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS
  store/zms.c
)
zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_SETTINGS
  store/settings.c
)
if (CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_NONE)
  message(ERROR "
    The secure storage ITS module is enabled but has no implementation.
    (CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_NONE)
    ")
endif()
