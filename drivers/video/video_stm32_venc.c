/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_venc

#include <errno.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>

#include "ewl.h"
#include "h264encapi.h"
#include "stm32n6xx_ll_venc.h"
#include "reg_offset_v7.h"

LOG_MODULE_REGISTER(video_stm32_venc, CONFIG_VIDEO_LOG_LEVEL);

#define ALIGNMENT_INCR 8UL

#define EWL_HEAP_ALIGNED_ALLOC(size)				\
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, ALIGNMENT_INCR, size)
#define EWL_HEAP_ALIGNED_FREE(block) shared_multi_heap_free(block)

#define EWL_USE_POLLING_SYNC

#define EWL_TIMEOUT 100UL

#define MEM_CHUNKS      32

uint32_t mem_counter = 0;

typedef struct
{
	uint32_t clientType;
	/* tables required for buffer alignment */
	uint32_t *chunks[MEM_CHUNKS];
	uint32_t *alignedChunks[MEM_CHUNKS];
	uint32_t totalChunks;

	struct k_sem complete;
	i32 status;
	i32 irq_status;
	i32 irq_cnt;
} VENC_EWL_TypeDef;

static VENC_EWL_TypeDef ewl_instance;

typedef void (*irq_config_func_t)(const struct device *dev);

struct video_stm32_venc_data {
	const struct device *dev;
	struct video_format fmt;
	struct k_fifo fifo_input;
	struct k_fifo fifo_output_in;
	struct k_fifo fifo_output_out;
	uint32_t pixelformat;
	uint32_t height;
	uint32_t width;
	uint32_t pitch;
	struct video_buffer *vbuf;
	H264EncInst encoder;
	uint32_t frame_nb;
	bool resync;
};

struct video_stm32_venc_config {
	struct stm32_pclken pclken;
	irq_config_func_t irq_config;
};

static int encoder_prepare(struct video_stm32_venc_data *data);
static int encode_frame(struct video_stm32_venc_data *data);
static int encoder_end(struct video_stm32_venc_data *data);
static int encoder_start(struct video_stm32_venc_data *data);

static inline H264EncPictureType to_h264pixfmt(uint32_t pixelformat)
{
	switch (pixelformat) {
	case VIDEO_PIX_FMT_NV12:
		return H264ENC_YUV420_SEMIPLANAR;
	case VIDEO_PIX_FMT_RGB565:
		return H264ENC_RGB565;
	default:
		CODE_UNREACHABLE;
	}
}

u32 EWLReadAsicID(void)
{
	return LL_VENC_ReadRegister(0UL);
}

