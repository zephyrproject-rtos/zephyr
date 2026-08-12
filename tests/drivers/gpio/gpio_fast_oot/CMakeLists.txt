# SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpio_fast_oot)

# Provide the OOT backend header (zephyr_gpio_fast_impl_oot.h)
# on the include path. CONFIG_GPIO_FAST_OUT_OF_TREE_BACKEND=y in
# prj.conf enables the include in gpio_fast.h.
zephyr_include_directories(oot_stub)

target_sources(app PRIVATE src/main.c)
