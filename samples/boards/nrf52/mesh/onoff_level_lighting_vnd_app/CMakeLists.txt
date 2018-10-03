cmake_minimum_required(VERSION 3.8.2)
set(QEMU_EXTRA_FLAGS -s)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_link_libraries(app PUBLIC subsys__bluetooth)

target_sources(app PRIVATE
	       src/main.c
	       src/mesh/ble_mesh.c
	       src/mesh/device_composition.c
	       src/mesh/publisher.c
	       src/mesh/state_binding.c
	       src/mesh/transition.c
	       )

zephyr_include_directories(
	src/
	src/mesh
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/hal
)
