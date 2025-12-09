/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_venc

#include <errno.h>
#include <stdlib.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

#include <ewl.h>
#include <h264encapi.h>
#include <reg_offset_v7.h>

LOG_MODULE_REGISTER(stm32_venc, CONFIG_VIDEO_LOG_LEVEL);

#define VENC_DEFAULT_WIDTH               320
#define VENC_DEFAULT_HEIGHT              240
#define VENC_DEFAULT_IN_FMT              VIDEO_PIX_FMT_NV12
#define VENC_DEFAULT_OUT_FMT             VIDEO_PIX_FMT_H264
#define VENC_DEFAULT_FRAMERATE           30
#define VENC_DEFAULT_LEVEL               H264ENC_LEVEL_4
#define VENC_DEFAULT_QP                  25
#define VENC_ESTIMATED_COMPRESSION_RATIO 10

#define ALIGNMENT_INCR 8UL

#define EWL_HEAP_ALIGNED_ALLOC(size)\
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, ALIGNMENT_INCR, (size))
#define EWL_HEAP_ALIGNED_FREE(block) shared_multi_heap_free(block)

#define EWL_TIMEOUT 100UL

#define MEM_CHUNKS 32

#define NUM_SLICES_READY_MASK GENMASK(23, 16)
#define LOW_LATENCY_HW_ITF_EN 29

typedef void (*irq_config_func_t)(const struct device *dev);

struct stm32_venc_config {
	mm_reg_t reg;
	const struct stm32_pclken pclken;
	const struct reset_dt_spec reset;
	irq_config_func_t irq_config;
};

struct stm32_venc_ewl {
	uint32_t client_type;
	uint32_t *chunks[MEM_CHUNKS];
	uint32_t *aligned_chunks[MEM_CHUNKS];
	uint32_t total_chunks;
	const struct stm32_venc_config *config;
	struct k_sem complete;
	uint32_t irq_status;
	uint32_t irq_cnt;
	uint32_t mem_cnt;
};

static struct stm32_venc_ewl ewl_instance;

struct stm32_venc_data {
	const struct device *dev;
	struct k_mutex lock;
	struct video_format in_fmt;
	struct video_format out_fmt;
	struct k_fifo in_fifo_in;
	struct k_fifo in_fifo_out;
	struct k_fifo out_fifo_in;
	struct k_fifo out_fifo_out;
	struct video_buffer *vbuf;
	H264EncInst encoder;
	uint32_t frame_nb;
	bool resync;
};

static H264EncPictureType to_h264pixfmt(uint32_t pixelformat)
{
	switch (pixelformat) {
	case VIDEO_PIX_FMT_NV12:
		return H264ENC_YUV420_SEMIPLANAR;
	case VIDEO_PIX_FMT_RGB565:
		return H264ENC_RGB565;
	default:
		__ASSERT_NO_MSG(false);
		return H264ENC_YUV420_SEMIPLANAR;
	}
}

u32 EWLReadAsicID(void)
{
	const struct stm32_venc_config *config = ewl_instance.config;

	return sys_read32(config->reg + BASE_HEncASIC);
}

EWLHwConfig_t EWLReadAsicConfig(void)
{
	const struct stm32_venc_config *config = ewl_instance.config;
	EWLHwConfig_t cfg_info;
	uint32_t cfgval, cfgval2;

	cfgval = sys_read32(config->reg + BASE_HEncSynth);
	cfgval2 = sys_read32(config->reg + BASE_HEncSynth1);

	cfg_info = EWLBuildHwConfig(cfgval, cfgval2);

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
	__ASSERT_NO_MSG(param != NULL);
	__ASSERT_NO_MSG(param->clientType == EWL_CLIENT_TYPE_H264_ENC);

	/* sync */
	k_sem_init(&ewl_instance.complete, 0, 1);

	/* set client type */
	ewl_instance.client_type = param->clientType;
	ewl_instance.irq_cnt = 0;

	return (void *)&ewl_instance;
}

