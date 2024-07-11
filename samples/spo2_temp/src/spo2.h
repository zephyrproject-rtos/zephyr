#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>


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

	void spo2_write(const struct device *dev, uint8_t i2c_number,uint8_t slave_address, uint8_t reg_addr, uint8_t data);
	uint8_t spo2_read(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr);
	float retrieveTemperature(const struct device *dev);

    #ifdef __cplusplus
}
#endif