EWLHwConfig_t EWLReadAsicConfig(void)
{
	EWLHwConfig_t cfg_info;
	uint32_t cfgval;

	/* read first part of the configuration stored in register 63 */
	cfgval = LL_VENC_ReadRegister(63UL);

	cfg_info.maxEncodedWidth = cfgval & ((1U << 12U) - 1U);
	cfg_info.h264Enabled = (cfgval >> 27U) & 1U;
	cfg_info.vp8Enabled = (cfgval >> 26U) & 1U;
	cfg_info.jpegEnabled = (cfgval >> 25U) & 1U;
	cfg_info.vsEnabled = (cfgval >> 24U) & 1U;
	cfg_info.rgbEnabled = (cfgval >> 28U) & 1U;
	cfg_info.searchAreaSmall = (cfgval >> 29U) & 1U;
	cfg_info.scalingEnabled = (cfgval >> 30U) & 1U;

	cfg_info.busType = (cfgval >> 20U) & 15U;
	cfg_info.synthesisLanguage = (cfgval >> 16U) & 15U;
	cfg_info.busWidth = (cfgval >> 12U) & 15U;

	/* read second part of the configuration stored in register 296 */
	cfgval = LL_VENC_ReadRegister(296UL);

	cfg_info.addr64Support = (cfgval >> 31U) & 1U;
	cfg_info.dnfSupport = (cfgval >> 30U) & 1U;
	cfg_info.rfcSupport = (cfgval >> 28U) & 3U;
	cfg_info.enhanceSupport = (cfgval >> 27U) & 1U;
	cfg_info.instantSupport = (cfgval >> 26U) & 1U;
	cfg_info.svctSupport = (cfgval >> 25U) & 1U;
	cfg_info.inAxiIdSupport = (cfgval >> 24U) & 1U;
	cfg_info.inLoopbackSupport = (cfgval >> 23U) & 1U;
	cfg_info.irqEnhanceSupport = (cfgval >> 22U) & 1U;

	LOG_DBG("maxEncodedWidth   = %d", cfg_info.maxEncodedWidth);
	LOG_DBG("h264Enabled       = %d", cfg_info.h264Enabled);
	LOG_DBG("jpegEnabled       = %d", cfg_info.jpegEnabled);
	LOG_DBG("vp8Enabled        = %d", cfg_info.vp8Enabled);
	LOG_DBG("vsEnabled         = %d", cfg_info.vsEnabled);
	LOG_DBG("rgbEnabled        = %d", cfg_info.rgbEnabled);
	LOG_DBG("searchAreaSmall   = %d", cfg_info.searchAreaSmall);
	LOG_DBG("scalingEnabled    = %d", cfg_info.scalingEnabled);
	LOG_DBG("address64bits     = %d", cfg_info.addr64Support);
	LOG_DBG("denoiseEnabled    = %d", cfg_info.dnfSupport);
	LOG_DBG("rfcEnabled        = %d", cfg_info.rfcSupport);
	LOG_DBG("instanctEnabled   = %d", cfg_info.instantSupport);
	LOG_DBG("busType           = %d", cfg_info.busType);
	LOG_DBG("synthesisLanguage = %d", cfg_info.synthesisLanguage);
	LOG_DBG("busWidth          = %d", cfg_info.busWidth * 32);

	return cfg_info;
}

const void *EWLInit(EWLInitParam_t *param)
{
	/* Check for NULL pointer */
	__ASSERT_NO_MSG(param != NULL);
	/* only H264 (0) and JPEG (1) are supported */
	__ASSERT_NO_MSG(param->clientType <= 1U);

	/* sync */
	k_sem_init(&ewl_instance.complete, 0, 1);
	k_sem_reset(&ewl_instance.complete);

	/* set client type */
	ewl_instance.clientType = param->clientType;
	ewl_instance.irq_cnt = 0;

	return (void *)&ewl_instance;
}

i32 EWLRelease(const void *inst)
{
	__ASSERT_NO_MSG(inst != NULL);

	return EWL_OK;
}

void EWLWriteReg(const void *inst, uint32_t offset, uint32_t val)
{
	/* offset is defined in the EWL API in bytes. convert to number of registers by shifting */
	LL_VENC_WriteRegister((offset >> 2), val);
}

void EWLEnableHW(const void *inst, uint32_t offset, uint32_t val)
{
	/* offset is defined in the EWL API in bytes. convert to number of registers by shifting */
	LL_VENC_WriteRegister((offset >> 2), val);
}

void EWLDisableHW(const void *inst, uint32_t offset, uint32_t val)
{
	/* offset is defined in the EWL API in bytes. convert to number of registers by shifting */
	LL_VENC_WriteRegister((offset >> 2), val);
}

uint32_t EWLReadReg(const void *inst, uint32_t offset)
{
	uint32_t ret;
	/* offset is defined in the EWL API in bytes. convert to number of registers by shifting */
	ret = LL_VENC_ReadRegister((offset >> 2));

	return ret;
}

i32 EWLMallocRefFrm(const void *instance, uint32_t size, EWLLinearMem_t *info)
{
	return EWLMallocLinear(instance, size, info);
}

void EWLFreeRefFrm(const void *instance, EWLLinearMem_t *info)
{
	EWLFreeLinear(instance, info);
}

