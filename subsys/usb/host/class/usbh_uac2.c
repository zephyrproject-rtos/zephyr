/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usbh_uac2.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

LOG_MODULE_REGISTER(usbh_uac2, CONFIG_USBH_UAC2_LOG_LEVEL);

/* UAC2 device descriptors collection */
struct uac2_device_descriptors {
	const struct uac2_ac_header_descriptor *ac_header;
	const struct uac2_clock_source_descriptor *clock_source;
	const struct uac2_clock_selector_descriptor *clock_selector;
	const struct uac2_clock_multiplier_descriptor *clock_multiplier;
	const struct uac2_input_terminal_descriptor *input_terminal;
	const struct uac2_output_terminal_descriptor *output_terminal;
	const struct uac2_mixer_unit_descriptor *mixer_unit;
	const struct uac2_selector_unit_descriptor *selector_unit;
	const struct uac2_feature_unit_descriptor *feature_unit;
	const struct uac2_effect_unit_descriptor *effect_unit;
	const struct uac2_processing_unit_descriptor *processing_unit;
	const struct uac2_extension_unit_descriptor *extension_unit;
	const struct uac2_as_general_descriptor *as_header;
	const struct uac2_format_type_common_descriptor *format_type;
};

/* Audio stream interface information */
struct uac2_stream_iface_info {
	const struct usb_if_descriptor *iface;
	const struct usb_ep_descriptor *ep;
	const struct uac2_cs_as_iso_endpoint_descriptor *cs_ep;
	uint16_t ep_mps;
	uint8_t ep_interval;
};

/* Audio format information */
struct uac2_format_info {
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bit_resolution;
	uint8_t subslot_size;
};

struct uac2_transfer_timing {
	uint32_t base_sample_count;
	uint32_t base_transfer_total;
	uint32_t extra_transfer_total;
	uint32_t base_transfer_left;
	uint32_t extra_transfer_left;
	uint32_t toggle_ratio;
	uint32_t toggle_counter;
};

/* Volume information (using Layout 2) */
struct uac2_volume_info {
	struct uac2_layout2_range range;
	int16_t cur_volume;
	bool is_valid;
};

/* Sample frequency information (using Layout 3) */
struct uac2_frequency_info {
	struct uac2_layout3_range range;
	uint32_t cur_frequency;
	bool is_valid;
};

/* UAC2 host data structure */
struct uac2_host_data {
	struct usb_device *udev;
	struct k_mutex lock;

	atomic_t device_flags;

	uint8_t in_transfer_count;
	struct uhc_transfer *audio_in_transfer[CONFIG_USBH_UAC2_STREAM_IN_CONCURRENT_TRANSFERS];
	usbh_uac2_transfer_cb_t in_callback;
	void *in_callback_param;

	uint8_t out_transfer_count;
	struct uhc_transfer *audio_out_transfer[CONFIG_USBH_UAC2_STREAM_OUT_CONCURRENT_TRANSFERS];
	usbh_uac2_transfer_cb_t out_callback;
	void *out_callback_param;

	const struct usb_if_descriptor *current_ctrl_iface;
	struct uac2_stream_iface_info in_stream_iface;
	struct uac2_stream_iface_info out_stream_iface;
	struct uac2_device_descriptors in_descriptors;
	struct uac2_device_descriptors out_descriptors;

	struct uac2_format_info current_in_format;
	struct uac2_format_info current_out_format;

	/* Volume information (Layout 2) */
	struct uac2_volume_info in_volume_info;
	struct uac2_volume_info out_volume_info;

	/* Frequency information (Layout 3) */
	struct uac2_frequency_info in_frequency_info;
	struct uac2_frequency_info out_frequency_info;

	struct uac2_transfer_timing out_timing;
};

/* Clock Source controls */
static const struct uac2_ctrl_mapping ctrl_map_clock_source[] = {
	{
		.cid = AUDIO_CID_SAMPLE_RATE,
		.ctrl_selector = CS_SAM_FREQ_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_CLOCK_SOURCE,
	},
	{
		.cid = AUDIO_CID_CLOCK_VALID,
		.ctrl_selector = CS_CLOCK_VALID_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_CLOCK_SOURCE,
	},
};

/* Clock Selector controls */
static const struct uac2_ctrl_mapping ctrl_map_clock_selector[] = {
	{
		.cid = AUDIO_CID_CLOCK_SELECTOR,
		.ctrl_selector = CX_CLOCK_SELECTOR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_CLOCK_SELECTOR,
	},
};

/* Clock Multiplier controls */
static const struct uac2_ctrl_mapping ctrl_map_clock_multiplier[] = {
	{
		.cid = AUDIO_CID_CLOCK_MULTIPLIER_NUMERATOR,
		.ctrl_selector = CM_NUMERATOR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_CLOCK_MULTIPLIER,
	},
	{
		.cid = AUDIO_CID_CLOCK_MULTIPLIER_DENOMINATOR,
		.ctrl_selector = CM_DENOMINATOR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_CLOCK_MULTIPLIER,
	},
};

/* Terminal controls */
static const struct uac2_ctrl_mapping ctrl_map_terminal[] = {
	{
		.cid = AUDIO_CID_COPY_PROTECT,
		.ctrl_selector = TE_COPY_PROTECT_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = TERMINAL_TYPE_ENCODE(AC_DESCRIPTOR_INPUT_TERMINAL,
						     AC_DESCRIPTOR_OUTPUT_TERMINAL),
	},
	{
		.cid = AUDIO_CID_CONNECTOR,
		.ctrl_selector = TE_CONNECTOR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = TERMINAL_TYPE_ENCODE(AC_DESCRIPTOR_INPUT_TERMINAL,
						     AC_DESCRIPTOR_OUTPUT_TERMINAL),
	},
	{
		.cid = AUDIO_CID_OVERFLOW,
		.ctrl_selector = TE_OVERFLOW_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = TERMINAL_TYPE_ENCODE(AC_DESCRIPTOR_INPUT_TERMINAL,
						     TERMINAL_NA),
	},
	{
		.cid = AUDIO_CID_UNDERFLOW,
		.ctrl_selector = TE_UNDERFLOW_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = TERMINAL_TYPE_ENCODE(TERMINAL_NA,
						     AC_DESCRIPTOR_OUTPUT_TERMINAL),
	},
	{
		.cid = AUDIO_CID_CLUSTER,
		.ctrl_selector = TE_CLUSTER_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = TERMINAL_TYPE_ENCODE(AC_DESCRIPTOR_INPUT_TERMINAL,
						     AC_DESCRIPTOR_INPUT_TERMINAL),
	},
	{
		.cid = AUDIO_CID_LATENCY,
		.ctrl_selector = TE_LATENCY_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = TERMINAL_TYPE_ENCODE(AC_DESCRIPTOR_OUTPUT_TERMINAL,
						     AC_DESCRIPTOR_INPUT_TERMINAL),
	},
	{
		.cid = AUDIO_CID_OVERLOAD,
		.ctrl_selector = TE_OVERLOAD_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = TERMINAL_TYPE_ENCODE(AC_DESCRIPTOR_INPUT_TERMINAL,
						     AC_DESCRIPTOR_OUTPUT_TERMINAL),
	},
};

/* Mixer Unit controls */
static const struct uac2_ctrl_mapping ctrl_map_mixer[] = {
	{
		.cid = AUDIO_CID_MIXER,
		.ctrl_selector = MU_MIXER_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = true,
		.unit_subtype = AC_DESCRIPTOR_MIXER_UNIT,
	},
	{
		.cid = AUDIO_CID_MIXER_CLUSTER,
		.ctrl_selector = MU_CLUSTER_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_MIXER_UNIT,
	},
	{
		.cid = AUDIO_CID_MIXER_UNDERFLOW,
		.ctrl_selector = MU_UNDERFLOW_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_MIXER_UNIT,
	},
	{
		.cid = AUDIO_CID_MIXER_OVERFLOW,
		.ctrl_selector = MU_OVERFLOW_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_MIXER_UNIT,
	},
	{
		.cid = AUDIO_CID_MIXER_LATENCY,
		.ctrl_selector = MU_LATENCY_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_MIXER_UNIT,
	},
};

/* Selector Unit controls */
static const struct uac2_ctrl_mapping ctrl_map_selector[] = {
	{
		.cid = AUDIO_CID_SELECTOR,
		.ctrl_selector = SU_SELECTOR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_SELECTOR_UNIT,
	},
	{
		.cid = AUDIO_CID_SELECTOR_LATENCY,
		.ctrl_selector = SU_LATENCY_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_SELECTOR_UNIT,
	},
};

