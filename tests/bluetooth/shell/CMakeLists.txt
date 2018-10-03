cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_BT_CONN app PRIVATE src/hrs.c)
