# Common routines used in net samples

if(CONFIG_BOARD_FRDM_K64F)
  if(CONFIG_IEEE802154_CC2520)
    target_include_directories(
      app
      PRIVATE
      $ENV{ZEPHYR_BASE}/drivers/
      $ENV{ZEPHYR_BASE}/include/drivers/
      )
    target_sources(
      app
      PRIVATE
      $ENV{ZEPHYR_BASE}/samples/net/common/cc2520_frdm_k64f.c
      )
  endif()
endif()

if(CONFIG_NET_TESTING)
  target_include_directories(
    app
    PRIVATE
    $ENV{ZEPHYR_BASE}/samples/net/common/
    )
  target_compile_definitions(
    app
    PRIVATE
    NET_TESTING_SERVER=1
    )
endif()