i32 EWLMallocLinear(const void *instance, uint32_t size, EWLLinearMem_t *info)
{
	if (!instance) {
		return EWL_ERROR;
	}

	VENC_EWL_TypeDef *inst = (VENC_EWL_TypeDef *) instance;
	/* make size 8-byte aligned */
	uint32_t size_aligned = ROUND_UP(size, 8);
	info->size = size_aligned;

	/* allocate using malloc and check return */
	inst->chunks[inst->totalChunks] = (uint32_t *)EWL_HEAP_ALIGNED_ALLOC(size_aligned);
	if (!inst->chunks[inst->totalChunks]) {
		LOG_DBG("unable to allocate %8d bytes", size_aligned);
		return EWL_ERROR;
	}

	/* align given malloc buffer */
	inst->alignedChunks[inst->totalChunks] = (uint32_t *) ROUND_UP((uint32_t)inst->chunks[inst->totalChunks], 8);
	/* put the aligned pointer in the return structure */
	info->virtualAddress = inst->alignedChunks[inst->totalChunks++];

	if (!info->virtualAddress)  {
		LOG_DBG("unable to get chunk for %8d bytes", size_aligned);
		return EWL_ERROR;
	}

	/* bus address is the same as virtual address because no MMU */
	info->busAddress = (ptr_t)info->virtualAddress;

	/* check buffer alignment */
	__ASSERT_NO_MSG((info->busAddress & 0b111UL) == 0U);

	mem_counter += size;
	LOG_DBG("allocated %8d bytes --> %p / 0x%x. Total : %d",
		size_aligned, (void *)info->virtualAddress,
		info->busAddress, mem_counter);

	return EWL_OK;
}

void EWLFreeLinear(const void *instance, EWLLinearMem_t *info)
{
	VENC_EWL_TypeDef *inst = (VENC_EWL_TypeDef *) instance;

	/* find the pointer corresponding to the aligned buffer */
	for (uint32_t i = 0; i < inst->totalChunks; i++) {
		if (inst->alignedChunks[i] == info->virtualAddress) {
			EWL_HEAP_ALIGNED_FREE(inst->chunks[i]);
			break;
		}
	}
	info->virtualAddress = NULL;
	info->busAddress = 0;
	info->size = 0;
}

i32 EWLReserveHw(const void *inst)
{
	__ASSERT_NO_MSG(inst != NULL);

	return EWL_OK;
}

void EWLReleaseHw(const void *inst)
{
	__ASSERT_NO_MSG(inst != NULL);
}

void *EWLmalloc(uint32_t n)
{
	void * p = NULL;

	p = EWLcalloc(n, 1);

	mem_counter += n;
	LOG_DBG("%8d bytes --> %p, total : %d", n, p, mem_counter);

	return p;
}

void EWLfree(void *p)
{
	EWL_HEAP_ALIGNED_FREE(p);
}

void *EWLcalloc(uint32_t n, uint32_t s)
{
	void *p;

	p = EWL_HEAP_ALIGNED_ALLOC(n * s);
	if (!p) {
		LOG_ERR("alloc failed for size=%d", n * s);
		return NULL;
	}

	memset(p, 0, n * s);

	return p;
}

void *EWLmemcpy(void *d, const void *s, uint32_t n)
{
	return memcpy(d, s, (size_t)n);
}

void *EWLmemset(void *d, i32 c, uint32_t n)
{
	return memset(d, c, (size_t)n);
}

int EWLmemcmp(const void *s1, const void *s2, uint32_t n)
{
	return memcmp((const uint8_t *) s1, (const uint8_t *) s2, (size_t)n);
}

