# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_FIDO2_UP_INPUT fido2_up_input.c)
zephyr_library_sources_ifdef(CONFIG_FIDO2_UP_ALWAYS fido2_up_always.c)

if(CONFIG_FIDO2_UP_ALWAYS)
  message(WARNING "
    Warning: CONFIG_FIDO2_UP_ALWAYS bypasses user presence. This reduces security
    and should only be used for testing or in very specific use cases."
  )
endif()
