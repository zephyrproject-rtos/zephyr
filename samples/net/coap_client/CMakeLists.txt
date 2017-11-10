include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_NET_TESTING)
  target_include_directories(app PRIVATE
    $ENV{ZEPHYR_BASE}/samples/net/common
    )
  target_compile_definitions(app PRIVATE
    NET_TESTING_SERVER=1
    )
endif()
