set(QEMU_EXTRA_FLAGS -s)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_link_libraries(app subsys__bluetooth)

target_sources(app PRIVATE 
	       src/main.c 
	       src/common.c
	       src/mesh/ble_mesh.c
               src/mesh/DEVICE_composition.c 
               src/mesh/publisher.c 
               src/mesh/doer.c 
	       src/pwm.c
	       src/saadc.c
	       )
target_sources_ifdef(CONFIG_BOARD_BBC_MICROBIT app PRIVATE src/microbit.c)

zephyr_include_directories(
	src/
	src/mesh
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/hal
	
)
