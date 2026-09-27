# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LORAWAN)
  zephyr_library_sources(common.c)
  add_subdirectory_ifdef(CONFIG_LORA_MODULE_BACKEND_LORAMAC_NODE loramac-node)
  add_subdirectory_ifdef(CONFIG_LORA_MODULE_BACKEND_NATIVE native)
  add_subdirectory(emul)
  add_subdirectory(services)
  add_subdirectory(nvm)
  if(NOT CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT)
    message(WARNING "LoRaWAN requires join delay randomization to be standards compliant")
  endif()
endif()