i32 EWLRelease(const void *inst)
{
	ARG_UNUSED(inst);

	return EWL_OK;
}

void EWLWriteReg(const void *instance, uint32_t offset, uint32_t val)
{
	struct stm32_venc_ewl *inst = (struct stm32_venc_ewl *)instance;
	const struct stm32_venc_config *config = inst->config;

	sys_write32(val, config->reg + offset);
}

void EWLEnableHW(const void *instance, uint32_t offset, uint32_t val)
{
	struct stm32_venc_ewl *inst = (struct stm32_venc_ewl *)instance;
	const struct stm32_venc_config *config = inst->config;

	sys_write32(val, config->reg + offset);
}

void EWLDisableHW(const void *instance, uint32_t offset, uint32_t val)
{
	struct stm32_venc_ewl *inst = (struct stm32_venc_ewl *)instance;
	const struct stm32_venc_config *config = inst->config;

	sys_write32(val, config->reg + offset);
}

uint32_t EWLReadReg(const void *instance, uint32_t offset)
{
	struct stm32_venc_ewl *inst = (struct stm32_venc_ewl *)instance;
	const struct stm32_venc_config *config = inst->config;

	return sys_read32(config->reg + offset);
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
	struct stm32_venc_ewl *inst = (struct stm32_venc_ewl *)instance;

	__ASSERT_NO_MSG(inst != NULL);
	__ASSERT_NO_MSG(info != NULL);

	/* align size */
	uint32_t size_aligned = ROUND_UP(size, ALIGNMENT_INCR);

	info->size = size_aligned;

	/* allocate */
	inst->chunks[inst->total_chunks] = (uint32_t *)EWL_HEAP_ALIGNED_ALLOC(size_aligned);
	if (inst->chunks[inst->total_chunks] == NULL) {
		LOG_DBG("unable to allocate %8d bytes", size_aligned);
		return EWL_ERROR;
	}

	/* align given allocated buffer */
	inst->aligned_chunks[inst->total_chunks] =
		(uint32_t *)ROUND_UP((uint32_t)inst->chunks[inst->total_chunks], ALIGNMENT_INCR);
	/* put the aligned pointer in the return structure */
	info->virtualAddress = inst->aligned_chunks[inst->total_chunks];
	if (info->virtualAddress == NULL) {
		LOG_DBG("unable to get chunk for %8d bytes", size_aligned);
		EWL_HEAP_ALIGNED_FREE(inst->chunks[inst->total_chunks]);
		return EWL_ERROR;
	}
	inst->total_chunks++;

	/* bus address is the same as virtual address because no MMU */
	info->busAddress = (ptr_t)info->virtualAddress;

	inst->mem_cnt += size;
	LOG_DBG("allocated %8d bytes --> %p / 0x%x. Total : %d", size_aligned,
		(void *)info->virtualAddress, info->busAddress, inst->mem_cnt);

	return EWL_OK;
}

