/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/arch_interface.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define EMIOS_CHANNEL_COUNT 2U
#define EMIOS_CW_CH_IDX     0
#define EMIOS_CCW_CH_IDX    1

/* LCU LUT control values for each of the 4 LC outputs */
/* These values decide the direction of motor rotation */
#define LCU_O0_LUT 0xAAAA
#define LCU_O1_LUT 0xCCCC
#define LCU_O2_LUT 0x4182
#define LCU_O3_LUT 0x2814

LOG_MODULE_REGISTER(nxp_qdec_s32, CONFIG_SENSOR_LOG_LEVEL);

#include <Emios_Icu_Ip.h>
#include <Trgmux_Ip.h>
#include <Lcu_Ip.h>

#define DT_DRV_COMPAT nxp_qdec_s32

typedef void (*emios_callback_t)(void);

/* Configuration variables from eMIOS Icu driver */
extern eMios_Icu_Ip_ChStateType eMios_Icu_Ip_ChState[EMIOS_ICU_IP_NUM_OF_CHANNELS_USED];
extern uint8 eMios_Icu_Ip_IndexInChState[EMIOS_ICU_IP_INSTANCE_COUNT][EMIOS_ICU_IP_NUM_OF_CHANNELS];

struct qdec_s32_config {
	uint8_t emios_inst;
	uint8_t emios_channels[EMIOS_CHANNEL_COUNT];
	const struct pinctrl_dev_config *pincfg;

	const Trgmux_Ip_InitType *trgmux_config;

	const Lcu_Ip_InitType *lcu_config;
	emios_callback_t emios_cw_overflow_cb;
	emios_callback_t emios_ccw_overflow_cb;
};

struct qdec_s32_data {
	uint32_t counter_CW;
	uint32_t counter_CCW;
	int32_t abs_counter;
	double micro_ticks_per_rev;
	uint32_t ticks_per_sec;
	uint32_t emios_cw_overflow_count;
	uint32_t emios_ccw_overflow_count;
};

static void qdec_emios_overflow_count_cw_callback(const struct device *dev)
{
	struct qdec_s32_data *data = dev->data;

	data->emios_cw_overflow_count++;
}

static void qdec_emios_overflow_count_ccw_callback(const struct device *dev)
{
	struct qdec_s32_data *data = dev->data;

	data->emios_ccw_overflow_count++;
}

