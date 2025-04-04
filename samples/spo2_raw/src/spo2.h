// #include <math.h>
// #include <stdio.h>
// #include <stdint.h>
// #include<zephyr/device.h>
// #include<zephyr/devicetree.h>
// #include <zephyr/drivers/i2c.h>


// #ifdef __cplusplus
// extern "C" 
// {
// 	#endif

// 	// I2C frequency configuration
// 	#define PRESCALE_DIV 6
// 	#define SCL_DIV 6

// 	#define I2C0 0
// 	#define CLOCK_FREQUENCY 40000000
// 	#define I2C0 0
// 	#define I2C1 1
// 	#define WRITE_MODE 0
// 	#define READ_MODE 1

// 	#define BEATDETECTOR_INIT_HOLDOFF                2000    // in ms, how long to wait before counting
// 	#define BEATDETECTOR_MASKING_HOLDOFF             200     // in ms, non-retriggerable window after beat detection
// 	#define BEATDETECTOR_BPFILTER_ALPHA              0.6     // EMA factor for the beat period value
// 	#define BEATDETECTOR_MIN_THRESHOLD               20      // minimum threshold (filtered) value
// 	#define BEATDETECTOR_MAX_THRESHOLD               800     // maximum threshold (filtered) value
// 	#define BEATDETECTOR_STEP_RESILIENCY             30      // maximum negative jump that triggers the beat edge
// 	#define BEATDETECTOR_THRESHOLD_FALLOFF_TARGET    0.3     // thr chasing factor of the max value when beat
// 	#define BEATDETECTOR_THRESHOLD_DECAY_FACTOR      0.99    // thr chasing factor when no beat
// 	#define BEATDETECTOR_INVALID_READOUT_DELAY       2000    // in ms, no-beat time to cause a reset
// 	#define BEATDETECTOR_SAMPLES_PERIOD              10      // in ms, 1/Fs

// 	#define MAX30100_I2C_ADDRESS                    0x57

// 	// Interrupt status register (RO)
// 	#define MAX30100_REG_INTERRUPT_STATUS           0x00
// 	#define MAX30100_IS_PWR_RDY                     (1 << 0)
// 	#define MAX30100_IS_SPO2_RDY                    (1 << 4)
// 	#define MAX30100_IS_HR_RDY                      (1 << 5)
// 	#define MAX30100_IS_TEMP_RDY                    (1 << 6)
// 	#define MAX30100_IS_A_FULL                      (1 << 7)

// 	// Interrupt enable register
// 	#define MAX30100_REG_INTERRUPT_ENABLE           0x01
// 	#define MAX30100_IE_ENB_SPO2_RDY                (1 << 4)
// 	#define MAX30100_IE_ENB_HR_RDY                  (1 << 5)
// 	#define MAX30100_IE_ENB_TEMP_RDY                (1 << 6)
// 	#define MAX30100_IE_ENB_A_FULL                  (1 << 7)

// 	// FIFO control and data registers
// 	#define MAX30100_REG_FIFO_WRITE_POINTER         0x02
// 	#define MAX30100_REG_FIFO_OVERFLOW_COUNTER      0x03
// 	#define MAX30100_REG_FIFO_READ_POINTER          0x04
// 	#define MAX30100_REG_FIFO_DATA                  0x05  // Burst read does not autoincrement addr

// 	// Mode Configuration register
// 	#define MAX30100_REG_MODE_CONFIGURATION         0x06
// 	#define MAX30100_MC_TEMP_EN                     (1 << 3)
// 	#define MAX30100_MC_RESET                       (1 << 6)
// 	#define MAX30100_MC_SHDN                        (1 << 7)

// 	#define MAX30100_MODE_HRONLY    0x02
// 	#define MAX30100_MODE_SPO2_HR    0x03

// 	// SpO2 Configuration register
// 	#define MAX30100_REG_SPO2_CONFIGURATION         0x07
// 	#define MAX30100_SPC_SPO2_HI_RES_EN             (1 << 6)

