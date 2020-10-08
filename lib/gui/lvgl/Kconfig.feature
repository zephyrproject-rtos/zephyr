# Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
# Copyright (c) 2020 Teslabs Engineering S.L.
# SPDX-License-Identifier: Apache-2.0

menu "Feature usage"

config LVGL_USE_ANIMATION
	bool "Enable animations"
	help
	  Enable animations

config LVGL_USE_SHADOW
	bool "Enable shadows"
	help
	  Enable shadows

config LVGL_SHADOW_CACHE_SIZE
	int "Shadow cache size"
	depends on LVGL_USE_SHADOW
	default 0
	help
	  Allow buffering some shadow calculation. This parameter is the maximum
	  shadow size to buffer.

config LVGL_USE_OUTLINE
	bool "Enable outline drawing on rectangles"
	help
	  Enable outline drawing on rectangles

config LVGL_USE_PATTERN
	bool "Enable pattern drawing on rectangles"
	help
	  Enable pattern drawing on rectangles

config LVGL_USE_VALUE_STR
	bool "Enable value string drawing on rectangles"
	help
	  Enable value string drawing on rectangles

config LVGL_USE_BLEND_MODES
	bool "Enable other blend modes"
	help
	  Use other blend modes than normal

config LVGL_USE_OPA_SCALE
	bool "Enable opa_scale style property"
	help
	  Use the opa_scale style property to set the opacity of an object and
	  its children at once

config LVGL_USE_IMG_TRANSFORM
	bool "Enable image transformations"
	help
	  Use image zoom and rotation

config LVGL_USE_GROUP
	bool "Enable group support"
	help
	  Enable group support.
	  Used by keyboard and button input

config LVGL_USE_GPU
	bool "Enable GPU support"
	help
	  Enable GPU support

config LVGL_USE_FILESYSTEM
	bool "Enable file system"
	depends on FILE_SYSTEM
	default y if FILE_SYSTEM
	help
	  Enable LittlevGL file system

config LVGL_USE_PERF_MONITOR
	bool "Enable performance monitor"
	help
	  Show CPU usage and FPS count in the right bottom corner

config LVGL_USE_API_EXTENSION_V6
	bool "Enable V6 API extensions"
	help
	  Use the functions and types from the older API if possible

config LVGL_USE_API_EXTENSION_V7
	bool "Enable V7 API extensions"
	help
	  Use the functions and types from the older API if possible

endmenu
