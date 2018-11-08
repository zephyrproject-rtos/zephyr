cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(quark_se_c1000_power_mgr)

target_sources(app PRIVATE src/main.c)