/* Feature Unit controls */
static const struct uac2_ctrl_mapping ctrl_map_feature[] = {
	{
		.cid = AUDIO_CID_MUTE,
		.ctrl_selector = FU_MUTE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_VOLUME,
		.ctrl_selector = FU_VOLUME_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = true,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_BASS,
		.ctrl_selector = FU_BASS_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = true,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_MID,
		.ctrl_selector = FU_MID_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = true,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_TREBLE,
		.ctrl_selector = FU_TREBLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = true,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_GRAPHIC_EQUALIZER,
		.ctrl_selector = FU_GRAPHIC_EQUALIZER_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_AUTO_GAIN,
		.ctrl_selector = FU_AUTOMATIC_GAIN_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_DELAY,
		.ctrl_selector = FU_DELAY_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_BASS_BOOST,
		.ctrl_selector = FU_BASS_BOOST_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_LOUDNESS,
		.ctrl_selector = FU_LOUDNESS_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_INPUT_GAIN,
		.ctrl_selector = FU_INPUT_GAIN_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_INPUT_GAIN_PAD,
		.ctrl_selector = FU_INPUT_GAIN_PAD_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
	{
		.cid = AUDIO_CID_PHASE_INVERTER,
		.ctrl_selector = FU_PHASE_INVERTER_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_FEATURE_UNIT,
	},
};

#if defined(CONFIG_USBH_UAC2_AUDIO_EFFECT_CONTROLS)
/* Effect Unit controls - Parametric Equalizer */
static const struct uac2_ctrl_mapping ctrl_map_effect_pe[] = {
	{
		.cid = AUDIO_CID_PE_ENABLE,
		.ctrl_selector = PE_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_PE_CENTERFREQ,
		.ctrl_selector = PE_CENTERFREQ_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_PE_QFACTOR,
		.ctrl_selector = PE_QFACTOR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_PE_GAIN,
		.ctrl_selector = PE_GAIN_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = true,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
};

/* Reverberation Effect controls */
static const struct uac2_ctrl_mapping ctrl_map_effect_rv[] = {
	{
		.cid = AUDIO_CID_REVERB_ENABLE,
		.ctrl_selector = RV_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_REVERB_TYPE,
		.ctrl_selector = RV_TYPE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_REVERB_LEVEL,
		.ctrl_selector = RV_LEVEL_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_REVERB_TIME,
		.ctrl_selector = RV_TIME_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_REVERB_FEEDBACK,
		.ctrl_selector = RV_FEEDBACK_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_REVERB_PREDELAY,
		.ctrl_selector = RV_PREDELAY_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_REVERB_DENSITY,
		.ctrl_selector = RV_DENSITY_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_REVERB_HIFREQ_ROLLOFF,
		.ctrl_selector = RV_HIFREQ_ROLLOFF_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
};

/* Modulation Delay Effect controls (Chorus/Flanger) */
static const struct uac2_ctrl_mapping ctrl_map_effect_md[] = {
	{
		.cid = AUDIO_CID_CHORUS_ENABLE,
		.ctrl_selector = MD_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_CHORUS_BALANCE,
		.ctrl_selector = MD_BALANCE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_CHORUS_RATE,
		.ctrl_selector = MD_RATE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_CHORUS_DEPTH,
		.ctrl_selector = MD_DEPTH_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_CHORUS_TIME,
		.ctrl_selector = MD_TIME_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_CHORUS_FEEDBACK,
		.ctrl_selector = MD_FEEDBACK_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
};

/* Dynamic Range Compressor Effect controls */
static const struct uac2_ctrl_mapping ctrl_map_effect_dr[] = {
	{
		.cid = AUDIO_CID_COMPRESSOR_ENABLE,
		.ctrl_selector = DR_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_COMPRESSOR_RATIO,
		.ctrl_selector = DR_COMPRESSION_RATE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_COMPRESSOR_MAX_AMPLITUDE,
		.ctrl_selector = DR_MAXAMPL_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_COMPRESSOR_THRESHOLD,
		.ctrl_selector = DR_THRESHOLD_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = true,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_COMPRESSOR_ATTACK_TIME,
		.ctrl_selector = DR_ATTACK_TIME_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
	{
		.cid = AUDIO_CID_COMPRESSOR_RELEASE_TIME,
		.ctrl_selector = DR_RELEASE_TIME_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EFFECT_UNIT,
	},
};
#endif /* CONFIG_USBH_UAC2_AUDIO_EFFECT_CONTROLS */

#if defined(CONFIG_USBH_UAC2_AUDIO_PROCESSING_CONTROLS)
/* Processing Unit controls - Up/Down-mix */
static const struct uac2_ctrl_mapping ctrl_map_processing_ud[] = {
	{
		.cid = AUDIO_CID_UD_ENABLE,
		.ctrl_selector = UD_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_PROCESSING_UNIT,
	},
	{
		.cid = AUDIO_CID_UD_MODE_SELECT,
		.ctrl_selector = UD_MODE_SELECT_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_PROCESSING_UNIT,
	},
};

/* Dolby Prologic Processing controls */
static const struct uac2_ctrl_mapping ctrl_map_proc_dp[] = {
	{
		.cid = AUDIO_CID_DOLBY_ENABLE,
		.ctrl_selector = DP_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_PROCESSING_UNIT,
	},
	{
		.cid = AUDIO_CID_DOLBY_MODE,
		.ctrl_selector = DP_MODE_SELECT_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_PROCESSING_UNIT,
	},
};

/* Stereo Extender Processing controls */
static const struct uac2_ctrl_mapping ctrl_map_proc_st[] = {
	{
		.cid = AUDIO_CID_STEREO_EXTENDER_ENABLE,
		.ctrl_selector = ST_EXT_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_PROCESSING_UNIT,
	},
	{
		.cid = AUDIO_CID_STEREO_EXTENDER_WIDTH,
		.ctrl_selector = ST_EXT_WIDTH_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_2,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_PROCESSING_UNIT,
	},
};
#endif /* CONFIG_USBH_UAC2_AUDIO_PROCESSING_CONTROLS */

/* Extension Unit controls */
static const struct uac2_ctrl_mapping ctrl_map_extension[] = {
	{
		.cid = AUDIO_CID_XU_ENABLE,
		.ctrl_selector = XU_ENABLE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
		.unit_subtype = AC_DESCRIPTOR_EXTENSION_UNIT,
	},
};

/* AudioStreaming Interface controls */
static const struct uac2_ctrl_mapping ctrl_map_as_interface[] = {
	{
		.cid = AUDIO_CID_ACTIVE_ALT_SETTING,
		.ctrl_selector = AS_ACT_ALT_SETTING_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_VALID_ALT_SETTINGS,
		.ctrl_selector = AS_VAL_ALT_SETTINGS_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_AUDIO_DATA_FORMAT,
		.ctrl_selector = AS_AUDIO_DATA_FORMAT_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
};

/* Encoder controls */
static const struct uac2_ctrl_mapping ctrl_map_encoder[] = {
	{
		.cid = AUDIO_CID_ENCODER_BIT_RATE,
		.ctrl_selector = EN_BIT_RATE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_QUALITY,
		.ctrl_selector = EN_QUALITY_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_VBR,
		.ctrl_selector = EN_VBR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_TYPE,
		.ctrl_selector = EN_TYPE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_ERROR,
		.ctrl_selector = EN_ENCODER_ERROR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM1,
		.ctrl_selector = EN_PARAM1_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM2,
		.ctrl_selector = EN_PARAM2_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM3,
		.ctrl_selector = EN_PARAM3_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM4,
		.ctrl_selector = EN_PARAM4_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM5,
		.ctrl_selector = EN_PARAM5_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM6,
		.ctrl_selector = EN_PARAM6_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM7,
		.ctrl_selector = EN_PARAM7_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_ENCODER_PARAM8,
		.ctrl_selector = EN_PARAM8_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_3,
		.is_signed = false,
	},
};

/* Endpoint controls */
static const struct uac2_ctrl_mapping ctrl_map_endpoint[] = {
	{
		.cid = AUDIO_CID_PITCH,
		.ctrl_selector = EP_PITCH_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_DATA_OVERRUN,
		.ctrl_selector = EP_DATA_OVERRUN_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_DATA_UNDERRUN,
		.ctrl_selector = EP_DATA_UNDERRUN_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
};

#if defined(CONFIG_USBH_UAC2_COMPRESSED_AUDIO_CONTROLS)
/* MPEG Decoder controls */
static const struct uac2_ctrl_mapping ctrl_map_decoder_mpeg[] = {
	{
		.cid = AUDIO_CID_MPEG_DUAL_CHANNEL,
		.ctrl_selector = MD_DUAL_CHANNEL_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_MPEG_SECOND_STEREO,
		.ctrl_selector = MD_SECOND_STEREO_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_MPEG_MULTILINGUAL,
		.ctrl_selector = MD_MULTILINGUAL_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_MPEG_DYN_RANGE,
		.ctrl_selector = MD_DYN_RANGE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_MPEG_SCALING,
		.ctrl_selector = MD_SCALING_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_MPEG_HILO_SCALING,
		.ctrl_selector = MD_HILO_SCALING_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_MPEG_DECODER_ERROR,
		.ctrl_selector = MD_DECODER_ERROR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
};

/* AC-3 Decoder controls */
static const struct uac2_ctrl_mapping ctrl_map_decoder_ac3[] = {
	{
		.cid = AUDIO_CID_AC3_MODE,
		.ctrl_selector = AD_MODE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_AC3_DYN_RANGE,
		.ctrl_selector = AD_DYN_RANGE_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_AC3_SCALING,
		.ctrl_selector = AD_SCALING_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_AC3_HILO_SCALING,
		.ctrl_selector = AD_HILO_SCALING_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
	{
		.cid = AUDIO_CID_AC3_DECODER_ERROR,
		.ctrl_selector = AD_DECODER_ERROR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
};

/* WMA Decoder controls */
static const struct uac2_ctrl_mapping ctrl_map_decoder_wma[] = {
	{
		.cid = AUDIO_CID_WMA_DECODER_ERROR,
		.ctrl_selector = WD_DECODER_ERROR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
};

/* DTS Decoder controls */
static const struct uac2_ctrl_mapping ctrl_map_decoder_dts[] = {
	{
		.cid = AUDIO_CID_DTS_DECODER_ERROR,
		.ctrl_selector = DD_DECODER_ERROR_CONTROL,
		.layout = UAC2_PARAM_BLOCK_LAYOUT_1,
		.is_signed = false,
	},
};
#endif /* CONFIG_USBH_UAC2_COMPRESSED_AUDIO_CONTROLS */

/* All control mappings */
static const struct {
	const struct uac2_ctrl_mapping *map;
	size_t count;
} all_ctrl_maps[] = {
	{ ctrl_map_clock_source, ARRAY_SIZE(ctrl_map_clock_source) },
	{ ctrl_map_clock_selector, ARRAY_SIZE(ctrl_map_clock_selector) },
	{ ctrl_map_clock_multiplier, ARRAY_SIZE(ctrl_map_clock_multiplier) },
	{ ctrl_map_terminal, ARRAY_SIZE(ctrl_map_terminal) },
	{ ctrl_map_mixer, ARRAY_SIZE(ctrl_map_mixer) },
	{ ctrl_map_selector, ARRAY_SIZE(ctrl_map_selector) },
	{ ctrl_map_feature, ARRAY_SIZE(ctrl_map_feature) },
#if defined(CONFIG_USBH_UAC2_AUDIO_EFFECT_CONTROLS)
	{ ctrl_map_effect_pe, ARRAY_SIZE(ctrl_map_effect_pe) },
	{ ctrl_map_effect_rv, ARRAY_SIZE(ctrl_map_effect_rv) },
	{ ctrl_map_effect_md, ARRAY_SIZE(ctrl_map_effect_md) },
	{ ctrl_map_effect_dr, ARRAY_SIZE(ctrl_map_effect_dr) },
#endif /* CONFIG_USBH_UAC2_AUDIO_EFFECT_CONTROLS */
#if defined(CONFIG_USBH_UAC2_AUDIO_PROCESSING_CONTROLS)
	{ ctrl_map_processing_ud, ARRAY_SIZE(ctrl_map_processing_ud) },
	{ ctrl_map_proc_dp, ARRAY_SIZE(ctrl_map_proc_dp) },
	{ ctrl_map_proc_st, ARRAY_SIZE(ctrl_map_proc_st) },
#endif /* CONFIG_USBH_UAC2_AUDIO_PROCESSING_CONTROLS */
	{ ctrl_map_extension, ARRAY_SIZE(ctrl_map_extension) },
	{ ctrl_map_as_interface, ARRAY_SIZE(ctrl_map_as_interface) },
	{ ctrl_map_encoder, ARRAY_SIZE(ctrl_map_encoder) },
	{ ctrl_map_endpoint, ARRAY_SIZE(ctrl_map_endpoint) },
#if defined(CONFIG_USBH_UAC2_COMPRESSED_AUDIO_CONTROLS)
	{ ctrl_map_decoder_mpeg, ARRAY_SIZE(ctrl_map_decoder_mpeg) },
	{ ctrl_map_decoder_ac3, ARRAY_SIZE(ctrl_map_decoder_ac3) },
	{ ctrl_map_decoder_wma, ARRAY_SIZE(ctrl_map_decoder_wma) },
	{ ctrl_map_decoder_dts, ARRAY_SIZE(ctrl_map_decoder_dts) },
#endif /* CONFIG_USBH_UAC2_COMPRESSED_AUDIO_CONTROLS */
};

static int audio_iso_in_req_cb(struct usb_device *const dev,
			       struct uhc_transfer *const xfer);
static int audio_iso_out_req_cb(struct usb_device *const dev,
				struct uhc_transfer *const xfer);

/* Find control mapping by CID */
static const struct uac2_ctrl_mapping *find_ctrl_mapping_by_cid(uint32_t cid)
{
	for (size_t i = 0; i < ARRAY_SIZE(all_ctrl_maps); i++) {
		for (size_t j = 0; j < all_ctrl_maps[i].count; j++) {
			if (all_ctrl_maps[i].map[j].cid == cid) {
				return &all_ctrl_maps[i].map[j];
			}
		}
	}

	return NULL;
}

/* Find entity descriptor in audio path by subtype */
static const void *find_entity_in_path(struct uac2_host_data *host_data,
				       enum usbh_uac2_stream_dir dir,
				       uint8_t unit_subtype)
{
	const struct uac2_device_descriptors *desc;

	if (dir == USBH_CAPTURE) {
		desc = &host_data->in_descriptors;
	} else {
		desc = &host_data->out_descriptors;
	}

	if (TERMINAL_TYPE_IS_ENCODED(unit_subtype)) {
		unit_subtype = (dir == USBH_CAPTURE)
				? TERMINAL_TYPE_CAPTURE(unit_subtype)
				: TERMINAL_TYPE_PLAYBACK(unit_subtype);
		if (unit_subtype == 0) {
			LOG_ERR("Control not applicable for %s stream",
				dir == USBH_CAPTURE ? "Capture" : "Playback");
			return NULL;
		}
	}

	switch (unit_subtype) {
	case AC_DESCRIPTOR_CLOCK_SOURCE:
		return desc->clock_source;

	case AC_DESCRIPTOR_CLOCK_SELECTOR:
		return desc->clock_selector;

	case AC_DESCRIPTOR_CLOCK_MULTIPLIER:
		return desc->clock_multiplier;

	case AC_DESCRIPTOR_INPUT_TERMINAL:
		return desc->input_terminal;

	case AC_DESCRIPTOR_OUTPUT_TERMINAL:
		return desc->output_terminal;

	case AC_DESCRIPTOR_MIXER_UNIT:
		return desc->mixer_unit;

	case AC_DESCRIPTOR_SELECTOR_UNIT:
		return desc->selector_unit;

	case AC_DESCRIPTOR_FEATURE_UNIT:
		return desc->feature_unit;

	case AC_DESCRIPTOR_EFFECT_UNIT:
		return desc->effect_unit;

	case AC_DESCRIPTOR_PROCESSING_UNIT:
		return desc->processing_unit;

	case AC_DESCRIPTOR_EXTENSION_UNIT:
		return desc->extension_unit;

	default:
		LOG_WRN("Unknown unit subtype: 0x%02x", unit_subtype);
		return NULL;
	}
}

/* Get entity ID from descriptor */
static uint8_t get_entity_id(const void *entity, uint8_t unit_subtype)
{
	switch (unit_subtype) {
	case AC_DESCRIPTOR_CLOCK_SOURCE:
		return ((const struct uac2_clock_source_descriptor *)entity)
			->bClockID;
	case AC_DESCRIPTOR_CLOCK_SELECTOR:
		return ((const struct uac2_clock_selector_descriptor *)entity)
			->bClockID;
	case AC_DESCRIPTOR_CLOCK_MULTIPLIER:
		return ((const struct uac2_clock_multiplier_descriptor *)entity)
			->bClockID;
	case AC_DESCRIPTOR_INPUT_TERMINAL:
		return ((const struct uac2_input_terminal_descriptor *)entity)
			->bTerminalID;
	case AC_DESCRIPTOR_OUTPUT_TERMINAL:
		return ((const struct uac2_output_terminal_descriptor *)entity)
			->bTerminalID;
	case AC_DESCRIPTOR_MIXER_UNIT:
		return ((const struct uac2_mixer_unit_descriptor *)entity)->bUnitID;
	case AC_DESCRIPTOR_SELECTOR_UNIT:
		return ((const struct uac2_selector_unit_descriptor *)entity)->bUnitID;
	case AC_DESCRIPTOR_FEATURE_UNIT:
		return ((const struct uac2_feature_unit_descriptor *)entity)->bUnitID;
	case AC_DESCRIPTOR_EFFECT_UNIT:
		return ((const struct uac2_effect_unit_descriptor *)entity)->bUnitID;
	case AC_DESCRIPTOR_PROCESSING_UNIT:
		return ((const struct uac2_processing_unit_descriptor *)entity)->bUnitID;
	case AC_DESCRIPTOR_EXTENSION_UNIT:
		return ((const struct uac2_extension_unit_descriptor *)entity)->bUnitID;
	default:
		return 0;
	}
}

static bool ac_header_is_valid(const void *const desc)
{
	const struct uac2_ac_header_descriptor *const header_desc = desc;

	if (!usbh_desc_is_valid(desc, sizeof(struct uac2_ac_header_descriptor),
				USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return header_desc->bDescriptorSubtype == AC_DESCRIPTOR_HEADER;
}

static const void *get_desc_ac_end(const struct usb_if_descriptor *const if_desc)
{
	const struct uac2_ac_header_descriptor *header;
	uint16_t total_length;

	if (!usbh_desc_is_valid_interface(if_desc)) {
		LOG_ERR("Invalid interface descriptor");
		return NULL;
	}

	header = (const void *)usbh_desc_get_next(if_desc);
	if (!ac_header_is_valid(header)) {
		LOG_ERR("Invalid AC header descriptor");
		return NULL;
	}

	total_length = sys_le16_to_cpu(header->wTotalLength);
	return (const uint8_t *)header + total_length;
}

/* Check if terminal type is USB Streaming */
static bool is_usb_streaming_terminal(uint16_t terminal_type)
{
	return (terminal_type == USB_TERMINAL_STREAMING);
}

static const void *find_entity_by_id(const struct usb_if_descriptor *ctrl_iface,
				     const void *ctrl_end,
				     uint8_t entity_id,
				     uint8_t descriptor_subtype)
{
	const struct usb_desc_header *desc = (const void *)ctrl_iface;
	const struct usb_cs_desc_header *cs_desc;
	uint8_t desc_id = 0;

	while ((desc = usbh_desc_get_next(desc)) != NULL &&
	       (const uint8_t *)desc < (const uint8_t *)ctrl_end) {

		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType != CS_INTERFACE) {
			continue;
		}

		cs_desc = (const void *)desc;

		if (cs_desc->bDescriptorSubtype != descriptor_subtype) {
			continue;
		}

		switch (descriptor_subtype) {
		case AC_DESCRIPTOR_CLOCK_SOURCE:
			desc_id = ((const struct uac2_clock_source_descriptor *)desc)
				->bClockID;
			break;
		case AC_DESCRIPTOR_INPUT_TERMINAL:
			desc_id = ((const struct uac2_input_terminal_descriptor *)desc)
				->bTerminalID;
			break;
		case AC_DESCRIPTOR_OUTPUT_TERMINAL:
			desc_id = ((const struct uac2_output_terminal_descriptor *)desc)
				->bTerminalID;
			break;
		case AC_DESCRIPTOR_MIXER_UNIT:
			desc_id = ((const struct uac2_mixer_unit_descriptor *)desc)
				->bUnitID;
			break;
		case AC_DESCRIPTOR_SELECTOR_UNIT:
			desc_id = ((const struct uac2_selector_unit_descriptor *)desc)
				->bUnitID;
			break;
		case AC_DESCRIPTOR_FEATURE_UNIT:
			desc_id = ((const struct uac2_feature_unit_descriptor *)desc)
				->bUnitID;
			break;
		case AC_DESCRIPTOR_EFFECT_UNIT:
			desc_id = ((const struct uac2_effect_unit_descriptor *)desc)
				->bUnitID;
			break;
		case AC_DESCRIPTOR_PROCESSING_UNIT:
			desc_id = ((const struct uac2_processing_unit_descriptor *)desc)
				->bUnitID;
			break;
		case AC_DESCRIPTOR_EXTENSION_UNIT:
			desc_id = ((const struct uac2_extension_unit_descriptor *)desc)
				->bUnitID;
			break;
		default:
			continue;
		}

		if (desc_id == entity_id) {
			return desc;
		}
	}

	return NULL;
}

static const void *find_unit_by_source_id(const struct usb_if_descriptor *ctrl_iface,
					const void *ctrl_end,
					uint8_t source_id,
					uint8_t unit_subtype)
{
	const struct usb_desc_header *desc = (const void *)ctrl_iface;
	const struct usb_cs_desc_header *cs_desc;

	while ((desc = usbh_desc_get_next(desc)) != NULL &&
	       (const uint8_t *)desc < (const uint8_t *)ctrl_end) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType != CS_INTERFACE) {
			continue;
		}

		cs_desc = (const void *)desc;
		if (cs_desc->bDescriptorSubtype != unit_subtype) {
			continue;
		}

		switch (unit_subtype) {
		case AC_DESCRIPTOR_FEATURE_UNIT: {
			const struct uac2_feature_unit_descriptor *fu;

			fu = (const void *)desc;
			if (fu->bSourceID == source_id) {
				return fu;
			}
			break;
		}

		case AC_DESCRIPTOR_OUTPUT_TERMINAL: {
			const struct uac2_output_terminal_descriptor *ot;

			ot = (const void *)desc;
			if (ot->bSourceID == source_id) {
				return ot;
			}
			break;
		}

		case AC_DESCRIPTOR_EFFECT_UNIT: {
			const struct uac2_effect_unit_descriptor *eu;

			eu = (const void *)desc;
			if (eu->bSourceID == source_id) {
				return eu;
			}
			break;
		}

		case AC_DESCRIPTOR_MIXER_UNIT: {
			const struct uac2_mixer_unit_descriptor *mu;
			const uint8_t *baSourceID;

			mu = (const void *)desc;
			baSourceID = (const uint8_t *)mu + 5;
			for (uint8_t i = 0; i < mu->bNrInPins; i++) {
				if (baSourceID[i] == source_id) {
					return mu;
				}
			}
			break;
		}

		case AC_DESCRIPTOR_SELECTOR_UNIT: {
			const struct uac2_selector_unit_descriptor *su;
			const uint8_t *baSourceID;

			su = (const void *)desc;
			baSourceID = (const uint8_t *)su + 5;
			for (uint8_t i = 0; i < su->bNrInPins; i++) {
				if (baSourceID[i] == source_id) {
					return su;
				}
			}
			break;
		}

		case AC_DESCRIPTOR_PROCESSING_UNIT: {
			const struct uac2_processing_unit_descriptor *pu;
			const uint8_t *baSourceID;

			pu = (const void *)desc;
			baSourceID = (const uint8_t *)pu + 7;
			for (uint8_t i = 0; i < pu->bNrInPins; i++) {
				if (baSourceID[i] == source_id) {
					return pu;
				}
			}
			break;
		}

		case AC_DESCRIPTOR_EXTENSION_UNIT: {
			const struct uac2_extension_unit_descriptor *xu;
			const uint8_t *baSourceID;

			xu = (const void *)desc;
			baSourceID = (const uint8_t *)xu + 7;
			for (uint8_t i = 0; i < xu->bNrInPins; i++) {
				if (baSourceID[i] == source_id) {
					return xu;
				}
			}
			break;
		}

		default:
			break;
		}
	}

	return NULL;
}

static int parse_playback_path(const struct usb_if_descriptor *ctrl_iface,
			       const void *ctrl_end,
			       const struct uac2_input_terminal_descriptor *it,
			       struct uac2_device_descriptors *descriptors,
			       uint8_t *clock_source_id)
{
	const struct uac2_output_terminal_descriptor *ot;
	const struct uac2_feature_unit_descriptor *fu;
	const struct uac2_mixer_unit_descriptor *mu;
	const struct uac2_effect_unit_descriptor *eu;
	const struct uac2_processing_unit_descriptor *pu;
	const struct uac2_extension_unit_descriptor *xu;
	uint8_t entity_id;
	uint16_t terminal_type;

	LOG_DBG("USB Streaming IT ID=%u -> Playback path", it->bTerminalID);

	descriptors->input_terminal = it;
	*clock_source_id = it->bCSourceID;
	entity_id = it->bTerminalID;

	eu = find_unit_by_source_id(ctrl_iface, ctrl_end, entity_id,
				     AC_DESCRIPTOR_EFFECT_UNIT);
	if (eu != NULL) {
		descriptors->effect_unit = eu;
		LOG_DBG("EU: ID=%u Type=0x%04x", eu->bUnitID,
			sys_le16_to_cpu(eu->wEffectType));
		entity_id = eu->bUnitID;
	}

	pu = find_unit_by_source_id(ctrl_iface, ctrl_end, entity_id,
				     AC_DESCRIPTOR_PROCESSING_UNIT);
	if (pu != NULL) {
		descriptors->processing_unit = pu;
		LOG_DBG("PU: ID=%u Type=0x%04x", pu->bUnitID,
			sys_le16_to_cpu(pu->wProcessType));
		entity_id = pu->bUnitID;
	}

	xu = find_unit_by_source_id(ctrl_iface, ctrl_end, entity_id,
				     AC_DESCRIPTOR_EXTENSION_UNIT);
	if (xu != NULL) {
		descriptors->extension_unit = xu;
		LOG_DBG("XU: ID=%u", xu->bUnitID);
		entity_id = xu->bUnitID;
	}

	mu = find_unit_by_source_id(ctrl_iface, ctrl_end, entity_id,
				     AC_DESCRIPTOR_MIXER_UNIT);
	if (mu != NULL) {
		descriptors->mixer_unit = mu;
		LOG_DBG("MU: ID=%u", mu->bUnitID);
		entity_id = mu->bUnitID;
	}

	fu = find_unit_by_source_id(ctrl_iface, ctrl_end, entity_id,
				     AC_DESCRIPTOR_FEATURE_UNIT);
	if (fu != NULL) {
		descriptors->feature_unit = fu;
		LOG_DBG("FU: ID=%u", fu->bUnitID);
		entity_id = fu->bUnitID;
	}

	ot = find_unit_by_source_id(ctrl_iface, ctrl_end, entity_id,
				     AC_DESCRIPTOR_OUTPUT_TERMINAL);
	if (ot != NULL) {
		terminal_type = sys_le16_to_cpu(ot->wTerminalType);

		if (is_usb_streaming_terminal(terminal_type)) {
			LOG_ERR("Playback path cannot have USB Streaming OT");
			return -EINVAL;
		}

		descriptors->output_terminal = ot;
		LOG_DBG("OT: ID=%u Type=0x%04x", ot->bTerminalID, terminal_type);

		if (*clock_source_id == 0) {
			*clock_source_id = ot->bCSourceID;
		}
	}

	return 0;
}

static int parse_capture_path(const struct usb_if_descriptor *ctrl_iface,
			      const void *ctrl_end,
			      const struct uac2_output_terminal_descriptor *ot,
			      struct uac2_device_descriptors *descriptors,
			      uint8_t *clock_source_id)
{
	const struct uac2_input_terminal_descriptor *it;
	const struct uac2_feature_unit_descriptor *fu;
	const struct uac2_selector_unit_descriptor *su;
	const struct uac2_effect_unit_descriptor *eu;
	const struct uac2_processing_unit_descriptor *pu;
	const struct uac2_extension_unit_descriptor *xu;
	const uint8_t *baSourceID;
	uint8_t entity_id;
	uint16_t terminal_type;

	LOG_DBG("USB Streaming OT ID=%u -> Capture path", ot->bTerminalID);

	descriptors->output_terminal = ot;
	*clock_source_id = ot->bCSourceID;
	entity_id = ot->bSourceID;

	su = find_entity_by_id(ctrl_iface, ctrl_end, entity_id,
			       AC_DESCRIPTOR_SELECTOR_UNIT);
	if (su != NULL) {
		descriptors->selector_unit = su;
		LOG_DBG("SU: ID=%u", su->bUnitID);

		baSourceID = (const uint8_t *)su + 5;
		entity_id = baSourceID[0];
	}

	fu = find_entity_by_id(ctrl_iface, ctrl_end, entity_id,
			       AC_DESCRIPTOR_FEATURE_UNIT);
	if (fu != NULL) {
		descriptors->feature_unit = fu;
		LOG_DBG("FU: ID=%u", fu->bUnitID);
		entity_id = fu->bSourceID;
	}

	xu = find_entity_by_id(ctrl_iface, ctrl_end, entity_id,
			       AC_DESCRIPTOR_EXTENSION_UNIT);
	if (xu != NULL) {
		descriptors->extension_unit = xu;
		LOG_DBG("XU: ID=%u", xu->bUnitID);

		baSourceID = (const uint8_t *)xu + 7;
		if (xu->bNrInPins > 0) {
			entity_id = baSourceID[0];
		}
	}

	pu = find_entity_by_id(ctrl_iface, ctrl_end, entity_id,
			       AC_DESCRIPTOR_PROCESSING_UNIT);
	if (pu != NULL) {
		descriptors->processing_unit = pu;
		LOG_DBG("PU: ID=%u, Type=0x%04x", pu->bUnitID,
			sys_le16_to_cpu(pu->wProcessType));

		baSourceID = (const uint8_t *)pu + 7;
		if (pu->bNrInPins > 0) {
			entity_id = baSourceID[0];
		}
	}

	eu = find_entity_by_id(ctrl_iface, ctrl_end, entity_id,
			       AC_DESCRIPTOR_EFFECT_UNIT);
	if (eu != NULL) {
		descriptors->effect_unit = eu;
		LOG_DBG("EU: ID=%u, Type=0x%04x", eu->bUnitID,
			sys_le16_to_cpu(eu->wEffectType));
		entity_id = eu->bSourceID;
	}

	it = find_entity_by_id(ctrl_iface, ctrl_end, entity_id,
			       AC_DESCRIPTOR_INPUT_TERMINAL);
	if (it != NULL) {
		terminal_type = sys_le16_to_cpu(it->wTerminalType);

		if (is_usb_streaming_terminal(terminal_type)) {
			LOG_ERR("Capture path has USB Streaming IT");
			return -EINVAL;
		}

		descriptors->input_terminal = it;
		LOG_DBG("IT: ID=%u, Type=0x%04x", it->bTerminalID, terminal_type);

		if (it->bCSourceID != 0) {
			*clock_source_id = it->bCSourceID;
		}
	}

	if (*clock_source_id == 0) {
		*clock_source_id = ot->bCSourceID;
		LOG_DBG("Using OT bCSourceID=%u as fallback", *clock_source_id);
	}

	return 0;
}

static int parse_ac_descriptors(struct uac2_host_data *const host_data,
				const struct usb_if_descriptor *ctrl_iface,
				const void *ctrl_end,
				uint8_t terminal_link_id,
				struct uac2_device_descriptors *descriptors)
{
	const struct uac2_input_terminal_descriptor *it;
	const struct uac2_output_terminal_descriptor *ot;
	const struct uac2_clock_source_descriptor *clk;
	uint8_t clock_source_id = 0;
	uint16_t terminal_type;
	int ret;

	LOG_DBG("Tracing audio path from Terminal Link ID=%u", terminal_link_id);

	it = find_entity_by_id(ctrl_iface, ctrl_end, terminal_link_id,
			       AC_DESCRIPTOR_INPUT_TERMINAL);

	if (it != NULL) {
		terminal_type = sys_le16_to_cpu(it->wTerminalType);
		if (!is_usb_streaming_terminal(terminal_type)) {
			LOG_ERR("IT ID=%u is not USB Streaming (Type=0x%04x)",
				it->bTerminalID, terminal_type);
			return -EINVAL;
		}

		ret = parse_playback_path(ctrl_iface, ctrl_end, it, descriptors,
					  &clock_source_id);
		if (ret != 0) {
			return ret;
		}
	} else {
		ot = find_entity_by_id(ctrl_iface, ctrl_end, terminal_link_id,
				       AC_DESCRIPTOR_OUTPUT_TERMINAL);
		if (ot == NULL) {
			LOG_ERR("Terminal ID=%u not found", terminal_link_id);
			return -ENOENT;
		}

		terminal_type = sys_le16_to_cpu(ot->wTerminalType);
		if (!is_usb_streaming_terminal(terminal_type)) {
			LOG_ERR("OT ID=%u is not USB Streaming (Type=0x%04x)",
				ot->bTerminalID, terminal_type);
			return -EINVAL;
		}

		ret = parse_capture_path(ctrl_iface, ctrl_end, ot, descriptors,
					 &clock_source_id);
		if (ret != 0) {
			return ret;
		}
	}

	if (clock_source_id == 0) {
		LOG_ERR("No Clock Source ID found in audio path");
		return -EINVAL;
	}

	clk = find_entity_by_id(ctrl_iface, ctrl_end, clock_source_id,
				AC_DESCRIPTOR_CLOCK_SOURCE);
	if (clk == NULL) {
		LOG_ERR("Clock Source ID=%u not found", clock_source_id);
		return -ENOENT;
	}

	descriptors->clock_source = clk;
	LOG_DBG("Clock Source ID=%u found", clk->bClockID);

	return 0;
}

/* Get AS General descriptor from AS interface */
static const struct uac2_as_general_descriptor *
get_as_general_descriptor(const void *const if_desc, const void *const as_end)
{
	const struct usb_desc_header *desc = usbh_desc_get_next(if_desc);
	const struct usb_cs_desc_header *cs_desc;

	for (; desc != NULL && (const uint8_t *)desc < (const uint8_t *)as_end;
	     desc = usbh_desc_get_next(desc)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType != CS_INTERFACE) {
			continue;
		}

		cs_desc = (const void *)desc;
		if (cs_desc->bDescriptorSubtype == AS_DESCRIPTOR_GENERAL) {
			return (const struct uac2_as_general_descriptor *)desc;
		}
	}

	return NULL;
}

/* Determine stream direction and assign descriptors */
static int assign_stream_by_terminal(struct uac2_host_data *const host_data,
				     const void *ctrl_end,
				     uint8_t terminal_link_id,
				     struct uac2_stream_iface_info **stream_info,
				     struct uac2_device_descriptors **descriptors)
{
	const struct uac2_input_terminal_descriptor *it;
	const struct uac2_output_terminal_descriptor *ot;
	uint16_t terminal_type;

	/* Try Input Terminal first */
	it = find_entity_by_id(host_data->current_ctrl_iface, ctrl_end,
			       terminal_link_id, AC_DESCRIPTOR_INPUT_TERMINAL);

	if (it != NULL) {
		terminal_type = sys_le16_to_cpu(it->wTerminalType);

		if (!is_usb_streaming_terminal(terminal_type)) {
			LOG_ERR("IT ID=%u is not USB Streaming (Type=0x%04x)",
				terminal_link_id, terminal_type);
			return -EINVAL;
		}

		/* USB Streaming IT -> Playback (Speaker) */
		if (host_data->out_stream_iface.iface != NULL) {
			LOG_DBG("Playback stream already assigned");
			return -EEXIST;
		}

		*stream_info = &host_data->out_stream_iface;
		*descriptors = &host_data->out_descriptors;

		LOG_DBG("Found Playback stream: IT ID=%u, Type=0x%04x",
			it->bTerminalID, terminal_type);
		return 0;
	}

	/* Try Output Terminal */
	ot = find_entity_by_id(host_data->current_ctrl_iface, ctrl_end,
			       terminal_link_id, AC_DESCRIPTOR_OUTPUT_TERMINAL);

	if (ot != NULL) {
		terminal_type = sys_le16_to_cpu(ot->wTerminalType);

		if (!is_usb_streaming_terminal(terminal_type)) {
			LOG_ERR("OT ID=%u is not USB Streaming (Type=0x%04x)",
				terminal_link_id, terminal_type);
			return -EINVAL;
		}

		/* USB Streaming OT -> Capture (Recorder) */
		if (host_data->in_stream_iface.iface != NULL) {
			LOG_WRN("Capture stream already assigned");
			return -EEXIST;
		}

		*stream_info = &host_data->in_stream_iface;
		*descriptors = &host_data->in_descriptors;

		LOG_DBG("Found Capture stream: OT ID=%u, Type=0x%04x",
			ot->bTerminalID, terminal_type);
		return 0;
	}

	LOG_ERR("Terminal ID=%u not found", terminal_link_id);
	return -ENOENT;
}

static int get_control_capability(const struct uac2_feature_unit_descriptor *const fu,
				  uint8_t channel, uint8_t ctrl_type)
{
	const uint8_t *controls_ptr;
	uint32_t controls;
	uint8_t num_ch;
	uint8_t shift;

	if (fu->bLength < 7) {
		return -EINVAL;
	}

	/*
	 * Calculate number of channels:
	 * bLength = 6 + (num_ch + 1) * 4 + 1
	 */
	num_ch = (fu->bLength - 7) / 4;

	if (channel > num_ch) {
		return -EINVAL;
	}

	/* Get controls bitmap for this channel (4 bytes per channel) */
	controls_ptr = (const uint8_t *)(fu + 1);
	controls_ptr += channel * 4;
	controls = sys_get_le32(controls_ptr);

	/* Each control uses 2 bits */
	if (ctrl_type < FU_MUTE_CONTROL || ctrl_type > FU_LATENCY_CONTROL) {
		return -EINVAL;
	}

	shift = (ctrl_type - FU_MUTE_CONTROL) * 2;

	return (controls >> shift) & 0x3;
}

/* Parse AudioStreaming descriptors */
static int parse_as_descriptors(struct uac2_host_data *const host_data,
				const struct usb_if_descriptor *if_desc,
				const void *ctrl_end,
				uint8_t *terminal_link_id,
				struct uac2_stream_iface_info **stream_info,
				struct uac2_device_descriptors **descriptors)
{
	const struct uac2_format_type_common_descriptor *common_format;
	const struct uac2_cs_as_iso_endpoint_descriptor *cs_ep;
	const struct uac2_as_general_descriptor *as_header;
	const struct uac2_format_type_i_descriptor *format;
	const struct uac2_as_general_descriptor *header;
	const struct usb_cs_desc_header *cs_desc;
	const struct usb_ep_descriptor *ep;
	const struct usb_desc_header *desc;
	const void *as_end;
	int ret;

	/* Get AS interface boundary */
	as_end = usbh_desc_get_next_function(if_desc);
	if (as_end == NULL) {
		const struct usb_cfg_descriptor *cfg = host_data->udev->cfg_desc;

		as_end = (const uint8_t *)cfg + sys_le16_to_cpu(cfg->wTotalLength);
	}

	/* Get AS General Descriptor */
	as_header = get_as_general_descriptor(if_desc, as_end);
	if (as_header == NULL) {
		LOG_ERR("No AS General Descriptor found");
		return -EINVAL;
	}

	*terminal_link_id = as_header->bTerminalLink;

	/* Determine direction and assign stream slot */
	ret = assign_stream_by_terminal(host_data, ctrl_end, *terminal_link_id,
					stream_info, descriptors);
	if (ret != 0) {
		return ret;
	}

	(*stream_info)->iface = if_desc;

	/* Parse AS interface descriptors */
	desc = usbh_desc_get_next(if_desc);

	for (; desc != NULL && (const uint8_t *)desc < (const uint8_t *)as_end;
	     desc = usbh_desc_get_next(desc)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			ep = (const void *)desc;

			if ((ep->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_ISO) {
				continue;
			}

			LOG_DBG("AudioStreaming Endpoint: 0x%02x", ep->bEndpointAddress);

			(*stream_info)->ep = ep;
			(*stream_info)->ep_mps = USB_MPS_EP_SIZE(ep->wMaxPacketSize);
			(*stream_info)->ep_interval = ep->bInterval;

			continue;
		}

		if (desc->bDescriptorType == CS_ENDPOINT) {
			cs_ep = (const void *)desc;

			LOG_DBG("CS Endpoint: bmAttributes=0x%02x, bmControls=0x%02x",
				cs_ep->bmAttributes, cs_ep->bmControls);

			(*stream_info)->cs_ep = cs_ep;

			continue;
		}

		if (desc->bDescriptorType != CS_INTERFACE) {
			LOG_WRN("AudioStreaming descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
			continue;
		}

		cs_desc = (const void *)desc;

		switch (cs_desc->bDescriptorSubtype) {
		case AS_DESCRIPTOR_GENERAL: {
			header = (const void *)desc;

			if (desc->bLength < sizeof(*header)) {
				LOG_ERR("Invalid AS header descriptor length: %u", desc->bLength);
				return -EINVAL;
			}

			(*descriptors)->as_header = header;
			LOG_DBG("Found AS Header: bTerminalLink=%u, Channels=%u",
				header->bTerminalLink, header->bNrChannels);
			break;
		}

		case AS_DESCRIPTOR_FORMAT_TYPE: {
			common_format = (const void *)desc;

			if (desc->bLength < sizeof(*common_format)) {
				LOG_ERR("Invalid format type descriptor length: %u", desc->bLength);
				return -EINVAL;
			}

			(*descriptors)->format_type = common_format;

			if (common_format->bFormatType == FORMAT_TYPE_I ||
			    common_format->bFormatType == EXT_FORMAT_TYPE_I) {
				format = (const void *)desc;

				LOG_DBG("Found Format Type I: SubslotSize=%u, BitRes=%u",
					format->bSubslotSize, format->bBitResolution);
			}
			break;
		}

		default:
			LOG_DBG("AudioStreaming descriptor: unknown subtype %u, skipping",
				cs_desc->bDescriptorSubtype);
			break;
		}
	}

	return 0;
}

/* Parse AudioControl interface header */
static int parse_ac_header(struct uac2_host_data *const host_data,
			   const struct usb_if_descriptor *if_desc)
{
	const struct usb_desc_header *header;

	header = usbh_desc_get_next(if_desc);
	if (!ac_header_is_valid(header)) {
		LOG_ERR("Invalid AC header descriptor");
		return -EINVAL;
	}

	/* Save AC header descriptor for later use */
	host_data->in_descriptors.ac_header = (const void *)header;
	host_data->out_descriptors.ac_header = (const void *)header;

	LOG_DBG("AudioControl interface %u parsed", if_desc->bInterfaceNumber);

	return 0;
}

static int parse_descriptors(struct usbh_class_data *const c_data, uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct uac2_host_data *const host_data = dev->data;
	const struct usb_association_descriptor *iad_desc;
	struct uac2_device_descriptors *descriptors = NULL;
	struct uac2_stream_iface_info *stream_info = NULL;
	const struct usb_desc_header *desc_start;
	const struct usb_if_descriptor *if_desc;
	const struct usb_desc_header *header;
	const void *desc_end = NULL;
	const void *ctrl_end = NULL;
	uint8_t terminal_link_id;
	int ret;

	desc_start = usbh_desc_get_iface(host_data->udev, iface);
	if (desc_start == NULL) {
		LOG_ERR("Interface %u not found", iface);
		return -ENOENT;
	}

	iad_desc = (const void *)usbh_desc_get_iad(host_data->udev, iface);
	if (iad_desc != NULL) {
		desc_start = (const void *)iad_desc;
	}

	desc_end = usbh_desc_get_next_function(desc_start);
	if (desc_end == NULL) {
		LOG_ERR("Failed to find end of function descriptors");
		return -ENOENT;
	}

	for (header = desc_start;
	     header != NULL && (void *)header < desc_end;
	     header = usbh_desc_get_next(header)) {
		if (!usbh_desc_is_valid_interface(header)) {
			continue;
		}

		if_desc = (const void *)header;

		if (if_desc->bInterfaceClass != AUDIO) {
			LOG_DBG("Interface %u is not Audio class, skipping",
				if_desc->bInterfaceNumber);
			continue;
		}

		if (if_desc->bInterfaceSubClass == AUDIOCONTROL) {
			if (host_data->current_ctrl_iface != NULL) {
				LOG_WRN("Skipping extra AudioControl interface");
				break;
			}

			host_data->current_ctrl_iface = if_desc;
			ctrl_end = get_desc_ac_end(if_desc);
			ret = parse_ac_header(host_data, if_desc);
			if (ret != 0) {
				return ret;
			}
		}
	}

	if (host_data->current_ctrl_iface == NULL) {
		LOG_ERR("No AudioControl interface found");
		return -EINVAL;
	}

	for (header = desc_start;
	     header != NULL && (void *)header < desc_end;
	     header = usbh_desc_get_next(header)) {
		if (!usbh_desc_is_valid_interface(header)) {
			continue;
		}

		if_desc = (const void *)header;

		if (if_desc->bInterfaceClass != AUDIO ||
		    if_desc->bInterfaceSubClass != AUDIOSTREAMING) {
			continue;
		}

		if (if_desc->bAlternateSetting == 0) {
			LOG_DBG("Skipping AS interface %u alt 0",
				if_desc->bInterfaceNumber);
			continue;
		}

		ret = parse_as_descriptors(host_data, if_desc, ctrl_end,
					   &terminal_link_id, &stream_info,
					   &descriptors);
		if (ret == -EEXIST) {
			LOG_DBG("Skipping alt setting %u for interface %u "
				"(stream already assigned)",
				if_desc->bAlternateSetting,
				if_desc->bInterfaceNumber);
			continue;
		}

		if (ret != 0) {
			LOG_ERR("Failed to process AS interface %u: %d",
				if_desc->bInterfaceNumber, ret);
			return ret;
		}

		/* Parse AudioControl descriptors */
		ret = parse_ac_descriptors(host_data,
					   host_data->current_ctrl_iface,
					   ctrl_end, terminal_link_id,
					   descriptors);
		if (ret != 0) {
			LOG_ERR("Failed to parse AC descriptors for terminal %u: %d",
				terminal_link_id, ret);
			return ret;
		}
	}

	if (host_data->in_stream_iface.iface == NULL &&
	    host_data->out_stream_iface.iface == NULL) {
		LOG_ERR("No AudioStreaming interface found");
		return -EINVAL;
	}

	LOG_DBG("Interface %u associated with UAC2 class", iface);
	if (host_data->in_stream_iface.iface != NULL) {
		LOG_DBG("  ISO IN stream: interface %u",
			host_data->in_stream_iface.iface->bInterfaceNumber);
	}
	if (host_data->out_stream_iface.iface != NULL) {
		LOG_DBG("  ISO OUT stream: interface %u",
			host_data->out_stream_iface.iface->bInterfaceNumber);
	}

	return 0;
}

static void uac2_reset_session(struct uac2_host_data *host_data)
{
	size_t offset = offsetof(struct uac2_host_data, current_ctrl_iface);

	memset((uint8_t *)host_data + offset, 0, sizeof(*host_data) - offset);
}

static int configure_device(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct uac2_host_data *const host_data = dev->data;
	struct usb_device *const udev = host_data->udev;
	const struct usb_if_descriptor *ctrl_iface;
	int ret;

	if (host_data->current_ctrl_iface == NULL) {
		LOG_ERR("No control interface found");
		return -ENODEV;
	}

	ctrl_iface = host_data->current_ctrl_iface;

	ret = usbh_device_interface_set(udev, ctrl_iface->bInterfaceNumber, 0, false);
	if (ret != 0) {
		LOG_ERR("Failed to set control interface: %d", ret);
		return ret;
	}

	if (host_data->in_stream_iface.iface != NULL) {
		ret = usbh_device_interface_set(udev,
			host_data->in_stream_iface.iface->bInterfaceNumber, 0, false);
		if (ret != 0) {
			LOG_ERR("Failed to set IN streaming interface: %d", ret);
			return ret;
		}
	}

	if (host_data->out_stream_iface.iface != NULL) {
		ret = usbh_device_interface_set(udev,
			host_data->out_stream_iface.iface->bInterfaceNumber, 0, false);
		if (ret != 0) {
			LOG_ERR("Failed to set OUT streaming interface: %d", ret);
			return ret;
		}
	}

	LOG_INF("Device configured successfully");

	return 0;
}

static int init_format(struct uac2_host_data *const host_data)
{
	const struct uac2_format_type_i_descriptor *in_format;
	const struct uac2_format_type_i_descriptor *out_format;
	const struct uac2_as_general_descriptor *in_as_header;
	const struct uac2_as_general_descriptor *out_as_header;
	uint32_t default_sample_rate;

	if (host_data->in_stream_iface.iface != NULL) {
		in_as_header = host_data->in_descriptors.as_header;
		in_format = (const void *)host_data->in_descriptors.format_type;

		if (in_as_header == NULL || in_format == NULL) {
			LOG_ERR("Missing Capture AS descriptors");
			return -EINVAL;
		}

		if (host_data->in_frequency_info.is_valid &&
		    host_data->in_frequency_info.cur_frequency > 0) {
			default_sample_rate = host_data->in_frequency_info.cur_frequency;
		} else if (host_data->in_frequency_info.is_valid) {
			default_sample_rate = host_data->in_frequency_info.range.dMIN;
		} else {
			LOG_ERR("Capture: No valid sample rate information");
			return -EINVAL;
		}

		host_data->current_in_format.sample_rate = default_sample_rate;
		host_data->current_in_format.channels = in_as_header->bNrChannels;
		host_data->current_in_format.bit_resolution = in_format->bBitResolution;
		host_data->current_in_format.subslot_size = in_format->bSubslotSize;

		LOG_INF("Capture format: %u Hz, %u ch, %u bits",
			host_data->current_in_format.sample_rate,
			host_data->current_in_format.channels,
			host_data->current_in_format.bit_resolution);
	}

	if (host_data->out_stream_iface.iface != NULL) {
		out_as_header = host_data->out_descriptors.as_header;
		out_format = (const void *)host_data->out_descriptors.format_type;

		if (out_as_header == NULL || out_format == NULL) {
			LOG_ERR("Missing Playback AS descriptors");
			return -EINVAL;
		}

		if (host_data->out_frequency_info.is_valid &&
		    host_data->out_frequency_info.cur_frequency > 0) {
			default_sample_rate = host_data->out_frequency_info.cur_frequency;
		} else if (host_data->out_frequency_info.is_valid) {
			default_sample_rate = host_data->out_frequency_info.range.dMIN;
		} else {
			LOG_ERR("Playback: No valid sample rate information");
			return -EINVAL;
		}

		host_data->current_out_format.sample_rate = default_sample_rate;
		host_data->current_out_format.channels = out_as_header->bNrChannels;
		host_data->current_out_format.bit_resolution = out_format->bBitResolution;
		host_data->current_out_format.subslot_size = out_format->bSubslotSize;

		LOG_INF("Playback format: %u Hz, %u ch, %u bits",
			host_data->current_out_format.sample_rate,
			host_data->current_out_format.channels,
			host_data->current_out_format.bit_resolution);
	}

	return 0;
}

static uint32_t calculate_transfers_per_second(uint8_t bInterval,
					       enum usb_device_speed speed)
{
	uint32_t interval = 1U << (bInterval - 1);

	if (speed == USB_SPEED_SPEED_HS) {
		if (interval < 8) {
			return 1000 * (8 / interval);
		} else {
			return 1000 / (interval / 8);
		}
	}

	return 1000 / interval;
}

static uint32_t try_optimize_hs_interval(struct uac2_host_data *host_data,
					 uint32_t sample_rate,
					 uint8_t channels,
					 uint8_t bytes_per_sample)
{
	uint32_t interval = 1U << (host_data->out_stream_iface.ep_interval - 1);
	uint16_t ep_mps = host_data->out_stream_iface.ep_mps;
	uint32_t bytes_per_ms;
	uint32_t frame_size;

	if (interval >= 8) {
		return 0;
	}

	frame_size = channels * bytes_per_sample;
	bytes_per_ms = ((sample_rate + 999) / 1000) * frame_size;

	if (bytes_per_ms <= ep_mps) {
		LOG_DBG("Optimizing to 1ms interval");
		/* bInterval = 0x04: 2^(4-1) = 8 microframes = 1ms in HS */
		host_data->out_stream_iface.ep_interval = 0x04;
		return 1000;
	}

	if ((bytes_per_ms / 2) <= ep_mps) {
		LOG_DBG("Optimizing to 0.5ms interval");
		/* bInterval = 0x03: 2^(3-1) = 4 microframes = 0.5ms in HS */
		host_data->out_stream_iface.ep_interval = 0x03;
		return 2000;
	}

	return 0;
}

static void calculate_out_timing(struct uac2_host_data *host_data)
{
	uint8_t bytes_per_sample = host_data->current_out_format.subslot_size;
	uint32_t sample_rate = host_data->current_out_format.sample_rate;
	uint8_t channels = host_data->current_out_format.channels;
	struct uac2_transfer_timing *t = &host_data->out_timing;
	enum usb_device_speed speed;
	uint32_t transfers_per_sec;
	uint32_t optimized_tps;

	memset(t, 0, sizeof(*t));

	host_data->out_stream_iface.ep_interval =
		host_data->out_stream_iface.ep->bInterval;

	speed = host_data->udev->speed;
	transfers_per_sec = calculate_transfers_per_second(
		host_data->out_stream_iface.ep_interval, speed);

	if (speed == USB_SPEED_SPEED_HS) {
		optimized_tps = try_optimize_hs_interval(host_data,
							 sample_rate, channels,
							 bytes_per_sample);
		if (optimized_tps > 0) {
			transfers_per_sec = optimized_tps;
		}
	}

	LOG_DBG("Playback: %u Hz, %u transfers/sec", sample_rate, transfers_per_sec);

	t->base_sample_count = sample_rate / transfers_per_sec;
	if (t->base_sample_count * transfers_per_sec != sample_rate) {
		t->base_transfer_total = t->base_sample_count * transfers_per_sec +
					 transfers_per_sec - sample_rate;
		t->extra_transfer_total = transfers_per_sec - t->base_transfer_total;

		if (t->base_transfer_total >= t->extra_transfer_total) {
			t->toggle_ratio = t->base_transfer_total / t->extra_transfer_total;
		} else {
			t->toggle_ratio = t->extra_transfer_total / t->base_transfer_total;
		}

		LOG_DBG("Playback: Alternating %u/%u samples (ratio %u:1)",
			t->base_sample_count,
			t->base_sample_count + 1,
			t->toggle_ratio);
	} else {
		t->toggle_ratio = 0;
		LOG_DBG("Playback: Fixed %u samples per transfer", t->base_sample_count);
	}

	t->base_transfer_left = t->base_transfer_total;
	t->extra_transfer_left = t->extra_transfer_total;
	t->toggle_counter = 0;
}

static uint32_t get_out_sample_count(struct uac2_host_data *host_data)
{
	struct uac2_transfer_timing *t = &host_data->out_timing;
	uint32_t samples;

	if (t->toggle_ratio == 0) {
		return t->base_sample_count;
	}

	if (t->base_transfer_left > 0 && t->extra_transfer_left > 0) {
		if (t->base_transfer_total >= t->extra_transfer_total) {
			if (t->toggle_counter == t->toggle_ratio) {
				t->toggle_counter = 0;
				t->extra_transfer_left--;
				samples = t->base_sample_count + 1;
			} else {
				t->toggle_counter++;
				t->base_transfer_left--;
				samples = t->base_sample_count;
			}
		} else {
			if (t->toggle_counter == t->toggle_ratio) {
				t->toggle_counter = 0;
				t->base_transfer_left--;
				samples = t->base_sample_count;
			} else {
				t->toggle_counter++;
				t->extra_transfer_left--;
				samples = t->base_sample_count + 1;
			}
		}
	} else {
		/* Reset counters first if both exhausted, then start a new cycle. */
		if (t->base_transfer_left == 0 && t->extra_transfer_left == 0) {
			t->base_transfer_left = t->base_transfer_total;
			t->extra_transfer_left = t->extra_transfer_total;
			t->toggle_counter = 0;
		}

		if (t->base_transfer_left > 0) {
			t->base_transfer_left--;
			samples = t->base_sample_count;
		} else {
			t->extra_transfer_left--;
			samples = t->base_sample_count + 1;
		}
	}

	return samples;
}

static int audio_iso_in_req_cb(struct usb_device *const dev,
			       struct uhc_transfer *const xfer)
{
	struct uac2_host_data *const host_data = (void *)xfer->priv;
	struct usb_device *const udev = host_data->udev;
	struct net_buf *buf = xfer->buf;
	struct net_buf *new_buf;

	if (udev == NULL) {
		/* Device already removed; nothing safe to do with xfer. */
		return 0;
	}

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_IN)) {
		LOG_DBG("Device not streaming in, ignoring callback");
		goto cleanup;
	}

	if (xfer->err == -ECONNRESET) {
		LOG_DBG("ISO IN transfer canceled");
		goto cleanup;
	} else if (xfer->err) {
		LOG_WRN("ISO IN transfer failed: %d", xfer->err);
		goto cleanup;
	}

	LOG_DBG("Received %u bytes", buf->len);

	if (host_data->in_callback != NULL) {
		host_data->in_callback(buf->data, buf->len,
				       host_data->in_callback_param);
	}

cleanup:
	net_buf_unref(buf);

	if (atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_IN)) {
		new_buf = usbh_xfer_buf_alloc(host_data->udev,
					      host_data->in_stream_iface.ep_mps);
		if (new_buf != NULL) {
			new_buf->len = 0;
			xfer->buf = new_buf;
			usbh_xfer_enqueue(host_data->udev, xfer);
		} else {
			for (uint8_t i = 0;
			     i < host_data->in_transfer_count; i++) {
				if (host_data->audio_in_transfer[i] == xfer) {
					host_data->audio_in_transfer[i] = NULL;
					break;
				}
			}
			host_data->in_transfer_count--;
			usbh_xfer_free(host_data->udev, xfer);
			LOG_ERR("ISO IN: buffer alloc failed, transfer dropped");
		}
	} else {
		/* Streaming stopped: free the transfer to avoid leak. */
		usbh_xfer_free(host_data->udev, xfer);
	}

	return 0;
}

static int audio_iso_out_req_cb(struct usb_device *const dev,
				struct uhc_transfer *const xfer)
{
	struct uac2_host_data *const host_data = (void *)xfer->priv;
	struct usb_device *const udev = host_data->udev;
	struct net_buf *buf = xfer->buf;
	struct net_buf *new_buf;
	uint32_t sample_count;
	uint32_t buffer_size;
	uint16_t data_len;

	if (udev == NULL) {
		/* Device already removed; nothing safe to do with xfer. */
		return 0;
	}

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_OUT)) {
		LOG_DBG("Device not streaming out, ignoring callback");
		goto cleanup;
	}

	if (xfer->err == -ECONNRESET) {
		LOG_DBG("ISO OUT transfer canceled");
		goto cleanup;
	} else if (xfer->err) {
		LOG_WRN("ISO OUT transfer failed: %d", xfer->err);
		goto cleanup;
	}

	LOG_DBG("Sent %u bytes", buf->len);

cleanup:
	net_buf_unref(buf);

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_OUT)) {
		return 0;
	}

	sample_count = get_out_sample_count(host_data);

	buffer_size = sample_count *
		      host_data->current_out_format.channels *
		      host_data->current_out_format.subslot_size;

	new_buf = usbh_xfer_buf_alloc(host_data->udev, buffer_size);
	if (new_buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		usbh_xfer_free(host_data->udev, xfer);
		return -ENOMEM;
	}

	data_len = 0;
	if (host_data->out_callback != NULL) {
		data_len = host_data->out_callback(new_buf->data,
						   buffer_size,
						   host_data->out_callback_param);
	}

	new_buf->len = data_len;
	xfer->buf = new_buf;
	usbh_xfer_enqueue(host_data->udev, xfer);

	return 0;
}

static int start_streaming_in(struct uac2_host_data *const host_data)
{
	const struct usb_if_descriptor *stream_iface = host_data->in_stream_iface.iface;
	const struct usb_ep_descriptor *ep = host_data->in_stream_iface.ep;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	uint8_t alloc_cnt;
	int ret;

	if (stream_iface == NULL || ep == NULL) {
		LOG_ERR("No IN streaming interface configured");
		return -ENOTSUP;
	}

	ret = usbh_device_interface_set(host_data->udev,
					stream_iface->bInterfaceNumber,
					stream_iface->bAlternateSetting, false);
	if (ret != 0) {
		LOG_ERR("Failed to set IN streaming interface: %d", ret);
		return ret;
	}

	alloc_cnt = 0;

	for (uint8_t i = 0; i < CONFIG_USBH_UAC2_STREAM_IN_CONCURRENT_TRANSFERS; i++) {
		xfer = usbh_xfer_alloc(host_data->udev, ep->bEndpointAddress,
				       audio_iso_in_req_cb, host_data);
		if (xfer == NULL) {
			LOG_ERR("Failed to allocate IN transfer %u", i);
			ret = -ENOMEM;
			goto cleanup_on_error;
		}

		buf = usbh_xfer_buf_alloc(host_data->udev,
					  host_data->in_stream_iface.ep_mps);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate IN buffer %u", i);
			usbh_xfer_free(host_data->udev, xfer);
			ret = -ENOMEM;
			goto cleanup_on_error;
		}

		buf->len = 0;
		xfer->buf = buf;

		host_data->audio_in_transfer[i] = xfer;
		alloc_cnt = i + 1;

		ret = usbh_xfer_enqueue(host_data->udev, xfer);
		if (ret != 0) {
			LOG_ERR("Failed to enqueue IN transfer %u: %d", i, ret);
			net_buf_unref(buf);
			xfer->buf = NULL;
			goto cleanup_on_error;
		}

		host_data->in_transfer_count++;
	}

	atomic_set_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_IN);
	LOG_DBG("Audio Capture IN streaming started with %u transfers",
		host_data->in_transfer_count);

	return 0;

cleanup_on_error:
	LOG_ERR("Cleaning up %u allocated IN transfers", alloc_cnt);

	for (uint8_t i = 0; i < alloc_cnt; i++) {
		if (host_data->audio_in_transfer[i] == NULL) {
			continue;
		}

		usbh_xfer_dequeue(host_data->udev,
				  host_data->audio_in_transfer[i]);

		if (host_data->audio_in_transfer[i]->buf != NULL) {
			net_buf_unref(host_data->audio_in_transfer[i]->buf);
			host_data->audio_in_transfer[i]->buf = NULL;
		}

		usbh_xfer_free(host_data->udev, host_data->audio_in_transfer[i]);
		host_data->audio_in_transfer[i] = NULL;
	}

	host_data->in_transfer_count = 0;

	usbh_device_interface_set(host_data->udev,
				  stream_iface->bInterfaceNumber,
				  0, false);

	return ret;
}

static void cleanup_in_transfers(struct uac2_host_data *const host_data)
{
	for (uint8_t i = 0; i < host_data->in_transfer_count; i++) {
		if (host_data->audio_in_transfer[i] == NULL) {
			continue;
		}

		usbh_xfer_dequeue(host_data->udev,
				  host_data->audio_in_transfer[i]);

		if (host_data->audio_in_transfer[i]->buf != NULL) {
			net_buf_unref(host_data->audio_in_transfer[i]->buf);
			host_data->audio_in_transfer[i]->buf = NULL;
		}

		usbh_xfer_free(host_data->udev, host_data->audio_in_transfer[i]);
		host_data->audio_in_transfer[i] = NULL;
	}

	host_data->in_transfer_count = 0;
}

static int stop_streaming_in(struct uac2_host_data *const host_data)
{
	const struct usb_if_descriptor *stream_iface = host_data->in_stream_iface.iface;
	int ret;

	atomic_clear_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_IN);

	if (stream_iface != NULL) {
		ret = usbh_device_interface_set(host_data->udev,
						stream_iface->bInterfaceNumber,
						0, false);
		if (ret != 0) {
			LOG_ERR("Failed to set IN interface to idle: %d", ret);
		}
	}

	cleanup_in_transfers(host_data);

	LOG_DBG("Audio Capture IN streaming stopped");

	return 0;
}

