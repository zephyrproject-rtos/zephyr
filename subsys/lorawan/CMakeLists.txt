# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LORAWAN)
  add_subdirectory_ifdef(CONFIG_LORA_MODULE_BACKEND_LORAMAC_NODE loramac-node)
  add_subdirectory_ifdef(CONFIG_LORA_MODULE_BACKEND_LORA_BASICS_MODEM lora-basics-modem)
  add_subdirectory(emul)
  add_subdirectory(services)
  add_subdirectory(nvm)
endif()
