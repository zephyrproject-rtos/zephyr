# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(QEMU_EXTRA_FLAGS -s)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(onoff_level_lighting_vnd_app)

target_sources(app PRIVATE
	       src/main.c
	       src/app_gpio.c
	       src/storage.c
	       src/mesh/ble_mesh.c
	       src/mesh/device_composition.c
	       src/mesh/no_transition_work_handler.c
	       src/mesh/publisher.c
	       src/mesh/state_binding.c
	       src/mesh/transition.c
	       )

target_sources_ifdef(CONFIG_MCUMGR app PRIVATE src/smp_svr.c)

zephyr_include_directories(
	src/
	src/mesh
)