static int start_streaming_out(struct uac2_host_data *const host_data)
{
	const struct usb_if_descriptor *stream_iface = host_data->out_stream_iface.iface;
	const struct usb_ep_descriptor *ep = host_data->out_stream_iface.ep;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	uint32_t sample_count;
	uint32_t buffer_size;
	uint16_t data_len;
	uint8_t alloc_cnt;
	int ret;

	if (stream_iface == NULL || ep == NULL) {
		LOG_ERR("No OUT streaming interface configured");
		return -ENOTSUP;
	}

	calculate_out_timing(host_data);

	ret = usbh_device_interface_set(host_data->udev,
					stream_iface->bInterfaceNumber,
					stream_iface->bAlternateSetting, false);
	if (ret != 0) {
		LOG_ERR("Failed to set OUT streaming interface: %d", ret);
		return ret;
	}

	alloc_cnt = 0;

	for (uint8_t i = 0; i < CONFIG_USBH_UAC2_STREAM_OUT_CONCURRENT_TRANSFERS; i++) {
		xfer = usbh_xfer_alloc(host_data->udev, ep->bEndpointAddress,
				       audio_iso_out_req_cb, host_data);
		if (xfer == NULL) {
			LOG_ERR("Failed to allocate OUT transfer %u", i);
			ret = -ENOMEM;
			goto cleanup_on_error;
		}

		xfer->interval = host_data->out_stream_iface.ep_interval;

		sample_count = get_out_sample_count(host_data);

		buffer_size = sample_count *
			      host_data->current_out_format.channels *
			      host_data->current_out_format.subslot_size;

		buf = usbh_xfer_buf_alloc(host_data->udev, buffer_size);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate OUT buffer %u", i);
			usbh_xfer_free(host_data->udev, xfer);
			ret = -ENOMEM;
			goto cleanup_on_error;
		}

		data_len = 0;
		if (host_data->out_callback != NULL) {
			data_len = host_data->out_callback(buf->data,
							   buffer_size,
							   host_data->out_callback_param);
		}

		buf->len = data_len;
		xfer->buf = buf;

		host_data->audio_out_transfer[i] = xfer;
		alloc_cnt = i + 1;

		ret = usbh_xfer_enqueue(host_data->udev, xfer);
		if (ret != 0) {
			LOG_ERR("Failed to enqueue OUT transfer %u: %d", i, ret);
			net_buf_unref(buf);
			xfer->buf = NULL;
			goto cleanup_on_error;
		}

		host_data->out_transfer_count++;
	}

	atomic_set_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_OUT);
	LOG_DBG("Audio Playback OUT streaming started with %u transfers",
		host_data->out_transfer_count);

	return 0;

