/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file ch_driver.h
 *
 * @brief Internal driver functions for operation with the Chirp ultrasonic sensor.
 *
 * This file contains definitions for the internal Chirp sensor driver functions
 * and structures within SonicLib.  These functions are provided in source code form
 * to simplify integration with an embedded application and for reference only.
 *
 * The Chirp driver functions provide an interface between the SonicLib public API
 * layer and the actual sensor devices.  The driver manages all software-defined
 * aspects of the Chirp sensor, including the register set.
 *
 * You should not need to edit this file or call the driver functions directly.  Doing so
 * will reduce your ability to benefit from future enhancements and releases from Chirp.
 *
 */

#ifndef CH_DRIVER_H_
#define CH_DRIVER_H_

#include "chirp_board_config.h"
#include "soniclib.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/** maximum number of bytes in a single I2C write */
#define CHDRV_I2C_MAX_WRITE_BYTES 256

/** standard non-blocking I/O transaction */
#define CHDRV_NB_TRANS_TYPE_STD	     (0)
/** non-blocking I/O via low-level programming interface */
#define CHDRV_NB_TRANS_TYPE_PROG     (1)
/** externally requested non-blocking I/O transaction */
#define CHDRV_NB_TRANS_TYPE_EXTERNAL (2)

/* Programming interface register addresses */
/** Read-only register used during device discovery. */
#define CH_PROG_REG_PING 0x00
/** Processor control register address. */
#define CH_PROG_REG_CPU	 0x42
/** Processor status register address. */
#define CH_PROG_REG_STAT 0x43
/** Data transfer control register address. */
#define CH_PROG_REG_CTL	 0x44
/** Data transfer starting address register address. */
#define CH_PROG_REG_ADDR 0x05
/** Data transfer size register address. */
#define CH_PROG_REG_CNT	 0x07
/** Data transfer value register address. */
#define CH_PROG_REG_DATA 0x06

/** Macro to determine programming register size. */
#define CH_PROG_SIZEOF(R) ((R)&0x40 ? 1 : 2)

/** max size of a read operation via programming interface */
#define CH_PROG_XFER_SIZE (256)

/** debug pin number (index) to use for debug indication */
#define CHDRV_DEBUG_PIN_NUM (0)

/** Max queued non-blocking I2C transactions - value from chirp_board_config.h */
#define CHDRV_MAX_I2C_QUEUE_LENGTH CHIRP_MAX_NUM_SENSORS

/** Time to wait in chdrv_group_start() for sensor initialization, in milliseconds.  */
#define CHDRV_FREQLOCK_TIMEOUT_MS 100
/** Index of first sample to use for calculating bandwidth. */
#define CHDRV_BANDWIDTH_INDEX_1	  6
/** Index of second sample to use for calculating bandwidth. */
#define CHDRV_BANDWIDTH_INDEX_2	  (CHDRV_BANDWIDTH_INDEX_1 + 1)

/** Index for calculating scalefactor. */
#define CHDRV_SCALEFACTOR_INDEX 4

/** Length of INT pulse to trigger sensor, in microseconds - minimum 800ns.  */
#define CHDRV_TRIGGER_PULSE_US	  5
/** Tuning parameter to adjust pre-trigger timing */
#define CHDRV_DELAY_OVERHEAD_US	  12
/** Time to delay between triggering rx-only and tx/rx nodes, in us */
#define CHDRV_PRETRIGGER_DELAY_US 600

/** Hook routine pointer typedefs */
typedef uint8_t (*chdrv_discovery_hook_t)(ch_dev_t *dev_ptr);

/** I2C transaction control structure */
typedef struct chdrv_i2c_transaction {
	/** I2C transaction type: 0 = std, 1 = prog interface, 2 = external */
	uint8_t type;
	/** Read/write indicator: 0 if write operation, 1 if read operation */
	uint8_t rd_wrb;
	/** current transfer within this transaction */
	uint8_t xfer_num;
	/** I2C address */
	uint16_t addr;
	/** Number of bytes to transfer */
	uint16_t nbytes;
	/** Pointer to ch_dev_t descriptor structure for individual sensor */
	ch_dev_t *dev_ptr;
	/** Pointer to buffer to receive data or containing data to send */
	uint8_t *databuf;
} chdrv_i2c_transaction_t;

