/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HAR_SMARTPHONE_CONSTANTS_H_
#define _HAR_SMARTPHONE_CONSTANTS_H_

#include "mli_config.h"

#include "har_smartphone_model.h"
#include "tests_aux.h"

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
#endif

/* -------------------------------------------------------------------------- */
/*           Common data transform (Qmn) defines (round-to-nearest)           */
/* -------------------------------------------------------------------------- */

#define EL_MAX(type) (type)((1u << (sizeof(type) * 8 - 1)) - 1)
#define EL_MIN(type) (type)(-(1u << (sizeof(type) * 8 - 1)))
#define SAT(type, val) (MIN(EL_MAX(type), MAX(EL_MIN(type), val)))

#define QMN(type, fraq, val)                                                                       \
	(type) SAT(type, ((val) * (1u << (fraq)) + (((val) >= 0) ? 0.5f : -0.5f)))
#define FRQ_BITS(int_part, el_type) ((sizeof(el_type) * 8) - int_part - 1)

/* -------------------------------------------------------------------------- */
/*              Quantized model parameters (statically allocated)             */
/* -------------------------------------------------------------------------- */

extern const w_type _W L1_fc_wt_buf[];
extern const w_type _W L1_fc_bias_buf[];

extern const w_type _W L2_lstm_wt_buf[];
extern const w_type _W L2_lstm_bias_buf[];

extern const w_type _W L3_lstm_wt_buf[];
extern const w_type _W L3_lstm_bias_buf[];

extern const w_type _W L4_fc_wt_buf[];
extern const w_type _W L4_fc_bias_buf[];

/* -------------------------------------------------------------------------- */
/*                           labels and tests array                           */
/* -------------------------------------------------------------------------- */

extern const unsigned int tests[];
extern const unsigned int labels[];

/* -------------------------------------------------------------------------- */
/*                 Tensor's Integer bits per layer definitions                */
/* -------------------------------------------------------------------------- */

#define FC1_W_INT (1)
#define FC1_B_INT (1)
#define FC1_OUT_INT (3)

#define LSTM2_W_INT (0)
#define LSTM2_B_INT (1)
#define LSTM2_CELL_INT (4)

#define LSTM3_W_INT (0)
#define LSTM3_B_INT (1)
#define LSTM3_CELL_INT (3)

#define FC4_W_INT (0)
#define FC4_B_INT (-2)
#define FC4_OUT_INT (3)

/* -------------------------------------------------------------------------- */
/*               Shape and Fractional bits per layer definitions              */
/* -------------------------------------------------------------------------- */

/* FC1 */
#define FC1_W_SHAPE                                                                                \
	{                                                                                          \
		32, 9                                                                              \
	}
#define FC1_W_ELEMENTS (32 * 9)
#define FC1_W_RANK (2)

#if defined(CONVOLUTION_LAYER1)
#define FC1_W_SHAPE_CONV                                                                           \
	{                                                                                          \
		32, 1, 9, 1                                                                        \
	}
#define FC1_W_RANK_CONV (4)
#endif

#define FC1_W_FRAQ (FRQ_BITS(FC1_W_INT, w_type))
#define QW1(val) QMN(w_type, FC1_W_FRAQ, val)

#define FC1_B_ELEMENTS (32)
#define FC1_B_SHAPE                                                                                \
	{                                                                                          \
		32                                                                                 \
	}
#define FC1_B_RANK (1)

#define FC1_B_FRAQ (FRQ_BITS(FC1_B_INT, w_type))
#define QB1(val) QMN(w_type, FC1_B_FRAQ, val)

#define FC1_OUT_FRAQ (FRQ_BITS(FC1_OUT_INT, d_type))

/* LSTM2 */
#define LSTM2_W_SHAPE                                                                              \
	{                                                                                          \
		4, 32, 64                                                                          \
	}
#define LSTM2_W_ELEMENTS (4 * 32 * 64)
#define LSTM2_W_RANK (3)

#define LSTM2_W_FRAQ (FRQ_BITS(LSTM2_W_INT, w_type))
#define QW2(val) QMN(w_type, LSTM2_W_FRAQ, val)

#define LSTM2_B_ELEMENTS (4 * 32)
#define LSTM2_B_SHAPE                                                                              \
	{                                                                                          \
		4, 32                                                                              \
	}
#define LSTM2_B_RANK (2)

#define LSTM2_B_FRAQ (FRQ_BITS(LSTM2_B_INT, w_type))
#define QB2(val) QMN(w_type, LSTM2_B_FRAQ, val)

#define LSTM2_OUT_FRAQ (FRQ_BITS(0, d_type))
#define LSTM2_CELL_FRAQ (FRQ_BITS(LSTM2_CELL_INT, d_type))

/* LSTM3 */
#define LSTM3_W_SHAPE                                                                              \
	{                                                                                          \
		4, 32, 64                                                                          \
	}
#define LSTM3_W_ELEMENTS (4 * 32 * 64)
#define LSTM3_W_RANK (3)

#define LSTM3_W_FRAQ (FRQ_BITS(LSTM3_W_INT, w_type))
#define QW3(val) QMN(w_type, LSTM3_W_FRAQ, val)

#define LSTM3_B_ELEMENTS (4 * 32)
#define LSTM3_B_SHAPE                                                                              \
	{                                                                                          \
		4, 32                                                                              \
	}
#define LSTM3_B_RANK (2)

#define LSTM3_B_FRAQ (FRQ_BITS(LSTM3_B_INT, w_type))
#define QB3(val) QMN(w_type, LSTM3_B_FRAQ, val)

#define LSTM3_OUT_FRAQ (FRQ_BITS(0, d_type))
#define LSTM3_CELL_FRAQ (FRQ_BITS(LSTM3_CELL_INT, d_type))

/* FC4 */
#define FC4_W_SHAPE                                                                                \
	{                                                                                          \
		6, 32                                                                              \
	}
#define FC4_W_ELEMENTS (6 * 32)
#define FC4_W_RANK (2)

#define FC4_W_FRAQ (FRQ_BITS(FC4_W_INT, w_type))
#define QW4(val) QMN(w_type, FC4_W_FRAQ, val)

#define FC4_B_ELEMENTS (6)
#define FC4_B_SHAPE                                                                                \
	{                                                                                          \
		6                                                                                  \
	}
#define FC4_B_RANK (1)

#define FC4_B_FRAQ (FRQ_BITS(FC4_B_INT, w_type))
#define QB4(val) QMN(w_type, FC4_B_FRAQ, val)

#define FC4_OUT_FRAQ (FRQ_BITS(FC4_OUT_INT, d_type))

#endif /* _HAR_SMARTPHONE_CONSTANTS_H_ */
