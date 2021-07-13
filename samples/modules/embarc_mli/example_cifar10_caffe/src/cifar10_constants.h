/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CIFAR10_CONSTANTS_H_
#define _CIFAR10_CONSTANTS_H_

#include "mli_config.h"

#include "cifar10_model.h"

/* -------------------------- labels and tests code ------------------------- */
enum tIdxArrayVal { LABELS = 0x0, TESTS };

struct tIdxArrayFlag {
	long position;
	enum tIdxArrayVal flag;
};

/* ------------------------ Defining weight data type ----------------------- */
#if (MODEL_BIT_DEPTH != MODEL_FX_16)
#define W_EL_TYPE (MLI_EL_FX_8)
typedef int8_t w_type;
#else
#define W_EL_TYPE (MLI_EL_FX_16)
typedef int16_t w_type;
#endif

/* -------------------- Defining data sections attributes ------------------- */
#if (ARC_PLATFORM == V2DSP_XY)
#if defined(__GNUC__) && !defined(__CCAC__)
/* ARC GNU tools */
/* Model Weights attribute */
#define _Wdata_attr __attribute__((section(".mli_model")))
#define _W _Wdata_attr

/* Model Weights (part 2) attribute */
#define _W2data_attr __attribute__((section(".mli_model_p2")))
#define _W2 _W2data_attr

/* Bank X (XCCM) attribute */
#define __Xdata_attr __attribute__((section(".Xdata")))
#define _X __Xdata_attr

/* Bank Y (YCCM) attribute */
#define __Ydata_attr __attribute__((section(".Ydata")))
#define _Y __Ydata_attr

/* Bank Z (DCCM) attribute */
#define __Zdata_attr __attribute__((section(".Zdata")))
#define _Z __Zdata_attr

#else
/* Metaware tools */
/* Model Weights attribute */
#define _Wdata_attr __attribute__((section(".mli_model")))
#define _W __xy _Wdata_attr

/* Model Weights (part 2) attribute */
#define _W2data_attr __attribute__((section(".mli_model_p2")))
#define _W2 __xy _W2data_attr

/* Bank X (XCCM) attribute */
#define __Xdata_attr __attribute__((section(".Xdata")))
#define _X __xy __Xdata_attr

/* Bank Y (YCCM) attribute */
#define __Ydata_attr __attribute__((section(".Ydata")))
#define _Y __xy __Ydata_attr

/* Bank Z (DCCM) attribute */
#define __Zdata_attr __attribute__((section(".Zdata")))
#define _Z __xy __Zdata_attr
#endif /* if defined (__GNUC__) && !defined (__CCAC__) */

#else
#define _X __attribute__((section(".mli_ir_buf")))
#define _Y __attribute__((section(".mli_ir_buf")))
#define _Z __attribute__((section(".mli_ir_buf")))
#define _W __attribute__((section(".mli_model")))
#define _W2 __attribute__((section(".mli_model")))
#endif

/* -------------------------------------------------------------------------- */
/*                     Common data transform (Qmn) defines                    */
/* -------------------------------------------------------------------------- */

#define QMN(type, fraq, val) (type)(val * (1u << (fraq)) + ((val >= 0) ? 0.5f : -0.5f))
#define FRQ_BITS(int_part, el_type) ((sizeof(el_type) * 8) - int_part - 1)

/* -------------------------------------------------------------------------- */
/*                     Common data transform (Qmn) defines                    */
/* -------------------------------------------------------------------------- */

extern const w_type _W L1_conv_wt_buf[];
extern const w_type _W L1_conv_bias_buf[];

extern const w_type _W L2_conv_wt_buf[];
extern const w_type _W L2_conv_bias_buf[];

extern const w_type _W2 L3_conv_wt_buf[];
extern const w_type _W2 L3_conv_bias_buf[];

extern const w_type _W2 L4_fc_wt_buf[];
extern const w_type _W2 L4_fc_bias_buf[];

#if defined(MODEL_BIG)
extern const w_type _W2 L5_fc_wt_buf[];
extern const w_type _W2 L5_fc_bias_buf[];
#endif

/* -------------------------------------------------------------------------- */
/*                           labels and tests array                           */
/* -------------------------------------------------------------------------- */

extern const unsigned int tests[];
extern const unsigned int labels[];

/* -------------------------------------------------------------------------- */
/*                 Tensor's Integer bits per layer definitions                */
/* -------------------------------------------------------------------------- */

