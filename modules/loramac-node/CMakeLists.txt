if(${CONFIG_HAS_SEMTECH_RADIO_DRIVERS})

set(ZEPHYR_CURRENT_LIBRARY loramac-node)

zephyr_library_include_directories(
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/boards
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/system
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/radio
)

zephyr_library_sources(
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/boards/mcu/utilities.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/system/systime.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/system/timer.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/system/delay.c
)

zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_SX1272
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/radio/sx1272/sx1272.c
)
zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_SX1276
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/radio/sx1276/sx1276.c
)
zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_SX126X
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/radio/sx126x/sx126x.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/radio/sx126x/radio.c
)

if(${CONFIG_HAS_SEMTECH_LORAMAC})
  zephyr_library_include_directories(
    ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac
    ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region
    # required for FUOTA FragDecoder.h
    ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/apps/LoRaMac/common/LmHandler/packages
  )
endif()

zephyr_library_compile_definitions_ifdef(CONFIG_HAS_SEMTECH_SOFT_SE SOFT_SE)

zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_SOFT_SE
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/aes.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/cmac.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/soft-se.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/soft-se-hal.c
)

zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_LORAMAC
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMac.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacAdr.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacClassB.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacCommands.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacConfirmQueue.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacCrypto.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacParser.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/LoRaMacSerializer.c
)

zephyr_library_sources_ifdef(CONFIG_LORAWAN_FRAG_TRANSPORT
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/apps/LoRaMac/common/LmHandler/packages/FragDecoder.c
)

zephyr_library_sources_ifdef(CONFIG_HAS_SEMTECH_LORAMAC
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/Region.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionCommon.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_EU868
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionEU868.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_US915
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionBaseUS.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionUS915.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_CN779
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionCN779.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_EU433
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionEU433.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_AU915
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionBaseUS.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionAU915.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_AS923
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionAS923.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_CN470
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionBaseUS.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionCN470.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionCN470A20.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionCN470A26.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionCN470B20.c
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionCN470B26.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_KR920
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionKR920.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_IN865
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionIN865.c
)
zephyr_library_sources_ifdef(CONFIG_LORAMAC_REGION_RU864
  ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/mac/region/RegionRU864.c
)

endif()
