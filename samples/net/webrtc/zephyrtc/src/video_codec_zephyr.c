/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "video_codec_zephyr.h"

#include <H264SwDecApi.h>
#include <errno.h>
#include <minih264e.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "platform.h"

#define ENCODER_PERSISTENT_CAPACITY 288368U
#define ENCODER_SCRATCH_CAPACITY    182607U
#define MAX_DECODE_STEPS            16U
#define PARAMETER_SET_CACHE_BYTES   512U

struct decoder_diagnostic {
	const char *feature;
	const char *source_file;
	uint32_t source_line;
};

static uint8_t encoder_persistent[ENCODER_PERSISTENT_CAPACITY] __aligned(64);
static uint8_t encoder_scratch[ENCODER_SCRATCH_CAPACITY] __aligned(64);
static uint8_t decoder_input[VIDEO_CODEC_MAX_NAL_BYTES] __aligned(16);
static uint8_t cached_sps[PARAMETER_SET_CACHE_BYTES];
static uint8_t cached_pps[PARAMETER_SET_CACHE_BYTES];

static H264E_run_param_t encoder_run;
static H264SwDecInst decoder;
static struct decoder_diagnostic decoder_diagnostic;
static struct video_codec_stats codec_stats;
static uint32_t decoded_width;
static uint32_t decoded_height;
static uint32_t decoded_pictures;
static size_t cached_sps_size;
static size_t cached_pps_size;
static bool waiting_for_idr;
static bool replaying_parameter_sets;
static atomic_t keyframe_request;
static bool initialized;

static const char *source_basename(const char *path)
{
	const char *slash;

	if (path == NULL) {
		return "unknown";
	}
	slash = strrchr(path, '/');
	return slash != NULL ? slash + 1 : path;
}

static void decoder_error_callback(const char *feature, const char *source_file, u32 source_line,
				   void *user_data)
{
	ARG_UNUSED(user_data);
	if (decoder_diagnostic.feature == NULL) {
		decoder_diagnostic.feature = feature;
		decoder_diagnostic.source_file = source_file;
		decoder_diagnostic.source_line = source_line;
	}
}

static int decoder_reset(void)
{
	H264SwDecRet result;

	if (decoder != NULL) {
		H264SwDecRelease(decoder);
		decoder = NULL;
	}
	decoded_width = 0U;
	decoded_height = 0U;
	result = H264SwDecInit(&decoder, 1U);
	if (result != H264SWDEC_OK) {
		platform_log("error", "h264swdec_reset", "status=fail result=%d", result);
		decoder = NULL;
		return result == H264SWDEC_MEMFAIL ? -ENOMEM : -EIO;
	}
	return 0;
}

static size_t annex_b_start_code_size(const uint8_t *data, size_t size, size_t index)
{
	if (index + 3U <= size && data[index] == 0U && data[index + 1U] == 0U &&
	    data[index + 2U] == 1U) {
		return 3U;
	}
	if (index + 4U <= size && data[index] == 0U && data[index + 1U] == 0U &&
	    data[index + 2U] == 0U && data[index + 3U] == 1U) {
		return 4U;
	}
	return 0U;
}

static void cache_parameter_sets(const uint8_t *data, size_t size)
{
	size_t current = 0U;

	while (current < size) {
		size_t start_code = annex_b_start_code_size(data, size, current);
		size_t next;
		size_t nal_size;
		uint8_t nal_type;
		uint8_t *destination;
		size_t *destination_size;

		if (start_code == 0U) {
			current++;
			continue;
		}
		next = current + start_code;
		if (next >= size) {
			return;
		}
		nal_type = data[next] & 0x1fU;
		next++;
		while (next < size && annex_b_start_code_size(data, size, next) == 0U) {
			next++;
		}
		nal_size = next - current;
		if (nal_type != 7U && nal_type != 8U) {
			current = next;
			continue;
		}
		destination = nal_type == 7U ? cached_sps : cached_pps;
		destination_size = nal_type == 7U ? &cached_sps_size : &cached_pps_size;
		if (nal_size <= PARAMETER_SET_CACHE_BYTES) {
			memcpy(destination, data + current, nal_size);
			*destination_size = nal_size;
		}
		current = next;
	}
}

static bool annex_b_contains_nal_type(const uint8_t *data, size_t size, uint8_t expected)
{
	for (size_t index = 0U; index + 3U < size; index++) {
		size_t header = 0U;

		if (data[index] == 0U && data[index + 1U] == 0U && data[index + 2U] == 1U) {
			header = index + 3U;
		} else if (index + 4U < size && data[index] == 0U && data[index + 1U] == 0U &&
			   data[index + 2U] == 0U && data[index + 3U] == 1U) {
			header = index + 4U;
		}
		if (header < size && header != 0U && (data[header] & 0x1fU) == expected) {
			return true;
		}
	}
	return false;
}

