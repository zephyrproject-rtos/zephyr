include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)

generate_inc_file_for_target(
  app
  src/echo-apps-cert.der
  ${ZEPHYR_BINARY_DIR}/include/generated/echo-apps-cert.der.inc
  )