i32 EWLWaitHwRdy(const void *inst, uint32_t *slicesReady)
{
	/* Check invalid parameters */
	if (!inst) {
		return EWL_HW_WAIT_ERROR;
	}

#ifdef EWL_USE_POLLING_SYNC
	uint32_t ret = EWL_HW_WAIT_TIMEOUT;
	volatile uint32_t irq_stats;
	uint32_t prevSlicesReady = 0;
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(EWL_TIMEOUT));
	uint32_t start = sys_clock_tick_get_32();

	/* check how to clear IRQ flags for VENC */
	uint32_t clrByWrite1 = (EWLReadReg(inst, BASE_HWFuse2) & HWCFGIrqClearSupport);
	do {
		irq_stats = VENC_REG(1UL);
		/* get the number of completed slices from ASIC registers. */
		if (slicesReady)
			*slicesReady = (VENC_REG(21UL) >> 16) & 0xFFUL;

		LOG_DBG("IRQ stat = %08x", irq_stats);
		uint32_t hw_handshake_status = READ_BIT(VENC_REG(BASE_HEncInstantInput >> 2U), (1U << 29U));

		/* ignore the irq status of input line buffer in hw handshake mode */
		if ((irq_stats == ASIC_STATUS_LINE_BUFFER_DONE) && (hw_handshake_status != 0UL)) {
			VENC_REG(1U) = (1<<9);
			continue;
		}

		if ((irq_stats & ASIC_STATUS_ALL) != 0UL) {
			/* clear IRQ and slice ready status */
			uint32_t clr_stats;
			irq_stats &= (~(ASIC_STATUS_SLICE_READY | ASIC_IRQ_LINE));

			if (clrByWrite1 != 0UL)
				clr_stats = ASIC_STATUS_SLICE_READY | ASIC_IRQ_LINE;
			else
				clr_stats = irq_stats;

			EWLWriteReg(inst, 0x04, clr_stats);
			ret = EWL_OK;
			break;
		}

		if (slicesReady) {
			if (*slicesReady > prevSlicesReady) {
				ret = EWL_OK;
				break;
			}
		}

	} while (!sys_timepoint_expired(timeout));

	LOG_DBG("encoding = %d ms", k_ticks_to_ms_ceil32(sys_clock_tick_get_32() - start));

	if (slicesReady != NULL) {
		LOG_DBG("slicesReady = %d", *slicesReady);
	}

	return ret;
#else
	uint32_t start = sys_clock_tick_get_32();

	VENC_EWL_TypeDef *enc = (VENC_EWL_TypeDef *) inst;

	enc->status = EWL_OK;

	if (k_sem_take(&enc->complete, K_MSEC(EWL_TIMEOUT))) {
		uint32_t irq_status = VENC_REG(1U);

		LOG_DBG("interrupt timeout VENC status=0x%x", irq_status);
		return EWL_OK;
	}

	LOG_DBG("encoding = %ld ms", k_ticks_to_ms_ceil32(sys_clock_tick_get_32() - start));

	/* Get the number of completed slices from ASIC registers. */
	if (slicesReady) {
		*slicesReady = (VENC_REG(21UL) >> 16) & 0xFFUL;
		LOG_DBG("slicesReady = %d", *slicesReady);
	}

	return enc->status;
#endif
}

static int stm32_venc_enable_clock(const struct device *dev)
{
	const struct video_stm32_venc_config *config = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk,
			     (clock_control_subsys_t) &config->pclken) != 0) {
		return -EIO;
	}

	return 0;
}

static int video_stm32_venc_set_fmt(const struct device *dev,
				  struct video_format *fmt)
{
	struct video_stm32_venc_data *data = dev->data;
	unsigned int bpp = video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	if (bpp == 0) {
		LOG_ERR("invalid 0 bpp");
		return -EINVAL;
	}

	data->pixelformat = fmt->pixelformat;
	data->pitch = fmt->pitch;
	data->height = fmt->height;
	data->width = fmt->width;

	return 0;
}

static int video_stm32_venc_get_fmt(const struct device *dev,
				  struct video_format *fmt)
{
	struct video_stm32_venc_data *data = dev->data;

	if (fmt == NULL) {
		LOG_ERR("invalid null fmt");
		return -EINVAL;
	}

	fmt->pixelformat = data->pixelformat;
	fmt->height = data->height;
	fmt->width = data->width;
	fmt->pitch = data->pitch;

	return 0;
}

