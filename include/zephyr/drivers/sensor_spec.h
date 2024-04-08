/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPEC_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPEC_H_

#include <zephyr/devicetree.h>

#define SENSOR_SPEC_COMPAT(compat)        UTIL_CAT(SENSOR_SPEC, DT_DASH(compat))
#define SENSOR_SPEC_COMPAT_EXISTS(compat) IS_ENABLED(UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _EXISTS))
#define SENSOR_SPEC_CHAN_COUNT(compat)    UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _CHAN_COUNT)
#define SENSOR_SPEC_ATTR_COUNT(compat)    UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _ATTR_COUNT)
#define SENSOR_SPEC_TRIG_COUNT(compat)    UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _TRIG_COUNT)

#define SENSOR_SPEC_CHAN_INST_COUNT(compat, chan)                                                  \
	UTIL_CAT(SENSOR_SPEC_COMPAT(compat), DT_DASH(CH, chan, COUNT))
#define SENSOR_SPEC_CHAN_INST(compat, chan, inst)                                                  \
	UTIL_CAT(SENSOR_SPEC_COMPAT(compat), DT_DASH(CH, chan, inst))
#define SENSOR_SPEC_CHAN_INST_EXISTS(compat, chan, inst)                                           \
	IS_ENABLED(UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _EXISTS))
#define SENSOR_SPEC_CHAN_INST_NAME(compat, chan, inst)                                             \
	UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _NAME)
#define SENSOR_SPEC_CHAN_INST_DESC(compat, chan, inst)                                             \
	UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _DESC)
#define SENSOR_SPEC_CHAN_INST_SPEC(compat, chan, inst)                                             \
	UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _SPEC)

#define SENSOR_SPEC_ATTR_EXISTS(compat, attr, chan)                                                \
	IS_ENABLED(UTIL_CAT(SENSOR_SPEC_COMPAT(compat), DT_DASH(AT, attr, CH, chan, EXISTS)))

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPEC_H_ */