// 	// Samplerate Configuration 
// 	#define MAX30100_SAMPRATE_50HZ       0x00
// 	#define MAX30100_SAMPRATE_100HZ      0x01
// 	#define MAX30100_SAMPRATE_167HZ      0x02
// 	#define MAX30100_SAMPRATE_200HZ      0x03
// 	#define MAX30100_SAMPRATE_400HZ      0x04
// 	#define MAX30100_SAMPRATE_600HZ      0x05
// 	#define MAX30100_SAMPRATE_800HZ      0x06
// 	#define MAX30100_SAMPRATE_1000HZ     0x07

// 	// Pulsewidth configuration
// 	#define MAX30100_SPC_PW_200US_13BITS     0x00
// 	#define MAX30100_SPC_PW_400US_14BITS     0x01
// 	#define MAX30100_SPC_PW_800US_15BITS     0x02
// 	#define MAX30100_SPC_PW_1600US_16BITS    0x03

// 	// LED Configuration register
// 	#define MAX30100_REG_LED_CONFIGURATION          0x09

// 	#define RED_LED_CURRENT         MAX30100_LED_CURR_20_8MA
// 	#define IR_LED_CURRENT          MAX30100_LED_CURR_20_8MA

// 	//SpO2 configuration definitions
// 	#ifndef CIRCULAR_BUFFER_XS
// 	#define __CB_ST__ uint16_t
// 	#else
// 	#define __CB_ST__ uint8_t
// 	#endif

// 	// Default configuration
// 	#define DEFAULT_MODE                MAX30100_MODE_HRONLY
// 	#define DEFAULT_SAMPLING_RATE       MAX30100_SAMPRATE_50HZ
// 	#define DEFAULT_PULSE_WIDTH         MAX30100_SPC_PW_1600US_16BITS
// 	#define DEFAULT_RED_LED_CURRENT     MAX30100_LED_CURR_30_6MA
// 	#define DEFAULT_IR_LED_CURRENT      MAX30100_LED_CURR_30_6MA
// 	#define EXPECTED_PART_ID            0x11
// 	#define RINGBUFFER_SIZE             16

// 	#define I2C_BUS_SPEED               400000UL

// 	// Temperature integer part register
// 	#define MAX30100_REG_TEMPERATURE_DATA_INT       0x16

// 	// Temperature fractional part register
// 	#define MAX30100_REG_TEMPERATURE_DATA_FRAC      0x17

// 	// Revision ID register (RO)
// 	#define MAX30100_REG_REVISION_ID                0xfe

// 	// Part ID register
// 	#define MAX30100_REG_PART_ID                    0xff

// 	// Depth of fifo data register
// 	#define MAX30100_FIFO_DEPTH                     0x10

// 	#define CALCULATE_EVERY_N_BEATS         		1

// 	#define SAMPLING_FREQUENCY                  100
// 	#define CURRENT_ADJUSTMENT_PERIOD_MS        500
// 	#define RED_LED_CURRENT_START               MAX30100_LED_CURR_27_1MA
// 	#define DC_REMOVER_ALPHA                    0.95

// 	// Depth of Fifo data register
// 	#define S  16

// 	typedef enum LEDCurrent {
// 		MAX30100_LED_CURR_0MA      = 0x00,
// 		MAX30100_LED_CURR_4_4MA    = 0x01,
// 		MAX30100_LED_CURR_7_6MA    = 0x02,
// 		MAX30100_LED_CURR_11MA     = 0x03,
// 		MAX30100_LED_CURR_14_2MA   = 0x04,
// 		MAX30100_LED_CURR_17_4MA   = 0x05,
// 		MAX30100_LED_CURR_20_8MA   = 0x06,
// 		MAX30100_LED_CURR_24MA     = 0x07,
// 		MAX30100_LED_CURR_27_1MA   = 0x08,
// 		MAX30100_LED_CURR_30_6MA   = 0x09,
// 		MAX30100_LED_CURR_33_8MA   = 0x0a,
// 		MAX30100_LED_CURR_37MA     = 0x0b,
// 		MAX30100_LED_CURR_40_2MA   = 0x0c,
// 		MAX30100_LED_CURR_43_6MA   = 0x0d,
// 		MAX30100_LED_CURR_46_8MA   = 0x0e,
// 		MAX30100_LED_CURR_50MA     = 0x0f
// 	} LEDCurrent;