/**  I2C queue structure, for non-blocking access */
typedef struct chdrv_i2c_queue {
	/** Read transaction status: non-zero if read operation is pending */
	uint8_t read_pending;
	/** I2C transaction status: non-zero if I/O operation in progress */
	uint8_t running;
	/** Number of transactions in queue */
	uint8_t len;
	/** Index of current transaction within queue */
	uint8_t idx;
	/** List of transactions in queue */
	chdrv_i2c_transaction_t transaction[CHDRV_MAX_I2C_QUEUE_LENGTH];
} chdrv_i2c_queue_t;

/**
 * @brief  Calibrate the sensor real-time clock against the host microcontroller clock.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * This function sends a pulse (timed by the host MCU) on the INT line to each device in the group,
 * then reads back the count of sensor RTC cycles that elapsed during that pulse on each individual
 * device.  The result is stored in the ch_dev_t descriptor structure for each device and is
 * subsequently used during range calculations.
 *
 * The length of the pulse is grp_ptr->rtc_cal_pulse_ms milliseconds (typically 100ms).
 *
 * @note The calibration pulse is sent to all devices in the group at the same time.  Therefore all
 * connected devices will see the same reference pulse length.
 *
 */
void chdrv_group_measure_rtc(ch_group_t *grp_ptr);

/**
 * @brief Convert the sensor register values to a range using the calibration data in the ch_dev_t
 * struct.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param tof value of TOF register
 * @param tof_sf value of TOF_SF register
 *
 * @return range in millimeters, or \a CH_NO_TARGET (0xFFFFFFFF) if no object is detected.
 * The range result format is fixed point with 5 binary fractional digits (divide by 32 to convert
 * to mm).
 *
 * This function takes the time-of-flight and scale factor values from the sensor,
 * and computes the actual one-way range based on the formulas given in the sensor datasheet.
 */
uint32_t chdrv_one_way_range(ch_dev_t *dev_ptr, uint16_t tof, uint16_t tof_sf);

/**
 * @brief Convert the sensor register values to a round-trip range using the calibration data in the
 * ch_dev_t struct.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param tof value of TOF register
 * @param tof_sf value of TOF_SF register
 *
 * @return range in millimeters, or \a CH_NO_TARGET (0xFFFFFFFF) if no object is detected.
 * The range result format is fixed point with 5 binary fractional digits (divide by 32 to convert
 * to mm).
 *
 * This function takes the time-of-flight and scale factor values from the sensor,
 * and computes the actual round-trip range based on the formulas given in the sensor datasheet.
 */
uint32_t chdrv_round_trip_range(ch_dev_t *dev_ptr, uint16_t tof, uint16_t tof_sf);

/**
 * @brief Add an I2C transaction to the non-blocking queue
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 * @param instance pointer to an individual descriptor structure for a sensor
 * @param rd_wrb read/write indicator: 0 if write operation, 1 if read operation
 * @param type I2C transaction type: 0 = std, 1 = prog interface, 2 = external
 * @param addr I2C address for transfer
 * @param nbytes number of bytes to read/write
 * @param data pointer to buffer to receive data or containing data to send
 *
 * @return 0 if successful, non-zero otherwise
 */
int chdrv_group_i2c_queue(ch_group_t *grp_ptr, ch_dev_t *instance, uint8_t rd_wrb, uint8_t type,
			  uint16_t addr, uint16_t nbytes, uint8_t *data);

/**
 * @brief Add an I2C transaction for an external device to the non-blocking queue
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 * @param instance pointer to the ch_dev_t descriptor structure for a sensor
 * @param rd_wrb read/write indicator: 0 if write operation, 1 if read operation
 * @param addr I2C address for transfer
 * @param nbytes number of bytes to read/write
 * @param data pointer to buffer to receive data or containing data to send
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function queues an I2C transaction for an "external" device (i.e. not a Chirp sensor).  It
 * is used when the I2C bus is shared between the Chirp sensor(s) and other devices.
 *
 * The transaction is flagged for special handling when the I/O operation completes.  Specifically,
 * the \a chbsp_external_i2c_irq_handler() will be called by the driver to allow the board support
 * packate (BSP) to perform any necessary operations.
 */
int chdrv_external_i2c_queue(ch_group_t *grp_ptr, ch_dev_t *instance, uint8_t rd_wrb, uint16_t addr,
			     uint16_t nbytes, uint8_t *data);