void EWLFreeLinear(const void *instance, EWLLinearMem_t *info)
{
	struct stm32_venc_ewl *inst = (struct stm32_venc_ewl *)instance;

	__ASSERT_NO_MSG(inst != NULL);
	__ASSERT_NO_MSG(info != NULL);

	/* find the pointer corresponding to the aligned buffer */
	for (uint32_t i = 0; i < inst->total_chunks; i++) {
		if (inst->aligned_chunks[i] == info->virtualAddress) {
			if (inst->chunks[i] != NULL) {
				EWL_HEAP_ALIGNED_FREE(inst->chunks[i]);
			}
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
	struct stm32_venc_ewl *inst = &ewl_instance;
	void *p = NULL;

	p = EWL_HEAP_ALIGNED_ALLOC(n);
	if (p == NULL) {
		LOG_ERR("alloc failed for size=%d", n);
		return NULL;
	}

	inst->mem_cnt += n;
	LOG_DBG("%8d bytes --> %p, total : %d", n, p, inst->mem_cnt);

	return p;
}

void EWLfree(void *p)
{
	if (p != NULL) {
		EWL_HEAP_ALIGNED_FREE(p);
	}
}

void *EWLcalloc(uint32_t n, uint32_t s)
{
	void *p = EWLmalloc(n * s);

	if (p != NULL) {
		EWLmemset(p, 0, n * s);
	}

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
	return memcmp(s1, s2, n);
}

i32 EWLWaitHwRdy(const void *instance, uint32_t *slices_ready)
{
	struct stm32_venc_ewl *inst = (struct stm32_venc_ewl *)instance;
	const struct stm32_venc_config *config = inst->config;
	int32_t ret = EWL_HW_WAIT_TIMEOUT;
	volatile uint32_t irq_stats;
	uint32_t prev_slices_ready = 0;
	k_timepoint_t timeout = sys_timepoint_calc(K_MSEC(EWL_TIMEOUT));
	uint32_t start = sys_clock_tick_get_32();

	__ASSERT_NO_MSG(inst != NULL);

	/* check how to clear IRQ flags for VENC */
	uint32_t clr_by_write_1 = EWLReadReg(inst, BASE_HWFuse2) & HWCFGIrqClearSupport;

	do {
		irq_stats = sys_read32(config->reg + BASE_HEncIRQ);
		/* get the number of completed slices from ASIC registers. */
		if (slices_ready != NULL && *slices_ready > prev_slices_ready) {
			*slices_ready = FIELD_GET(NUM_SLICES_READY_MASK,
						 sys_read32(config->reg + BASE_HEncControl7));
		}

		LOG_DBG("IRQ stat = %08x", irq_stats);

		uint32_t hw_handshake_status = IS_BIT_SET(
			sys_read32(config->reg + BASE_HEncInstantInput), LOW_LATENCY_HW_ITF_EN);

		/* ignore the irq status of input line buffer in hw handshake mode */
		if ((irq_stats == ASIC_STATUS_LINE_BUFFER_DONE) && (hw_handshake_status != 0UL)) {
			sys_write32(ASIC_STATUS_FUSE, config->reg + BASE_HEncIRQ);
			continue;
		}

		if ((irq_stats & ASIC_STATUS_ALL) != 0UL) {
			/* clear IRQ and slice ready status */
			uint32_t clr_stats;

			irq_stats &= ~(ASIC_STATUS_SLICE_READY | ASIC_IRQ_LINE);

			if (clr_by_write_1 != 0UL) {
				clr_stats = ASIC_STATUS_SLICE_READY | ASIC_IRQ_LINE;
			} else {
				clr_stats = irq_stats;
			}

			sys_write32(clr_stats, config->reg + BASE_HEncIRQ);
			ret = EWL_OK;
			break;
		}

		if (slices_ready != NULL && *slices_ready > prev_slices_ready) {
			ret = EWL_OK;
			break;
		}

	} while (!sys_timepoint_expired(timeout));

	if (ret != EWL_OK) {
		LOG_ERR("Timeout");
		return ret;
	}

	LOG_DBG("encoding = %d ms", k_ticks_to_ms_ceil32(sys_clock_tick_get_32() - start));

	if (slices_ready != NULL) {
		LOG_DBG("slices_ready = %d", *slices_ready);
	}

	return EWL_OK;
}

void EWLassert(bool expr, const char *str_expr, const char *file, unsigned int line)
{
	__ASSERT(expr, "ASSERTION FAIL [%s] @ %s:%d", str_expr, file, line);
}

/* Set CONFIG_VC8000NANOE_LOG_LEVEL_DBG to enable library tracing */
void EWLtrace(const char *s)
{
	printk("%s\n", s);
}

void EWLtraceparam(const char *fmt, const char *param, unsigned int val)
{
	printk(fmt, param, val);
}

static int stm32_venc_enable_clock(const struct device *dev)
{
	const struct stm32_venc_config *config = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&config->pclken) != 0) {
		return -EIO;
	}

	return 0;
}

