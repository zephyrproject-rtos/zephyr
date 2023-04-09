cmake_minimum_required(VERSION 3.15.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pio_shell_uart)

target_sources(app PRIVATE src/main.c)