cleanup_on_error:
	LOG_ERR("Cleaning up %u allocated OUT transfers", alloc_cnt);

	for (uint8_t i = 0; i < alloc_cnt; i++) {
		if (host_data->audio_out_transfer[i] == NULL) {
			continue;
		}

		usbh_xfer_dequeue(host_data->udev,
				  host_data->audio_out_transfer[i]);

		if (host_data->audio_out_transfer[i]->buf != NULL) {
			net_buf_unref(host_data->audio_out_transfer[i]->buf);
			host_data->audio_out_transfer[i]->buf = NULL;
		}

		usbh_xfer_free(host_data->udev, host_data->audio_out_transfer[i]);
		host_data->audio_out_transfer[i] = NULL;
	}

	host_data->out_transfer_count = 0;
	memset(&host_data->out_timing, 0, sizeof(host_data->out_timing));

	usbh_device_interface_set(host_data->udev,
				  stream_iface->bInterfaceNumber,
				  0, false);

	return ret;
}

static void cleanup_out_transfers(struct uac2_host_data *const host_data)
{
	for (uint8_t i = 0; i < host_data->out_transfer_count; i++) {
		if (host_data->audio_out_transfer[i] == NULL) {
			continue;
		}

		usbh_xfer_dequeue(host_data->udev,
				  host_data->audio_out_transfer[i]);

		if (host_data->audio_out_transfer[i]->buf != NULL) {
			net_buf_unref(host_data->audio_out_transfer[i]->buf);
			host_data->audio_out_transfer[i]->buf = NULL;
		}

		usbh_xfer_free(host_data->udev, host_data->audio_out_transfer[i]);
		host_data->audio_out_transfer[i] = NULL;
	}

	host_data->out_transfer_count = 0;
	memset(&host_data->out_timing, 0, sizeof(host_data->out_timing));
}