static int stm32_venc_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct stm32_venc_data *data = dev->data;

	if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
		if ((fmt->pixelformat != VIDEO_PIX_FMT_NV12) &&
		    (fmt->pixelformat != VIDEO_PIX_FMT_RGB565)) {
			LOG_ERR("invalid input pixel format");
			return -EINVAL;
		}

		fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
		data->in_fmt = *fmt;
	} else {
		if (fmt->pixelformat != VIDEO_PIX_FMT_H264) {
			LOG_ERR("invalid output pixel format");
			return -EINVAL;
		}

		fmt->size = ROUND_UP(fmt->width * fmt->height / VENC_ESTIMATED_COMPRESSION_RATIO,
				     ALIGNMENT_INCR);

		data->out_fmt = *fmt;
	}

	return 0;
}

static int stm32_venc_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct stm32_venc_data *data = dev->data;

	if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
		*fmt = data->in_fmt;
	} else {
		*fmt = data->out_fmt;
	}

	return 0;
}

static int encoder_prepare(struct stm32_venc_data *data)
{
	H264EncRet h264ret;
	H264EncConfig cfg = {0};
	H264EncPreProcessingCfg preproc_cfg = {0};
	H264EncRateCtrl ratectrl_cfg = {0};
	H264EncCodingCtrl codingctrl_cfg = {0};

	data->frame_nb = 0;

	/* set config to 1 reference frame */
	cfg.refFrameAmount = 1;
	/* frame rate */
	cfg.frameRateDenom = 1;
	cfg.frameRateNum = VENC_DEFAULT_FRAMERATE;
	/* image resolution */
	cfg.width = data->out_fmt.width;
	cfg.height = data->out_fmt.height;
	/* stream type */
	cfg.streamType = H264ENC_BYTE_STREAM;

	/* encoding level*/
	cfg.level = VENC_DEFAULT_LEVEL;
	cfg.svctLevel = 0;
	cfg.viewMode = H264ENC_BASE_VIEW_SINGLE_BUFFER;

	h264ret = H264EncInit(&cfg, &data->encoder);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncInit error=%d", h264ret);
		return -EIO;
	}

	/* set format conversion for preprocessing */
	h264ret = H264EncGetPreProcessing(data->encoder, &preproc_cfg);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncGetPreProcessing error=%d", h264ret);
		return -EIO;
	}
	preproc_cfg.inputType = to_h264pixfmt(data->in_fmt.pixelformat);
	h264ret = H264EncSetPreProcessing(data->encoder, &preproc_cfg);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncSetPreProcessing error=%d", h264ret);
		return -EIO;
	}

	/* setup coding ctrl */
	h264ret = H264EncGetCodingCtrl(data->encoder, &codingctrl_cfg);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncGetCodingCtrl error=%d", h264ret);
		return -EIO;
	}

	h264ret = H264EncSetCodingCtrl(data->encoder, &codingctrl_cfg);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncSetCodingCtrl error=%d", h264ret);
		return -EIO;
	}

	/* set bit rate configuration */
	h264ret = H264EncGetRateCtrl(data->encoder, &ratectrl_cfg);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncGetRateCtrl error=%d", h264ret);
		return -EIO;
	}

	/* Constant bitrate */
	ratectrl_cfg.pictureRc = 0;
	ratectrl_cfg.mbRc = 0;
	ratectrl_cfg.pictureSkip = 0;
	ratectrl_cfg.hrd = 0;
	ratectrl_cfg.qpHdr = VENC_DEFAULT_QP;
	ratectrl_cfg.qpMin = ratectrl_cfg.qpHdr;
	ratectrl_cfg.qpMax = ratectrl_cfg.qpHdr;

	h264ret = H264EncSetRateCtrl(data->encoder, &ratectrl_cfg);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncSetRateCtrl error=%d", h264ret);
		return -EIO;
	}

	return 0;
}

