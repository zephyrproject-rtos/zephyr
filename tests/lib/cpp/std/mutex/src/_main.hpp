/*
 * Copyright (c) 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

using time_point = std::chrono::time_point<std::chrono::steady_clock>;

time_point now();
void time_init();

extern bool lock_succeeded;
extern time_point t0, t1, t2, t3;
constexpr auto dt = 100ms;