int encoder_prepare(struct video_stm32_venc_data *data)
{
	H264EncRet ret;
	H264EncConfig cfg = {0};
	H264EncPreProcessingCfg preproc_cfg = {0};
	H264EncRateCtrl ratectrl_cfg = {0};
	H264EncCodingCtrl codingctrl_cfg = {0};

	data->frame_nb = 0;

	/* set config to 1 reference frame */
	cfg.refFrameAmount = 1;
	/* 30 fps frame rate */
	cfg.frameRateDenom = 1;
	cfg.frameRateNum = 30;
	/* image resolution */
	cfg.width = data->width;
	cfg.height = data->height;
	/* stream type */
	cfg.streamType = H264ENC_BYTE_STREAM;

	/* encoding level*/
	cfg.level = H264ENC_LEVEL_4;
	cfg.svctLevel = 0;
	cfg.viewMode = H264ENC_BASE_VIEW_SINGLE_BUFFER;

	ret = H264EncInit(&cfg, &data->encoder);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncInit error=%d", ret);
		return -1;
	}

	/* set format conversion for preprocessing */
	ret = H264EncGetPreProcessing(data->encoder, &preproc_cfg);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncGetPreProcessing error=%d", ret);
		return -1;
	}
	preproc_cfg.inputType = to_h264pixfmt(data->pixelformat);
	ret = H264EncSetPreProcessing(data->encoder, &preproc_cfg);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncSetPreProcessing error=%d", ret);
		return -1;
	}

	/* setup coding ctrl */
	ret = H264EncGetCodingCtrl(data->encoder, &codingctrl_cfg);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncGetCodingCtrl error=%d", ret);
		return -1;
	}

	ret = H264EncSetCodingCtrl(data->encoder, &codingctrl_cfg);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncSetCodingCtrl error=%d", ret);
		return -1;
	}

	/* set bit rate configuration */
	ret = H264EncGetRateCtrl(data->encoder, &ratectrl_cfg);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncGetRateCtrl error=%d", ret);
		return -1;
	}

	/* Constant bitrate */
	ratectrl_cfg.pictureRc = 0;
	ratectrl_cfg.mbRc = 0;
	ratectrl_cfg.pictureSkip = 0;
	ratectrl_cfg.hrd = 0;
	ratectrl_cfg.qpHdr = 25;
	ratectrl_cfg.qpMin = ratectrl_cfg.qpHdr;
	ratectrl_cfg.qpMax = ratectrl_cfg.qpHdr;

	ret = H264EncSetRateCtrl(data->encoder, &ratectrl_cfg);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncSetRateCtrl error=%d", ret);
		return -1;
	}

	return 0;
}

static int encoder_start(struct video_stm32_venc_data *data)
{
	H264EncRet ret;
	struct video_buffer *output;
	H264EncIn encIn = {0};
	H264EncOut encOut = {0};

	output = k_fifo_get(&data->fifo_output_in, K_FOREVER);

	encIn.pOutBuf = (uint32_t *)output->buffer;
	encIn.busOutBuf = (uint32_t) encIn.pOutBuf;
	encIn.outBufSize = ROUND_UP(output->size, 8);

	/* create stream */
	ret = H264EncStrmStart(data->encoder, &encIn, &encOut);
	if (ret != H264ENC_OK) {
		LOG_ERR("H264EncStrmStart error=%d", ret);
		return -1;
	}

	output->bytesused = encOut.streamSize;
	LOG_DBG("SPS/PPS generated, size= %d", output->bytesused);

	k_fifo_put(&data->fifo_output_in, output);
	k_fifo_put(&data->fifo_output_out, output);

	data->resync = true;

	return 0;
}