// 	void spo2_write(const struct device *dev, uint8_t i2c_number,uint8_t slave_address, uint8_t reg_addr, uint8_t data);
// 	uint8_t spo2_read(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr);
// 	void spo2_burstread(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr, uint8_t *buffer, uint8_t toRead);
// 	float retrieveTemperature(const struct device *dev);

//     #ifdef __cplusplus
// }
// #endif






/**
 * Project                           : Smartwatch
 * Name of the file                  : spo2_polling.h
 * Brief Description of file         : This is a Baremetal SpO2 sensor Driver header file for Mindgrove Silicon.
 * Name of Author                    : Harini Sree.S
 * Email ID                          : harini@mindgrovetech.in
 *
 * @file spo2_polling.h
 * @author Harini Sree. S (harini@mindgrovetech.in)
 * @brief This is a Baremetal SpO2 sensor Driver header file for Mindgrove Silicon.
 * @date 2024-01-29
 *
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 *
**/

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
// #include "platform.h"
// #include "utils.h"
// #include <stdlib.h>
// #include <string>
// #include <inttypes.h>

// #include "uart.h"
// #include "traps.h"
// #include "defines.h"
// #include "memory.h"

// #include "i2c_v2.h"
// #include "log.h"
// #include "gpio.h"
// #include "plic_driver.h"

double log(double y)
{
double x = (y-1)/(y+3);
double sum = 1.0;
double xpow=x;

for(int n = 1; n <= 151; n++)
{

if(n%2!=0)
{

sum = sum + xpow/(double)n;
}
xpow = xpow * x;

 }    

 sum *= 2;

 return sum;
 }