static int encoder_start(struct stm32_venc_data *data, struct video_buffer *output)
{
	H264EncRet h264ret;
	H264EncIn enc_in = {0};
	H264EncOut enc_out = {0};

	enc_in.pOutBuf = (uint32_t *)output->buffer;
	enc_in.busOutBuf = (uint32_t)enc_in.pOutBuf;
	enc_in.outBufSize = output->size;

	/* create stream */
	h264ret = H264EncStrmStart(data->encoder, &enc_in, &enc_out);
	if (h264ret != H264ENC_OK) {
		LOG_ERR("H264EncStrmStart error=%d", h264ret);
		return -EIO;
	}

	output->bytesused = enc_out.streamSize;
	LOG_DBG("SPS/PPS generated, size= %d", output->bytesused);

	data->resync = true;

	return 0;
}

static int encoder_end(struct stm32_venc_data *data)
{
	H264EncIn enc_in = {0};
	H264EncOut enc_out = {0};

	if (data->encoder != NULL) {
		H264EncStrmEnd(data->encoder, &enc_in, &enc_out);
		data->encoder = NULL;
	}

	return 0;
}

static int encode_frame(struct stm32_venc_data *data)
{
	int ret = 0;
	H264EncRet h264ret = H264ENC_FRAME_READY;
	struct video_buffer *input;
	struct video_buffer *output;
	H264EncIn enc_in = {0};
	H264EncOut enc_out = {0};

	if (k_fifo_is_empty(&data->in_fifo_in) || k_fifo_is_empty(&data->out_fifo_in)) {
		/* Encoding deferred to next buffer queueing */
		return 0;
	}

	input = k_fifo_get(&data->in_fifo_in, K_NO_WAIT);
	output = k_fifo_get(&data->out_fifo_in, K_NO_WAIT);

	if (data->encoder == NULL) {
		ret = encoder_prepare(data);
		if (ret) {
			goto out;
		}

		ret = encoder_start(data, output);

		/*
		 * Output buffer is now filled with SPS/PPS header, return it to application.
		 * Input buffer is dropped to not introduce latency.
		 */
		goto out;
	}

	/* one key frame every seconds */
	if ((data->frame_nb % VENC_DEFAULT_FRAMERATE) == 0 || data->resync) {
		/* if frame is the first or resync needed: set as intra coded */
		enc_in.codingType = H264ENC_INTRA_FRAME;
	} else {
		/* if there was a frame previously, set as predicted */
		enc_in.timeIncrement = 1;
		enc_in.codingType = H264ENC_PREDICTED_FRAME;
	}

	enc_in.ipf = H264ENC_REFERENCE_AND_REFRESH;
	enc_in.ltrf = H264ENC_REFERENCE;

	/* set input buffers to structures */
	enc_in.busLuma = (ptr_t)input->buffer;
	enc_in.busChromaU = (ptr_t)enc_in.busLuma + data->in_fmt.width * data->in_fmt.height;

	enc_in.pOutBuf = (uint32_t *)output->buffer;
	enc_in.busOutBuf = (uint32_t)enc_in.pOutBuf;
	enc_in.outBufSize = output->size;
	enc_out.streamSize = 0;

	h264ret = H264EncStrmEncode(data->encoder, &enc_in, &enc_out, NULL, NULL, NULL);
	output->bytesused = enc_out.streamSize;
	LOG_DBG("output=%p, enc_out.streamSize=%d", output, enc_out.streamSize);

	switch (h264ret) {
	case H264ENC_FRAME_READY:
		/* save stream */
		if (enc_out.streamSize == 0) {
			/* Nothing encoded */
			data->resync = true;
			goto out;
		}
		output->bytesused = enc_out.streamSize;
		break;
	case H264ENC_FUSE_ERROR:
		LOG_ERR("H264EncStrmEncode error=%d", h264ret);

		LOG_ERR("DCMIPP and VENC desync at frame %d, restart the video", data->frame_nb);
		encoder_end(data);

		ret = encoder_start(data, output);
		if (ret) {
			goto out;
		}
		break;
	default:
		LOG_ERR("H264EncStrmEncode error=%d", h264ret);
		LOG_ERR("error encoding frame %d", data->frame_nb);

		encoder_end(data);

		ret = encoder_start(data, output);
		if (ret) {
			goto out;
		}

		data->resync = true;

		ret = -EIO;
		goto out;
	}

	data->frame_nb++;

out:
	k_fifo_put(&data->in_fifo_out, input);
	k_fifo_put(&data->out_fifo_out, output);

	return ret;
}

