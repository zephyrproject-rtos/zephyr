/*
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZIO core API definitions.
 */

#ifndef ZEPHYR_INCLUDE_ZIO_DEFS_H_
#define ZEPHYR_INCLUDE_ZIO_DEFS_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZIO_CHAN_LIMIT (255)

/**
 * ZIO devices are composed of
 *
 *    o Device attributes (zio_dev_attr)
 *    o Channels (zio_dev_chan_type)
 *    o Channel attributes (zio_chan_attr)
 *
 * These device components are organised in the following manner:
 *
 * ZIO Device----------------------------------------------------------------+
 * |                                                                         |
 * | Device Attributes-----------------------------------------------------+ |
 * | | Device Attribute 0------------------------------------------------+ | |
 * | | |                          DEV_ATTR_NAME                          | | |
 * | | Device Attribute 1------------------------------------------------+ | |
 * | | |                        DEV_ATTR_OP_MODE                         | | |
 * | | +-----------------------------------------------------------------+ | |
 * | +---------------------------------------------------------------------+ |
 * |                                                                         |
 * | +---------------------------------------------------------------------+ |
 * | Channels--------------------------------------------------------------+ |
 * | | Channel 0---------------------------------------------------------+ | |
 * | | | Channel Attributes---------------------------------------------+| | |
 * | | | | Channel Attribute 0----------------------------------------+ || | |
 * | | | | |                   CHAN_ATTR_RAW_DATA                     | || | |
 * | | | | Channel Attribute 1----------------------------------------+ || | |
 * | | | | |                   CHAN_ATTR_SI_DATA                      | || | |
 * | | | | Channel Attribute 2----------------------------------------+ || | |
 * | | | | |                   CHAN_ATTR_SAMP_FREQ                    | || | |
 * | | | | +----------------------------------------------------------+ || | |
 * | | | +--------------------------------------------------------------+| | |
 * | | +-----------------------------------------------------------------+ | |
 * | | |Channel 1                                                        | | |
 * | | +-----------------------------------------------------------------+ | |
 * | | |Channel 2                                                        | | |
 * | | +-----------------------------------------------------------------+ | |
 * | +---------------------------------------------------------------------+ |
 * +-------------------------------------------------------------------------+
 */

/**
 * @brief ZIO device channel types.
 *
 * Each channel type is associated with a specific SI unit type, described
 * in the inline comments for each enum entry.
 */
