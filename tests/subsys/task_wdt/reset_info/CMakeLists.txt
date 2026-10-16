cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(subsys_task_wdt_reset_info_test)

target_sources(app PRIVATE src/main.c)
