# SPDX-FileCopyrightText: Copyright (c) 2026 Navimatix GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_STEPPER_CTRL_EVENT_HANDLER include)

zephyr_library_sources_ifdef(CONFIG_STEPPER_CTRL_EVENT_HANDLER stepper_ctrl_event_handler.c)
