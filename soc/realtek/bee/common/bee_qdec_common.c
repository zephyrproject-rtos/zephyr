/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bee_qdec_common.h"

/* Basic QDEC Implementations */

#if defined(CONFIG_HAL_REALTEK_BEE_QDEC)

static void basic_qdec_get_scale(uint8_t counts, uint32_t *scale)
{
	if (counts == 2) {
		*scale = CounterScale_2_Phase;
	} else {
		*scale = CounterScale_1_Phase;
	}
}

static int basic_qdec_init(uint32_t reg, const struct bee_qdec_axis_config *axis_cfgs)
{
	QDEC_InitTypeDef qdec_init_struct;
	QDEC_TypeDef *qdec = (QDEC_TypeDef *)reg;
	uint32_t scale_val = CounterScale_1_Phase;

	QDEC_StructInit(&qdec_init_struct);
	if (axis_cfgs[BEE_QDEC_AXIS_X].enable) {
		basic_qdec_get_scale(axis_cfgs[BEE_QDEC_AXIS_X].counts_per_revolution, &scale_val);
		qdec_init_struct.axisConfigX = ENABLE;
		qdec_init_struct.debounceEnableX = ENABLE;
		qdec_init_struct.debounceTimeX = 32 * axis_cfgs[BEE_QDEC_AXIS_X].debounce_time_ms;
		qdec_init_struct.initPhaseX = phaseMode0;
		qdec_init_struct.counterScaleX = scale_val;
	}

	if (axis_cfgs[BEE_QDEC_AXIS_Y].enable) {
		basic_qdec_get_scale(axis_cfgs[BEE_QDEC_AXIS_Y].counts_per_revolution, &scale_val);
		qdec_init_struct.axisConfigY = ENABLE;
		qdec_init_struct.debounceEnableY = ENABLE;
		qdec_init_struct.debounceTimeY = 32 * axis_cfgs[BEE_QDEC_AXIS_Y].debounce_time_ms;
		qdec_init_struct.initPhaseY = phaseMode0;
		qdec_init_struct.counterScaleY = scale_val;
	}

	if (axis_cfgs[BEE_QDEC_AXIS_Z].enable) {
		basic_qdec_get_scale(axis_cfgs[BEE_QDEC_AXIS_Z].counts_per_revolution, &scale_val);
		qdec_init_struct.axisConfigZ = ENABLE;
		qdec_init_struct.debounceEnableZ = ENABLE;
		qdec_init_struct.debounceTimeZ = 32 * axis_cfgs[BEE_QDEC_AXIS_Z].debounce_time_ms;
		qdec_init_struct.initPhaseZ = phaseMode0;
		qdec_init_struct.counterScaleZ = scale_val;
	}

	qdec_init_struct.manualLoadInitPhase = ENABLE;

	QDEC_Init(qdec, &qdec_init_struct);

	return 0;
}

static void basic_qdec_enable(uint32_t reg, enum bee_qdec_axis axis)
{
	uint32_t hal_axis;

	if (axis == BEE_QDEC_AXIS_X) {
		hal_axis = QDEC_AXIS_X;
	} else if (axis == BEE_QDEC_AXIS_Y) {
		hal_axis = QDEC_AXIS_Y;
	} else {
		hal_axis = QDEC_AXIS_Z;
	}

	QDEC_Cmd((QDEC_TypeDef *)reg, hal_axis, ENABLE);
}

static void basic_qdec_disable(uint32_t reg, enum bee_qdec_axis axis)
{
	uint32_t hal_axis;

	if (axis == BEE_QDEC_AXIS_X) {
		hal_axis = QDEC_AXIS_X;
	} else if (axis == BEE_QDEC_AXIS_Y) {
		hal_axis = QDEC_AXIS_Y;
	} else {
		hal_axis = QDEC_AXIS_Z;
	}

	QDEC_Cmd((QDEC_TypeDef *)reg, hal_axis, DISABLE);
}

static uint16_t basic_qdec_get_count(uint32_t reg, enum bee_qdec_axis axis)
{
	uint32_t hal_axis;

	if (axis == BEE_QDEC_AXIS_X) {
		hal_axis = QDEC_AXIS_X;
	} else if (axis == BEE_QDEC_AXIS_Y) {
		hal_axis = QDEC_AXIS_Y;
	} else {
		hal_axis = QDEC_AXIS_Z;
	}

	return QDEC_GetAxisCount((QDEC_TypeDef *)reg, hal_axis);
}