#if !defined(MODEL_BIG) /* Small Model */
#if (MODEL_BIT_DEPTH == MODEL_FX_16) || (MODEL_BIT_DEPTH == MODEL_FX_8W16D)

#define CONV1_W_INT (-1)
#define CONV1_B_INT (0)
#define CONV1_OUT_INT (4)

#define CONV2_W_INT (-1)
#define CONV2_B_INT (-1)
#define CONV2_OUT_INT (5)

#define CONV3_W_INT (-1)
#define CONV3_B_INT (-2)
#define CONV3_OUT_INT (5)

#define FC4_W_INT (-1)
#define FC4_B_INT (-2)
#define FC4_OUT_INT (5)

#else /* (MODEL_BIT_DEPTH == MODEL_FX_8) */

#define CONV1_W_INT (-1)
#define CONV1_B_INT (0)
#define CONV1_OUT_INT (3)

#define CONV2_W_INT (-1)
#define CONV2_B_INT (-1)
#define CONV2_OUT_INT (5)

#define CONV3_W_INT (-1)
#define CONV3_B_INT (-2)
#define CONV3_OUT_INT (5)

#define FC4_W_INT (-1)
#define FC4_B_INT (-2)
#define FC4_OUT_INT (5)

#endif

#else /* Big Model */
#if (MODEL_BIT_DEPTH == MODEL_FX_16) || (MODEL_BIT_DEPTH == MODEL_FX_8W16D)

#define CONV1_W_INT (0)
#define CONV1_B_INT (0)
#define CONV1_OUT_INT (5)

#define CONV2_W_INT (-1)
#define CONV2_B_INT (-1)
#define CONV2_OUT_INT (6)

#define CONV3_W_INT (-2)
#define CONV3_B_INT (-2)
#define CONV3_OUT_INT (5)

#define FC4_W_INT (-1)
#define FC4_B_INT (-3)
#define FC4_OUT_INT (3)

#define FC5_W_INT (0)
#define FC5_B_INT (-2)
#define FC5_OUT_INT (5)

#else /* (MODEL_BIT_DEPTH == MODEL_FX_8 or MODEL_FX_8W16D) */

#define CONV1_W_INT (0)
#define CONV1_B_INT (0)
#define CONV1_OUT_INT (4)

#define CONV2_W_INT (-1)
#define CONV2_B_INT (-1)
#define CONV2_OUT_INT (5)

#define CONV3_W_INT (-2)
#define CONV3_B_INT (-2)
#define CONV3_OUT_INT (4)

#define FC4_W_INT (-2)
#define FC4_B_INT (-3)
#define FC4_OUT_INT (3)

#define FC5_W_INT (0)
#define FC5_B_INT (-2)
#define FC5_OUT_INT (5)

#endif
#endif

/* -------------------------------------------------------------------------- */
/*               Shape and Fractional bits per layer definitions              */
/* -------------------------------------------------------------------------- */

/* ---------------------------------- CONV1 --------------------------------- */
#define CONV1_W_SHAPE                                                                              \
	{                                                                                          \
		32, 3, 5, 5                                                                        \
	}
#define CONV1_W_ELEMENTS (32 * 3 * 5 * 5)
#define CONV1_W_RANK (4)

#define CONV1_W_FRAQ (FRQ_BITS(CONV1_W_INT, w_type))
#define L1_WQ(val) QMN(w_type, CONV1_W_FRAQ, val)

#define CONV1_B_ELEMENTS (32)
#define CONV1_B_SHAPE                                                                              \
	{                                                                                          \
		32                                                                                 \
	}
#define CONV1_B_RANK (1)

#define CONV1_B_FRAQ (FRQ_BITS(CONV1_B_INT, w_type))
#define L1_BQ(val) QMN(w_type, CONV1_B_FRAQ, val)

#define CONV1_OUT_FRAQ (FRQ_BITS(CONV1_OUT_INT, d_type))

/* ---------------------------------- CONV2 --------------------------------- */
#if defined(MODEL_BIG)
#define CONV2_W_SHAPE                                                                              \
	{                                                                                          \
		32, 32, 5, 5                                                                       \
	}
#define CONV2_W_ELEMENTS (32 * 32 * 5 * 5)
#define CONV2_B_SHAPE                                                                              \
	{                                                                                          \
		32                                                                                 \
	}
#define CONV2_B_ELEMENTS (32)
#else /* Small Model */
#define CONV2_W_SHAPE                                                                              \
	{                                                                                          \
		16, 32, 5, 5                                                                       \
	}