typedef enum zio_dev_chan_type {
	/**
	 * Raw data channel.
	 *
	 * No SI type is prescribed for this channel.
	 */
	DEV_CHAN_RAW,

	/**
	 * Voltage.
	 *
	 * Expressed as volts (V).
	 */
	DEV_CHAN_VOLTAGE,

	/**
	 * Current.
	 *
	 * Expressed as milli-amps (mA).
	 */
	DEV_CHAN_CURRENT,

	/**
	 * Resistance.
	 *
	 * Expressed as ohms.
	 */
	DEV_CHAN_RESISTANCE,

	/**
	 * Capacitance.
	 *
	 * Expressed as picofarad (pF).
	 */
	DEV_CHAN_CAPACITANCE,

	/**
	 * Inductance.
	 *
	 * Expressed as microhenries (uH).
	 */
	DEV_CHAN_INDUCTANCE,

	/**
	 * Impedance.
	 *
	 * Expressed as ohms (Z).
	 */
	DEV_CHAN_IMPEDANCE,

	/**
	 * Frequency.
	 *
	 * Expressed as Hertz (Hz).
	 */
	DEV_CHAN_FREQUENCY,

	/**
	 * Amplitude.
	 *
	 * Note: While amplitude is a unitless value with a floating-point
	 * range of -1.0 .. 1.0, you can associate this with a range of integer
	 * values by means of the 'CHAN_ATTR_RAW_DATA' attribute. Normalised
	 * floating-point values should be assigned to 'CHAN_ATTR_SI_DATA',
	 * but the source integer values, such as 's16_t', can be provided
	 * as raw data to avoid expensive floating-point operations if
	 * desired.
	 *
	 * Expressed as a unitless value in the range of -1.0 to 1.0.
	 */
	DEV_CHAN_AMPLITUDE,

	/**
	 * Generic temperature.
	 *
	 * Expressed as degrees celsius (C).
	 */
	DEV_CHAN_TEMP,

	/**
	 * Ambient temperature.
	 *
	 * Expressed as degrees celsius (C).
	 */
	DEV_CHAN_TEMP_AMBIENT,

	/**
	 * Die temperature.
	 *
	 * Expressed as degrees celsius (C).
	 */
	DEV_CHAN_TEMP_DIE,

	/**
	 * Single-axis acceleration.
	 *
	 * Expressed as meters per second squared (m/s^2).
	 */
	DEV_CHAN_ACCEL,

	/**
	 * Three-axis (XYZ) acceleration.
	 *
	 * Expressed as meters per second squared (m/s^2).
	 */
	DEV_CHAN_ACCEL_VECTOR,

	/**
	 * Single-axis magentic field measurement.
	 *
	 * Expressed as micro-Tesla (uT).
	 */
	DEV_CHAN_MAG,

	/**
	 * Three-axis (XYZ) magnetic field measurement.
	 *
	 * Expressed as micro-Tesla (uT).
	 */
	DEV_CHAN_MAG_VECTOR,

	/**
	 * Single-axis angular velocity.
	 *
	 * Expressed as radians per second (rad/s).
	 */
	DEV_CHAN_GYRO,

	/**
	 * Three-axis (XYZ) angular velocity.
	 *
	 * Expressed as radians per second (rad/s).
	 */
	DEV_CHAN_GYRO_VECTOR,

	/**
	 * Irradiance, which is the measurement of radiant flux (analogous to
	 * 'power' or 'intensity') received on given surface area.
	 *
	 * Expressed as watts per square meter (W/m^2).
	 */
	DEV_CHAN_LIGHT_IRRAD,

	/**
	 * Irradiance in the visible light range (nominally 380-780 nm).
	 *
	 * Expressed as watts per square meter (W/m^2).
	 */
	DEV_CHAN_LIGHT_IRRAD_VIS,

	/**
	 * Irradiance in the ultaviolet range (nominally <380 nm).
	 *
	 * Expressed as watts per square meter (W/m^2).
	 */
	DEV_CHAN_LIGHT_IRRAD_UV,

	/**
	 * Irradiance in the infrared range (>780 nm).
	 *
	 * Expressed as watts per square meter (W/m^2).
	 */
	DEV_CHAN_LIGHT_IRRAD_IR,

	/**
	 * Irradiance limited to a specific spectral component. The exact
	 * wavelength or wavelength range of the component should be
	 * indicated with an appropriate channel attribute.
	 *
	 * Expressed as watts per square meter (W/m^2).
	 */
	DEV_CHAN_LIGHT_IRRAD_COMP,

	/**
	 * An approximation of human vision, which is the result of a
	 * conversion from irradiance (a radiometric unit expressed in
	 * W/m^2) to illuminance (a photometric unit expressed in lumen/m^2)
	 * by means of a version of the CIE luminous efficiency function.
	 *
	 * NOTE: More technically, this channel should probably be named
	 *       `ILLUMINANCE`, but `LUX` is used at present since it is
	 *       a significantly more familiar concept to most people.
	 *
	 * Expressed as 'lux', which is lumens per square meter.
	 */
	DEV_CHAN_LIGHT_LUX,

	/**
	 * Relative humidity. Indicates the amount of water vapor in the air,
	 * relative to the maximum amount of water vapor that could exist in
	 * the air at the current temperature.
	 *
	 * Expressed as a percentage.
	 */
	DEV_CHAN_REL_HUMIDITY,

	/**
	 * Latitude in a geo-localisation system.
	 *
	 * Expressed as a degree/minute/second notation string.
	 */
	DEV_CHAN_POS_LATITUDE,

	/**
	 * Longitude in a geo-localisation system.
	 *
	 * Expressed as a degree/minute/second notation string.
	 */
	DEV_CHAN_POS_LONGITUDE,

	/**
	 * The altitude or 'height' of an object or point in relation to an
	 * unspecified reference level (ground, sea-level, another object,
	 * etc.)
	 *
	 * Expressed in meters.
	 */
	DEV_CHAN_POS_ALT,

	/**
	 * The altitude or 'height' of an object or point in reference to sea
	 * level.
	 *
	 * Expressed in meters.
	 */
	DEV_CHAN_POS_ALT_SEALEVEL,

	/**
	 * The altitude or 'height' of an object or point in reference to the
	 * ground immediately below the point of measurement.
	 *
	 * Expressed in meters.
	 */
	DEV_CHAN_POS_ALT_GND,

	/**
	 * Distance in a straight line (regardless of obstacles) between the
	 * source of the measurement and a specific reference point.
	 *
	 * Expressed in meters (m).
	 */
	DEV_CHANNEL_POS_DIST,

	/**
	 * Distance in a straight line along the surface of an object (such as
	 * a sphere) between the source of the measurement and a specific
	 * reference point.
	 *
	 * Expressed in meters (m).
	 */
	DEV_CHANNEL_POS_DIST_SURF,

	/**
	 * Proximity to an external plane or object.
	 *
	 * Expressed in millimeters (mm).
	 */
	DEV_CHANNEL_POS_PROX,

	/**
	 * Encapsulates a raw NMEA sentence from a GPS-type device
	 *
	 * Expressed as a null-terminated string, with the exact string
	 * contents determined by NMEA standard. NMEA sentences are generally
	 * limited to 79 characters plus the null-termination character.
	 */
	DEV_CHAN_POS_NMEA,

	/**
	 * An Euler angle, which consists of the three familiar X/Y/Z
	 * headings.
	 *
	 * Expressed as a 3-vector (XYZ) of degrees.
	 */
	DEV_CHAN_ORIENT_EULER,

	/**
	 * A quaternion, which is generally used to represent a value in
	 * three dimensional space, avoiding the problem of gimbal lock
	 * associated with 3-vector Euler angles.
	 *
	 * Quaternions consist of four elements: a 3-vector and a scalar.
	 */
	DEV_CHAN_ORIENT_QUAT,

	/**
	 * An indication of tilt in degrees on a single axis relative to an
	 * unspecified reference plane.
	 *
	 * Expressed in degrees (+/-180).
	 */
	DEV_CHAN_ORIENT_TILT,


	/**
	 * Pressure, which is a measurement of the force applied
	 * perpendicularly to a specific surface area.
	 *
	 * Expressed in hectopascal (hPa)
	 */
	DEV_CHANNEL_PRESS,

	/**
	 * Barometric (or 'atmospheric') pressure, which is a measurement of
	 * the pressure within the atmosphere. Related to the concept of
	 * 'hydrostatic pressure' caused by the weight of the air above the
	 * point of measure.
	 *
	 * TODO: Would millibar (mb) be a better unit choice, or should this
	 * remain consist with the more general pressure channel unit (hPa)?
	 *
	 * Expressed in hectopascal (hPa)
	 */
	DEV_CHANNEL_PRESS_BAROM,

	/**
	 * Particulate matter.
	 *
	 * Expressed as micrograms per cubic meter (ug/m^3).
	 */
	DEV_CHAN_PART_MAT,

	/**
	 * Generic parts per million measurement. The exact type of matter
	 * being measured should be indicated with an appropriate channel
	 * attribute.
	 *
	 * Expressed as parts per million (ppm).
	 */
	DEV_CHAN_PPM,

	/**
	 * Detected CO2 levels.
	 *
	 * Expressed as parts per million (ppm).
	 */
	DEV_CHAN_PPM_CO2,

	/**
	 * Detected volatile organic compound (voc) levels.
	 *
	 * Expressed as parts per million (ppm).
	 */
	DEV_CHAN_PPM_VOC,

	/**
	 * User-defined channel. This can be used at the application level for
	 * whetever purpose the driver requires, provided it does not overlap
	 * with one of the existing standard channel definitions.
	 *
	 * If multiple user defined channels are present, they should be
	 * clearly distinguished by use of appropriate channel attributes.
	 *
	 * Expressed as a user-defined unit and scale. The exact unit type and
	 * scale should be clearly documented in the driver making use of this
	 * channel type.
	 */
	DEV_CHAN_USER_DEF = ZIO_CHAN_LIMIT,
} zio_dev_chan_type_t;