static int stop_streaming_out(struct uac2_host_data *const host_data)
{
	const struct usb_if_descriptor *stream_iface = host_data->out_stream_iface.iface;
	int ret;

	atomic_clear_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_OUT);

	if (stream_iface != NULL) {
		ret = usbh_device_interface_set(host_data->udev,
						stream_iface->bInterfaceNumber,
						0, false);
		if (ret != 0) {
			LOG_ERR("Failed to set OUT interface to idle: %d", ret);
		}
	}

	cleanup_out_transfers(host_data);

	LOG_DBG("Audio Playback OUT streaming stopped");
	return 0;
}

static int usbh_uac2_control_request(struct uac2_host_data *host_data,
				      enum usbh_uac2_stream_dir stream_dir,
				      const struct uac2_ctrl_mapping *mapping,
				      uint8_t channel,
				      uint8_t request,
				      bool is_get,
				      void *data,
				      uint16_t size)
{
	const void *entity;
	struct net_buf *buf;
	uint8_t bmRequestType;
	uint16_t wValue;
	uint16_t wIndex;
	uint8_t capability;
	uint8_t entity_id;
	int ret;

	if (data == NULL || size == 0 || mapping == NULL) {
		return -EINVAL;
	}

	/* Find entity in audio path using unit_subtype from mapping */
	entity = find_entity_in_path(host_data, stream_dir, mapping->unit_subtype);
	if (entity == NULL) {
		LOG_ERR("No entity (subtype=0x%02x) in %s stream",
			mapping->unit_subtype,
			stream_dir == USBH_CAPTURE ? "Capture" : "Playback");
		return -ENOTSUP;
	}

	/* Get entity ID based on unit_subtype */
	entity_id = get_entity_id(entity, mapping->unit_subtype);
	if (entity_id == 0) {
		LOG_ERR("Invalid entity ID for subtype 0x%02x", mapping->unit_subtype);
		return -EINVAL;
	}

	/*
	 * Extract control capability from bmControls field.
	 * Each control uses 2 bits (USB Audio 2.0 spec):
	 *   0b00 = Not present
	 *   0b01 = Read-only (Host readable)
	 *   0b10 = NOT ALLOWED (reserved/invalid)
	 *   0b11 = Read/Write (Host programmable)
	 */
	switch (mapping->unit_subtype) {
	case AC_DESCRIPTOR_CLOCK_SOURCE: {
		const struct uac2_clock_source_descriptor *clk = entity;
		uint8_t shift;

		/* Clock Source bmControls is 1 byte, each control uses 2 bits */
		switch (mapping->ctrl_selector) {
		case CS_SAM_FREQ_CONTROL:
			shift = 0;  /* Bits 0-1: Sample Frequency Control */
			break;
		case CS_CLOCK_VALID_CONTROL:
			shift = 2;  /* Bits 2-3: Clock Valid Control */
			break;
		default:
			LOG_ERR("Unknown Clock Source control: 0x%02x", mapping->ctrl_selector);
			return -EINVAL;
		}
		capability = (clk->bmControls >> shift) & 0x3;
		break;
	}

	case AC_DESCRIPTOR_CLOCK_SELECTOR: {
		const struct uac2_clock_selector_descriptor *cx = entity;
		const uint8_t *bmControls_ptr;

		if (cx->bLength < 7) {
			LOG_ERR("Invalid Clock Selector descriptor length");
			return -EINVAL;
		}

		/* bmControls is after baCSourceID array */
		bmControls_ptr = (const uint8_t *)(cx + 1);
		bmControls_ptr += cx->bNrInPins;

		if (mapping->ctrl_selector == CX_CLOCK_SELECTOR_CONTROL) {
			capability = (*bmControls_ptr) & 0x3;
		} else {
			LOG_ERR("Unknown Clock Selector control: 0x%02x", mapping->ctrl_selector);
			return -EINVAL;
		}
		break;
	}

	case AC_DESCRIPTOR_CLOCK_MULTIPLIER: {
		const struct uac2_clock_multiplier_descriptor *cm = entity;
		uint8_t shift;

		switch (mapping->ctrl_selector) {
		case CM_NUMERATOR_CONTROL:
			shift = 0;
			break;
		case CM_DENOMINATOR_CONTROL:
			shift = 2;
			break;
		default:
			LOG_ERR("Unknown Clock Multiplier control: 0x%02x",
				mapping->ctrl_selector);
			return -EINVAL;
		}
		capability = (cm->bmControls >> shift) & 0x3;
		break;
	}

	case AC_DESCRIPTOR_INPUT_TERMINAL: {
		const struct uac2_input_terminal_descriptor *it = entity;
		uint16_t bmControls = sys_le16_to_cpu(it->bmControls);
		uint8_t shift;

		switch (mapping->ctrl_selector) {
		case TE_COPY_PROTECT_CONTROL:
			shift = 0;
			break;
		case TE_CONNECTOR_CONTROL:
			shift = 2;
			break;
		case TE_OVERLOAD_CONTROL:
			shift = 4;
			break;
		case TE_CLUSTER_CONTROL:
			shift = 6;
			break;
		case TE_UNDERFLOW_CONTROL:
			shift = 8;
			break;
		case TE_OVERFLOW_CONTROL:
			shift = 10;
			break;
		case TE_LATENCY_CONTROL:
			shift = 12;
			break;
		default:
			LOG_ERR("Unknown Input Terminal control: 0x%02x",
				mapping->ctrl_selector);
			return -EINVAL;
		}
		capability = (bmControls >> shift) & 0x3;
		break;
	}

	case AC_DESCRIPTOR_OUTPUT_TERMINAL: {
		const struct uac2_output_terminal_descriptor *ot = entity;
		uint16_t bmControls = sys_le16_to_cpu(ot->bmControls);
		uint8_t shift;

		switch (mapping->ctrl_selector) {
		case TE_COPY_PROTECT_CONTROL:
			shift = 0;
			break;
		case TE_CONNECTOR_CONTROL:
			shift = 2;
			break;
		case TE_OVERLOAD_CONTROL:
			shift = 4;
			break;
		case TE_CLUSTER_CONTROL:
			shift = 6;
			break;
		case TE_UNDERFLOW_CONTROL:
			shift = 8;
			break;
		case TE_OVERFLOW_CONTROL:
			shift = 10;
			break;
		case TE_LATENCY_CONTROL:
			shift = 12;
			break;
		default:
			LOG_ERR("Unknown Output Terminal control: 0x%02x",
				mapping->ctrl_selector);
			return -EINVAL;
		}
		capability = (bmControls >> shift) & 0x3;
		break;
	}

	case AC_DESCRIPTOR_MIXER_UNIT: {
		/* TODO: Implement Mixer Unit control capability check */
		/* Mixer Unit has variable length bmControls field */
		LOG_WRN("Mixer Unit control capability check not implemented");
		capability = 0x3;
		break;
	}

	case AC_DESCRIPTOR_SELECTOR_UNIT: {
		const struct uac2_selector_unit_descriptor *su = entity;
		const uint8_t *bmControls_ptr;

		if (su->bLength < 7) {
			LOG_ERR("Invalid Selector Unit descriptor length");
			return -EINVAL;
		}

		/* bmControls is after baSourceID array */
		bmControls_ptr = (const uint8_t *)(su + 1);
		bmControls_ptr += su->bNrInPins;

		switch (mapping->ctrl_selector) {
		case SU_SELECTOR_CONTROL:
			capability = (*bmControls_ptr) & 0x3;
			break;
		case SU_LATENCY_CONTROL:
			capability = ((*bmControls_ptr) >> 2) & 0x3;
			break;
		default:
			LOG_ERR("Unknown Selector Unit control: 0x%02x",
				mapping->ctrl_selector);
			return -EINVAL;
		}
		break;
	}

	case AC_DESCRIPTOR_FEATURE_UNIT: {
		const struct uac2_feature_unit_descriptor *fu = entity;
		const uint8_t *controls_ptr;
		uint32_t controls;
		uint8_t num_channels;
		uint8_t shift;

		if (fu->bLength < 7) {
			LOG_ERR("Invalid Feature Unit descriptor length");
			return -EINVAL;
		}

		/* Calculate number of channels: bLength = 6 + (num_channels + 1) * 4 + 1 */
		num_channels = (fu->bLength - 7) / 4;

		if (channel > num_channels) {
			LOG_ERR("Channel %u exceeds Feature Unit channels (%u)",
				channel, num_channels);
			return -EINVAL;
		}

		/* Get controls bitmap for this channel (4 bytes per channel) */
		controls_ptr = (const uint8_t *)(fu + 1);
		controls_ptr += channel * 4;
		controls = sys_get_le32(controls_ptr);

		/* Each control uses 2 bits */
		if (mapping->ctrl_selector < FU_MUTE_CONTROL ||
		    mapping->ctrl_selector > FU_LATENCY_CONTROL) {
			LOG_ERR("Invalid Feature Unit control selector: 0x%02x",
				mapping->ctrl_selector);
			return -EINVAL;
		}

		shift = (mapping->ctrl_selector - FU_MUTE_CONTROL) * 2;
		capability = (controls >> shift) & 0x3;
		break;
	}

	case AC_DESCRIPTOR_EFFECT_UNIT: {
		const struct uac2_effect_unit_descriptor *eu = entity;
		const uint8_t *bmControls_ptr;
		uint32_t bmControls;
		uint16_t effect_type;
		uint8_t shift;

		/* Get effect type */
		effect_type = sys_le16_to_cpu(eu->wEffectType);

		/* bmControls is after bSourceID (1 byte) */
		bmControls_ptr = (const uint8_t *)(eu + 1) + 1;
		bmControls = sys_get_le32(bmControls_ptr);

		/* Map control selector to bit position based on effect type */
		switch (effect_type) {
		case PARAM_EQ_SECTION_EFFECT:
			/* Parametric Equalizer */
			switch (mapping->ctrl_selector) {
			case PE_ENABLE_CONTROL:
				shift = 0;
				break;
			case PE_CENTERFREQ_CONTROL:
				shift = 2;
				break;
			case PE_QFACTOR_CONTROL:
				shift = 4;
				break;
			case PE_GAIN_CONTROL:
				shift = 6;
				break;
			case PE_UNDERFLOW_CONTROL:
				shift = 8;
				break;
			case PE_OVERFLOW_CONTROL:
				shift = 10;
				break;
			case PE_LATENCY_CONTROL:
				shift = 12;
				break;
			default:
				LOG_ERR("Unknown PE control: 0x%02x", mapping->ctrl_selector);
				return -EINVAL;
			}
			break;

		case REVERBERATION_EFFECT:
			/* Reverberation */
			switch (mapping->ctrl_selector) {
			case RV_ENABLE_CONTROL:
				shift = 0;
				break;
			case RV_TYPE_CONTROL:
				shift = 2;
				break;
			case RV_LEVEL_CONTROL:
				shift = 4;
				break;
			case RV_TIME_CONTROL:
				shift = 6;
				break;
			case RV_FEEDBACK_CONTROL:
				shift = 8;
				break;
			case RV_PREDELAY_CONTROL:
				shift = 10;
				break;
			case RV_DENSITY_CONTROL:
				shift = 12;
				break;
			case RV_HIFREQ_ROLLOFF_CONTROL:
				shift = 14;
				break;
			case RV_UNDERFLOW_CONTROL:
				shift = 16;
				break;
			case RV_OVERFLOW_CONTROL:
				shift = 18;
				break;
			case RV_LATENCY_CONTROL:
				shift = 20;
				break;
			default:
				LOG_ERR("Unknown RV control: 0x%02x", mapping->ctrl_selector);
				return -EINVAL;
			}
			break;

		case MOD_DELAY_EFFECT:
			/* Modulation Delay (Chorus/Flanger) */
			switch (mapping->ctrl_selector) {
			case MD_ENABLE_CONTROL:
				shift = 0;
				break;
			case MD_BALANCE_CONTROL:
				shift = 2;
				break;
			case MD_RATE_CONTROL:
				shift = 4;
				break;
			case MD_DEPTH_CONTROL:
				shift = 6;
				break;
			case MD_TIME_CONTROL:
				shift = 8;
				break;
			case MD_FEEDBACK_CONTROL:
				shift = 10;
				break;
			case MD_UNDERFLOW_CONTROL:
				shift = 12;
				break;
			case MD_OVERFLOW_CONTROL:
				shift = 14;
				break;
			case MD_LATENCY_CONTROL:
				shift = 16;
				break;
			default:
				LOG_ERR("Unknown MD control: 0x%02x", mapping->ctrl_selector);
				return -EINVAL;
			}
			break;

		case DYN_RANGE_COMP_EFFECT:
			/* Dynamic Range Compressor */
			switch (mapping->ctrl_selector) {
			case DR_ENABLE_CONTROL:
				shift = 0;
				break;
			case DR_COMPRESSION_RATE_CONTROL:
				shift = 2;
				break;
			case DR_MAXAMPL_CONTROL:
				shift = 4;
				break;
			case DR_THRESHOLD_CONTROL:
				shift = 6;
				break;
			case DR_ATTACK_TIME_CONTROL:
				shift = 8;
				break;
			case DR_RELEASE_TIME_CONTROL:
				shift = 10;
				break;
			case DR_UNDERFLOW_CONTROL:
				shift = 12;
				break;
			case DR_OVERFLOW_CONTROL:
				shift = 14;
				break;
			case DR_LATENCY_CONTROL:
				shift = 16;
				break;
			default:
				LOG_ERR("Unknown DR control: 0x%02x", mapping->ctrl_selector);
				return -EINVAL;
			}
			break;

		default:
			LOG_WRN("Unknown effect type: 0x%04x", effect_type);
			capability = 0x3;  /* Assume read-write */
			goto skip_capability_extract;
		}

		capability = (bmControls >> shift) & 0x3;
skip_capability_extract:
		break;
	}

	case AC_DESCRIPTOR_PROCESSING_UNIT: {
		const struct uac2_processing_unit_descriptor *pu = entity;
		const uint8_t *bmControls_ptr;
		uint32_t bmControls;
		uint16_t process_type;
		uint8_t shift;

		/* Get process type */
		process_type = sys_le16_to_cpu(pu->wProcessType);

		/*
		 * Calculate bmControls offset in Processing Unit descriptor:
		 * After fixed header: bNrInPins(1) + baSourceID(N) +
		 * bNrChannels(1) + bmChannelConfig(4) + iChannelNames(1)
		 */
		bmControls_ptr = (const uint8_t *)(pu + 1);
		bmControls_ptr += pu->bNrInPins;
		bmControls_ptr += 1 + 4 + 1;

		bmControls = sys_get_le16(bmControls_ptr);

		/* Map control selector to bit position based on process type */
		switch (process_type) {
		case UP_DOWNMIX_PROCESS:
			/* Up/Down-mix */
			switch (mapping->ctrl_selector) {
			case UD_ENABLE_CONTROL:
				shift = 0;
				break;
			case UD_MODE_SELECT_CONTROL:
				shift = 2;
				break;
			case UD_CLUSTER_CONTROL:
				shift = 4;
				break;
			case UD_UNDERFLOW_CONTROL:
				shift = 6;
				break;
			case UD_OVERFLOW_CONTROL:
				shift = 8;
				break;
			case UD_LATENCY_CONTROL:
				shift = 10;
				break;
			default:
				LOG_ERR("Unknown UD control: 0x%02x", mapping->ctrl_selector);
				return -EINVAL;
			}
			break;

		case DOLBY_PROLOGIC_PROCESS:
			/* Dolby Prologic */
			switch (mapping->ctrl_selector) {
			case DP_ENABLE_CONTROL:
				shift = 0;
				break;
			case DP_MODE_SELECT_CONTROL:
				shift = 2;
				break;
			case DP_CLUSTER_CONTROL:
				shift = 4;
				break;
			case DP_UNDERFLOW_CONTROL:
				shift = 6;
				break;
			case DP_OVERFLOW_CONTROL:
				shift = 8;
				break;
			case DP_LATENCY_CONTROL:
				shift = 10;
				break;
			default:
				LOG_ERR("Unknown DP control: 0x%02x", mapping->ctrl_selector);
				return -EINVAL;
			}
			break;

		case STEREO_EXTENDER_PROCESS:
			/* Stereo Extender */
			switch (mapping->ctrl_selector) {
			case ST_EXT_ENABLE_CONTROL:
				shift = 0;
				break;
			case ST_EXT_WIDTH_CONTROL:
				shift = 2;
				break;
			case ST_EXT_UNDERFLOW_CONTROL:
				shift = 4;
				break;
			case ST_EXT_OVERFLOW_CONTROL:
				shift = 6;
				break;
			case ST_EXT_LATENCY_CONTROL:
				shift = 8;
				break;
			default:
				LOG_ERR("Unknown ST control: 0x%02x", mapping->ctrl_selector);
				return -EINVAL;
			}
			break;

		default:
			LOG_WRN("Unknown process type: 0x%04x", process_type);
			capability = 0x3;  /* Assume read-write */
			goto skip_pu_capability;
		}

		capability = (bmControls >> shift) & 0x3;
skip_pu_capability:
		break;
	}

	case AC_DESCRIPTOR_EXTENSION_UNIT: {
		const struct uac2_extension_unit_descriptor *xu = entity;
		const uint8_t *bmControls_ptr;
		uint32_t bmControls;
		uint8_t shift;

		/*
		 * Calculate bmControls offset in Extension Unit descriptor:
		 * After fixed header: bNrInPins(1) + baSourceID(N) +
		 * bNrChannels(1) + bmChannelConfig(4) + iChannelNames(1)
		 */
		bmControls_ptr = (const uint8_t *)(xu + 1);
		bmControls_ptr += xu->bNrInPins;
		bmControls_ptr += 1 + 4 + 1;

		bmControls = sys_get_le32(bmControls_ptr);

		/* Extension Unit controls */
		switch (mapping->ctrl_selector) {
		case XU_ENABLE_CONTROL:
			shift = 0;
			break;
		case XU_CLUSTER_CONTROL:
			shift = 2;
			break;
		case XU_UNDERFLOW_CONTROL:
			shift = 4;
			break;
		case XU_OVERFLOW_CONTROL:
			shift = 6;
			break;
		case XU_LATENCY_CONTROL:
			shift = 8;
			break;
		default:
			LOG_ERR("Unknown XU control: 0x%02x", mapping->ctrl_selector);
			return -EINVAL;
		}

		capability = (bmControls >> shift) & 0x3;
		break;
	}

	case AC_DESCRIPTOR_SAMPLE_RATE_CONVERTER: {
		/* TODO: Implement Sample Rate Converter control capability check */
		LOG_WRN("Sample Rate Converter control capability check not implemented");
		capability = 0x3;
		break;
	}

	default:
		LOG_ERR("Unknown unit subtype: 0x%02x", mapping->unit_subtype);
		return -EINVAL;
	}

	/* Check if control is present (capability != 0b00) */
	if (capability == 0x0) {
		LOG_DBG("Control 0x%02x not present on unit 0x%02x channel %u",
			mapping->ctrl_selector, mapping->unit_subtype, channel);
		return -ENOTSUP;
	}

	/* For SET operations, verify control is programmable (capability == 0b11) */
	if (!is_get && capability != 0x3) {
		LOG_ERR("Control 0x%02x is read-only (cap=0x%x)",
			mapping->ctrl_selector, capability);
		return -EACCES;
	}

	/* Allocate transfer buffer */
	buf = usbh_xfer_buf_alloc(host_data->udev, size);
	if (buf == NULL) {
		return -ENOMEM;
	}

	/* Determine bmRequestType based on direction */
	if (is_get) {
		bmRequestType = USB_REQTYPE_DIR_TO_HOST << 7 |
				USB_REQTYPE_TYPE_CLASS << 5 |
				USB_REQTYPE_RECIPIENT_INTERFACE;
	} else {
		bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				USB_REQTYPE_TYPE_CLASS << 5 |
				USB_REQTYPE_RECIPIENT_INTERFACE;

		/* Copy data to buffer for SET operations */
		memcpy(net_buf_add(buf, size), data, size);
	}

	/* Build request parameters (USB Audio 2.0 spec 5.2.1) */
	wValue = (mapping->ctrl_selector << 8) | channel;
	wIndex = (entity_id << 8) | host_data->current_ctrl_iface->bInterfaceNumber;

	LOG_DBG("Control: CID=0x%08x %s %s ch=%u sel=0x%02x req=0x%02x entity=%u cap=0x%x",
		mapping->cid,
		stream_dir == USBH_CAPTURE ? "Capture" : "Playback",
		is_get ? "GET" : "SET",
		channel, mapping->ctrl_selector, request, entity_id, capability);

	/* Send USB control request */
	ret = usbh_req_setup(host_data->udev, bmRequestType, request,
			     wValue, wIndex, size, buf);

	/* For GET requests, copy response data */
	if (ret == 0 && is_get) {
		if (buf->len > 0) {
			memcpy(data, buf->data, buf->len);
		} else {
			LOG_ERR("Received 0 bytes, expected %u", size);
			ret = -EIO;
		}
	}

	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int ctrl_get_value(struct uac2_host_data *host_data,
			  enum usbh_uac2_stream_dir dir,
			  uint8_t channel,
			  struct uac2_control *ctrl)
{
	uint8_t buf_layout1[sizeof(struct uac2_layout1_cur)];
	uint8_t buf_layout2[sizeof(struct uac2_layout2_cur)];
	uint8_t buf_layout3[sizeof(struct uac2_layout3_cur)];
	const struct uac2_ctrl_mapping *mapping;
	uint16_t size;
	void *data;
	int ret;

	/* Find control mapping by CID */
	mapping = find_ctrl_mapping_by_cid(ctrl->id);
	if (mapping == NULL) {
		LOG_ERR("Control ID 0x%08x not found", ctrl->id);
		return -ENOTSUP;
	}

	switch (mapping->layout) {
	case UAC2_PARAM_BLOCK_LAYOUT_1:
		data = buf_layout1;
		size = sizeof(struct uac2_layout1_cur);
		break;

	case UAC2_PARAM_BLOCK_LAYOUT_2:
		data = buf_layout2;
		size = sizeof(struct uac2_layout2_cur);
		break;

	case UAC2_PARAM_BLOCK_LAYOUT_3:
		data = buf_layout3;
		size = sizeof(struct uac2_layout3_cur);
		break;

	default:
		LOG_ERR("Invalid layout %u for CID 0x%08x", mapping->layout, ctrl->id);
		return -EINVAL;
	}

	ret = usbh_uac2_control_request(host_data, dir, mapping, channel,
					CUR, true, data, size);
	if (ret != 0) {
		LOG_ERR("Failed to get control 0x%08x: %d", ctrl->id, ret);
		goto exit;
	}

	switch (mapping->layout) {
	case UAC2_PARAM_BLOCK_LAYOUT_1: {
		struct uac2_layout1_cur *cur = data;

		ctrl->uval8 = cur->bCUR;
		LOG_DBG("GET CID=0x%08x: uval8=0x%02x", ctrl->id, ctrl->uval8);
		break;
	}

	case UAC2_PARAM_BLOCK_LAYOUT_2: {
		struct uac2_layout2_cur *cur = data;

		if (mapping->is_signed) {
			ctrl->val16 = (int16_t)sys_le16_to_cpu(cur->wCUR);
			LOG_DBG("GET CID=0x%08x: val16=%d", ctrl->id, ctrl->val16);
		} else {
			ctrl->uval16 = sys_le16_to_cpu(cur->wCUR);
			LOG_DBG("GET CID=0x%08x: uval16=%u", ctrl->id, ctrl->uval16);
		}
		break;
	}

	case UAC2_PARAM_BLOCK_LAYOUT_3: {
		struct uac2_layout3_cur *cur = data;

		ctrl->uval32 = sys_le32_to_cpu(cur->dCUR);
		LOG_DBG("GET CID=0x%08x: uval32=%u", ctrl->id, ctrl->uval32);
		break;
	}

	default:
		LOG_ERR("Unexpected layout %u", mapping->layout);
		ret = -EINVAL;
		goto exit;
	}

	ret = 0;

exit:
	return ret;
}

static int ctrl_set_value(struct uac2_host_data *host_data,
			  enum usbh_uac2_stream_dir dir,
			  uint8_t channel,
			  struct uac2_control *ctrl)
{
	uint8_t buf_layout1[sizeof(struct uac2_layout1_cur)];
	uint8_t buf_layout2[sizeof(struct uac2_layout2_cur)];
	uint8_t buf_layout3[sizeof(struct uac2_layout3_cur)];
	const struct uac2_ctrl_mapping *mapping;
	uint16_t size;
	void *data;
	int ret;

	mapping = find_ctrl_mapping_by_cid(ctrl->id);
	if (mapping == NULL) {
		LOG_ERR("Control ID 0x%08x not found", ctrl->id);
		return -ENOTSUP;
	}

	switch (mapping->layout) {
	case UAC2_PARAM_BLOCK_LAYOUT_1: {
		struct uac2_layout1_cur *cur = (struct uac2_layout1_cur *)buf_layout1;

		cur->bCUR = ctrl->uval8;
		data = buf_layout1;
		size = sizeof(struct uac2_layout1_cur);
		LOG_DBG("SET CID=0x%08x: uval8=0x%02x", ctrl->id, ctrl->uval8);
		break;
	}

	case UAC2_PARAM_BLOCK_LAYOUT_2: {
		struct uac2_layout2_cur *cur = (struct uac2_layout2_cur *)buf_layout2;

		if (mapping->is_signed) {
			cur->wCUR = sys_cpu_to_le16((uint16_t)ctrl->val16);
			LOG_DBG("SET CID=0x%08x: val16=%d", ctrl->id, ctrl->val16);
		} else {
			cur->wCUR = sys_cpu_to_le16(ctrl->uval16);
			LOG_DBG("SET CID=0x%08x: uval16=%u", ctrl->id, ctrl->uval16);
		}
		data = buf_layout2;
		size = sizeof(struct uac2_layout2_cur);
		break;
	}

	case UAC2_PARAM_BLOCK_LAYOUT_3: {
		struct uac2_layout3_cur *cur = (struct uac2_layout3_cur *)buf_layout3;

		cur->dCUR = sys_cpu_to_le32(ctrl->uval32);
		data = buf_layout3;
		size = sizeof(struct uac2_layout3_cur);
		LOG_DBG("SET CID=0x%08x: uval32=%u", ctrl->id, ctrl->uval32);
		break;
	}

	default:
		LOG_ERR("Invalid layout %u for CID 0x%08x", mapping->layout, ctrl->id);
		return -EINVAL;
	}

	ret = usbh_uac2_control_request(host_data, dir, mapping, channel,
					CUR, false, data, size);
	if (ret != 0) {
		goto exit;
	}

	switch (ctrl->id) {
	case AUDIO_CID_SAMPLE_RATE:
		if (dir == USBH_CAPTURE) {
			host_data->current_in_format.sample_rate = ctrl->uval32;
		} else {
			host_data->current_out_format.sample_rate = ctrl->uval32;
		}
		LOG_INF("%s sample rate set to %u Hz",
			dir == USBH_CAPTURE ? "Capture" : "Playback", ctrl->uval32);
		break;

	case AUDIO_CID_VOLUME:
		if (dir == USBH_CAPTURE) {
			host_data->in_volume_info.cur_volume = ctrl->val16;
		} else {
			host_data->out_volume_info.cur_volume = ctrl->val16;
		}
		LOG_INF("%s volume set to %d/256 dB",
			dir == USBH_CAPTURE ? "Capture" : "Playback", ctrl->val16);
		break;

	case AUDIO_CID_MUTE:
		LOG_INF("%s mute set to %u",
			dir == USBH_CAPTURE ? "Capture" : "Playback", ctrl->uval8);
		break;

	default:
		LOG_DBG("Control 0x%08x set successfully (no caching)", ctrl->id);
		break;
	}

	ret = 0;

exit:
	return ret;
}

static int ctrl_query_range(struct uac2_host_data *host_data,
			    enum usbh_uac2_stream_dir dir,
			    uint8_t channel,
			    struct uac2_ctrl_query *query)
{
	uint8_t buf_layout1[sizeof(struct uac2_layout1_range)];
	uint8_t buf_layout2[sizeof(struct uac2_layout2_range)];
	/* Layout 3 RANGE: wNumSubRanges(2) + N * (dMIN+dMAX+dRES)(12) */
	uint8_t buf_layout3[2 + CONFIG_USBH_UAC2_MAX_RANGE_SUBRANGES * 12];
	const struct uac2_ctrl_mapping *mapping;
	uint16_t size;
	void *data;
	int ret;

	mapping = find_ctrl_mapping_by_cid(query->id);
	if (mapping == NULL) {
		LOG_ERR("Control ID 0x%08x not found", query->id);
		return -ENOTSUP;
	}

	query->layout = mapping->layout;
	query->is_signed = mapping->is_signed;
	memset(&query->range, 0, sizeof(query->range));

	switch (mapping->layout) {
	case UAC2_PARAM_BLOCK_LAYOUT_1:
		data = buf_layout1;
		size = sizeof(struct uac2_layout1_range);
		break;

	case UAC2_PARAM_BLOCK_LAYOUT_2:
		data = buf_layout2;
		size = sizeof(struct uac2_layout2_range);
		break;

	case UAC2_PARAM_BLOCK_LAYOUT_3:
		data = buf_layout3;
		size = sizeof(buf_layout3);
		break;

	default:
		LOG_ERR("Invalid layout %u for CID 0x%08x", mapping->layout, query->id);
		return -EINVAL;
	}

	ret = usbh_uac2_control_request(host_data, dir, mapping, channel,
					RANGE, true, data, size);
	if (ret != 0) {
		goto exit;
	}

	switch (mapping->layout) {
	case UAC2_PARAM_BLOCK_LAYOUT_1: {
		struct uac2_layout1_range *r = data;

		query->range.num_subranges = sys_le16_to_cpu(r->wNumSubRanges);
		if (query->range.num_subranges > 0) {
			query->range.subranges[0].umin8 = r->bMIN;
			query->range.subranges[0].umax8 = r->bMAX;
			query->range.subranges[0].ustep8 = r->bRES;
		}
		LOG_DBG("Query CID=0x%08x: layout=1 range=%u-%u step=%u subranges=%u",
			query->id, r->bMIN, r->bMAX, r->bRES,
			query->range.num_subranges);
		break;
	}

	case UAC2_PARAM_BLOCK_LAYOUT_2: {
		struct uac2_layout2_range *r = data;

		query->range.num_subranges = sys_le16_to_cpu(r->wNumSubRanges);
		if (query->range.num_subranges > 0) {
			if (mapping->is_signed) {
				query->range.subranges[0].min16 =
					(int16_t)sys_le16_to_cpu(r->wMIN);
				query->range.subranges[0].max16 =
					(int16_t)sys_le16_to_cpu(r->wMAX);
				query->range.subranges[0].step16 =
					(int16_t)sys_le16_to_cpu(r->wRES);
				LOG_DBG("Query CID=0x%08x: layout=2 signed "
					"range=%d-%d step=%d subranges=%u",
					query->id,
					query->range.subranges[0].min16,
					query->range.subranges[0].max16,
					query->range.subranges[0].step16,
					query->range.num_subranges);
			} else {
				query->range.subranges[0].umin16 =
					sys_le16_to_cpu(r->wMIN);
				query->range.subranges[0].umax16 =
					sys_le16_to_cpu(r->wMAX);
				query->range.subranges[0].ustep16 =
					sys_le16_to_cpu(r->wRES);
				LOG_DBG("Query CID=0x%08x: layout=2 unsigned "
					"range=%u-%u step=%u subranges=%u",
					query->id,
					query->range.subranges[0].umin16,
					query->range.subranges[0].umax16,
					query->range.subranges[0].ustep16,
					query->range.num_subranges);
			}
		}
		break;
	}

	case UAC2_PARAM_BLOCK_LAYOUT_3: {
		const uint8_t *ptr = (const uint8_t *)data;
		uint16_t num_subranges;

		/* Read number of subranges */
		num_subranges = sys_get_le16(ptr);
		ptr += sizeof(uint16_t);

		query->range.num_subranges = MIN(num_subranges,
						 ARRAY_SIZE(query->range.subranges));

		LOG_DBG("Query CID=0x%08x: layout=3 num_subranges=%u (device reported %u)",
			query->id, query->range.num_subranges, num_subranges);

		for (uint8_t i = 0; i < query->range.num_subranges; i++) {
			query->range.subranges[i].umin32 = sys_get_le32(ptr);
			ptr += sizeof(uint32_t);
			query->range.subranges[i].umax32 = sys_get_le32(ptr);
			ptr += sizeof(uint32_t);
			query->range.subranges[i].ustep32 = sys_get_le32(ptr);
			ptr += sizeof(uint32_t);

			LOG_DBG("  Subrange %u: %u - %u (step %u)", i,
				query->range.subranges[i].umin32,
				query->range.subranges[i].umax32,
				query->range.subranges[i].ustep32);
		}

		if (num_subranges > query->range.num_subranges) {
			LOG_WRN("Device reported %u subranges, but only %u can be "
				"stored. Increase CONFIG_USBH_UAC2_MAX_RANGE_SUBRANGES.",
				num_subranges, query->range.num_subranges);
		}
		break;
	}

	default:
		LOG_ERR("Unexpected layout %u", mapping->layout);
		ret = -EINVAL;
		goto exit;
	}

	ret = 0;

exit:
	return ret;
}

int usbh_uac2_ctrl_query(const struct device *dev,
			 enum usbh_uac2_stream_dir dir,
			 uint8_t channel,
			 struct uac2_ctrl_query *query)
{
	struct uac2_host_data *host_data = dev->data;
	int ret;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	if (query == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);
	ret = ctrl_query_range(host_data, dir, channel, query);
	k_mutex_unlock(&host_data->lock);

	return ret;
}

static int query_uac_device_capabilities(struct usbh_class_data *const c_data,
					 enum usbh_uac2_stream_dir dir)
{
	const struct device *dev = c_data->priv;
	struct uac2_host_data *host_data = (void *)dev->data;
	const struct uac2_device_descriptors *desc;
	struct uac2_frequency_info *freq_info;
	struct uac2_volume_info *vol_info;
	struct uac2_ctrl_query query;
	struct uac2_control ctrl;
	const char *dir_str;
	uint8_t capability;
	int ret;

	desc = (dir == USBH_CAPTURE) ? &host_data->in_descriptors
				     : &host_data->out_descriptors;
	freq_info = (dir == USBH_CAPTURE) ? &host_data->in_frequency_info
					  : &host_data->out_frequency_info;
	vol_info = (dir == USBH_CAPTURE) ? &host_data->in_volume_info
					 : &host_data->out_volume_info;
	dir_str = (dir == USBH_CAPTURE) ? "Capture" : "Playback";

	if (desc->clock_source != NULL) {
		query.id = AUDIO_CID_SAMPLE_RATE;
		ret = ctrl_query_range(host_data, dir, 0, &query);
		if (ret == 0 && query.range.num_subranges > 0) {
			LOG_INF("%s sample rate range: ", dir_str);

			for (uint8_t i = 0; i < query.range.num_subranges; i++) {
				if (query.range.subranges[i].umin32 ==
				query.range.subranges[i].umax32) {
					/* Discrete value */
					LOG_INF("  [%u] %u Hz", i,
						query.range.subranges[i].umin32);
				} else {
					/* Range */
					LOG_INF("  [%u] %u - %u Hz (step %u)", i,
						query.range.subranges[i].umin32,
						query.range.subranges[i].umax32,
						query.range.subranges[i].ustep32);
				}
			}
		}

		if (ret == 0 && query.range.num_subranges > 0) {
			freq_info->range.wNumSubRanges = query.range.num_subranges;
			freq_info->range.dMIN = query.range.subranges[0].umin32;
			freq_info->range.dMAX =
				query.range.subranges[query.range.num_subranges - 1].umax32;
			freq_info->range.dRES = query.range.subranges[0].ustep32;
			freq_info->is_valid = true;
		} else if (ret != 0) {
			LOG_WRN("%s: failed to query sample rate range: %d", dir_str, ret);
		} else {
			LOG_WRN("%s: device reported 0 sample rate subranges", dir_str);
		}
	}

	if (desc->feature_unit == NULL) {
		return 0;
	}

	capability = get_control_capability(desc->feature_unit, 0, FU_VOLUME_CONTROL);
	if (capability != 0x0) {
		query.id = AUDIO_CID_VOLUME;
		ret = ctrl_query_range(host_data, dir, 0, &query);
		if (ret == 0) {
			LOG_INF("%s volume range: %d - %d dB (step %d/256)",
				dir_str,
				query.range.subranges[0].min16,
				query.range.subranges[0].max16,
				query.range.subranges[0].step16);

			vol_info->range.wNumSubRanges = query.range.num_subranges;
			vol_info->range.wMIN = query.range.subranges[0].min16;
			vol_info->range.wMAX = query.range.subranges[0].max16;
			vol_info->range.wRES = query.range.subranges[0].step16;
			vol_info->is_valid = true;
		}

		ctrl.id = AUDIO_CID_VOLUME;
		ret = ctrl_get_value(host_data, dir, 0, &ctrl);
		if (ret == 0) {
			LOG_INF("%s current volume: %d/256 dB", dir_str, ctrl.val16);
			vol_info->cur_volume = ctrl.val16;
		}
	}

	capability = get_control_capability(desc->feature_unit, 0, FU_MUTE_CONTROL);
	if (capability != 0x0) {
		ctrl.id = AUDIO_CID_MUTE;
		ret = ctrl_get_value(host_data, dir, 0, &ctrl);
		if (ret == 0) {
			LOG_INF("%s mute: %s",
				dir == USBH_CAPTURE ? "Capture" : "Playback",
				ctrl.uval8 ? "ON" : "OFF");
		}
	}

	return 0;
}

static int query_device_capabilities(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct uac2_host_data *host_data = (void *)dev->data;
	int ret;

	if (host_data->in_stream_iface.iface != NULL) {
		ret = query_uac_device_capabilities(c_data, USBH_CAPTURE);
		if (ret != 0) {
			LOG_WRN("Failed to query IN stream capabilities: %d", ret);
		}
	}

	if (host_data->out_stream_iface.iface != NULL) {
		ret = query_uac_device_capabilities(c_data, USBH_PLAYBACK);
		if (ret != 0) {
			LOG_WRN("Failed to query OUT stream capabilities: %d", ret);
		}
	}

	return 0;
}

static int usbh_uac2_init(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct uac2_host_data *host_data = (void *)dev->data;

	memset(host_data, 0x00, sizeof(*host_data));
	k_mutex_init(&host_data->lock);

	LOG_INF("UAC2 host data initialized successfully");
	return 0;
}

static int usbh_uac2_probe(struct usbh_class_data *const c_data,
			   struct usb_device *const udev, uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct uac2_host_data *host_data = (void *)dev->data;
	uint8_t target_iface;
	int ret;

	LOG_INF("UAC2 device connected");

	if ((udev == NULL) || (udev->state != USB_STATE_CONFIGURED)) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);

	host_data->udev = udev;

	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		target_iface = 0;
	} else {
		target_iface = iface;
	}

	ret = parse_descriptors(c_data, target_iface);
	if (ret != 0) {
		LOG_ERR("Failed to parse UAC2 descriptors: %d", ret);
		goto error_cleanup;
	}

	ret = configure_device(c_data);
	if (ret != 0) {
		LOG_ERR("Failed to configure UAC2 device: %d", ret);
		goto error_cleanup;
	}

	ret = query_device_capabilities(c_data);
	if (ret != 0) {
		LOG_WRN("Failed to query device capabilities: %d", ret);
	}

	ret = init_format(host_data);
	if (ret != 0) {
		LOG_ERR("Failed to initialize format: %d", ret);
		goto error_cleanup;
	}

	k_mutex_unlock(&host_data->lock);

	atomic_set_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED);

	LOG_INF("UAC2 device (addr=%d) initialization completed", host_data->udev->addr);
	return 0;

