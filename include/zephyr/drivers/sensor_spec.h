/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPEC_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPEC_H_

#include <zephyr/devicetree.h>

/**
 * @addtogroup sensor_interface
 * @{
 */

/**
 * @def SENSOR_SPEC_COMPAT(compat)
 * Generates the prefix for any sensor spec for a given compatible value
 */
#define SENSOR_SPEC_COMPAT(compat) UTIL_CAT(SENSOR_SPEC, DT_DASH(compat))

/**
 * @def SENSOR_SPEC_COMPAT_EXISTS(compat)
 * 1 if the given compat value is a sensor, 0 otherwise
 */
#define SENSOR_SPEC_COMPAT_EXISTS(compat) IS_ENABLED(UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _EXISTS))

/**
 * @def SENSOR_SPEC_CHAN_COUNT(compat)
 * The number of channels supported by the sensor with the given compatible value
 */
#define SENSOR_SPEC_CHAN_COUNT(compat) UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _CHAN_COUNT)

/**
 * @def SENSOR_SPEC_ATTR_COUNT(compat)
 * The number of attributes supported by the sensor with the given compatible value
 */
#define SENSOR_SPEC_ATTR_COUNT(compat) UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _ATTR_COUNT)

/**
 * @def SENSOR_SPEC_TRIG_COUNT(compat)
 * The number of triggers supported by the sensor with the given compatible value
 */
#define SENSOR_SPEC_TRIG_COUNT(compat) UTIL_CAT(SENSOR_SPEC_COMPAT(compat), _TRIG_COUNT)

/**
 * @def SENSOR_SPEC_CHAN_INST_COUNT(compat, chan)
 * The number of channel instances for the given compatible/channel pair.
 */
#define SENSOR_SPEC_CHAN_INST_COUNT(compat, chan)                                                  \
	UTIL_CAT(SENSOR_SPEC_COMPAT(compat), DT_DASH(CH, chan, COUNT))

/**
 * @def SENSOR_SPEC_CHAN_INST(compat, chan, inst)
 * The prefix used for a given compatible/channel/instance tuple.
 */
#define SENSOR_SPEC_CHAN_INST(compat, chan, inst)                                                  \
	UTIL_CAT(SENSOR_SPEC_COMPAT(compat), DT_DASH(CH, chan, inst))

/**
 * @def SENSOR_SPEC_CHAN_INST_EXISTS(compat, chan, inst)
 * 1 if the given compatible/channel/instance tuple exists, 0 otherwise
 */
#define SENSOR_SPEC_CHAN_INST_EXISTS(compat, chan, inst)                                           \
	IS_ENABLED(UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _EXISTS))

/**
 * @def SENSOR_SPEC_CHAN_INST_NAME(compat, chan, inst)
 * The C string name of the given channel instance of a sensor
 */
#define SENSOR_SPEC_CHAN_INST_NAME(compat, chan, inst)                                             \
	UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _NAME)

/**
 * @def SENSOR_SPEC_CHAN_INST_DESC(compat, chan, inst)
 * The C string description of the given channel instance of a sensor
 */
#define SENSOR_SPEC_CHAN_INST_DESC(compat, chan, inst)                                             \
	UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _DESC)

/**
 * @def SENSOR_SPEC_CHAN_INST_SPEC(compat, chan, inst)
 * The sensor_chan_spec values of the given channel instance of a sensor
 */
#define SENSOR_SPEC_CHAN_INST_SPEC(compat, chan, inst)                                             \
	UTIL_CAT(SENSOR_SPEC_CHAN_INST(compat, chan, inst), _SPEC)

/**
 * @def SENSOR_SPEC_ATTR_EXISTS(compat, attr, chan)
 * 1 if the given attribute/channel combination is configurable for the given sensor, 0 otherwise.
 */
#define SENSOR_SPEC_ATTR_EXISTS(compat, attr, chan)                                                \
	IS_ENABLED(UTIL_CAT(SENSOR_SPEC_COMPAT(compat), DT_DASH(AT, attr, CH, chan, EXISTS)))

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPEC_H_ */