static int encode_frame(struct video_stm32_venc_data *data)
{
	int ret = H264ENC_FRAME_READY;
	struct video_buffer *input;
	struct video_buffer *output;
	H264EncIn encIn = {0};
	H264EncOut encOut = {0};

	if (!data->encoder) {
		ret = encoder_prepare(data);
		if (ret) {
			return -1;
		}

		ret = encoder_start(data);
		if (ret) {
			return -1;
		}

		LOG_DBG("SPS/PPS generated and pushed");
		return 0;
	}

	input = k_fifo_get(&data->fifo_input, K_NO_WAIT);
	if (!input) {
		return 0;
	}

	output = k_fifo_get(&data->fifo_output_in, K_FOREVER);

	if (!(data->frame_nb % 30) || data->resync) {
		/* if frame is the first or resync needed: set as intra coded */
		encIn.codingType = H264ENC_INTRA_FRAME;
	} else {
		/* if there was a frame previously, set as predicted */
		encIn.timeIncrement = 1;
		encIn.codingType = H264ENC_PREDICTED_FRAME;
	}

	encIn.ipf = H264ENC_REFERENCE_AND_REFRESH;
	encIn.ltrf = H264ENC_REFERENCE;

	/* set input buffers to structures */
	encIn.busLuma = (ptr_t)input->buffer;
	encIn.busChromaU = (ptr_t)encIn.busLuma + data->width * data->height;

	encIn.pOutBuf = (uint32_t *)output->buffer;
	encIn.busOutBuf = (uint32_t)encIn.pOutBuf;
	encIn.outBufSize = ROUND_UP(output->size, 8);
	encOut.streamSize = 0;

	ret = H264EncStrmEncode(data->encoder, &encIn, &encOut, NULL, NULL, NULL);
	output->bytesused = encOut.streamSize;
	LOG_DBG("output=%p, encOut.streamSize=%d", output, encOut.streamSize);

	k_fifo_put(&data->fifo_output_in, output);
	k_fifo_put(&data->fifo_output_out, output);

	switch (ret) {
		case H264ENC_FRAME_READY:
		/* save stream */
		if (!encOut.streamSize) {
			/* Nothing encoded */
			data->resync = true;
			return -1;
		}
		output->bytesused = encOut.streamSize;
	break;
	case H264ENC_FUSE_ERROR:
		LOG_ERR("H264EncStrmEncode error=%d", ret);

		LOG_ERR("DCMIPP and VENC desync at frame %d, restart the video", data->frame_nb);
		encoder_end(data);

		ret = encoder_start(data);
		if (ret) {
			return -1;
		}

	break;
	default:
		LOG_ERR("H264EncStrmEncode error=%d", ret);
		LOG_ERR("error encoding frame %d", data->frame_nb);

		encoder_end(data);

		ret = encoder_start(data);
		if (ret) {
			return -1;
		}

		data->resync = true;

		return -1;
	break;
	}

	data->frame_nb++;

	return 0;
}


static int encoder_end(struct video_stm32_venc_data *data)
{
	H264EncIn encIn = {0};
	H264EncOut encOut = {0};

	if (data->encoder) {
		H264EncStrmEnd(data->encoder, &encIn, &encOut);
		data->encoder = NULL;
	}

	return 0;
}

static int video_stm32_venc_set_stream(const struct device *dev, bool enable,
				       enum video_buf_type type)
{
	struct video_stm32_venc_data *data = dev->data;
	ARG_UNUSED(type);

	if (!enable) {
		/* Stop VENC */
		encoder_end(data);

		return 0;
	}

	return 0;
}

static int video_stm32_venc_enqueue(const struct device *dev,
				  struct video_buffer *vbuf)
{
	struct video_stm32_venc_data *data = dev->data;
	int ret = 0;

	if (vbuf->type == VIDEO_BUF_TYPE_INPUT) {
		k_fifo_put(&data->fifo_input, vbuf);
		ret = encode_frame(data);
	} else {
		k_fifo_put(&data->fifo_output_in, vbuf);
	}

	return ret;
}