static int qdec_s32_fetch(const struct device *dev, enum sensor_channel ch)
{
	const struct qdec_s32_config *config = dev->config;
	struct qdec_s32_data *data = dev->data;

	if (ch != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	data->counter_CW = (uint32_t)(Emios_Icu_Ip_GetEdgeNumbers(
	config->emios_inst, config->emios_channels[EMIOS_CW_CH_IDX])); /* CW counter */
	data->counter_CCW = (uint32_t)(Emios_Icu_Ip_GetEdgeNumbers(
	config->emios_inst, config->emios_channels[EMIOS_CCW_CH_IDX]));/* CCW counter*/

	data->abs_counter = (int32_t)(
		+(data->counter_CW  + EMIOS_ICU_IP_COUNTER_MASK *
			data->emios_cw_overflow_count)
		-(data->counter_CCW + EMIOS_ICU_IP_COUNTER_MASK *
			data->emios_ccw_overflow_count));

	LOG_DBG("ABS_COUNT = %d CW = %u OverFlow_CW = %u CCW = %u Overflow_CCW = %u",
		data->abs_counter, data->counter_CW,
		data->emios_cw_overflow_count,
		data->counter_CCW, data->emios_ccw_overflow_count);

	return 0;
}

static int qdec_s32_ch_get(const struct device *dev, enum sensor_channel ch,
		struct sensor_value *val)
{
	struct qdec_s32_data *data = dev->data;

	double rotation = (data->abs_counter * 2.0 * M_PI) / data->micro_ticks_per_rev;

	switch (ch) {
	case SENSOR_CHAN_ROTATION:
		sensor_value_from_double(val, rotation);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, qdec_s32_api) = {
	.sample_fetch = &qdec_s32_fetch,
	.channel_get = &qdec_s32_ch_get,
};

static int qdec_s32_initialize(const struct device *dev)
{
	const struct qdec_s32_config *config = dev->config;
	uint8_t emios_inst, emios_hw_ch_cw, emios_hw_ch_ccw;

	pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	if (Trgmux_Ip_Init(config->trgmux_config)) {
		LOG_ERR("Could not initialize Trgmux");
		return -EINVAL;
	}

	LOG_DBG("TRGMUX ACCESS Input[0] =%d Output[0]=%d",
		config->trgmux_config->paxLogicTrigger[0]->Input,
		config->trgmux_config->paxLogicTrigger[0]->Output);

	if (Lcu_Ip_Init(config->lcu_config)) {
		LOG_ERR("Could not initialize Lcu");
		return -EINVAL;
	}

	/* Unmask relevant LCU OUT Channels */
	Lcu_Ip_SyncOutputValueType EncLcuEnable[4U];

	EncLcuEnable[0].LogicOutputId = LCU_LOGIC_OUTPUT_0;
	EncLcuEnable[0].Value = 1U;
	EncLcuEnable[1].LogicOutputId = LCU_LOGIC_OUTPUT_1;
	EncLcuEnable[1].Value = 1U;
	EncLcuEnable[2].LogicOutputId = LCU_LOGIC_OUTPUT_2;
	EncLcuEnable[2].Value = 1U;
	EncLcuEnable[3].LogicOutputId = LCU_LOGIC_OUTPUT_3;
	EncLcuEnable[3].Value = 1U;
	Lcu_Ip_SetSyncOutputEnable(EncLcuEnable, 4U);

	emios_inst = config->emios_inst;
	emios_hw_ch_cw =  config->emios_channels[EMIOS_CW_CH_IDX];
	emios_hw_ch_ccw =  config->emios_channels[EMIOS_CCW_CH_IDX];

	/* Initialize the positions of the eMios hw channels used for QDEC
	 * to be beyond the eMios pwm hw channels. Currently only pwm and qdec
	 * are using the eMios channels so qdec ones are the last two.
	 */
	eMios_Icu_Ip_IndexInChState[emios_inst][emios_hw_ch_cw]
		= EMIOS_ICU_IP_NUM_OF_CHANNELS_USED - 2;
	eMios_Icu_Ip_IndexInChState[emios_inst][emios_hw_ch_ccw]
		= EMIOS_ICU_IP_NUM_OF_CHANNELS_USED - 1;

	/* Set Overflow Notification for eMIOS channels meant
	 * for Clockwise and Counterclock rotation counters
	 */
	eMios_Icu_Ip_ChState[eMios_Icu_Ip_IndexInChState[emios_inst][emios_hw_ch_cw]]
		.eMiosOverflowNotification = config->emios_cw_overflow_cb;
	eMios_Icu_Ip_ChState[eMios_Icu_Ip_IndexInChState[emios_inst][emios_hw_ch_ccw]]
		.eMiosOverflowNotification = config->emios_ccw_overflow_cb;

	Emios_Icu_Ip_SetInitialCounterValue(
		config->emios_inst, config->emios_channels[EMIOS_CW_CH_IDX], (uint32_t)0x1U);
	Emios_Icu_Ip_SetInitialCounterValue(
		config->emios_inst, config->emios_channels[EMIOS_CCW_CH_IDX], (uint32_t)0x1U);

	Emios_Icu_Ip_SetMaxCounterValue(config->emios_inst,
		config->emios_channels[EMIOS_CW_CH_IDX],
		EMIOS_ICU_IP_COUNTER_MASK);
	Emios_Icu_Ip_SetMaxCounterValue(config->emios_inst,
		config->emios_channels[EMIOS_CCW_CH_IDX],
		EMIOS_ICU_IP_COUNTER_MASK);

	/* This API sets MCB/EMIOS_ICU_MODE_EDGE_COUNTER mode */
	Emios_Icu_Ip_EnableEdgeCount(config->emios_inst, config->emios_channels[EMIOS_CW_CH_IDX]);
	Emios_Icu_Ip_EnableEdgeCount(config->emios_inst, config->emios_channels[EMIOS_CCW_CH_IDX]);

	LOG_DBG("Init complete");

	return 0;
}

#define EMIOS_NXP_S32_MCB_OVERFLOW_CALLBACK(n)							\
	static void qdec##n##_emios_overflow_count_cw_callback(void)				\
	{											\
		qdec_emios_overflow_count_cw_callback(DEVICE_DT_INST_GET(n));			\
	}											\
												\
	static void qdec##n##_emios_overflow_count_ccw_callback(void)				\
	{											\
		qdec_emios_overflow_count_ccw_callback(DEVICE_DT_INST_GET(n));			\
	}

#define EMIOS_NXP_S32_INSTANCE_CHECK(idx, node_id)						\
	((DT_REG_ADDR(node_id) == IP_EMIOS_##idx##_BASE) ? idx : 0)

#define EMIOS_NXP_S32_GET_INSTANCE(node_id)							\
	LISTIFY(__DEBRACKET eMIOS_INSTANCE_COUNT, EMIOS_NXP_S32_INSTANCE_CHECK, (|), node_id)

#define LCU_NXP_S32_INSTANCE_CHECK(idx, node_id)						\
	((DT_REG_ADDR(node_id) == IP_LCU_##idx##_BASE) ? idx : 0)

#define LCU_NXP_S32_GET_INSTANCE(node_id)							\
	LISTIFY(__DEBRACKET LCU_INSTANCE_COUNT, LCU_NXP_S32_INSTANCE_CHECK, (|), node_id)

#define TRGMUX_NXP_S32_INSTANCE_CHECK(node_id)							\
	((DT_REG_ADDR(node_id) == IP_TRGMUX_BASE) ? 0 : -1)

#define TRGMUX_NXP_S32_GET_INSTANCE(node_id)	TRGMUX_NXP_S32_INSTANCE_CHECK(node_id)

/* LCU Logic Input Configuration */
#define LogicInputCfg_Common(n, mux_sel_idx)							\
	{											\
		.MuxSel = DT_INST_PROP_BY_IDX(n, lcu_mux_sel, mux_sel_idx),			\
		.SwSynMode = LCU_IP_SW_SYNC_IMMEDIATE,						\
		.SwValue = LCU_IP_SW_OVERRIDE_LOGIC_LOW,					\
	};
#define LogicInput_Config_Common(n, hw_lc_input_id, logic_input_n_cfg)				\
	{											\
		.xLogicInputId = {								\
			.HwInstId = LCU_NXP_S32_GET_INSTANCE(DT_INST_PHANDLE(n, lcu)),		\
			.HwLcInputId = DT_INST_PROP_BY_IDX(n, lcu_input_idx, hw_lc_input_id),	\
		},										\
		.pxLcInputConfig = &logic_input_n_cfg,						\
	};

/* LCU Logic Output Configuration */
#define LogicOutputCfg_Common(En_Debug_Mode, Lut_Control, Lut_Rise_Filt, Lut_Fall_Filt)		\
	{											\
		.EnDebugMode = (boolean)En_Debug_Mode,						\
		.LutControl = Lut_Control,							\
		.LutRiseFilt = Lut_Rise_Filt,							\
		.LutFallFilt = Lut_Fall_Filt,							\
		.EnLutDma = (boolean)FALSE,							\
		.EnForceDma = (boolean)FALSE,							\
		.EnLutInt = (boolean)FALSE,							\
		.EnForceInt = (boolean)FALSE,							\
		.InvertOutput = (boolean)FALSE,							\
		.ForceSignalSel = 0U,								\
		.ClearForceMode = LCU_IP_CLEAR_FORCE_SIGNAL_IMMEDIATE,				\
		.ForceSyncSel = LCU_IP_SYNC_SEL_INPUT0,						\
	};
#define LogicOutput_Config_Common(n, logic_output_cfg, hw_lc_output_id)				\
	{											\
		.xLogicOutputId = {								\
			.HwInstId = LCU_NXP_S32_GET_INSTANCE(DT_INST_PHANDLE(n, lcu)),		\
			.HwLcOutputId = hw_lc_output_id,					\
			.IntCallback = NULL_PTR,						\
		},										\
		.pxLcOutputConfig = &logic_output_cfg,						\
	};

#define LCU_IP_INIT_CONFIG(n)									\
	const Lcu_Ip_LogicInputConfigType LogicInput##n##_0_Cfg =				\
		LogicInputCfg_Common(n, 0)							\
	const Lcu_Ip_LogicInputConfigType LogicInput##n##_1_Cfg =				\
		LogicInputCfg_Common(n, 1)							\
	const Lcu_Ip_LogicInputConfigType LogicInput##n##_2_Cfg =				\
		LogicInputCfg_Common(n, 2)							\
	const Lcu_Ip_LogicInputConfigType LogicInput##n##_3_Cfg =				\
		LogicInputCfg_Common(n, 3)							\
												\
	const Lcu_Ip_LogicInputType LogicInput##n##_0_Config =					\
		LogicInput_Config_Common(n, 0, LogicInput##n##_0_Cfg)				\
	const Lcu_Ip_LogicInputType LogicInput##n##_1_Config =					\
		LogicInput_Config_Common(n, 1, LogicInput##n##_1_Cfg)				\
	const Lcu_Ip_LogicInputType LogicInput##n##_2_Config =					\
		LogicInput_Config_Common(n, 2, LogicInput##n##_2_Cfg)				\
	const Lcu_Ip_LogicInputType LogicInput##n##_3_Config =					\
		LogicInput_Config_Common(n, 3, LogicInput##n##_3_Cfg)				\
												\
	const Lcu_Ip_LogicInputType								\
		*const Lcu_Ip_ppxLogicInputArray##n##_Config[LCU_IP_NOF_CFG_LOGIC_INPUTS] = {	\
			&LogicInput##n##_0_Config,						\
			&LogicInput##n##_1_Config,						\
			&LogicInput##n##_2_Config,						\
			&LogicInput##n##_3_Config,						\
		};										\
												\
	const Lcu_Ip_LogicOutputConfigType LogicOutput##n##_0_Cfg = LogicOutputCfg_Common(	\
		LCU_IP_DEBUG_DISABLE, LCU_O0_LUT,						\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 1),				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 2))				\
	const Lcu_Ip_LogicOutputConfigType LogicOutput##n##_1_Cfg = LogicOutputCfg_Common(	\
		LCU_IP_DEBUG_DISABLE, LCU_O1_LUT,						\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 4),				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 5))				\
	const Lcu_Ip_LogicOutputConfigType LogicOutput##n##_2_Cfg = LogicOutputCfg_Common(	\
		LCU_IP_DEBUG_ENABLE, LCU_O2_LUT,						\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 7),				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 8))				\
	const Lcu_Ip_LogicOutputConfigType LogicOutput##n##_3_Cfg = LogicOutputCfg_Common(	\
		LCU_IP_DEBUG_ENABLE, LCU_O3_LUT,						\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 10),				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 11))				\
												\
	const Lcu_Ip_LogicOutputType LogicOutput##n##_0_Config =				\
		LogicOutput_Config_Common(n, LogicOutput##n##_0_Cfg,				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 0))				\
	const Lcu_Ip_LogicOutputType LogicOutput##n##_1_Config =				\
		LogicOutput_Config_Common(n, LogicOutput##n##_1_Cfg,				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 3))				\
	const Lcu_Ip_LogicOutputType LogicOutput##n##_2_Config =				\
		LogicOutput_Config_Common(n, LogicOutput##n##_2_Cfg,				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 6))				\
	const Lcu_Ip_LogicOutputType LogicOutput##n##_3_Config =				\
		LogicOutput_Config_Common(n, LogicOutput##n##_3_Cfg,				\
		DT_INST_PROP_BY_IDX(n, lcu_output_filter_config, 9))				\
												\
	const Lcu_Ip_LogicOutputType								\
		*const Lcu_Ip_ppxLogicOutputArray##n##_Config[LCU_IP_NOF_CFG_LOGIC_OUTPUTS] = {	\
			&LogicOutput##n##_0_Config,						\
			&LogicOutput##n##_1_Config,						\
			&LogicOutput##n##_2_Config,						\
			&LogicOutput##n##_3_Config,						\
		};										\
												\
	const Lcu_Ip_LogicInputConfigType Lcu_Ip_LogicInputResetConfig##n = {			\
		.MuxSel = LCU_IP_MUX_SEL_LOGIC_0,						\
		.SwSynMode = LCU_IP_SW_SYNC_IMMEDIATE,						\
		.SwValue = LCU_IP_SW_OVERRIDE_LOGIC_LOW,					\
	};											\
												\
	const Lcu_Ip_LogicOutputConfigType Lcu_Ip_LogicOutputResetConfig##n =			\
		LogicOutputCfg_Common(LCU_IP_DEBUG_DISABLE, 0U, 0U, 0U)				\
												\
	const Lcu_Ip_LogicInstanceType LcuLogicInstance##n##_0_Config = {			\
		.HwInstId = LCU_NXP_S32_GET_INSTANCE(DT_INST_PHANDLE(n, lcu)),			\
		.NumLogicCellConfig = 0U,							\
		.ppxLogicCellConfigArray = NULL_PTR,						\
		.OperationMode = LCU_IP_INTERRUPT_MODE,						\
	};											\
	const Lcu_Ip_LogicInstanceType								\
	*const Lcu_Ip_ppxLogicInstanceArray##n##_Config[LCU_IP_NOF_CFG_LOGIC_INSTANCES] = {	\
			&LcuLogicInstance##n##_0_Config,					\
		};										\
												\
	Lcu_Ip_HwOutputStateType HwOutput##n##_0_State_Config;					\
	Lcu_Ip_HwOutputStateType HwOutput##n##_1_State_Config;					\
	Lcu_Ip_HwOutputStateType HwOutput##n##_2_State_Config;					\
	Lcu_Ip_HwOutputStateType HwOutput##n##_3_State_Config;					\
	Lcu_Ip_HwOutputStateType								\
		*Lcu_Ip_ppxHwOutputStateArray##n##_Config[LCU_IP_NOF_CFG_LOGIC_OUTPUTS] = {	\
			&HwOutput##n##_0_State_Config,						\
			&HwOutput##n##_1_State_Config,						\
			&HwOutput##n##_2_State_Config,						\
			&HwOutput##n##_3_State_Config,						\
	};											\
												\
	const Lcu_Ip_InitType Lcu_Ip_Init_Config##n = {						\
		.ppxHwOutputStateArray = &Lcu_Ip_ppxHwOutputStateArray##n##_Config[0],		\
		.ppxLogicInstanceConfigArray = &Lcu_Ip_ppxLogicInstanceArray##n##_Config[0],	\
		.pxLogicOutputResetConfigArray = &Lcu_Ip_LogicOutputResetConfig##n,		\
		.pxLogicInputResetConfigArray = &Lcu_Ip_LogicInputResetConfig##n,		\
		.ppxLogicOutputConfigArray = &Lcu_Ip_ppxLogicOutputArray##n##_Config[0],	\
		.ppxLogicInputConfigArray = &Lcu_Ip_ppxLogicInputArray##n##_Config[0],		\
	};

