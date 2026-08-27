# SPDX-License-Identifier: Apache-2.0

add_library(mylib lib.c)

# Demonstrate that our library includes a generated header so we need the following dependency
add_dependencies(mylib nanopb_generated_headers)

# Add include directory to find the generated headers
target_include_directories(mylib PRIVATE ${CMAKE_BINARY_DIR})

# Link against zephyr
target_link_libraries(mylib zephyr)
