cmake_minimum_required(VERSION 3.28)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app C)
target_sources(app PRIVATE main.c)

