/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_mxcordic

#include <math.h>

#include <zephyr/drivers/misc/infineon_mxcordic/cordic_infineon_mxcordic.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <cy_cordic.h>

LOG_MODULE_REGISTER(infineon_mxcordic, CONFIG_INFINEON_MXCORDIC_LOG_LEVEL);

#define IFX_CORDIC_TIMEOUT K_MSEC(100)

/* Fixed-point scaling factors */
#define IFX_CORDIC_Q31_SCALE (2147483648.0) /* 2^31 */
#define IFX_CORDIC_Q30_SCALE (1073741824.0) /* 2^30 */
#define IFX_CORDIC_Q23_SCALE (8388608.0)    /* 2^23 */
#define IFX_CORDIC_Q11_SCALE (2048.0)       /* 2^11 */

/*
 * Vectoring-mode operands must satisfy the PDL 8Q23 valid range
 * [-2^23, 2^23 - 1]. Only the operand ratio matters for atan2/atanh
 */
#define IFX_CORDIC_VECTOR_IN_SCALE (4194304.0) /* 2^22 */

#define IFX_CORDIC_PI            (3.14159265358979323846)
#define IFX_CORDIC_CIRCULAR_GAIN (1.646760258121)

/* Hardware convergence limits (radians / input magnitude) */
#define IFX_CORDIC_CIRCULAR_MAX_RAD (1.74f)
#define IFX_CORDIC_HYP_MAX_RAD      (1.11f)
#define IFX_CORDIC_ATANH_MAX        (0.8f)

/* Keep scaled vector components small enough that gain*hypot stays inside the
 * signed 24-bit result field.
 */
#define IFX_CORDIC_VECTOR_SCALED_MAX (0.3)

/* Square-root helper constants */
#define IFX_CORDIC_SQRT_QUARTER   (0x8000000UL)
#define IFX_CORDIC_SQRT_Q15_SCALE (39568UL)

struct ifx_mxcordic_config {
	MXCORDIC_Type *base;
	void (*irq_config)(const struct device *dev);
};

struct ifx_mxcordic_data {
	struct k_mutex lock;
	struct k_sem done;
	uint32_t intr_status;
	bool faulted;
};

static inline CY_CORDIC_Q31_t mxcordic_encode_angle(float angle)
{
	return (CY_CORDIC_Q31_t)((double)angle * (IFX_CORDIC_Q31_SCALE / IFX_CORDIC_PI));
}

static int mxcordic_arm(const struct device *dev)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	int ret;

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	if (data->faulted) {
		k_mutex_unlock(&data->lock);
		return -EIO;
	}

	Cy_CORDIC_ClearInterrupt(cfg->base, CY_CORDIC_INTR_MASK);
	Cy_CORDIC_SetInterruptMask(cfg->base, CY_CORDIC_INTR_MASK);

	return 0;
}

static int mxcordic_wait(const struct device *dev)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	int ret;

	ret = k_sem_take(&data->done, IFX_CORDIC_TIMEOUT);
	if (ret != 0) {
		unsigned int key = irq_lock();

		data->faulted = true;
		Cy_CORDIC_SetInterruptMask(cfg->base, 0U);
		Cy_CORDIC_Disable(cfg->base);
		Cy_CORDIC_ClearInterrupt(cfg->base, CY_CORDIC_INTR_MASK);
		k_sem_reset(&data->done);
		data->intr_status = 0U;
		irq_unlock(key);

		LOG_ERR("%s: completion timeout; accelerator disabled until reboot", dev->name);
		return -ETIMEDOUT;
	}

	if ((data->intr_status & CY_CORDIC_INTR_ERROR_EVENT) != 0U) {
		LOG_ERR("%s: hardware error (interrupt status 0x%08x)", dev->name,
			data->intr_status);
		return -EIO;
	}

	return 0;
}

static void mxcordic_isr(const struct device *dev)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	uint32_t status = Cy_CORDIC_GetInterruptStatusMasked(cfg->base);

	if (status == 0U) {
		return;
	}

	/* Mask further events and acknowledge before waking the waiter. */
	Cy_CORDIC_SetInterruptMask(cfg->base, 0U);
	Cy_CORDIC_ClearInterrupt(cfg->base, status);

	data->intr_status = status;
	k_sem_give(&data->done);
}

