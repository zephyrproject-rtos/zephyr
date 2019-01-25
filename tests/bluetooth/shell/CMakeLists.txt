cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(bluetooth_shell)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_BT_CONN app PRIVATE src/hrs.c)