static void basic_get_int_params(enum bee_qdec_axis axis, enum bee_qdec_event_type type,
				 uint32_t *mask, uint32_t *flag)
{
	if (axis == BEE_QDEC_AXIS_X) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			*mask = QDEC_X_CT_INT_MASK;
			*flag = QDEC_X_INT_NEW_DATA;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			*mask = QDEC_X_ILLEGAL_INT_MASK;
			*flag = QDEC_X_INT_ILLEGAL;
		} else {
			/* Do nothing */
		}

	} else if (axis == BEE_QDEC_AXIS_Y) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			*mask = QDEC_Y_CT_INT_MASK;
			*flag = QDEC_Y_INT_NEW_DATA;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			*mask = QDEC_Y_ILLEGAL_INT_MASK;
			*flag = QDEC_Y_INT_ILLEGAL;
		} else {
			/* Do nothing */
		}
	} else if (axis == BEE_QDEC_AXIS_Z) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			*mask = QDEC_Z_CT_INT_MASK;
			*flag = QDEC_Z_INT_NEW_DATA;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			*mask = QDEC_Z_ILLEGAL_INT_MASK;
			*flag = QDEC_Z_INT_ILLEGAL;
		} else {
			/* Do nothing */
		}
	} else {
		/* Do nothing */
	}
}

static void basic_qdec_int_enable(uint32_t reg, enum bee_qdec_axis axis,
				  enum bee_qdec_event_type type)
{
	uint32_t mask_val = 0, config_val = 0;

	basic_get_int_params(axis, type, &mask_val, &config_val);

	if (type == BEE_QDEC_EVENT_NEW_DATA || type == BEE_QDEC_EVENT_ILLEGAL) {
		QDEC_INTMask((QDEC_TypeDef *)reg, mask_val, DISABLE);
		QDEC_INTConfig((QDEC_TypeDef *)reg, config_val, ENABLE);
	}
}

static void basic_qdec_int_disable(uint32_t reg, enum bee_qdec_axis axis,
				   enum bee_qdec_event_type type)
{
	uint32_t mask_val = 0, config_val = 0;

	basic_get_int_params(axis, type, &mask_val, &config_val);

	if (type == BEE_QDEC_EVENT_NEW_DATA || type == BEE_QDEC_EVENT_ILLEGAL) {
		QDEC_INTMask((QDEC_TypeDef *)reg, mask_val, ENABLE);
		QDEC_INTConfig((QDEC_TypeDef *)reg, config_val, DISABLE);
	}
}

static void basic_qdec_int_clear(uint32_t reg, enum bee_qdec_axis axis,
				 enum bee_qdec_event_type type)
{
	uint32_t val = 0;

	if (axis == BEE_QDEC_AXIS_X) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			val = QDEC_CLR_NEW_CT_X;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			val = QDEC_CLR_ILLEGAL_INT_X;
			QDEC_ClearINTPendingBit((QDEC_TypeDef *)reg, QDEC_CLR_ILLEGAL_CT_X);
		} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
			val = QDEC_CLR_OVERFLOW_X;
		} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
			val = QDEC_CLR_UNDERFLOW_X;
		} else {
			/* Do nothing */
		}
	} else if (axis == BEE_QDEC_AXIS_Y) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			val = QDEC_CLR_NEW_CT_Y;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			val = QDEC_CLR_ILLEGAL_INT_Y;
			QDEC_ClearINTPendingBit((QDEC_TypeDef *)reg, QDEC_CLR_ILLEGAL_CT_Y);
		} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
			val = QDEC_CLR_OVERFLOW_Y;
		} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
			val = QDEC_CLR_UNDERFLOW_Y;
		} else {
			/* Do nothing */
		}
	} else if (axis == BEE_QDEC_AXIS_Z) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			val = QDEC_CLR_NEW_CT_Z;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			val = QDEC_CLR_ILLEGAL_INT_Z;
			QDEC_ClearINTPendingBit((QDEC_TypeDef *)reg, QDEC_CLR_ILLEGAL_CT_Z);
		} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
			val = QDEC_CLR_OVERFLOW_Z;
		} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
			val = QDEC_CLR_UNDERFLOW_Z;
		} else {
			/* Do nothing */
		}
	} else {
		/* Do nothing */
	}

	if (val) {
		QDEC_ClearINTPendingBit((QDEC_TypeDef *)reg, val);
	}
}

static bool basic_qdec_get_status(uint32_t reg, enum bee_qdec_axis axis,
				  enum bee_qdec_event_type type)
{
	uint32_t flag = 0;

