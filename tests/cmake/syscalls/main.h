#pragma once

#include <zephyr/syscall.h>

// Newline between return type and function name
// clang-format off
__syscall void
test_wrapped(int foo);
// clang-format on

// Empty parameters
__syscall void test_empty();

#include <zephyr/syscalls/main.h>

