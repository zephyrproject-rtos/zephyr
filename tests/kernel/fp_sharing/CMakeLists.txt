cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

# Some boards are significantly slower than others resulting in the test
# output being in the range of every few seconds to every few minutes. To
# compensate for this, one can control the number of iterations in the PI
# calculation through PI_NUM_ITERATIONS. Lowering this value will increase
# the speed of the test but it will come at the expense of precision.

set_ifndef(PI_NUM_ITERATIONS 700000)
zephyr_compile_definitions(PI_NUM_ITERATIONS=${PI_NUM_ITERATIONS})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
