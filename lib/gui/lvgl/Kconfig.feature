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

if LVGL_SHADOW

config LVGL_SHADOW_CACHE_SIZE
	int "Shadow cache size"
	default 0
	help
	  Allow buffering some shadow calculation. This parameter is the maximum
	  shadow size to buffer.

endif # LVGL_SHADOW

config LVGL_BLEND_MODES
	bool "Enable other blend modes"
	help
	  Use other blend modes than normal

config LVGL_OPA_SCALE
	bool "Enable opa_scale style property"
	help
	  Use the opa_scale style property to set the opacity of an object and
	  its children at once

config LVGL_IMG_TRANSFORM
	bool "Enable image transformations"
	help
	  Use image zoom and rotation

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

config LVGL_PERF_MONITOR
	bool "Enable performance monitor"
	help
	  Show CPU usage and FPS count in the right bottom corner

config LVGL_API_EXTENSION_V6
	bool "Enable V6 API extensions"
	help
	  Use the functions and types from the older API if possible

endmenu
