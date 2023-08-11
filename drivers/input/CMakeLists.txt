# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_INPUT_CAP1203 input_cap1203.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_CST816S input_cst816s.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_FT5336 input_ft5336.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_GPIO_KEYS input_gpio_keys.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_GPIO_QDEC input_gpio_qdec.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_GT911 input_gt911.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_NPCX_KBD input_npcx_kbd.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_STMPE811 input_stmpe811.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_XPT2046 input_xpt2046.c)

if (CONFIG_INPUT_SDL_TOUCH)
  zephyr_library_sources(input_sdl_touch.c)
  if (CONFIG_NATIVE_APPLICATION)
    zephyr_library_sources(input_sdl_touch_bottom.c)
  else()
    target_sources(native_simulator INTERFACE input_sdl_touch_bottom.c)
  endif()
endif()
