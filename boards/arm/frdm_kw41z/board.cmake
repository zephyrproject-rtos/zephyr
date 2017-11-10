set_ifndef(OPENSDA_FW jlink)

if(OPENSDA_FW STREQUAL jlink)
  set_ifndef(DEBUG_SCRIPT jlink.sh)
elseif(OPENSDA_FW STREQUAL daplink)
  set_ifndef(DEBUG_SCRIPT pyocd.sh)
  set_ifndef(FLASH_SCRIPT pyocd.sh)
endif()

set(JLINK_DEVICE MKW41Z512xxx4)
set(PYOCD_TARGET kw41z4)

set_property(GLOBAL APPEND PROPERTY FLASH_SCRIPT_ENV_VARS
  JLINK_DEVICE
  PYOCD_TARGET
  )