enum ifx_mxcordic_operation {
	IFX_MXCORDIC_OP_SIN,
	IFX_MXCORDIC_OP_COS,
	IFX_MXCORDIC_OP_TAN,
	IFX_MXCORDIC_OP_SINH,
	IFX_MXCORDIC_OP_COSH,
	IFX_MXCORDIC_OP_TANH,
	IFX_MXCORDIC_OP_ATANH,
};

struct ifx_mxcordic_operation_config {
	void (*launch)(MXCORDIC_Type *base, int32_t input);
	int32_t (*read)(MXCORDIC_Type *base);
};

static void mxcordic_launch_atanh(MXCORDIC_Type *base, int32_t input)
{
	Cy_CORDIC_ArcTanhNB(base, (CY_CORDIC_8Q23_t)IFX_CORDIC_VECTOR_IN_SCALE, input);
}

/* clang-format off */
static const struct ifx_mxcordic_operation_config mxcordic_operations[] = {
	[IFX_MXCORDIC_OP_SIN] = {
		.launch = Cy_CORDIC_SinNB,
		.read = Cy_CORDIC_GetSinResult,
	},
	[IFX_MXCORDIC_OP_COS] = {
		.launch = Cy_CORDIC_CosNB,
		.read = Cy_CORDIC_GetCosResult,
	},
	[IFX_MXCORDIC_OP_TAN] = {
		.launch = Cy_CORDIC_TanNB,
		.read = Cy_CORDIC_GetTanResult,
	},
	[IFX_MXCORDIC_OP_SINH] = {
		.launch = Cy_CORDIC_SinhNB,
		.read = Cy_CORDIC_GetSinhResult,
	},
	[IFX_MXCORDIC_OP_COSH] = {
		.launch = Cy_CORDIC_CoshNB,
		.read = Cy_CORDIC_GetCoshResult,
	},
	[IFX_MXCORDIC_OP_TANH] = {
		.launch = Cy_CORDIC_TanhNB,
		.read = Cy_CORDIC_GetTanhResult,
	},
	[IFX_MXCORDIC_OP_ATANH] = {
		.launch = mxcordic_launch_atanh,
		.read = Cy_CORDIC_GetArcTanhResult,
	},
};
/* clang-format on */