/**
 * @brief Start a non-blocking sensor readout
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * This function starts a non-blocking I/O operation on the specified group of sensors.
 */
void chdrv_group_i2c_start_nb(ch_group_t *grp_ptr);

/**
 * @brief Continue a non-blocking readout
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of
 * sensors
 * @param i2c_bus_index index value identifying I2C bus within group
 *
 * Call this function once from your I2C interrupt handler each time it executes.
 * It will call \a chdrv_group_i2c_complete_callback() when all transactions are complete.
 */
void chdrv_group_i2c_irq_handler(ch_group_t *grp_ptr, uint8_t i2c_bus_index);

/**
 * @brief Wait for an individual sensor to finish start-up procedure.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param timeout_ms number of milliseconds to wait for sensor to finish start-up before
 * returning failure
 *
 * @return 0 if startup sequence finished, non-zero if startup sequence timed out or sensor is not
 * connected
 *
 * After the sensor is programmed, it executes an internal start-up and self-test sequence. This
 * function waits the specified time in milliseconds for the sensor to finish this sequence.
 */
int chdrv_wait_for_lock(ch_dev_t *dev_ptr, uint16_t timeout_ms);

/**
 * @brief Wait for all sensors to finish start-up procedure.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * @return 0 if startup sequence finished on all detected sensors, non-zero if startup sequence
 * timed out on any sensor(s).
 *
 * After each sensor is programmed, it executes an internal start-up and self-test sequence. This
 * function waits for all sensor devices to finish this sequence.  For each device, the maximum
 * time to wait is \a CHDRV_FREQLOCK_TIMEOUT_MS milliseconds.
 */
int chdrv_group_wait_for_lock(ch_group_t *grp_ptr);

/**
 * @brief Start a measurement in hardware triggered mode.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * @return 0 if success, non-zero if \a grp_ptr pointer is invalid
 *
 * This function starts a triggered measurement on each sensor in a group, by briefly asserting
 * the INT line to each device.  Each sensor must have already been placed in hardware triggered
 * mode before this function is called.
 */
int chdrv_group_hw_trigger(ch_group_t *grp_ptr);

/**
 * @brief Start a measurement in hardware triggered mode on one sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 *
 * @return 0 if success, non-zero if \a dev_ptr pointer is invalid
 *
 * This function starts a triggered measurement on a single sensor, by briefly asserting the INT
 * line to the device. The sensor must have already been placed in hardware triggered mode before
 * this function is called.
 *
 * @note This function requires implementing the optional chirp_bsp.h functions to control the
 * INT pin direction and level for individual sensors (\a chbsp_set_io_dir_in(), \a
 * chbsp_set_io_dir_out(), \a chbsp_io_set(), and \a chbsp_io_clear()).
 */
int chdrv_hw_trigger(ch_dev_t *dev_ptr);

/**
 * @brief Detect a connected sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 *
 * @return 1 if sensor is found, 0 if no sensor is found
 *
 * This function checks for a sensor on the I2C bus by attempting to reset, halt, and read from the
 * device using the programming interface I2C address (0x45).
 *
 * In order for the device to respond, the PROG pin for the device must be asserted before this
 * function is called.  If there are multiple sensors in an application, only one device's PROG pin
 * should be active at any time.
 */
int chdrv_prog_ping(ch_dev_t *dev_ptr);

/**
 * @brief Detect, program, and start a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 *
 * @return 0 if write to sensor succeeded, non-zero otherwise
 *
 * This function probes the I2C bus for the device.  If it is found, the sensor firmware is
 * programmed into the device, and the application I2C address is set.  Then the sensor is reset
 * and execution starts.
 *
 * Once started, the sensor device will begin an internal initialization and self-test sequence.
 * The \a chdrv_wait_for_lock() function may be used to wait for this sequence to complete.
 *
 * @note This routine will leave the PROG pin de-asserted when it completes.
 */
int chdrv_detect_and_program(ch_dev_t *dev_ptr);

/**
 * @brief Detect, program, and start all sensors in a group.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * @return 0 for success, non-zero if write(s) failed to any sensor initially detected as present
 *
 * This function probes the I2C bus for each device in the group.  For each detected sensor, the
 * firmware is programmed into the device, and the application I2C address is set.  Then the sensor
 * is reset and execution starts.
 *
 * Once started, each device will begin an internal initialization and self-test sequence.  The
 * \a chdrv_group_wait_for_lock() function may be used to wait for this sequence to complete on all
 * devices in the group.
 *
 * @note This routine will leave the PROG pin de-asserted for all devices in the group when it
 * completes.
 */
