#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

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
    typedef struct {
        uint16_t buffer[S];
        uint16_t* head;
        uint16_t* tail;
        uint16_t count;
    } CBuffer;

    /**
     * @brief Initializes the circular buffer by setting the head and tail pointers
     * to the beginning of the buffer, and the count of elements in the buffer
     * to 0.
     */
    void CBuffer_init(CBuffer* cb);

    /**
     * Adds an element to the end of the buffer.
     * 
     * @param cb The circular buffer.
     * @param value The value to be added.
     * @return bool Returns false if the addition caused overwriting an existing element.
     */
    bool CBuffer_push(CBuffer* cb, uint16_t value);

    /**
     * Removes an element from the end of the buffer.
     * 
     * @param cb The circular buffer.
     * @return uint16_t The value retrieved from the buffer.
     */
    uint16_t CBuffer_pop(CBuffer* cb);

    /**
     * Returns true if no elements can be removed from the buffer.
     * 
     * @param cb The circular buffer.
     * @return bool True if the buffer is empty, false otherwise.
     */
    bool CBuffer_isEmpty(CBuffer* cb);


    /*The below code defines a class called MAX30100, which is a sensor module. The class contains several public methods for interacting with the sensor, such as initializing the sensor, updating sensor data, retrieving raw sensor values, resetting the FIFO buffer, starting temperature sampling, checking temperature readiness, retrieving temperature data, shutting down and resuming the sensor, and getting the part ID. Additionally, there are some private methods that are currently commented out, such as reading and writing registers and performing burst reads. The class provides a structured interface for working with the MAX30100 sensor module.*/
    typedef struct {
        // Add any necessary fields here, e.g., configuration states
    } MAX30100;

    /**
     * @brief Initializes the MAX30100 sensor structure.
     */
    void MAX30100_init(MAX30100 *sensor);

    /**
     * @brief Initializes the MAX30100 sensor.
     * 
     * @param dev The device structure.
     * 
     * @return bool True if the initialization is successful, false otherwise.
     */
    bool MAX30100_begin(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Reads FIFO data from the MAX30100 sensor.
     * 
     * @param dev The device structure.
     */
    void MAX30100_update(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Retrieves raw IR and red values from the MAX30100 sensor.
     * 
     * @param ir Pointer to store the retrieved raw IR value.
     * @param red Pointer to store the retrieved raw red value.
     * 
     * @return bool True if raw values are successfully retrieved, false otherwise.
     */
    bool MAX30100_getRawValues(MAX30100 *sensor, uint16_t *ir, uint16_t *red);

    /**
     * @brief Resets the FIFO registers of the MAX30100 sensor.
     * 
     * @param dev The device structure.
     */
    void MAX30100_resetFifo(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Starts temperature sampling on the MAX30100 sensor.
     * 
     * @param dev The device structure.
     */
    void MAX30100_startTemperatureSampling(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Checks if temperature sampling is ready on the MAX30100 sensor.
     * 
     * @param dev The device structure.
     * 
     * @return bool True if temperature sampling is not in progress, false otherwise.
     */
    bool MAX30100_isTemperatureReady(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Retrieves the temperature reading from the MAX30100 sensor.
     * 
     * @param dev The device structure.
     * 
     * @return float The temperature reading in degrees Celsius.
     */
    float MAX30100_retrieveTemperature(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Shuts down the MAX30100 sensor.
     * 
     * @param dev The device structure.
     */
    void MAX30100_shutdown(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Resumes normal operation of the MAX30100 sensor.
     * 
     * @param dev The device structure.
     */
    void MAX30100_resume(MAX30100 *sensor, const struct device *dev);

    /**
     * @brief Retrieves the part ID of the MAX30100 sensor.
     * 
     * @param dev The device structure.
     * 
     * @return uint8_t The part ID of the MAX30100 sensor.
     */
    uint8_t MAX30100_getPartId(MAX30100 *sensor, const struct device *dev);


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


    /*The below code is defining an enumeration type `BeatDetectorState` with five possible values: `BEATDETECTOR_STATE_INIT`, `BEATDETECTOR_STATE_WAITING`, `BEATDETECTOR_STATE_FOLLOWING_SLOPE`, `BEATDETECTOR_STATE_MAYBE_DETECTED`, and `BEATDETECTOR_STATE_MASKING`. This enumeration is used to represent different states of a beat detector system.*/
    typedef enum BeatDetectorState 
    {
        BEATDETECTOR_STATE_INIT,
        BEATDETECTOR_STATE_WAITING,
        BEATDETECTOR_STATE_FOLLOWING_SLOPE,
        BEATDETECTOR_STATE_MAYBE_DETECTED,
        BEATDETECTOR_STATE_MASKING
    } BeatDetectorState;

    // Structure to hold the state of the BeatDetector
    typedef struct {
        BeatDetectorState state;
        float threshold;
        float beatPeriod;
        float lastMaxValue;
        uint32_t tsLastBeat;
    } BeatDetector;

    /**
     * @brief Initializes the BeatDetector structure.
     * 
     * @param detector Pointer to the BeatDetector structure to initialize.
     */
    void BeatDetector_init(BeatDetector *detector);

    /**
     * @brief Adds a sample to the BeatDetector for analysis.
     * 
     * @param detector Pointer to the BeatDetector structure.
     * @param sample The sample value to be added to the BeatDetector for analysis.
     * @return bool True if a beat is detected, otherwise false.
     */
    bool BeatDetector_addSample(BeatDetector *detector, float sample);

    /**
     * @brief Retrieves the heart rate calculated by the BeatDetector.
     * 
     * @param detector Pointer to the BeatDetector structure.
     * @return uint8_t The heart rate calculated by the BeatDetector in beats per minute.
     */
    uint8_t BeatDetector_getRate(BeatDetector *detector);

    /**
     * @brief Retrieves the current threshold value used by the BeatDetector.
     * 
     * @param detector Pointer to the BeatDetector structure.
     * @return float The current threshold value used by the BeatDetector.
     */
    float BeatDetector_getCurrentThreshold(BeatDetector *detector);


    static bool checkForBeat(BeatDetector *detector, float sample);

    // Low pass butterworth filter order=1 alpha1=0.1
    // Fs=100Hz, Fc=6Hz
    // Structure to hold the state of the FilterBuLp1
    typedef struct {
        float v[2];
    } FilterBuLp1;

    /**
     * @brief Initializes the FilterBuLp1 structure.
     * 
     * @param filter Pointer to the FilterBuLp1 structure to initialize.
     */
    void FilterBuLp1_init(FilterBuLp1 *filter);

    /**
     * @brief Processes a sample through the low-pass filter.
     * 
     * @param filter Pointer to the FilterBuLp1 structure.
     * @param x The input sample to process.
     * @return float The filtered output.
     */
    float FilterBuLp1_step(FilterBuLp1 *filter, float x);

    // Structure to hold the state of the DCRemover
    typedef struct {
        float alpha;
        float dcw;
    } DCRemover;

    /**
     * @brief Initializes the DCRemover structure with default values.
     * 
     * @param remover Pointer to the DCRemover structure to initialize.
     */
    void DCRemover_init(DCRemover *remover);

    /**
     * @brief Initializes the DCRemover structure with a specified alpha value.
     * 
     * @param remover Pointer to the DCRemover structure to initialize.
     * @param alpha The alpha value to set.
     */
    void DCRemover_init_with_alpha(DCRemover *remover, float alpha);

    /**
     * @brief Processes a sample through the DC remover.
     * 
     * @param remover Pointer to the DCRemover structure.
     * @param x The input sample to process.
     * @return float The DC-removed output.
     */
    float DCRemover_step(DCRemover *remover, float x);

    /**
     * @brief Retrieves the current DCW value.
     * 
     * @param remover Pointer to the DCRemover structure.
     * @return float The current DCW value.
     */
    float DCRemover_getDCW(const DCRemover *remover);

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

    // Structure to hold the state of the PulseOximeter
    typedef struct {
        PulseOximeterState state;
        PulseOximeterDebuggingMode debuggingMode;
        uint32_t tsFirstBeatDetected;
        uint32_t tsLastBeatDetected;
        uint32_t tsLastBiasCheck;
        uint32_t tsLastCurrentAdjustment;
        BeatDetector beatDetector;
        float irDCRemover;
        float redDCRemover;
        FilterBuLp1 lpf;
        uint8_t redLedCurrentIndex;
        MAX30100 hrm;
        void (*onBeatDetected)();
    } PulseOximeter;

    /**
     * @brief Initializes the PulseOximeter structure.
     * 
     * @param ox Pointer to the PulseOximeter structure to initialize.
     */
    void PulseOximeter_init(PulseOximeter *ox);

    /**
     * @brief Initializes the PulseOximeter structure and the underlying hardware.
     * 
     * @param ox Pointer to the PulseOximeter structure to initialize.
     * @param dev Pointer to the device structure.
     * @param debuggingMode_ The debugging mode to set.
     * @return bool True if initialization is successful, otherwise false.
     */
    bool PulseOximeter_begin(PulseOximeter *ox, const struct device *dev, PulseOximeterDebuggingMode debuggingMode_);

    /**
     * @brief Updates the PulseOximeter structure.
     * 
     * @param ox Pointer to the PulseOximeter structure.
     * @param dev Pointer to the device structure.
     */
    void PulseOximeter_update(PulseOximeter *ox, const struct device *dev);

    /**
     * @brief Retrieves the heart rate calculated by the PulseOximeter.
     * 
     * @param ox Pointer to the PulseOximeter structure.
     * @return uint8_t The heart rate in beats per minute.
     */
    uint8_t PulseOximeter_getHeartRate(const PulseOximeter *ox);

    /**
     * @brief Retrieves the SpO2 value calculated by the PulseOximeter.
     * 
     * @param ox Pointer to the PulseOximeter structure.
     * @return uint8_t The SpO2 value.
     */
    uint8_t PulseOximeter_getSpO2(const PulseOximeter *ox);

    /**
     * @brief Retrieves the red LED current bias.
     * 
     * @param ox Pointer to the PulseOximeter structure.
     * @return uint8_t The red LED current bias.
     */
    uint8_t PulseOximeter_getRedLedCurrentBias(const PulseOximeter *ox);

    /**
     * @brief Sets a callback function to be called when a beat is detected.
     * 
     * @param ox Pointer to the PulseOximeter structure.
     * @param cb A pointer to the callback function.
     */
    void PulseOximeter_setOnBeatDetectedCallback(PulseOximeter *ox, void (*cb)());

    /**
     * @brief Shuts down the PulseOximeter.
     * 
     * @param ox Pointer to the PulseOximeter structure.
     * @param dev Pointer to the device structure.
     */
    void PulseOximeter_shutdown(PulseOximeter *ox, const struct device *dev);

    /**
     * @brief Resumes the operation of the PulseOximeter.
     * 
     * @param ox Pointer to the PulseOximeter structure.
     * @param dev Pointer to the device structure.
     */
    void PulseOximeter_resume(PulseOximeter *ox, const struct device *dev);

    // /* This defines a class called `PulseOximeter` which represents a pulse oximeter device. It has public methods for initializing the device, updating readings, getting heart rate and SpO2 values, getting red LED current bias, setting a callback function for beat detection, shutting down and resuming the device. The class also has private methods for checking samples and current bias. It contains member variables to store various timestamps, a beat detector object, DC removers for infrared and red LEDs, a low-pass filter, red LED current index, and a MAX30100 sensor object.*/
    // class PulseOximeter 
    // {
    // 	public:
    // 		PulseOximeter();

    // 		bool begin(const struct device *dev, PulseOximeterDebuggingMode debuggingMode_=PULSEOXIMETER_DEBUGGINGMODE_NONE);
    // 		void update(const struct device *dev);
    // 		uint8_t getHeartRate();
    // 		uint8_t getSpO2();
    // 		uint8_t getRedLedCurrentBias();
    // 		void setOnBeatDetectedCallback(void (*cb)());
    // 		// void setIRLedCurrent(LEDCurrent irLedCurrent);
    // 		void shutdown(const struct device *dev);
    // 		void resume(const struct device *dev);

    // 	private:
    // 		void checkSample();
    // 		void checkCurrentBias(const struct device *dev);

    // 		PulseOximeterState state;
    // 		PulseOximeterDebuggingMode debuggingMode;
    // 		uint32_t tsFirstBeatDetected;
    // 		uint32_t tsLastBeatDetected;
    // 		uint32_t tsLastBiasCheck;
    // 		uint32_t tsLastCurrentAdjustment;
    // 		BeatDetector beatDetector;
    // 		DCRemover irDCRemover;
    // 		DCRemover redDCRemover;
    // 		FilterBuLp1 lpf;
    // 		uint8_t redLedCurrentIndex;
    // 		// LEDCurrent irLedCurrent;
    // 		MAX30100 hrm;

    // 		void (*onBeatDetected)();
    // };

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