static int mxcordic_execute(const struct device *dev, enum ifx_mxcordic_operation operation,
			    int32_t input, int32_t *result)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	const struct ifx_mxcordic_operation_config *op = &mxcordic_operations[operation];
	int ret;

	ret = mxcordic_arm(dev);
	if (ret != 0) {
		return ret;
	}

	op->launch(cfg->base, input);
	ret = mxcordic_wait(dev);
	if (ret == 0) {
		*result = op->read(cfg->base);
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

int cordic_ifx_mxcordic_sin(const struct device *dev, float angle, float *result)
{
	CY_CORDIC_Q31_t res;
	int ret;

	if ((result == NULL) || !isfinite(angle) || (fabsf(angle) > IFX_CORDIC_CIRCULAR_MAX_RAD)) {
		return -EINVAL;
	}

	ret = mxcordic_execute(dev, IFX_MXCORDIC_OP_SIN, mxcordic_encode_angle(angle), &res);
	if (ret != 0) {
		return ret;
	}

	*result = (float)((double)res / IFX_CORDIC_Q31_SCALE);

	return 0;
}

int cordic_ifx_mxcordic_cos(const struct device *dev, float angle, float *result)
{
	CY_CORDIC_Q31_t res;
	int ret;

	if ((result == NULL) || !isfinite(angle) || (fabsf(angle) > IFX_CORDIC_CIRCULAR_MAX_RAD)) {
		return -EINVAL;
	}

	ret = mxcordic_execute(dev, IFX_MXCORDIC_OP_COS, mxcordic_encode_angle(angle), &res);
	if (ret != 0) {
		return ret;
	}

	*result = (float)((double)res / IFX_CORDIC_Q31_SCALE);

	return 0;
}

int cordic_ifx_mxcordic_tan(const struct device *dev, float angle, float *result)
{
	CY_CORDIC_20Q11_t res;
	int ret;

	if ((result == NULL) || !isfinite(angle) || (fabsf(angle) > IFX_CORDIC_CIRCULAR_MAX_RAD)) {
		return -EINVAL;
	}

	ret = mxcordic_execute(dev, IFX_MXCORDIC_OP_TAN, mxcordic_encode_angle(angle), &res);
	if (ret != 0) {
		return ret;
	}

	*result = (float)((double)res / IFX_CORDIC_Q11_SCALE);

	return 0;
}

int cordic_ifx_mxcordic_atan2(const struct device *dev, float y, float x, float *result)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	double angle;
	float m, scaled_x, scaled_y, sign_x, sign_y;
	bool x_positive;
	CY_CORDIC_Q31_t res;
	int ret;

	if ((result == NULL) || !isfinite(x) || !isfinite(y)) {
		return -EINVAL;
	}

	/* The x axis (denominator) must be non-zero for the vectoring engine. */
	if (x == 0.0f) {
		if (y > 0.0f) {
			*result = (float)(IFX_CORDIC_PI / 2.0);
		} else if (y < 0.0f) {
			*result = (float)(-IFX_CORDIC_PI / 2.0);
		} else {
			*result = 0.0f;
		}
		return 0;
	}

	m = (fabsf(x) > fabsf(y)) ? fabsf(x) : fabsf(y);
	scaled_x = x / m;
	scaled_y = y / m;
	x_positive = x > 0.0f;
	sign_x = x_positive ? scaled_x : -scaled_x;
	sign_y = x_positive ? scaled_y : -scaled_y;

	ret = mxcordic_arm(dev);
	if (ret != 0) {
		return ret;
	}

	Cy_CORDIC_ArcTanNB(cfg->base,
			   (CY_CORDIC_8Q23_t)((double)sign_x * IFX_CORDIC_VECTOR_IN_SCALE),
			   (CY_CORDIC_8Q23_t)((double)sign_y * IFX_CORDIC_VECTOR_IN_SCALE));
	ret = mxcordic_wait(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}
	res = Cy_CORDIC_GetArcTanResult(cfg->base);
	k_mutex_unlock(&data->lock);

	angle = (double)res * (IFX_CORDIC_PI / IFX_CORDIC_Q31_SCALE);
	if (x < 0.0f) {
		angle += (y >= 0.0f) ? IFX_CORDIC_PI : -IFX_CORDIC_PI;
	}

	*result = (float)angle;

	return 0;
}

int cordic_ifx_mxcordic_asin(const struct device *dev, float x, float *result)
{
	float adjacent;

	if ((result == NULL) || !isfinite(x) || (fabsf(x) > 1.0f)) {
		return -EINVAL;
	}

	adjacent = sqrtf(1.0f - x * x);
	return cordic_ifx_mxcordic_atan2(dev, x, adjacent, result);
}

int cordic_ifx_mxcordic_acos(const struct device *dev, float x, float *result)
{
	float opposite;

	if ((result == NULL) || !isfinite(x) || (fabsf(x) > 1.0f)) {
		return -EINVAL;
	}

	opposite = sqrtf(1.0f - x * x);
	return cordic_ifx_mxcordic_atan2(dev, opposite, x, result);
}

int cordic_ifx_mxcordic_sinh(const struct device *dev, float angle, float *result)
{
	CY_CORDIC_1Q30_t res;
	int ret;

	if ((result == NULL) || !isfinite(angle) || (fabsf(angle) >= IFX_CORDIC_HYP_MAX_RAD)) {
		return -EINVAL;
	}

	ret = mxcordic_execute(dev, IFX_MXCORDIC_OP_SINH, mxcordic_encode_angle(angle), &res);
	if (ret != 0) {
		return ret;
	}

	*result = (float)((double)res / IFX_CORDIC_Q30_SCALE);

	return 0;
}

int cordic_ifx_mxcordic_cosh(const struct device *dev, float angle, float *result)
{
	CY_CORDIC_1Q30_t res;
	int ret;

	if ((result == NULL) || !isfinite(angle) || (fabsf(angle) >= IFX_CORDIC_HYP_MAX_RAD)) {
		return -EINVAL;
	}

	ret = mxcordic_execute(dev, IFX_MXCORDIC_OP_COSH, mxcordic_encode_angle(angle), &res);
	if (ret != 0) {
		return ret;
	}

	*result = (float)((double)res / IFX_CORDIC_Q30_SCALE);

	return 0;
}