	if (axis == BEE_QDEC_AXIS_X) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			flag = QDEC_FLAG_NEW_CT_STATUS_X;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			flag = QDEC_FLAG_ILLEGAL_STATUS_X;
		} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
			flag = QDEC_FLAG_OVERFLOW_X;
		} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
			flag = QDEC_FLAG_UNDERFLOW_X;
		} else {
			/* Do nothing */
		}
	} else if (axis == BEE_QDEC_AXIS_Y) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			flag = QDEC_FLAG_NEW_CT_STATUS_Y;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			flag = QDEC_FLAG_ILLEGAL_STATUS_Y;
		} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
			flag = QDEC_FLAG_OVERFLOW_Y;
		} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
			flag = QDEC_FLAG_UNDERFLOW_Y;
		} else {
			/* Do nothing */
		}
	} else if (axis == BEE_QDEC_AXIS_Z) {
		if (type == BEE_QDEC_EVENT_NEW_DATA) {
			flag = QDEC_FLAG_NEW_CT_STATUS_Z;
		} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
			flag = QDEC_FLAG_ILLEGAL_STATUS_Z;
		} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
			flag = QDEC_FLAG_OVERFLOW_Z;
		} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
			flag = QDEC_FLAG_UNDERFLOW_Z;
		} else {
			/* Do nothing */
		}
	} else {
		/* Do nothing */
	}

	return QDEC_GetFlagState((QDEC_TypeDef *)reg, flag) ? true : false;
}

static const struct bee_qdec_ops bee_basic_qdec_ops = {
	.init = basic_qdec_init,
	.enable = basic_qdec_enable,
	.disable = basic_qdec_disable,
	.get_count = basic_qdec_get_count,
	.int_enable = basic_qdec_int_enable,
	.int_disable = basic_qdec_int_disable,
	.int_clear = basic_qdec_int_clear,
	.get_status = basic_qdec_get_status,
};

#endif /* CONFIG_HAL_REALTEK_BEE_QDEC */

/* AON QDEC Implementations */

#if defined(CONFIG_HAL_REALTEK_BEE_AON_QDEC)

static int aon_qdec_init(uint32_t reg, const struct bee_qdec_axis_config *axis_cfgs)
{
	AON_QDEC_InitTypeDef qdec_init_struct;
	AON_QDEC_TypeDef *qdec = (AON_QDEC_TypeDef *)reg;

	if (axis_cfgs[BEE_QDEC_AXIS_Y].enable || axis_cfgs[BEE_QDEC_AXIS_Z].enable) {
		return -ENOTSUP;
	}

	if (!axis_cfgs[BEE_QDEC_AXIS_X].enable) {
		return 0;
	}

	AON_QDEC_StructInit(&qdec_init_struct);
	qdec_init_struct.axisConfigX = ENABLE;
	qdec_init_struct.debounceEnableX = ENABLE;
	qdec_init_struct.debounceTimeX = 32 * axis_cfgs[BEE_QDEC_AXIS_X].debounce_time_ms;
	qdec_init_struct.initPhaseX = phaseMode0;

	if (axis_cfgs[BEE_QDEC_AXIS_X].counts_per_revolution == 2) {
		qdec_init_struct.counterScaleX = CounterScale_2_Phase;
	} else if (axis_cfgs[BEE_QDEC_AXIS_X].counts_per_revolution == 4) {
		qdec_init_struct.counterScaleX = CounterScale_1_Phase;
	} else {
		return -ENOTSUP;
	}

	qdec_init_struct.manualLoadInitPhase = ENABLE;

	AON_QDEC_DeInit();

	AON_QDEC_Init(qdec, &qdec_init_struct);

	return 0;
}

static void aon_qdec_enable(uint32_t reg, enum bee_qdec_axis axis)
{
	if (axis == BEE_QDEC_AXIS_X) {
		AON_QDEC_Cmd((AON_QDEC_TypeDef *)reg, AON_QDEC_AXIS_X, ENABLE);
	}
}

static void aon_qdec_disable(uint32_t reg, enum bee_qdec_axis axis)
{
	if (axis == BEE_QDEC_AXIS_X) {
		AON_QDEC_Cmd((AON_QDEC_TypeDef *)reg, AON_QDEC_AXIS_X, DISABLE);
	}
}

static uint16_t aon_qdec_get_count(uint32_t reg, enum bee_qdec_axis axis)
{
	if (axis == BEE_QDEC_AXIS_X) {
		return AON_QDEC_GetAxisCount((AON_QDEC_TypeDef *)reg, AON_QDEC_AXIS_X);
	}
	return 0;
}

static void aon_qdec_int_enable(uint32_t reg, enum bee_qdec_axis axis,
				enum bee_qdec_event_type type)
{
	AON_QDEC_TypeDef *qdec = (AON_QDEC_TypeDef *)reg;

