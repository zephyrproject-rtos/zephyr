# Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
# Copyright (c) 2020 Teslabs Engineering S.L.
# SPDX-License-Identifier: Apache-2.0

menu "Feature usage"

config LVGL_ANIMATION
	bool "Enable animations"
	help
	  Enable animations

config LVGL_SHADOW
	bool "Enable shadows"
	help
	  Enable shadows

config LVGL_GROUP
	bool "Enable group support"
	help
	  Enable group support.
	  Used by keyboard and button input

config LVGL_GPU
	bool "Enable GPU support"
	help
	  Enable GPU support

config LVGL_FILESYSTEM
	bool "Enable file system"
	depends on FILE_SYSTEM
	default y if FILE_SYSTEM
	help
	  Enable LittlevGL file system

endmenu