#define Trgmux_Ip_LogicTrigger_Config(n, logic_channel, output, input)				\
	{											\
		.LogicChannel = logic_channel,							\
		.Output = output,								\
		.Input = input,									\
		.HwInstId = TRGMUX_NXP_S32_GET_INSTANCE(DT_INST_PHANDLE(n, trgmux)),		\
		.Lock = (boolean)FALSE,								\
	};

#define TRGMUX_IP_INIT_CONFIG(n)								\
	const Trgmux_Ip_LogicTriggerType							\
		Trgmux_Ip_LogicTrigger##n##_0_Config = Trgmux_Ip_LogicTrigger_Config(n,		\
		DT_INST_PROP_BY_IDX(n, trgmux_io_config, 0),					\
		DT_INST_PROP_BY_IDX(n, trgmux_io_config, 1),					\
		DT_INST_PROP_BY_IDX(n, trgmux_io_config, 2))					\
	const Trgmux_Ip_LogicTriggerType							\
		Trgmux_Ip_LogicTrigger##n##_1_Config = Trgmux_Ip_LogicTrigger_Config(n,		\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 3),				\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 4),				\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 5))				\
	const Trgmux_Ip_LogicTriggerType							\
		Trgmux_Ip_LogicTrigger##n##_2_Config = Trgmux_Ip_LogicTrigger_Config(n,		\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 6),				\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 7),				\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 8))				\
	const Trgmux_Ip_LogicTriggerType							\
		Trgmux_Ip_LogicTrigger##n##_3_Config = Trgmux_Ip_LogicTrigger_Config(n,		\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 9),				\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 10),				\
			DT_INST_PROP_BY_IDX(n, trgmux_io_config, 11))				\
	const Trgmux_Ip_InitType Trgmux_Ip_Init_##n##_Config = {				\
		.paxLogicTrigger = {								\
			&Trgmux_Ip_LogicTrigger##n##_0_Config,					\
			&Trgmux_Ip_LogicTrigger##n##_1_Config,					\
			&Trgmux_Ip_LogicTrigger##n##_2_Config,					\
			&Trgmux_Ip_LogicTrigger##n##_3_Config,					\
		},										\
	};