	if (axis != BEE_QDEC_AXIS_X) {
		return;
	}

	if (type == BEE_QDEC_EVENT_NEW_DATA) {
		AON_QDEC_INTMask(qdec, AON_QDEC_X_CT_INT_MASK, DISABLE);
		AON_QDEC_INTConfig(qdec, AON_QDEC_X_INT_NEW_DATA, ENABLE);
	} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
		AON_QDEC_INTMask(qdec, AON_QDEC_X_ILLEAGE_INT_MASK, DISABLE);
		AON_QDEC_INTConfig(qdec, AON_QDEC_X_INT_ILLEAGE, ENABLE);
	} else {
		/* Do nothing */
	}
}

static void aon_qdec_int_disable(uint32_t reg, enum bee_qdec_axis axis,
				 enum bee_qdec_event_type type)
{
	AON_QDEC_TypeDef *qdec = (AON_QDEC_TypeDef *)reg;

	if (axis != BEE_QDEC_AXIS_X) {
		return;
	}

	if (type == BEE_QDEC_EVENT_NEW_DATA) {
		AON_QDEC_INTMask(qdec, AON_QDEC_X_CT_INT_MASK, ENABLE);
		AON_QDEC_INTConfig(qdec, AON_QDEC_X_INT_NEW_DATA, DISABLE);
	} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
		AON_QDEC_INTMask(qdec, AON_QDEC_X_ILLEAGE_INT_MASK, ENABLE);
		AON_QDEC_INTConfig(qdec, AON_QDEC_X_INT_ILLEAGE, DISABLE);
	} else {
		/* Do nothing */
	}
}

static void aon_qdec_int_clear(uint32_t reg, enum bee_qdec_axis axis, enum bee_qdec_event_type type)
{
	if (axis != BEE_QDEC_AXIS_X) {
		return;
	}

	AON_QDEC_TypeDef *qdec = (AON_QDEC_TypeDef *)reg;
	uint32_t val = 0;

	if (type == BEE_QDEC_EVENT_NEW_DATA) {
		val = AON_QDEC_CLR_NEW_CT_X;
	} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
		val = AON_QDEC_CLR_ILLEGAL_INT_X;
		AON_QDEC_ClearINTPendingBit(qdec, AON_QDEC_CLR_ILLEGAL_CT_X);
	} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
		val = AON_QDEC_CLR_OVERFLOW_X;
	} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
		val = AON_QDEC_CLR_UNDERFLOW_X;
	} else {
		/* Do nothing */
	}

	if (val) {
		AON_QDEC_ClearINTPendingBit(qdec, val);
	}
}

static bool aon_qdec_get_status(uint32_t reg, enum bee_qdec_axis axis,
				enum bee_qdec_event_type type)
{
	if (axis != BEE_QDEC_AXIS_X) {
		return false;
	}

	AON_QDEC_TypeDef *qdec = (AON_QDEC_TypeDef *)reg;
	uint32_t flag = 0;

	if (type == BEE_QDEC_EVENT_NEW_DATA) {
		flag = AON_QDEC_FLAG_NEW_CT_STATUS_X;
	} else if (type == BEE_QDEC_EVENT_ILLEGAL) {
		flag = AON_QDEC_FLAG_ILLEGAL_STATUS_X;
	} else if (type == BEE_QDEC_EVENT_OVERFLOW) {
		flag = AON_QDEC_FLAG_OVERFLOW_X;
	} else if (type == BEE_QDEC_EVENT_UNDERFLOW) {
		flag = AON_QDEC_FLAG_UNDERFLOW_X;
	} else {
		/* Do nothing */
	}

	return AON_QDEC_GetFlagState(qdec, flag) ? true : false;
}

static const struct bee_qdec_ops bee_aon_qdec_ops = {
	.init = aon_qdec_init,
	.enable = aon_qdec_enable,
	.disable = aon_qdec_disable,
	.get_count = aon_qdec_get_count,
	.int_enable = aon_qdec_int_enable,
	.int_disable = aon_qdec_int_disable,
	.int_clear = aon_qdec_int_clear,
	.get_status = aon_qdec_get_status,
};

#endif /* CONFIG_HAL_REALTEK_BEE_AON_QDEC */

const struct bee_qdec_ops *bee_qdec_get_ops(void)
{
#if defined(CONFIG_HAL_REALTEK_BEE_AON_QDEC)
	return &bee_aon_qdec_ops;
#elif defined(CONFIG_HAL_REALTEK_BEE_QDEC)
	return &bee_basic_qdec_ops;
#else
	return NULL;
#endif
}