#ifdef __cplusplus
extern "C" 
{
	#endif

	// I2C frequency configuration
	#define PRESCALE_DIV 6
	#define SCL_DIV 6

	#define I2C0 0
	#define CLOCK_FREQUENCY 40000000
	#define I2C0 0
	#define I2C1 1
	#define WRITE_MODE 0
	#define READ_MODE 1

	#define BEATDETECTOR_INIT_HOLDOFF                2000    // in ms, how long to wait before counting
	#define BEATDETECTOR_MASKING_HOLDOFF             200     // in ms, non-retriggerable window after beat detection
	#define BEATDETECTOR_BPFILTER_ALPHA              0.6     // EMA factor for the beat period value
	#define BEATDETECTOR_MIN_THRESHOLD               20      // minimum threshold (filtered) value
	#define BEATDETECTOR_MAX_THRESHOLD               800     // maximum threshold (filtered) value
	#define BEATDETECTOR_STEP_RESILIENCY             30      // maximum negative jump that triggers the beat edge
	#define BEATDETECTOR_THRESHOLD_FALLOFF_TARGET    0.3     // thr chasing factor of the max value when beat
	#define BEATDETECTOR_THRESHOLD_DECAY_FACTOR      0.99    // thr chasing factor when no beat
	#define BEATDETECTOR_INVALID_READOUT_DELAY       2000    // in ms, no-beat time to cause a reset
	#define BEATDETECTOR_SAMPLES_PERIOD              10      // in ms, 1/Fs

	#define MAX30100_I2C_ADDRESS                    0x57

	// Interrupt status register (RO)
	#define MAX30100_REG_INTERRUPT_STATUS           0x00
	#define MAX30100_IS_PWR_RDY                     (1 << 0)
	#define MAX30100_IS_SPO2_RDY                    (1 << 4)
	#define MAX30100_IS_HR_RDY                      (1 << 5)
	#define MAX30100_IS_TEMP_RDY                    (1 << 6)
	#define MAX30100_IS_A_FULL                      (1 << 7)

	// Interrupt enable register
	#define MAX30100_REG_INTERRUPT_ENABLE           0x01
	#define MAX30100_IE_ENB_SPO2_RDY                (1 << 4)
	#define MAX30100_IE_ENB_HR_RDY                  (1 << 5)
	#define MAX30100_IE_ENB_TEMP_RDY                (1 << 6)
	#define MAX30100_IE_ENB_A_FULL                  (1 << 7)

	// FIFO control and data registers
	#define MAX30100_REG_FIFO_WRITE_POINTER         0x02
	#define MAX30100_REG_FIFO_OVERFLOW_COUNTER      0x03
	#define MAX30100_REG_FIFO_READ_POINTER          0x04
	#define MAX30100_REG_FIFO_DATA                  0x05  // Burst read does not autoincrement addr

	// Mode Configuration register
	#define MAX30100_REG_MODE_CONFIGURATION         0x06
	#define MAX30100_MC_TEMP_EN                     (1 << 3)
	#define MAX30100_MC_RESET                       (1 << 6)
	#define MAX30100_MC_SHDN                        (1 << 7)

	#define MAX30100_MODE_HRONLY    0x02
	#define MAX30100_MODE_SPO2_HR    0x03

	// SpO2 Configuration register
	#define MAX30100_REG_SPO2_CONFIGURATION         0x07
	#define MAX30100_SPC_SPO2_HI_RES_EN             (1 << 6)

	// Samplerate Configuration 
	#define MAX30100_SAMPRATE_50HZ       0x00
	#define MAX30100_SAMPRATE_100HZ      0x01
	#define MAX30100_SAMPRATE_167HZ      0x02
	#define MAX30100_SAMPRATE_200HZ      0x03
	#define MAX30100_SAMPRATE_400HZ      0x04
	#define MAX30100_SAMPRATE_600HZ      0x05
	#define MAX30100_SAMPRATE_800HZ      0x06
	#define MAX30100_SAMPRATE_1000HZ     0x07

	// Pulsewidth configuration
	#define MAX30100_SPC_PW_200US_13BITS     0x00
	#define MAX30100_SPC_PW_400US_14BITS     0x01
	#define MAX30100_SPC_PW_800US_15BITS     0x02
	#define MAX30100_SPC_PW_1600US_16BITS    0x03

	// LED Configuration register
	#define MAX30100_REG_LED_CONFIGURATION          0x09

	#define RED_LED_CURRENT         MAX30100_LED_CURR_20_8MA
	#define IR_LED_CURRENT          MAX30100_LED_CURR_20_8MA

	//SpO2 configuration definitions
	#ifndef CIRCULAR_BUFFER_XS
	#define __CB_ST__ uint16_t
	#else
	#define __CB_ST__ uint8_t
	#endif

	// Default configuration
	#define DEFAULT_MODE                MAX30100_MODE_HRONLY
	#define DEFAULT_SAMPLING_RATE       MAX30100_SAMPRATE_50HZ
	#define DEFAULT_PULSE_WIDTH         MAX30100_SPC_PW_1600US_16BITS
	#define DEFAULT_RED_LED_CURRENT     MAX30100_LED_CURR_30_6MA
	#define DEFAULT_IR_LED_CURRENT      MAX30100_LED_CURR_30_6MA
	#define EXPECTED_PART_ID            0x11
	#define RINGBUFFER_SIZE             16

	#define I2C_BUS_SPEED               400000UL

	// Temperature integer part register
	#define MAX30100_REG_TEMPERATURE_DATA_INT       0x16

	// Temperature fractional part register
	#define MAX30100_REG_TEMPERATURE_DATA_FRAC      0x17

	// Revision ID register (RO)
	#define MAX30100_REG_REVISION_ID                0xfe

	// Part ID register
	#define MAX30100_REG_PART_ID                    0xff

	// Depth of fifo data register
	#define MAX30100_FIFO_DEPTH                     0x10

	#define CALCULATE_EVERY_N_BEATS         		1

	#define SAMPLING_FREQUENCY                  100
	#define CURRENT_ADJUSTMENT_PERIOD_MS        500
	#define RED_LED_CURRENT_START               MAX30100_LED_CURR_27_1MA
	#define DC_REMOVER_ALPHA                    0.95

	// Depth of Fifo data register
	#define S  16

	/* The above code is defining a macro for finding the minimum of two values. The macro is named `min`
	and it takes two arguments `a` and `b`. It uses a ternary operator to compare the values of `a` and
	`b`, and returns the minimum of the two values. The macro is defined in a way that it can handle
	different data types by using `__typeof__` to determine the type of the arguments. */
	#ifndef min
	#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
		__typeof__ (b) _b = (b); \
		_a < _b ? _a : _b; })
	#endif

	/** @fn extern inline uint64_t get_mcycle_start()
	 * @brief  Reads the mcycle value
	 * @return The cycle count
	 */
	extern inline uint64_t get_mcycle_start() {
	uint64_t mcycle = 0;
	__asm__ volatile("csrr %0,mcycle":"=r"(mcycle));
	return mcycle;
	}

	/** @fn extern inline uint64_t get_mcycle_stop()
	 * @brief  Reads the mcycle value
	 * @return The cycle count
	 */
	extern inline uint64_t get_mcycle_stop(){
	uint64_t mcycle;
	
	__asm__ volatile("csrr %0,mcycle":"=r"(mcycle));
	return mcycle;
	}

	/** @fn extern int millis(int total_cycles)
	 * @brief  A simple helper function to convert the total number of
	 *         of cycles taken to milliseconds
	 * @return The real time in milliseconds
	 */
	extern int millis(int total_cycles)
	{
		return total_cycles/(CLOCK_FREQUENCY/1000);
	}



	// The above code defines a class `circularbuffer` that represents a circular buffer data structure. The circular buffer has a fixed size `S` and maintains an array `buffer` to store elements. It also keeps track of the head, tail, and count of elements in the buffer.
	class circularbuffer 
	{
		private:
			uint16_t buffer[S];
			uint16_t* head = buffer;
			uint16_t* tail = buffer;
			uint16_t count = 0;

		public:
			void init();
			/**
			 * Adds an element to the end of buffer: the operation returns `false` if the addition caused overwriting an existing element.
			 */
			bool push(uint16_t value);
			/**
			* Removes an element from the end of the buffer.
			*/
			uint16_t pop();
			/**
			* Returns `true` if no elements can be removed from the buffer.
			*/
			bool inline isEmpty();
	};

	/*The below code defines a class called MAX30100, which is a sensor module. The class contains several public methods for interacting with the sensor, such as initializing the sensor, updating sensor data, retrieving raw sensor values, resetting the FIFO buffer, starting temperature sampling, checking temperature readiness, retrieving temperature data, shutting down and resuming the sensor, and getting the part ID. Additionally, there are some private methods that are currently commented out, such as reading and writing registers and performing burst reads. The class provides a structured interface for working with the MAX30100 sensor module.*/
	class MAX30100 
	{
		public:
			MAX30100();
			bool begin(const struct device *dev);
			void update(const struct device *dev);
			bool getRawValues(uint16_t *ir, uint16_t *red);
			void resetFifo(const struct device *dev);
			void startTemperatureSampling(const struct device *dev);
			bool isTemperatureReady(const struct device *dev);
			float retrieveTemperature(const struct device *dev);
			void shutdown(const struct device *dev);
			void resume(const struct device *dev);
			uint8_t getPartId(const struct device *dev);

		private:
			void readFifoData(const struct device *dev);
	};


	typedef enum LEDCurrent {
		MAX30100_LED_CURR_0MA      = 0x00,
		MAX30100_LED_CURR_4_4MA    = 0x01,
		MAX30100_LED_CURR_7_6MA    = 0x02,
		MAX30100_LED_CURR_11MA     = 0x03,
		MAX30100_LED_CURR_14_2MA   = 0x04,
		MAX30100_LED_CURR_17_4MA   = 0x05,
		MAX30100_LED_CURR_20_8MA   = 0x06,
		MAX30100_LED_CURR_24MA     = 0x07,
		MAX30100_LED_CURR_27_1MA   = 0x08,
		MAX30100_LED_CURR_30_6MA   = 0x09,
		MAX30100_LED_CURR_33_8MA   = 0x0a,
		MAX30100_LED_CURR_37MA     = 0x0b,
		MAX30100_LED_CURR_40_2MA   = 0x0c,
		MAX30100_LED_CURR_43_6MA   = 0x0d,
		MAX30100_LED_CURR_46_8MA   = 0x0e,
		MAX30100_LED_CURR_50MA     = 0x0f
	} LEDCurrent;


	/*The bwlow code is defining an enumeration type `BeatDetectorState` with five possible values: `BEATDETECTOR_STATE_INIT`, `BEATDETECTOR_STATE_WAITING`, `BEATDETECTOR_STATE_FOLLOWING_SLOPE`, `BEATDETECTOR_STATE_MAYBE_DETECTED`, and `BEATDETECTOR_STATE_MASKING`. This enumeration is used to represent different states of a beat detector system.*/
	typedef enum BeatDetectorState 
	{
		BEATDETECTOR_STATE_INIT,
		BEATDETECTOR_STATE_WAITING,
		BEATDETECTOR_STATE_FOLLOWING_SLOPE,
		BEATDETECTOR_STATE_MAYBE_DETECTED,
		BEATDETECTOR_STATE_MASKING
	} BeatDetectorState;

	/*The below code declares a class `BeatDetector` with methods to add a sample, get the beat rate, and get the current threshold. The class maintains internal variables such as the current state, threshold, beat period, last maximum value, and timestamp of the last beat. The class is likely used to detect beats in a signal or data stream.*/
	class BeatDetector
	{
		public:
			BeatDetector();
			bool addSample(float sample);
			uint8_t getRate();
			float getCurrentThreshold();

		private:
			bool checkForBeat(float value);
			void decreaseThreshold();

			BeatDetectorState state;
			float threshold;
			float beatPeriod;
			float lastMaxValue;
			uint32_t tsLastBeat;
	};

	// Low pass butterworth filter order=1 alpha1=0.1
	// Fs=100Hz, Fc=6Hz
	class  FilterBuLp1
	{
		public:
			FilterBuLp1()
			{
				v[0]=0.0;
			}
		private:
			float v[2];
		public:
			float step(float x) //class II
			{
				v[0] = v[1];
				v[1] = (2.452372752527856026e-1 * x) + (0.50952544949442879485 * v[0]);
				return(v[0] + v[1]);
			}
	};

	class DCRemover
	{
		public:
			DCRemover() : alpha(0), dcw(0)
			{
			}
			DCRemover(float alpha_) : alpha(alpha_), dcw(0)
			{
			}

			// The above code defines a function named `step` that takes a single float parameter `x`. Inside the function, it calculates a new value for the variable `dcw` using the formula `dcw = (float)x + alpha * dcw`, where `alpha` is assumed to be a global variable.
			float step(float x)
			{
				float olddcw = dcw;
				dcw = (float)x + alpha * dcw;
				return dcw - olddcw;
			}

			// The code defines a function named `getDCW` that returns a floating-point value.The function simply returns the value of the variable `dcw`.
			float getDCW()
			{
				return dcw;
			}

		private:
			float alpha;
			float dcw;
	};

	// The above code is defining an enumeration type `PulseOximeterState` with three possible states: `PULSEOXIMETER_STATE_INIT`, `PULSEOXIMETER_STATE_IDLE`, and `PULSEOXIMETER_STATE_DETECTING`. This enumeration is used to represent different states of a pulse oximeter device.
	typedef enum PulseOximeterState 
	{
		PULSEOXIMETER_STATE_INIT,
		PULSEOXIMETER_STATE_IDLE,
		PULSEOXIMETER_STATE_DETECTING
	} PulseOximeterState;

	// The above code is defining an enumeration type `PulseOximeterDebuggingMode` with four possible values: `PULSEOXIMETER_DEBUGGINGMODE_NONE`, `PULSEOXIMETER_DEBUGGINGMODE_RAW_VALUES`, `PULSEOXIMETER_DEBUGGINGMODE_AC_VALUES`, and `PULSEOXIMETER_DEBUGGINGMODE_PULSEDETECT`. This enum is used to represent different debugging modes for a pulse oximeter device.
	typedef enum PulseOximeterDebuggingMode 
	{
		PULSEOXIMETER_DEBUGGINGMODE_NONE,
		PULSEOXIMETER_DEBUGGINGMODE_RAW_VALUES,
		PULSEOXIMETER_DEBUGGINGMODE_AC_VALUES,
		PULSEOXIMETER_DEBUGGINGMODE_PULSEDETECT
	} PulseOximeterDebuggingMode;


	/* This defines a class called `PulseOximeter` which represents a pulse oximeter device. It has public methods for initializing the device, updating readings, getting heart rate and SpO2 values, getting red LED current bias, setting a callback function for beat detection, shutting down and resuming the device. The class also has private methods for checking samples and current bias. It contains member variables to store various timestamps, a beat detector object, DC removers for infrared and red LEDs, a low-pass filter, red LED current index, and a MAX30100 sensor object.*/
	class PulseOximeter 
	{
		public:
			PulseOximeter();

			bool begin(const struct device *dev, PulseOximeterDebuggingMode debuggingMode_=PULSEOXIMETER_DEBUGGINGMODE_NONE);
			void update(const struct device *dev);
			uint8_t getHeartRate();
			uint8_t getSpO2();
			uint8_t getRedLedCurrentBias();
			void setOnBeatDetectedCallback(void (*cb)());
			// void setIRLedCurrent(LEDCurrent irLedCurrent);
			void shutdown(const struct device *dev);
			void resume(const struct device *dev);

		private:
			void checkSample();
			void checkCurrentBias(const struct device *dev);

			PulseOximeterState state;
			PulseOximeterDebuggingMode debuggingMode;
			uint32_t tsFirstBeatDetected;
			uint32_t tsLastBeatDetected;
			uint32_t tsLastBiasCheck;
			uint32_t tsLastCurrentAdjustment;
			BeatDetector beatDetector;
			DCRemover irDCRemover;
			DCRemover redDCRemover;
			FilterBuLp1 lpf;
			uint8_t redLedCurrentIndex;
			// LEDCurrent irLedCurrent;
			MAX30100 hrm;

			void (*onBeatDetected)();
	};

	/*The above code is defining a static constant array named `spO2LUT` with 43 elements of type `uint8_t`. The array contains a lookup table for SpO2 values (oxygen saturation levels) ranging from 100 to 93. Each element in the array corresponds to a specific SpO2 value.*/
	static const uint8_t spO2LUT[43] = {100,100,100,100,99,99,99,99,99,99,98,98,98,98,98,97,97,97,97,97,97,96,96,96,96,96,96,95,95,95,95,95,95,94,94,94,94,94,93,93,93,93,93};

	float irACValueSqSum;
	float redACValueSqSum;
	uint8_t beatsDetectedNum;
	uint32_t samplesRecorded;
	uint8_t spO2;

	void SpO2Calculator_update(float irACValue, float redACValue, bool beatDetected);
	void SpO2Calculator_reset();
	uint8_t SpO2Calculator_getSpO2();
	void spo2_write(const struct device *dev, uint8_t i2c_number,uint8_t slave_address, uint8_t reg_addr, uint8_t data);
	uint8_t spo2_read(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr);
	void spo2_burstread(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr, uint8_t *buffer, uint8_t toRead);
	void delayms(long delay);
	
	#ifdef __cplusplus
}
#endif