#define QDEC_NXP_S32_INIT(n)									\
												\
	static struct qdec_s32_data qdec_s32_##n##_data = {					\
		.micro_ticks_per_rev = (double)(DT_INST_PROP(n, micro_ticks_per_rev) / 1000000),\
		.counter_CW = 1,								\
		.counter_CCW = 1,								\
	};											\
												\
	PINCTRL_DT_INST_DEFINE(n);								\
	TRGMUX_IP_INIT_CONFIG(n)								\
	LCU_IP_INIT_CONFIG(n)									\
	EMIOS_NXP_S32_MCB_OVERFLOW_CALLBACK(n)							\
												\
	static const struct qdec_s32_config qdec_s32_##n##_config = {				\
		.emios_inst = EMIOS_NXP_S32_GET_INSTANCE(DT_INST_PHANDLE(n, emios)),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.trgmux_config = &Trgmux_Ip_Init_##n##_Config,					\
		.lcu_config = &Lcu_Ip_Init_Config##n,						\
		.emios_channels = {DT_INST_PROP_BY_IDX(n, emios_channels, EMIOS_CW_CH_IDX),	\
				   DT_INST_PROP_BY_IDX(n, emios_channels, EMIOS_CCW_CH_IDX)},	\
		.emios_cw_overflow_cb = &qdec##n##_emios_overflow_count_cw_callback,		\
		.emios_ccw_overflow_cb = &qdec##n##_emios_overflow_count_ccw_callback,		\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(n, qdec_s32_initialize, NULL, &qdec_s32_##n##_data,	\
				     &qdec_s32_##n##_config, POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY, &qdec_s32_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_NXP_S32_INIT)
