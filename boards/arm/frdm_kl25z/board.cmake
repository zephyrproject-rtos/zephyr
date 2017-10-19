set_ifndef(OPENSDA_FW daplink)

if(OPENSDA_FW STREQUAL jlink)
  set_ifndef(DEBUG_SCRIPT jlink.sh)
elseif(OPENSDA_FW STREQUAL daplink)
  set_ifndef(DEBUG_SCRIPT pyocd.sh)
  set_ifndef(FLASH_SCRIPT pyocd.sh)
endif()

set_property(GLOBAL APPEND PROPERTY FLASH_SCRIPT_ENV_VARS
  JLINK_DEVICE=MKL25Z128xxx4
  PYOCD_TARGET=kl25z
  )