static uint8_t annex_b_first_nal_type(const uint8_t *data, size_t size)
{
	for (size_t index = 0U; index + 3U < size; index++) {
		if (data[index] == 0U && data[index + 1U] == 0U && data[index + 2U] == 1U) {
			return data[index + 3U] & 0x1fU;
		}
		if (index + 4U < size && data[index] == 0U && data[index + 1U] == 0U &&
		    data[index + 2U] == 0U && data[index + 3U] == 1U) {
			return data[index + 4U] & 0x1fU;
		}
	}
	return 0U;
}

static int handle_decode_failure(H264SwDecRet result, uint64_t sequence, uint8_t nal_type)
{
	const char *feature = decoder_diagnostic.feature != NULL ? decoder_diagnostic.feature
								 : "unclassified_stream_error";
	const char *source = source_basename(decoder_diagnostic.source_file);
	uint32_t line = decoder_diagnostic.source_line;
	int reset_result;

	codec_stats.decode_errors++;
	waiting_for_idr = true;
	atomic_set(&keyframe_request, 1);
	reset_result = decoder_reset();
	if (reset_result == 0) {
		codec_stats.decoder_resets++;
	}
	platform_log("warning", "h264swdec_decode_error",
		     "sequence=%llu result=%d nal_type=%u feature=%s path=%s:%u "
		     "errors=%llu resets=%llu reset_result=%d action=await_idr",
		     (unsigned long long)sequence, result, nal_type, feature, source, line,
		     (unsigned long long)codec_stats.decode_errors,
		     (unsigned long long)codec_stats.decoder_resets, reset_result);
	if (reset_result != 0) {
		return reset_result;
	}
	return result == H264SWDEC_MEMFAIL ? -ENOMEM : -EBADMSG;
}

int video_codec_init(void)
{
	H264E_create_param_t create = {0};
	int persistent_bytes;
	int scratch_bytes;
	int result;

	if (initialized) {
		return -EALREADY;
	}
	create.width = VIDEO_CODEC_WIDTH;
	create.height = VIDEO_CODEC_HEIGHT;
	create.gop = 1;
	create.const_input_flag = 1;
	create.enableNEON = 0;
	create.temporal_denoise_flag = 0;
	result = H264E_sizeof(&create, &persistent_bytes, &scratch_bytes);
	if (result != H264E_STATUS_SUCCESS || persistent_bytes <= 0 || scratch_bytes <= 0 ||
	    (size_t)persistent_bytes > sizeof(encoder_persistent) ||
	    (size_t)scratch_bytes > sizeof(encoder_scratch)) {
		return -ENOMEM;
	}
	result = H264E_init((H264E_persist_t *)encoder_persistent, &create);
	if (result != H264E_STATUS_SUCCESS) {
		return -EIO;
	}
	memset(&encoder_run, 0, sizeof(encoder_run));
	memset(&codec_stats, 0, sizeof(codec_stats));
	memset(&decoder_diagnostic, 0, sizeof(decoder_diagnostic));
	encoder_run.encode_speed = H264E_SPEED_FASTEST;
	encoder_run.qp_min = 36;
	encoder_run.qp_max = 36;
	H264SwDecSetErrorCallback(decoder_error_callback, NULL);
	result = decoder_reset();
	if (result != 0) {
		H264SwDecSetErrorCallback(NULL, NULL);
		return result;
	}
	decoded_pictures = 0U;
	cached_sps_size = 0U;
	cached_pps_size = 0U;
	waiting_for_idr = false;
	replaying_parameter_sets = false;
	atomic_set(&keyframe_request, 0);
	initialized = true;
	return 0;
}

int video_codec_encode(const uint8_t *i420, const uint8_t **encoded, size_t *encoded_size)
{
	H264E_io_yuv_t yuv;
	unsigned char *coded_data;
	int coded_bytes;
	int result;

	if (!initialized || i420 == NULL || encoded == NULL || encoded_size == NULL) {
		return -EINVAL;
	}
	yuv.yuv[0] = (uint8_t *)i420;
	yuv.yuv[1] = (uint8_t *)i420 + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT;
	yuv.yuv[2] = yuv.yuv[1] + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT / 4U;
	yuv.stride[0] = VIDEO_CODEC_WIDTH;
	yuv.stride[1] = VIDEO_CODEC_WIDTH / 2U;
	yuv.stride[2] = VIDEO_CODEC_WIDTH / 2U;
	result = H264E_encode((H264E_persist_t *)encoder_persistent,
			      (H264E_scratch_t *)encoder_scratch, &encoder_run, &yuv, &coded_data,
			      &coded_bytes);
	if (result != H264E_STATUS_SUCCESS || coded_bytes <= 0) {
		return -EIO;
	}
	if ((size_t)coded_bytes > VIDEO_CODEC_MAX_NAL_BYTES) {
		return -E2BIG;
	}
	*encoded = coded_data;
	*encoded_size = (size_t)coded_bytes;
	return 0;
}