int cordic_ifx_mxcordic_tanh(const struct device *dev, float angle, float *result)
{
	CY_CORDIC_20Q11_t res;
	int ret;

	if ((result == NULL) || !isfinite(angle) || (fabsf(angle) >= IFX_CORDIC_HYP_MAX_RAD)) {
		return -EINVAL;
	}

	ret = mxcordic_execute(dev, IFX_MXCORDIC_OP_TANH, mxcordic_encode_angle(angle), &res);
	if (ret != 0) {
		return ret;
	}

	*result = (float)((double)res / IFX_CORDIC_Q11_SCALE);

	return 0;
}

int cordic_ifx_mxcordic_atanh(const struct device *dev, float x, float *result)
{
	CY_CORDIC_Q31_t res;
	int ret;

	if ((result == NULL) || !isfinite(x) || (fabsf(x) >= IFX_CORDIC_ATANH_MAX)) {
		return -EINVAL;
	}

	/* atanh(x) computed as atanh(y/x') with x' = 1.0 and y = x. */
	ret = mxcordic_execute(dev, IFX_MXCORDIC_OP_ATANH,
			       (CY_CORDIC_8Q23_t)((double)x * IFX_CORDIC_VECTOR_IN_SCALE), &res);
	if (ret != 0) {
		return ret;
	}

	*result = (float)((double)res * (IFX_CORDIC_PI / IFX_CORDIC_Q31_SCALE));

	return 0;
}

int cordic_ifx_mxcordic_sqrt(const struct device *dev, float x, float *result)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	MXCORDIC_Type *base = cfg->base;
	float scale = 1.0f;
	uint32_t xq, raw;
	int ret;

	if ((result == NULL) || !(x > 0.0f && x <= 1.0f)) {
		return -EINVAL;
	}

	while (x < 0.25f) {
		x *= 4.0f;
		scale *= 0.5f;
	}

	/* Hyperbolic vectoring square root, sqrt(x) = sqrt((x+0.25)^2 - (x-0.25)^2) */
	xq = (uint32_t)((double)x * IFX_CORDIC_Q31_SCALE) >> 2UL;

	ret = mxcordic_arm(dev);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Cy_CORDIC_Sqrt() launches and reads the result in one blocking call,
	 * with no nonblocking variant. Use the same register sequence so mxcordic_wait()
	 * can handle timeout or hardware errors before reading CORRX.
	 */
	MXCORDIC_CON(base) = _VAL2FLD(MXCORDIC_CON_MODE, CY_CORDIC_OPERATING_MODE_HYPERBOLIC) |
			     _VAL2FLD(MXCORDIC_CON_ROTVEC, CY_CORDIC_ROTVEC_MODE_VECTORING) |
			     _VAL2FLD(MXCORDIC_CON_N_ITER, CY_CORDIC_NUMBER_OF_ITERATIONS);
	MXCORDIC_CORDX(base) = (uint32_t)(xq + IFX_CORDIC_SQRT_QUARTER);
	MXCORDIC_CORDY(base) = (uint32_t)(xq - IFX_CORDIC_SQRT_QUARTER);
	MXCORDIC_CORDZ(base) = 0UL;
	ret = mxcordic_wait(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}
	raw = MXCORDIC_CORRX(base);
	k_mutex_unlock(&data->lock);

	*result = (float)((double)((raw >> 13UL) * IFX_CORDIC_SQRT_Q15_SCALE) /
			  IFX_CORDIC_Q31_SCALE) *
		  scale;

	return 0;
}