/**
 * @brief ZIO channel attribute types.
 */
typedef enum zio_chan_attr_type {
	/** Raw data attribute. Mandatory with 'DEV_CHAN_RAW'. Read-only. */
	CHAN_ATTR_RAW_DATA = 0,

	/** SI data attribute. Read-only. */
	CHAN_ATTR_SI_DATA,

	/** Short presentation name. Read-only. */
	CHAN_ATTR_NAME,

	/** Current HW sampling frequency. Read/write. */
	CHAN_ATTR_SAMP_FREQ,

	/** Supported HW sample frequencies. Read-only. */
	CHAN_ATTR_SAMP_FREQ_LIST,

	/** Value offset. Read/write. */
	CHAN_ATTR_OFFSET,

	/** Raw value to SI scale factor. Read-only. */
	CHAN_ATTR_SCALE,

	/** Calibration bias. Read/write. */
	CHAN_ATTR_CAL_BIAS,

	/** Calibration scale factor. Read/write. */
	CHAN_ATTR_CAL_SCALE,

	/** Current read/event mode. Read/write. */
	CHAN_ATTR_EVENT,

	/** Supported read/event modes. Read-only. */
	CHAN_ATTR_EVENT_LIST,

	/** Current trigger. Read/write. */
	CHAN_ATTR_TRIGGER,

	/** Supported triggers. Read-only. */
	CHAN_ATTR_TRIGGER_LIST,

	/** Enable or disable the channel buffer. Read/write. */
	CHAN_ATTR_BUF_ENABLED,

	/** Indicate if channel data is available. Read-only. */
	CHAN_ATTR_DATA_AVAIL,
} zio_chan_attr_type_t;

/**
 * @brief ZIO device attribute types.
 */
typedef enum zio_dev_attr_type {
	/** Short presentation name (<20 chars). Read-only. */
	DEV_ATTR_NAME = 0,

	/** Short device description string (<80 chars). Read-only. */
	DEV_ATTR_DESC,

	/** Public driver version string. Read-only. */
	DEV_ATTR_DRIVER_VERSION,

	/** Unique identifier for a device. Read/write. */
	DEV_ATTR_UNIQUE_ID,

	/** Current operating/power mode. Read/write. */
	DEV_ATTR_OP_MODE,

	/** Supported operating/power modes. Read-only. */
	DEV_ATTR_OP_MODE_LIST,
} zio_dev_attr_type_t;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_ZIO_DEFS_H_ */