static int stm32_venc_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct stm32_venc_data *data = dev->data;

	ARG_UNUSED(type);

	if (!enable) {
		/* Stop VENC */
		encoder_end(data);
	}

	return 0;
}

static int stm32_venc_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct stm32_venc_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (vbuf->type == VIDEO_BUF_TYPE_INPUT) {
		k_fifo_put(&data->in_fifo_in, vbuf);
	} else {
		k_fifo_put(&data->out_fifo_in, vbuf);
	}

	ret = encode_frame(data);

	k_mutex_unlock(&data->lock);
	return ret;
}

static int stm32_venc_dequeue(const struct device *dev, struct video_buffer **vbuf,
			      k_timeout_t timeout)
{
	struct stm32_venc_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((*vbuf)->type == VIDEO_BUF_TYPE_INPUT) {
		*vbuf = k_fifo_get(&data->in_fifo_out, timeout);
	} else {
		*vbuf = k_fifo_get(&data->out_fifo_out, timeout);
	}

	if (*vbuf == NULL) {
		ret = -EAGAIN;
		goto out;
	}

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

ISR_DIRECT_DECLARE(stm32_venc_isr)
{
	struct stm32_venc_ewl *inst = &ewl_instance;
	const struct stm32_venc_config *config = inst->config;
	uint32_t hw_handshake_status =
		IS_BIT_SET(sys_read32(config->reg + BASE_HEncInstantInput), LOW_LATENCY_HW_ITF_EN);
	uint32_t irq_status = sys_read32(config->reg + BASE_HEncIRQ);

	inst->irq_status = irq_status;
	inst->irq_cnt++;

	if (hw_handshake_status == 0 && (irq_status & ASIC_STATUS_FUSE) != 0) {
		sys_write32(ASIC_STATUS_FUSE | ASIC_IRQ_LINE, config->reg + BASE_HEncIRQ);
		/* read back the IRQ status to update its value */
		irq_status = sys_read32(config->reg + BASE_HEncIRQ);
	}

	if (irq_status != 0U) {
		/* status flag is raised,
		 * clear the ones that the IRQ needs to clear
		 * and signal to EWLWaitHwRdy
		 */
		sys_write32(ASIC_STATUS_SLICE_READY | ASIC_IRQ_LINE, config->reg + BASE_HEncIRQ);
	}

	k_sem_give(&inst->complete);

	return 0;
}

#define VENC_FORMAT_CAP(pixfmt)                \
	{                                      \
		.pixelformat = pixfmt,         \
		.width_min = 48,               \
		.width_max = 1920,             \
		.height_min = 48,              \
		.height_max = 1088,            \
		.width_step = 16,              \
		.height_step = 16,             \
	}

static const struct video_format_cap in_fmts[] = {
	VENC_FORMAT_CAP(VIDEO_PIX_FMT_NV12),
	VENC_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	{0},
};

static const struct video_format_cap out_fmts[] = {
	VENC_FORMAT_CAP(VIDEO_PIX_FMT_H264),
	{0},
};

static int stm32_venc_get_caps(const struct device *dev, struct video_caps *caps)
{
	if (caps->type == VIDEO_BUF_TYPE_INPUT) {
		caps->format_caps = in_fmts;
	} else {
		caps->format_caps = out_fmts;
	}

	/* VENC produces full frames */
	caps->min_vbuf_count = 1;

	return 0;
}

static DEVICE_API(video, stm32_venc_driver_api) = {
	.set_format = stm32_venc_set_fmt,
	.get_format = stm32_venc_get_fmt,
	.set_stream = stm32_venc_set_stream,
	.enqueue = stm32_venc_enqueue,
	.dequeue = stm32_venc_dequeue,
	.get_caps = stm32_venc_get_caps,
};

static void stm32_venc_irq_config_func(const struct device *dev)
{
	IRQ_DIRECT_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), stm32_venc_isr, 0);
	irq_enable(DT_INST_IRQN(0));
}

