/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#define PXP_NODE DT_NODELABEL(pxp)
#if DT_NODE_HAS_STATUS(PXP_NODE, okay)
#define PXP_LABEL DT_LABEL(PXP_NODE)
#include "drivers/pxp.h"
#endif

#if CONFIG_PXP_ROTATE_FLIP
#define SCALER (1)
#define DEMO_NAME "Rotate & Flip"
#endif

#if CONFIG_PXP_SCALER
#define SCALER (4)
#define DEMO_NAME "Scaler"
#endif

#define WIDTH 720
#define HEIGHT 720
static uint16_t s_psBufferPxp[HEIGHT][WIDTH];
static uint16_t s_outputBuffer[HEIGHT][WIDTH];
void main(void)
{
	uint16_t outputWidth = WIDTH / SCALER, outputHeight = HEIGHT / SCALER;

	/* init the ps buffer */
	memset(s_psBufferPxp, 0x56, sizeof(s_psBufferPxp) / 2);
	memset((char *)s_psBufferPxp + HEIGHT * WIDTH, 0x89, sizeof(s_psBufferPxp) / 2);

	const struct device *dev = device_get_binding(PXP_LABEL);
	const pxp_ps_buffer_config_t psBufferConfig = {
		.pixelFormat = kPXP_PsPixelFormatRGB565,
		.swapByte = false,
		.bufferAddr = (uint32_t)s_psBufferPxp,
		.bufferAddrU = 0U,
		.bufferAddrV = 0U,
		.pitchBytes = WIDTH * 2,
	};

	pxp_set_ps_bg_color(dev, 0U);
	pxp_set_ps_buffer(dev, &psBufferConfig);
	pxp_set_as_position(dev, 0xFFFFU, 0xFFFFU, 0U, 0U);

	pxp_output_buffer_config_t outputBufferConfig;

	outputBufferConfig.pixelFormat    = kPXP_OutputPixelFormatRGB565;
	outputBufferConfig.interlacedMode = kPXP_OutputProgressive;
	outputBufferConfig.buffer0Addr    = (uint32_t)s_outputBuffer;
	outputBufferConfig.buffer1Addr    = 0U;
	outputBufferConfig.pitchBytes     = WIDTH * 2;
	outputBufferConfig.width          = WIDTH;
	outputBufferConfig.height         = HEIGHT;

	pxp_set_output_buffer(dev, &outputBufferConfig);

	pxp_enable_csc1(dev, false);

	pxp_set_ps_position(dev, 0, 0, outputWidth - 1, outputHeight - 1);

#if CONFIG_PXP_ROTATE_FLIP
	pxp_set_rotate(dev, kPXP_RotateOutputBuffer, kPXP_Rotate90, kPXP_FlipVertical);
#endif

#if CONFIG_PXP_SCALER
	pxp_set_ps_scaler(dev, WIDTH, HEIGHT, outputWidth, outputHeight);
#endif

	pxp_start(dev);

	pxp_wait_complete(dev);

	printk(DEMO_NAME " Covert Done! \r\n");
}