static int video_stm32_venc_dequeue(const struct device *dev,
				  struct video_buffer **vbuf,
				  k_timeout_t timeout)
{
	struct video_stm32_venc_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_output_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

#ifdef EWL_USE_POLLING_SYNC
ISR_DIRECT_DECLARE(stm32_venc_isr)
{
	VENC_EWL_TypeDef *enc = &ewl_instance;
	uint32_t hw_handshake_status = READ_BIT(VENC_REG(BASE_HEncInstantInput >> 2U), (1U << 29U));
	uint32_t irq_status = VENC_REG(1U);

	enc->irq_status = irq_status;
	enc->irq_cnt++;

	if (!hw_handshake_status && (irq_status & ASIC_STATUS_FUSE)) {
		VENC_REG(1U) = ASIC_STATUS_FUSE | ASIC_IRQ_LINE;
		/* read back the IRQ status to update its value */
		irq_status = VENC_REG(1U);
	}

	if (irq_status != 0U) {
		/* status flag is raised,
		 * clear the ones that the IRQ needs to clear
		 * and signal to EWLWaitHwRdy */
		VENC_REG(1U) = ASIC_STATUS_SLICE_READY | ASIC_IRQ_LINE;
	}

	k_sem_give(&enc->complete);

	return 0;
}
#endif

static int video_stm32_venc_get_caps(const struct device *dev,
				   struct video_caps *caps)
{
	int ret = -ENODEV;

	/* VENC produces full frames */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	ret = 0;

	return ret;
}

static DEVICE_API(video, video_stm32_venc_driver_api) = {
	.set_format = video_stm32_venc_set_fmt,
	.get_format = video_stm32_venc_get_fmt,
	.set_stream = video_stm32_venc_set_stream,
	.enqueue = video_stm32_venc_enqueue,
	.dequeue = video_stm32_venc_dequeue,
	.get_caps = video_stm32_venc_get_caps,
};

#ifdef EWL_USE_POLLING_SYNC
static void video_stm32_venc_irq_config_func(const struct device *dev)
{
	IRQ_DIRECT_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		stm32_venc_isr, 0);
	irq_enable(DT_INST_IRQN(0));
}
#endif

static struct video_stm32_venc_data video_stm32_venc_data_0 = {
};

static const struct video_stm32_venc_config video_stm32_venc_config_0 = {
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	},
#ifdef EWL_USE_POLLING_SYNC
	.irq_config = video_stm32_venc_irq_config_func,
#endif
};

static void RISAF_Config(void)
{
	/* Define and initialize the master configuration structure */
	RIMC_MasterConfig_t RIMC_master = {0};

	/* Enable the clock for the RIFSC (RIF Security Controller) */
	__HAL_RCC_RIFSC_CLK_ENABLE();

	RIMC_master.MasterCID = RIF_CID_1;
	RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;

	/* Configure the master attributes for the Ethernet peripheral (VENC) */
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_VENC, &RIMC_master);

	/* Set the secure and privileged attributes for the Ethernet peripheral (VENC) as a slave */
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_VENC,
					      RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

static int video_stm32_venc_init(const struct device *dev)
{
	const struct video_stm32_venc_config *config = dev->config;
	struct video_stm32_venc_data *data = dev->data;
	int err;

	/* Enable VENC clock */
	err = stm32_venc_enable_clock(dev);
	if (err < 0) {
		LOG_ERR("Clock enabling failed.");
		return -EIO;
	}

	data->dev = dev;
	k_fifo_init(&data->fifo_input);
	k_fifo_init(&data->fifo_output_in);
	k_fifo_init(&data->fifo_output_out);

	/* Run IRQ init */
	config->irq_config(dev);

	RISAF_Config();

	LOG_DBG("CPU frequency    : %d", HAL_RCC_GetCpuClockFreq() / 1000000);
	LOG_DBG("sysclk frequency : %d", HAL_RCC_GetSysClockFreq() / 1000000);
	LOG_DBG("pclk5 frequency  : %d", HAL_RCC_GetPCLK5Freq() / 1000000);

	/* default */
	data->width = 320;
	data->height = 240;
	data->pixelformat = VIDEO_PIX_FMT_NV12;
	data->pitch = data->width;

	LOG_DBG("%s inited", dev->name);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, &video_stm32_venc_init,
		    NULL, &video_stm32_venc_data_0,
		    &video_stm32_venc_config_0,
		    POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,
		    &video_stm32_venc_driver_api);