int chdrv_group_detect_and_program(ch_group_t *grp_ptr);

/**
 * @brief Initialize the sensor device configuration.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure to be initialized
 * @param i2c_addr I2C address to assign to this device.  This will be the "application
 I2C address" used to access the device after it is initialized.  Each sensor on an I2C interface
 must use a unique application I2C address.
 * @param io_index index identifying this device.  Each sensor in a group must have a
 * 				    	unique \a io_index value.
 * @param i2c_bus_index index identifying the I2C interface (bus) to use with this device
 * @param part_number integer part number for sensor (e.g. 101 for CH101 device, or 201 for
 CH201))
 *
 * @return 0 (always)
 *
 * This function initializes the ch_dev_t descriptor structure for the device with the specified
 values.
 */
int chdrv_init(ch_dev_t *dev_ptr, uint8_t i2c_addr, uint8_t io_index, uint8_t i2c_bus_index,
	       uint16_t part_number);

/**
 * @brief Initialize data structures and hardware for sensor interaction and reset sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * @return 0 if hardware initialization is successful, non-zero otherwise
 *
 * This function is called internally by \a chdrv_group_start().
 */
int chdrv_group_prepare(ch_group_t *grp_ptr);

/**
 * @brief Initalize and start a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * @return 0 if successful, 1 if device doesn't respond
 *
 * This function resets each sensor in programming mode, transfers the firmware image to the
 * sensor's on-chip memory, changes the sensor's application I2C address from the default, then
 * starts the sensor and sends a timed pulse on the INT line for real-time clock calibration.
 *
 * This function assumes firmware-specific initialization has already been performed for each
 * ch_dev_t descriptor in the sensor group.  (See \a ch_init()).
 */
int chdrv_group_start(ch_group_t *grp_ptr);

/**
 * @brief Write byte to a sensor application register.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param reg_addr register address
 * @param data data value to transmit
 *
 * @return 0 if successful, non-zero otherwise
 */
int chdrv_write_byte(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t data);

/**
 * @brief Write 16 bits to a sensor application register.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param mem_addr sensor memory/register address
 * @param data data value to transmit
 *
 * @return 0 if successful, non-zero otherwise
 */
int chdrv_write_word(ch_dev_t *dev_ptr, uint16_t mem_addr, uint16_t data);

/**
 * @brief Read byte from a sensor application register.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param mem_addr sensor memory/register address
 * @param data pointer to receive buffer
 *
 * @return 0 if successful, non-zero otherwise
 */
int chdrv_read_byte(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data);

/**
 * @brief Read 16 bits from a sensor application register.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param mem_addr sensor memory/register address
 * @param data pointer to receive buffer
 *
 * @return 0 if successful, non-zero otherwise
 */
int chdrv_read_word(ch_dev_t *dev_ptr, uint16_t mem_addr, uint16_t *data);

/**
 * @brief Read multiple bytes from a sensor application register location.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param mem_addr sensor memory/register address
 * @param data pointer to receive buffer
 * @param len number of bytes to read
 *
 * @return 0 if successful, non-zero otherwise
 *
 */
int chdrv_burst_read(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t len);

/**
 * @brief Write multiple bytes to a sensor application register location.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param mem_addr sensor memory/register address
 * @param data pointer to transmit buffer containing data to send
 * @param len number of bytes to write
 *
 * @return 0 if successful, non-zero otherwise
 *
 */
int chdrv_burst_write(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint8_t len);

/**
 * @brief Perform a soft reset on a sensor.
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 * @param mem_addr sensor memory/register address
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function performs a soft reset on an individual sensor by writing to a special control
 * register.
 */
int chdrv_soft_reset(ch_dev_t *dev_ptr);

/**
 * @brief Perform a hard reset on a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function performs a hardware reset on each device in a group of sensors by asserting each
 * device's RESET_N pin.
 */
int chdrv_group_hard_reset(ch_group_t *grp_ptr);

/**
 * @brief Perform a soft reset on a group of sensors.
 *
 * @param grp_ptr pointer to the ch_group_t descriptor structure for a group of sensors
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function performs a soft reset on each device in a group of sensors by writing to a special
 * control register.
 */
int chdrv_group_soft_reset(ch_group_t *grp_ptr);

