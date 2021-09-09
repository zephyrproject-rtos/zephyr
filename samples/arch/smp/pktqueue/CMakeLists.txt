# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(smp_pktqueue)

FILE(GLOB pktqueue_sources src/*.c)

target_sources(app PRIVATE ${pktqueue_sources})
