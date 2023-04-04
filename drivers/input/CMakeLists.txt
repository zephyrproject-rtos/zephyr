# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_INPUT_FT5336 input_ft5336.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_GPIO_KEYS input_gpio_keys.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_SDL_TOUCH input_sdl_touch.c)
