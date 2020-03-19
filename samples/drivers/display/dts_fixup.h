/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_PROP(DT_INST(0, ilitek_ili9340), label)
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, ilitek_ili9340))
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, solomon_ssd1306fb), label)
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, solomon_ssd1306fb))
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, gooddisplay_gdeh0213b1), label)
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, gooddisplay_gdeh0213b1))
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, sitronix_st7789v), label)
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7789v))
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, fsl_imx6sx_lcdif), label)
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, fsl_imx6sx_lcdif))
#endif

#ifdef CONFIG_SDL_DISPLAY_DEV_NAME
#define DISPLAY_DEV_NAME CONFIG_SDL_DISPLAY_DEV_NAME
#endif

#ifdef CONFIG_DUMMY_DISPLAY_DEV_NAME
#define DISPLAY_DEV_NAME CONFIG_DUMMY_DISPLAY_DEV_NAME
#endif