static struct stm32_venc_data stm32_venc_data_0 = {};

static const struct stm32_venc_config stm32_venc_config_0 = {
	.reg = DT_INST_REG_ADDR(0),
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
	.reset = RESET_DT_SPEC_INST_GET_BY_IDX(0, 0),
	.irq_config = stm32_venc_irq_config_func,
};

static void risaf_config(void)
{
	/* Define and initialize the master configuration structure */
	RIMC_MasterConfig_t rimc_master = {0};

	/* Enable the clock for the RIFSC (RIF Security Controller) */
	__HAL_RCC_RIFSC_CLK_ENABLE();

	rimc_master.MasterCID = RIF_CID_1;
	rimc_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;

	/* Configure the master attributes for the video encoder peripheral (VENC) */
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_VENC, &rimc_master);

	/* Set the secure and privileged attributes for the VENC as a slave */
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_VENC,
					      RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

static int stm32_venc_init(const struct device *dev)
{
	const struct stm32_venc_config *config = dev->config;
	struct stm32_venc_data *data = dev->data;
	int err;

	/* Enable VENC clock */
	err = stm32_venc_enable_clock(dev);
	if (err < 0) {
		LOG_ERR("clock enabling failed.");
		return err;
	}

	/* Reset VENC */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}
	reset_line_toggle_dt(&config->reset);

	data->dev = dev;
	k_mutex_init(&data->lock);
	k_fifo_init(&data->in_fifo_in);
	k_fifo_init(&data->in_fifo_out);
	k_fifo_init(&data->out_fifo_in);
	k_fifo_init(&data->out_fifo_out);

	/* Run IRQ init */
	config->irq_config(dev);

	risaf_config();

	LOG_DBG("CPU frequency    : %d", HAL_RCC_GetCpuClockFreq() / 1000000);
	LOG_DBG("sysclk frequency : %d", HAL_RCC_GetSysClockFreq() / 1000000);
	LOG_DBG("pclk5 frequency  : %d", HAL_RCC_GetPCLK5Freq() / 1000000);

	/* default input */
	data->in_fmt.width = VENC_DEFAULT_WIDTH;
	data->in_fmt.height = VENC_DEFAULT_HEIGHT;
	data->in_fmt.pixelformat = VENC_DEFAULT_IN_FMT;
	data->in_fmt.pitch = data->in_fmt.width;

	/* default output */
	data->out_fmt.width = VENC_DEFAULT_WIDTH;
	data->out_fmt.height = VENC_DEFAULT_HEIGHT;
	data->out_fmt.pixelformat = VENC_DEFAULT_OUT_FMT;

	/* store config for register accesses */
	ewl_instance.config = config;

	LOG_DBG("%s inited", dev->name);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, &stm32_venc_init, NULL, &stm32_venc_data_0, &stm32_venc_config_0,
		      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &stm32_venc_driver_api);