error_cleanup:
	uac2_reset_session(host_data);
	host_data->udev = NULL;
	k_mutex_unlock(&host_data->lock);
	return ret;
}

static int usbh_uac2_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct uac2_host_data *host_data = (void *)dev->data;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	atomic_clear_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_IN);
	atomic_clear_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_OUT);
	atomic_clear_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED);

	cleanup_in_transfers(host_data);
	cleanup_out_transfers(host_data);

	host_data->in_callback = NULL;
	host_data->in_callback_param = NULL;
	host_data->out_callback = NULL;
	host_data->out_callback_param = NULL;

	uac2_reset_session(host_data);
	host_data->udev = NULL;

	k_mutex_unlock(&host_data->lock);

	LOG_INF("UAC2 device removal completed");

	return 0;
}

int usbh_uac2_get_format(const struct device *dev,
			 enum usbh_uac2_stream_dir stream_dir,
			 uint32_t *sample_rate,
			 uint8_t *channels,
			 uint8_t *bit_resolution)
{
	struct uac2_host_data *host_data = dev->data;
	const struct uac2_format_info *format;
	int ret = 0;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (stream_dir == USBH_CAPTURE) {
		if (host_data->in_stream_iface.iface == NULL) {
			LOG_ERR("Device does not support audio capture");
			ret = -ENOTSUP;
			goto exit;
		}
		format = &host_data->current_in_format;
	} else {
		if (host_data->out_stream_iface.iface == NULL) {
			LOG_ERR("Device does not support audio playback");
			ret = -ENOTSUP;
			goto exit;
		}
		format = &host_data->current_out_format;
	}

	if (sample_rate != NULL) {
		*sample_rate = format->sample_rate;
	}
	if (channels != NULL) {
		*channels = format->channels;
	}
	if (bit_resolution != NULL) {
		*bit_resolution = format->bit_resolution;
	}

exit:
	k_mutex_unlock(&host_data->lock);
	return ret;
}

