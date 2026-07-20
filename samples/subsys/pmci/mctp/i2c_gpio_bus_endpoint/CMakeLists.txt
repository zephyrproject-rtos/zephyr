cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mctp_i2c_gpio_bus_endpoint)

target_sources(app PRIVATE src/main.c)
