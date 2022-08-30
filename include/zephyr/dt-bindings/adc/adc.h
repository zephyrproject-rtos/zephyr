/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADC_H_

/*
 * Provide the BIT_MASK() macro for when this file is included from
 * devicetrees.
 */
#ifndef BIT_MASK
#define BIT_MASK(n) ((1 << (n)) - 1)
#endif

/** Acquisition time is expressed in microseconds. */
#define ADC_ACQ_TIME_MICROSECONDS  (1)
/** Acquisition time is expressed in nanoseconds. */
#define ADC_ACQ_TIME_NANOSECONDS   (2)
/** Acquisition time is expressed in ADC ticks. */
#define ADC_ACQ_TIME_TICKS         (3)
/** Macro for composing the acquisition time value in given units. */
#define ADC_ACQ_TIME(unit, value)  (((unit) << 14) | ((value) & BIT_MASK(14)))
/** Value indicating that the default acquisition time should be used. */
#define ADC_ACQ_TIME_DEFAULT       0
#define ADC_ACQ_TIME_MAX           BIT_MASK(14)

#define ADC_ACQ_TIME_UNIT(time)    (((time) >> 14) & BIT_MASK(2))
#define ADC_ACQ_TIME_VALUE(time)   ((time) & BIT_MASK(14))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADC_H_ */