int usbh_uac2_ctrl_set(const struct device *dev,
		       enum usbh_uac2_stream_dir dir,
		       uint8_t channel,
		       struct uac2_control *ctrl)
{
	struct uac2_host_data *host_data = dev->data;
	int ret;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	if (ctrl == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);
	ret = ctrl_set_value(host_data, dir, channel, ctrl);
	k_mutex_unlock(&host_data->lock);

	return ret;
}

int usbh_uac2_ctrl_get(const struct device *dev,
		       enum usbh_uac2_stream_dir dir,
		       uint8_t channel,
		       struct uac2_control *ctrl)
{
	struct uac2_host_data *host_data = dev->data;
	int ret;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	if (ctrl == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);
	ret = ctrl_get_value(host_data, dir, channel, ctrl);
	k_mutex_unlock(&host_data->lock);

	return ret;
}

int usbh_uac2_start_stream_in(const struct device *dev,
			      usbh_uac2_transfer_cb_t callback,
			      void *user_data)
{
	struct uac2_host_data *host_data = dev->data;
	int ret;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	if (host_data->in_stream_iface.iface == NULL) {
		LOG_ERR("Device does not support audio capture");
		return -ENOTSUP;
	}

	if (atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_IN)) {
		LOG_WRN("Already streaming IN");
		return 0;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);

	host_data->in_callback = callback;
	host_data->in_callback_param = user_data;

	ret = start_streaming_in(host_data);

	k_mutex_unlock(&host_data->lock);

	return ret;
}

