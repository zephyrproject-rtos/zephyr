cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE src/main.c)

zephyr_sources_ifdef(CONFIG_SECURE_STORAGE_PS_TRANSFORM_IMPLEMENTATION_CUSTOM
  src/custom_transform.c)

zephyr_sources_ifdef(CONFIG_SECURE_STORAGE_PS_TRANSFORM_AEAD_CRYPT_CUSTOM
  src/custom_aead_crypt.c)

zephyr_sources_ifdef(CONFIG_SECURE_STORAGE_PS_STORE_IMPLEMENTATION_CUSTOM
  src/custom_store.c)

zephyr_sources_ifdef(CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_CUSTOM
  src/custom_replay_protection.c)

if(CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ZEPHYR)
  # The replay protection tests access the subsystem's internals to tamper with stored data.
  target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/secure_storage/include/internal)
endif()
