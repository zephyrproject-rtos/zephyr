# SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/stepper.h)

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_STEPPER_ADI_TMC adi_tmc)
add_subdirectory_ifdef(CONFIG_STEPPER_ALLEGRO allegro)
add_subdirectory_ifdef(CONFIG_STEPPER_TI ti)
# zephyr-keep-sorted-stop

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_STEPPER_TIMING_SOURCES stepper_timing_sources)
add_subdirectory_ifdef(CONFIG_STEP_DIR_STEPPER step_dir)
# zephyr-keep-sorted-stop

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_FAKE_STEPPER fake_stepper_drv.c)
zephyr_library_sources_ifdef(CONFIG_FAKE_STEPPER_CONTROLLER fake_stepper_controller.c)
zephyr_library_sources_ifdef(CONFIG_H_BRIDGE_STEPPER h_bridge_stepper.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_SHELL stepper_shell.c)
zephyr_library_sources_ifdef(CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL zephyr_stepper_motion_controller.c)