static int update_dimensions(void)
{
	H264SwDecInfo info;
	H264SwDecRet result = H264SwDecGetInfo(decoder, &info);

	if (result != H264SWDEC_OK) {
		return -EBADMSG;
	}
	if (info.croppingFlag != 0U) {
		platform_log("warning", "h264swdec_headers",
			     "feature=cropping coded=%ux%u crop=%u,%u_%ux%u", info.picWidth,
			     info.picHeight, info.cropParams.cropLeftOffset,
			     info.cropParams.cropTopOffset, info.cropParams.cropOutWidth,
			     info.cropParams.cropOutHeight);
		return -ENOTSUP;
	}
	decoded_width = info.picWidth;
	decoded_height = info.picHeight;
	platform_log("info", "h264swdec_headers", "profile=%u dimensions=%ux%u", info.profile,
		     decoded_width, decoded_height);
	return decoded_width == VIDEO_CODEC_WIDTH && decoded_height == VIDEO_CODEC_HEIGHT
		       ? 0
		       : -ENOTSUP;
}

int video_codec_decode(const uint8_t *annex_b_nal, size_t size, uint64_t sequence,
		       const uint8_t **i420, size_t *i420_size, bool *picture_ready)
{
	H264SwDecInput input = {0};
	H264SwDecOutput output = {0};
	uint8_t nal_type;
	bool contains_idr;
	bool contains_non_idr;
	bool decoded_idr = false;

	if (!initialized || decoder == NULL || annex_b_nal == NULL || size == 0U ||
	    size > sizeof(decoder_input) || i420 == NULL || i420_size == NULL ||
	    picture_ready == NULL) {
		return -EINVAL;
	}
	nal_type = annex_b_first_nal_type(annex_b_nal, size);
	contains_idr = annex_b_contains_nal_type(annex_b_nal, size, 5U);
	contains_non_idr = annex_b_contains_nal_type(annex_b_nal, size, 1U);
	*i420 = NULL;
	*i420_size = 0U;
	*picture_ready = false;

#if defined(ZEPHYRTC_DIAGNOSTIC_FORCE_DECODER_RESET_SEQUENCE)
	if (!replaying_parameter_sets && !waiting_for_idr &&
	    sequence == ZEPHYRTC_DIAGNOSTIC_FORCE_DECODER_RESET_SEQUENCE) {
		decoder_diagnostic.feature = "diagnostic_forced_reset";
		decoder_diagnostic.source_file = __FILE__;
		decoder_diagnostic.source_line = __LINE__;
		return handle_decode_failure(H264SWDEC_STRM_ERR, sequence, nal_type);
	}
#endif

	if (waiting_for_idr && contains_non_idr && !contains_idr) {
		codec_stats.awaiting_idr_drops++;
		platform_log("warning", "h264swdec_waiting_for_idr",
			     "sequence=%llu nal_type=%u drops=%llu", (unsigned long long)sequence,
			     nal_type, (unsigned long long)codec_stats.awaiting_idr_drops);
		return -EAGAIN;
	}
	if (waiting_for_idr && contains_idr && !replaying_parameter_sets) {
		const uint8_t *ignored_i420;
		size_t ignored_size;
		bool ignored_ready;
		int replay_result = 0;

		replaying_parameter_sets = true;
		if (!annex_b_contains_nal_type(annex_b_nal, size, 7U) && cached_sps_size != 0U) {
			replay_result =
				video_codec_decode(cached_sps, cached_sps_size, sequence,
						   &ignored_i420, &ignored_size, &ignored_ready);
			codec_stats.parameter_set_replays++;
		}
		if (replay_result == 0 && !annex_b_contains_nal_type(annex_b_nal, size, 8U) &&
		    cached_pps_size != 0U) {
			replay_result =
				video_codec_decode(cached_pps, cached_pps_size, sequence,
						   &ignored_i420, &ignored_size, &ignored_ready);
			codec_stats.parameter_set_replays++;
		}
		replaying_parameter_sets = false;
		if (replay_result != 0) {
			return replay_result;
		}
	}

	memcpy(decoder_input, annex_b_nal, size);
	input.pStream = decoder_input;
	input.dataLen = (u32)size;
	input.picId = decoded_pictures;
	input.intraConcealmentMethod = 0U;
	memset(&decoder_diagnostic, 0, sizeof(decoder_diagnostic));

	for (size_t step = 0U; step < MAX_DECODE_STEPS && input.dataLen > 0U; step++) {
		H264SwDecPicture picture;
		H264SwDecRet result;
		u32 consumed;

#if defined(ZEPHYRTC_DIAGNOSTIC_LOG_DECODER_CALLS)
		platform_log("info", "h264swdec_call_begin",
			     "sequence=%llu step=%zu bytes=%u nal_type=%u",
			     (unsigned long long)sequence, step, input.dataLen, nal_type);
#endif
		result = H264SwDecDecode(decoder, &input, &output);
#if defined(ZEPHYRTC_DIAGNOSTIC_LOG_DECODER_CALLS)
		platform_log("info", "h264swdec_call_end", "sequence=%llu step=%zu result=%d",
			     (unsigned long long)sequence, step, result);
#endif
		if (output.pStrmCurrPos == NULL || output.pStrmCurrPos < input.pStream ||
		    output.pStrmCurrPos > input.pStream + input.dataLen) {
			return handle_decode_failure(H264SWDEC_PARAM_ERR, sequence, nal_type);
		}
		consumed = (u32)(output.pStrmCurrPos - input.pStream);
		input.pStream = output.pStrmCurrPos;
		input.dataLen -= consumed;

		if (result == H264SWDEC_HDRS_RDY_BUFF_NOT_EMPTY) {
			int info_result = update_dimensions();

			if (info_result != 0) {
				return handle_decode_failure(H264SWDEC_STRM_ERR, sequence,
							     nal_type);
			}
		} else if (result == H264SWDEC_PIC_RDY ||
			   result == H264SWDEC_PIC_RDY_BUFF_NOT_EMPTY) {
			while (H264SwDecNextPicture(decoder, &picture, 0U) == H264SWDEC_PIC_RDY) {
				if (picture.pOutputPicture == NULL ||
				    decoded_width != VIDEO_CODEC_WIDTH ||
				    decoded_height != VIDEO_CODEC_HEIGHT) {
					return handle_decode_failure(H264SWDEC_STRM_ERR, sequence,
								     nal_type);
				}
				if (picture.nbrOfErrMBs != 0U) {
					codec_stats.concealed_pictures++;
					platform_log(
						"warning", "h264swdec_concealment",
						"sequence=%llu picture=%u concealed_mbs=%u "
						"total=%llu",
						(unsigned long long)sequence, picture.picId,
						picture.nbrOfErrMBs,
						(unsigned long long)codec_stats.concealed_pictures);
				}
				*i420 = (const uint8_t *)picture.pOutputPicture;
				*i420_size = VIDEO_CODEC_I420_BYTES;
				*picture_ready = true;
				decoded_idr = decoded_idr || picture.isIdrPicture != 0U;
				decoded_pictures++;
			}
		} else if (result < 0) {
			return handle_decode_failure(result, sequence, nal_type);
		}

		if (input.dataLen != 0U && consumed == 0U &&
		    result != H264SWDEC_HDRS_RDY_BUFF_NOT_EMPTY &&
		    result != H264SWDEC_PIC_RDY_BUFF_NOT_EMPTY) {
			return handle_decode_failure(H264SWDEC_STRM_ERR, sequence, nal_type);
		}
	}
	if (input.dataLen != 0U) {
		decoder_diagnostic.feature = "decode_step_limit";
		decoder_diagnostic.source_file = __FILE__;
		decoder_diagnostic.source_line = __LINE__;
		return handle_decode_failure(H264SWDEC_STRM_ERR, sequence, nal_type);
	}
	if (waiting_for_idr && contains_idr && decoded_idr) {
		waiting_for_idr = false;
		codec_stats.recovered_idrs++;
		platform_log("info", "h264swdec_recovered",
			     "sequence=%llu kind=idr recoveries=%llu replays=%llu decoded=%u",
			     (unsigned long long)sequence,
			     (unsigned long long)codec_stats.recovered_idrs,
			     (unsigned long long)codec_stats.parameter_set_replays,
			     decoded_pictures);
	}
	cache_parameter_sets(annex_b_nal, size);
	return 0;
}

bool video_codec_take_keyframe_request(void)
{
	return atomic_cas(&keyframe_request, 1, 0);
}

void video_codec_get_stats(struct video_codec_stats *stats)
{
	if (stats != NULL) {
		*stats = codec_stats;
	}
}

void video_codec_deinit(void)
{
	if (!initialized) {
		return;
	}
	if (decoder != NULL) {
		H264SwDecRelease(decoder);
		decoder = NULL;
	}
	H264SwDecSetErrorCallback(NULL, NULL);
	waiting_for_idr = false;
	initialized = false;
}