int cordic_ifx_mxcordic_magnitude(const struct device *dev, float x, float y, float *result)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	MXCORDIC_Type *base = cfg->base;
	double scale, xs, ys;
	float m;
	int32_t raw;
	int ret;

	if ((result == NULL) || !isfinite(x) || !isfinite(y)) {
		return -EINVAL;
	}

	if ((x == 0.0f) && (y == 0.0f)) {
		*result = 0.0f;
		return 0;
	}

	/*
	 * Magnitude is homogeneous of degree one, so scale the inputs into the
	 * valid hardware range and undo the scaling on the result.
	 */
	m = (fabsf(x) > fabsf(y)) ? fabsf(x) : fabsf(y);
	scale = IFX_CORDIC_VECTOR_SCALED_MAX / (double)m;

	xs = (double)x * scale;
	ys = (double)y * scale;

	ret = mxcordic_arm(dev);
	if (ret != 0) {
		return ret;
	}

	/*
	 * The PDL has no dedicated magnitude API. Circular vectoring leaves the
	 * gain-scaled magnitude in CORRX; direct access retrieves it after mxcordic_wait()
	 * confirms completion, then the gain and input scaling are removed below.
	 */
	MXCORDIC_CON(base) = _VAL2FLD(MXCORDIC_CON_MODE, CY_CORDIC_OPERATING_MODE_CIRCULAR) |
			     _VAL2FLD(MXCORDIC_CON_ROTVEC, CY_CORDIC_ROTVEC_MODE_VECTORING) |
			     _VAL2FLD(MXCORDIC_CON_N_ITER, CY_CORDIC_NUMBER_OF_ITERATIONS);
	MXCORDIC_CORDX(base) =
		_VAL2FLD(MXCORDIC_CORDX_DATA, (uint32_t)(int32_t)(xs * IFX_CORDIC_Q23_SCALE));
	MXCORDIC_CORDY(base) =
		_VAL2FLD(MXCORDIC_CORDY_DATA, (uint32_t)(int32_t)(ys * IFX_CORDIC_Q23_SCALE));
	MXCORDIC_CORDZ(base) = 0UL;

	ret = mxcordic_wait(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}
	raw = (int32_t)MXCORDIC_CORRX(base);
	k_mutex_unlock(&data->lock);

	/* raw / 2^31 == gain * hypot(xs, ys); undo gain and input scaling. */
	*result =
		(float)((((double)raw / IFX_CORDIC_Q31_SCALE) / IFX_CORDIC_CIRCULAR_GAIN) / scale);

	return 0;
}

int cordic_ifx_mxcordic_park_transform(const struct device *dev, float angle, float alpha,
				       float beta, float *d, float *q)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;
	cy_stc_cordic_parkTransform_result_t res;
	int ret;

	if ((d == NULL) || (q == NULL) || !isfinite(angle) ||
	    (fabsf(angle) > IFX_CORDIC_CIRCULAR_MAX_RAD) || !(alpha >= -1.0f && alpha < 1.0f) ||
	    !(beta >= -1.0f && beta < 1.0f)) {
		return -EINVAL;
	}

	ret = mxcordic_arm(dev);
	if (ret != 0) {
		return ret;
	}

	Cy_CORDIC_ParkTransformNB(cfg->base, mxcordic_encode_angle(angle),
				  (CY_CORDIC_Q31_t)((double)alpha * IFX_CORDIC_Q31_SCALE),
				  (CY_CORDIC_Q31_t)((double)beta * IFX_CORDIC_Q31_SCALE));
	ret = mxcordic_wait(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}
	Cy_CORDIC_GetParkResult(cfg->base, &res);
	k_mutex_unlock(&data->lock);

	*d = (float)((double)res.parkTransformId / IFX_CORDIC_Q23_SCALE);
	*q = (float)((double)res.parkTransformIq / IFX_CORDIC_Q23_SCALE);

	return 0;
}

static int ifx_mxcordic_init(const struct device *dev)
{
	const struct ifx_mxcordic_config *cfg = dev->config;
	struct ifx_mxcordic_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_sem_init(&data->done, 0, 1);
	data->faulted = false;
	data->intr_status = 0U;

	Cy_CORDIC_Enable(cfg->base);
	Cy_CORDIC_SetInterruptMask(cfg->base, 0U);
	Cy_CORDIC_ClearInterrupt(cfg->base, CY_CORDIC_INTR_MASK);

	cfg->irq_config(dev);

	return 0;
}

#define IFX_MXCORDIC_INIT(inst)                                                                    \
	static void ifx_mxcordic_irq_config_##inst(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), mxcordic_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	static const struct ifx_mxcordic_config ifx_mxcordic_config_##inst = {                     \
		.base = (MXCORDIC_Type *)DT_INST_REG_ADDR(inst),                                   \
		.irq_config = ifx_mxcordic_irq_config_##inst,                                      \
	};                                                                                         \
	static struct ifx_mxcordic_data ifx_mxcordic_data_##inst;                                  \
	DEVICE_DT_INST_DEFINE(inst, ifx_mxcordic_init, NULL, &ifx_mxcordic_data_##inst,            \
			      &ifx_mxcordic_config_##inst, POST_KERNEL,                            \
			      CONFIG_INFINEON_MXCORDIC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(IFX_MXCORDIC_INIT)