int usbh_uac2_stop_stream_in(const struct device *dev)
{
	struct uac2_host_data *host_data = dev->data;
	int ret;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		LOG_ERR("Device not connected");
		return -ENODEV;
	}

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_IN)) {
		LOG_WRN("Not streaming IN");
		return 0;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);

	ret = stop_streaming_in(host_data);

	k_mutex_unlock(&host_data->lock);

	return ret;
}

int usbh_uac2_start_stream_out(const struct device *dev,
			       usbh_uac2_transfer_cb_t callback,
			       void *user_data)
{
	struct uac2_host_data *host_data = dev->data;
	int ret;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	if (host_data->out_stream_iface.iface == NULL) {
		LOG_ERR("Device does not support audio playback");
		return -ENOTSUP;
	}

	if (atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_OUT)) {
		LOG_WRN("Already streaming OUT");
		return 0;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);

	host_data->out_callback = callback;
	host_data->out_callback_param = user_data;

	ret = start_streaming_out(host_data);

	k_mutex_unlock(&host_data->lock);

	return ret;
}

int usbh_uac2_stop_stream_out(const struct device *dev)
{
	struct uac2_host_data *host_data = dev->data;
	int ret;

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_CONNECTED)) {
		LOG_ERR("Device not connected");
		return -ENODEV;
	}

	if (!atomic_test_bit(&host_data->device_flags, UAC2_DEVICE_FLAG_STREAMING_OUT)) {
		LOG_WRN("Not streaming OUT");
		return 0;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);

	ret = stop_streaming_out(host_data);

	k_mutex_unlock(&host_data->lock);

	return ret;
}

static struct usbh_class_api usbh_uac2_class_api = {
	.init = usbh_uac2_init,
	.probe = usbh_uac2_probe,
	.removed = usbh_uac2_removed,
};

static struct usbh_class_filter usbh_uac2_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = AUDIO,
		.sub = FUNCTION_SUBCLASS_UNDEFINED,
		.proto = AF_VERSION_02_00,
	},
	/* Also match on AudioControl interface (fallback) */
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = AUDIO,
		.sub = AUDIOCONTROL,
		.proto = AF_VERSION_02_00,
	},
	{0},
};

#define USBH_UAC2_DEVICE_DEFINE(n, _)						\
	static struct uac2_host_data uac2_host_data##n;				\
										\
	DEVICE_DEFINE(usbh_uac2_##n, "usbh_uac2_" #n, NULL, NULL,		\
		      &uac2_host_data##n, NULL, POST_KERNEL,			\
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);		\
										\
	USBH_DEFINE_CLASS(uac2_host_c_data_##n, &usbh_uac2_class_api,		\
			  (void *)DEVICE_GET(usbh_uac2_##n), usbh_uac2_filters);

LISTIFY(CONFIG_USBH_UAC2_INSTANCES_COUNT, USBH_UAC2_DEVICE_DEFINE, (;), _)
