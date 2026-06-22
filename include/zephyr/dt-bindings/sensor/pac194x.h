/*
 * Copyright (c) 2026 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PAC194X_H_DTS_BINDINGS
#define PAC194X_H_DTS_BINDINGS

/**
 * @file pac194x.h
 * @brief Driver definitions for the PAC194X power monitor.
 */

/** Unipolar FSR range */
#define PAC_UNIPOLAR_FSR	0
/** Bipolar FSR range */
#define PAC_BIPOLAR_FSR		1
/** Bipolar FSR/2 range */
#define PAC_BIPOLAR_HALF_FSR	2

/** Accumulate Power data */
#define PAC_ACCUM_VPOWER	0
/** Accumulate Current data */
#define PAC_ACCUM_VSENSE	1
/** Accumulate Voltage data */
#define PAC_ACCUM_VBUS		2


/** Refresh mode manual */
#define PAC_REFRESH_MANUAL	0

/** Refresh mode */
#define PAC_REFRESH_AUTO_WAIT	1

/** Refresh mode */
#define PAC_REFRESH_AUTO_NOWAIT	2

#endif /* PAC194X_H_DTS_BINDINGS */
