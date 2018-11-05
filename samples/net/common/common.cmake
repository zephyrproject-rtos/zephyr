# Common routines used in net samples

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