#define CONV2_W_ELEMENTS (16 * 32 * 5 * 5)
#define CONV2_B_SHAPE                                                                              \
	{                                                                                          \
		16                                                                                 \
	}
#define CONV2_B_ELEMENTS (16)
#endif

#define CONV2_W_RANK (4)
#define CONV2_B_RANK (1)

#define CONV2_W_FRAQ (FRQ_BITS(CONV2_W_INT, w_type))
#define L2_WQ(val) QMN(w_type, CONV2_W_FRAQ, val)
#define CONV2_B_FRAQ (FRQ_BITS(CONV2_B_INT, w_type))
#define L2_BQ(val) QMN(w_type, CONV2_B_FRAQ, val)
#define CONV2_OUT_FRAQ (FRQ_BITS(CONV2_OUT_INT, d_type))

/* CONV3 */
#if defined(MODEL_BIG)
#define CONV3_W_SHAPE                                                                              \
	{                                                                                          \
		64, 32, 5, 5                                                                       \
	}
#define CONV3_W_ELEMENTS (64 * 32 * 5 * 5)
#define CONV3_B_SHAPE                                                                              \
	{                                                                                          \
		64                                                                                 \
	}
#define CONV3_B_ELEMENTS (64)
#else /* Small Model */
#define CONV3_W_SHAPE                                                                              \
	{                                                                                          \
		32, 16, 5, 5                                                                       \
	}
#define CONV3_W_ELEMENTS (32 * 16 * 5 * 5)
#define CONV3_B_SHAPE                                                                              \
	{                                                                                          \
		32                                                                                 \
	}
#define CONV3_B_ELEMENTS (32)
#endif
#define CONV3_W_RANK (4)
#define CONV3_B_RANK (1)

#define CONV3_W_FRAQ (FRQ_BITS(CONV3_W_INT, w_type))
#define L3_WQ(val) QMN(w_type, CONV3_W_FRAQ, val)
#define CONV3_B_FRAQ (FRQ_BITS(CONV3_B_INT, w_type))
#define L3_BQ(val) QMN(w_type, CONV3_B_FRAQ, val)
#define CONV3_OUT_FRAQ (FRQ_BITS(CONV3_OUT_INT, d_type))

/* FC4 */
#if defined(MODEL_BIG)
#define FC4_W_SHAPE                                                                                \
	{                                                                                          \
		64, (64 * 16)                                                                      \
	}
#define FC4_W_ELEMENTS (64 * (64 * 16))
#define FC4_B_SHAPE                                                                                \
	{                                                                                          \
		64                                                                                 \
	}
#define FC4_B_ELEMENTS (64)
#else /* Small Model */
#define FC4_W_SHAPE                                                                                \
	{                                                                                          \
		10, (32 * 16)                                                                      \
	}
#define FC4_W_ELEMENTS (10 * (32 * 16))
#define FC4_B_SHAPE                                                                                \
	{                                                                                          \
		10                                                                                 \
	}
#define FC4_B_ELEMENTS (10)
#endif
#define FC4_W_RANK (2)
#define FC4_B_RANK (1)

#define FC4_W_FRAQ (FRQ_BITS(FC4_W_INT, w_type))
#define L4_WQ(val) QMN(w_type, FC4_W_FRAQ, val)
#define FC4_B_FRAQ (FRQ_BITS(FC4_B_INT, w_type))
#define L4_BQ(val) QMN(w_type, FC4_B_FRAQ, val)
#define FC4_OUT_FRAQ (FRQ_BITS(FC4_OUT_INT, d_type))

#if defined(MODEL_BIG)
/* ----------------------------------- FC5 ---------------------------------- */
#define FC5_W_SHAPE                                                                                \
	{                                                                                          \
		10, 64                                                                             \
	}
#define FC5_W_ELEMENTS (10 * 64)
#define FC5_W_RANK (2)

#define FC5_W_FRAQ (FRQ_BITS(FC5_W_INT, w_type))
#define L5_WQ(val) QMN(w_type, FC5_W_FRAQ, val)

#define FC5_B_ELEMENTS (10)
#define FC5_B_SHAPE                                                                                \
	{                                                                                          \
		10                                                                                 \
	}
#define FC5_B_RANK (1)

#define FC5_B_FRAQ (FRQ_BITS(FC5_B_INT, w_type))
#define L5_BQ(val) QMN(w_type, FC5_B_FRAQ, val)

#define FC5_OUT_FRAQ (FRQ_BITS(FC5_OUT_INT, d_type))
#endif /* defined(MODEL_BIG) */

#endif /* _CIFAR10_CONSTANTS_H_ */