/**
 * @brief Put sensor(s) in idle state
 *
 * @param dev_ptr pointer to the ch_dev_t descriptor structure for a sensor
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function places the sensor in an idle state by loading an idle loop instruction sequence.
 * This is used only during early initialization of the device.  This is NOT the same as putting a
 * running device into "idle mode" by using the \a ch_set_mode() function.
 */
int chdrv_set_idle(ch_dev_t *dev_ptr);

/**
 * @brief Write to a sensor programming register.
 *
 * @param dev_ptr pointer to the ch_dev_t config structure for a sensor
 * @param reg_addr sensor programming register address.
 * @param data 8-bit or 16-bit data to transmit.
 *
 * @return 0 if write to sensor succeeded, non-zero otherwise
 *
 * This function writes a value to a sensor programming register.
 */
int chdrv_prog_write(ch_dev_t *dev_ptr, uint8_t reg_addr, uint16_t data);

/**
 * @brief Write bytes to a sensor device in programming mode.
 *
 * @param dev_ptr pointer to the ch_dev_t config structure for a sensor
 * @param message pointer to a buffer containing the bytes to write
 * @param len number of bytes to write
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function writes bytes to the device using the programming I2C address.  The
 * PROG line for the device must have been asserted before this function is called.
 */
int chdrv_prog_i2c_write(ch_dev_t *dev_ptr, uint8_t *message, uint16_t len);

/**
 * @brief Read bytes from a sensor device in programming mode.
 *
 * @param dev_ptr pointer to the ch_dev_t config structure for a sensor
 * @param message pointer to a buffer where read bytes will be placed
 * @param len number of bytes to read
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function reads bytes from the device using the programming I2C address.  The
 * PROG line for the device must have been asserted before this function is called.
 */
int chdrv_prog_i2c_read(ch_dev_t *dev_ptr, uint8_t *message, uint16_t len);

/**
 * @brief Read bytes from a sensor device in programming mode, non-blocking.
 *
 * @param dev_ptr pointer to the ch_dev_t config structure for a sensor
 * @param message pointer to a buffer where read bytes will be placed
 * @param len number of bytes to read
 *
 * @return 0 if successful, non-zero otherwise
 *
 * This function temporarily changes the device I2C address to the low-level programming
 * interface, and issues a non-blocking read request. The PROG line for the device must have
 * been asserted before this function is called.
 */
int chdrv_prog_i2c_read_nb(ch_dev_t *dev_ptr, uint8_t *message, uint16_t len);

/**
 * @brief Write to sensor memory.
 *
 * @param dev_ptr pointer to the ch_dev_t config structure for a sensor
 * @param addr sensor programming register start address
 * @param message pointer to data to transmit
 * @param nbytes number of bytes to write
 *
 * @return 0 if write to sensor succeeded, non-zero otherwise
 *
 * This function writes to sensor memory using the low-level programming interface.  The type
 * of write is automatically determined based on data length and target address alignment.
 */
int chdrv_prog_mem_write(ch_dev_t *dev_ptr, uint16_t addr, uint8_t *message, uint16_t nbytes);

/**
 * @brief Register a hook routine to be called after device discovery.
 *
 * @param grp_ptr pointer to the ch_group_t config structure for a group of sensors
 * @param hook_func_ptr address of hook routine to be called
 *
 * This function sets a pointer to a hook routine, which will be called from the
 * Chirp driver when each device is discovered on the I2C bus, before the device is initialized.
 *
 * This function should be called between \a ch_init() and \a ch_group_start().
 */
void chdrv_discovery_hook_set(ch_group_t *grp_ptr, chdrv_discovery_hook_t hook_func_ptr);

/**
 * @brief Set the pre-trigger delay for rx-only sensors
 *
 * @param grp_ptr pointer to the ch_group_t config structure for a group of sensors
 * @param delay_us time to delay between triggering rx-only and tx/rx nodes, in
 * microseconds
 *
 * This function sets a delay interval that will be inserted between triggering rx-only sensors
 * and tx/rx sensors.  This delay allows the rx-only sensor(s) to settle from any startup disruption
 * (e.g. PMUT "ringdown") before the ultrasound pulse is generated by the tx node.
 *
 */
void chdrv_pretrigger_delay_set(ch_group_t *grp_ptr, uint16_t delay_us);

#endif /* CH_DRIVER_H_ */
