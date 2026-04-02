# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LORAWAN)
  add_subdirectory_ifdef(CONFIG_LORA_MODULE_BACKEND_LORAMAC_NODE loramac-node)
  add_subdirectory(emul)
  add_subdirectory(services)
  add_subdirectory(nvm)
endif()
