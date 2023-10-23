# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(include)

zephyr_library_sources(
	sensor_mgmt.c
	dispatch.c
	sensing.c
	sensing_sensor.c
)

add_subdirectory_ifdef(CONFIG_SENSING_SENSOR_PHY_3D_SENSOR sensor/phy_3d_sensor)
