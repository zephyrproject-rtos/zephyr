include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

zephyr_include_directories(
  .
  $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/kernel/zephyr/dpl
  $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source
  $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source/ti/drivers/net/wifi/porting
  $ENV{ZEPHYR_BASE}/ext/hal/ti/simplelink/source/ti/drivers/net/wifi/bsd
  )
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/simplelink_support.c)
