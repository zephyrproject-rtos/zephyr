#include "spo2.h"

#define REPORTING_PERIOD_MS     1000
#define DELAY_FREQ_BASE 40000000
// using namespace std; 

uint64_t total_cycles  = 0; // Used to store the value of mcycle.
int tsLastReport = 0;		// Used to store previous millis value.

CBuffer redBuffer;	// Object of structure circularbuffer to store IR values.
CBuffer irBuffer;	// Object of structure circularbuffer to store RED values.

PulseOximeter pox;			// Object of structure PulseOximeter.

void FilterBuLp1_init(FilterBuLp1 *filter) {
    filter->v[0] = 0.0;
    filter->v[1] = 0.0;
}

float FilterBuLp1_step(FilterBuLp1 *filter, float x) {
    filter->v[0] = filter->v[1];
    filter->v[1] = (2.452372752527856026e-1 * x) + (0.50952544949442879485 * filter->v[0]);
    return filter->v[0] + filter->v[1];
}

void DCRemover_init(DCRemover *remover) {
    remover->alpha = 0;
    remover->dcw = 0;
}

void DCRemover_init_with_alpha(DCRemover *remover, float alpha) {
    remover->alpha = alpha;
    remover->dcw = 0;
}

float DCRemover_step(DCRemover *remover, float x) {
    float olddcw = remover->dcw;
    remover->dcw = x + remover->alpha * remover->dcw;
    return remover->dcw - olddcw;
}

float DCRemover_getDCW(const DCRemover *remover) {
    return remover->dcw;
}

/** 
 * @fn spo2_write
 * 
 * @brief Writes data to a register of the MAX30100 sensor.
 * 
 * @details This function writes data to a register of the MAX30100 sensor via I2C communication.
 * 
 * @param i2c_number The I2C number used for communication
 * @param slave_address The address of the module MAX30100
 * @param reg_addr The address of the register to read.
 * @param data The data to write to the register.
 */
void spo2_write(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr, uint8_t data)
{
	uint8_t msg_arr[2];
	struct i2c_msg msg_obj;
	msg_arr[0] = reg_addr;
	msg_arr[1] = data;
	msg_obj.buf = msg_arr;
	msg_obj.len = 2;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
}

/** 
 * @fn spo2_read
 * 
 * @brief Reads a register from the MAX30100 sensor.
 * 
 * @details This function reads a register from the MAX30100 sensor via I2C communication.
 * 
 * @param i2c_number The I2C number used for communication
 * @param slave_address The address of the module MAX30100
 * @param reg_addr The address of the register to read.
 * 
 * @return uint8_t The value read from the specified register.
 */
uint8_t spo2_read(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr)
{
	uint8_t msg_arr[1];
	struct i2c_msg msg_obj;

	msg_arr[0] = reg_addr;
	msg_obj.buf = msg_arr;
	msg_obj.len = 1;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);

	msg_obj.flags = I2C_MSG_READ;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	return msg_arr[0];
}

/** 
 * @fn spo2_burstread
 * 
 * @brief Reads multiple registers from the MAX30100 sensor in burst mode.
 * 
 * @details This function reads multiple registers from the MAX30100 sensor in burst mode via I2C communication.
 * 
 * @param i2c_number The I2C number used for communication
 * @param slave_address The address of the module MAX30100
 * @param reg_addr The address of the register to read.
 * @param buffer Pointer to a buffer to store the read data.
 * @param toRead The number of registers to read.
 */
void spo2_burstread(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr, uint8_t *buffer, uint8_t toRead)
{
	// uint64_t check;
	// while(1)
	// check = 0;
	for (uint8_t i=0; i< toRead*4; i++)
	{
		buffer[i] = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA);
	}
}

/** 
 * @brief Constructor for the BeatDetector structure.
 * 
 * Sets the initial state to BEATDETECTOR_STATE_INIT,
 * the threshold to the minimum threshold value, the beat period to 0,
 * the last maximum value to 0, and the timestamp of the last beat to 0.
 */
BeatDetector* BeatDetector_create() {
    BeatDetector* bd = (BeatDetector*)malloc(sizeof(BeatDetector));
    bd->state = BEATDETECTOR_STATE_INIT;
    bd->threshold = 0.0;
    bd->beatPeriod = 0.0;
    bd->lastMaxValue = 0.0;
    bd->tsLastBeat = 0;
    return bd;
}

/** 
 * @brief Initializes the circular buffer by setting the head and tail pointers
 * to the beginning of the buffer, and the count of elements in the buffer
 * to 0.
 */
void CBuffer_init(CBuffer* cb) {
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
    cb->count = 0;
}

/** @fn push
 * 
 * @brief The function `push` in the circular buffer structure is used to add a value to the buffer while handling
 * circular buffer logic.
 * 
 * @param unsigned value The value to be added in the circular buffer
 * 
 * @return bool Returns 0 when Circular buffer is full
 * 				Returns 1 when Circular buffer is not full 
 */
bool CBuffer_push(CBuffer* cb, uint16_t value) {
    // Increment the tail pointer and wrap around if necessary
    if (++cb->tail == cb->buffer + S) {
        cb->tail = cb->buffer;
    }
    // Store the value at the tail position
    *cb->tail = value;
    // If the buffer is full, adjust the head pointer
    if (cb->count == S) {
        // Increment head and wrap around if necessary
        if (++cb->head == cb->buffer + S) {
            cb->head = cb->buffer;
        }
        // Return false as the buffer is full
        return false;
    } else {
        // If the buffer is not full, increment count and set head if it's the first element
        if (cb->count++ == 0) {
            cb->head = cb->tail;
        }
        // Return true as the push operation was successful
        return true;
    }
}

/** 
 * @fn pop
 * 
 * @brief The function `pop` in the circular buffer structure is used to retrieve and remove the oldest value from the buffer.
 * 
 * @return uint16_t The value retrieved from the circular buffer.
 * 
 * @remarks If the buffer is empty, the function causes a crash.
 */

uint16_t CBuffer_pop(CBuffer* cb) {
    // Pointer to function for crashing if the buffer is empty
    void(*crash)(void) = 0;
    // Check if the buffer is empty, and crash if so
    if (cb->count <= 0) {
        crash();
    }
    // Retrieve the value from the tail position
    uint16_t result = *cb->tail--;
    // Wrap around if tail goes below the buffer
    if (cb->tail < cb->buffer) {
        cb->tail = cb->buffer + S - 1;
    }
    // Decrease the count as an element is removed
    cb->count--;
    // Return the retrieved value
    return result;
}



/** 
 * @fn isEmpty
 * 
 * @brief Checks if the circular buffer is empty.
 * 
 * @return bool True if the circular buffer is empty, false otherwise.
 */
bool CBuffer_isEmpty(CBuffer* cb) {
    return cb->count == 0;
}


/**
 * The above function is a constructor for the MAX30100 structure.
 */
void MAX30100_init(MAX30100 *sensor) {
}

/**
 * The begin function initializes the MAX30100 sensor by setting various parameters and enabling
 * high-resolution mode.
 * 
 * @param dev The `dev` parameter in the `begin` function is a pointer to a structure of type `device`.
 * This structure likely contains information or configurations related to the device being used with
 * the MAX30100 sensor. The function uses this parameter to interact with the device through I2C
 * communication and configure various
 * 
 * @return True if the initialization is successful, false otherwise.
 */
/** 
 * @fn begin
 * 
 * @brief Initializes the MAX30100 sensor.
 * 
 * @details This function initializes the MAX30100 sensor by performing the following steps:
 * - Initializes the I2C communication.
 * - Sets the I2C clock speed.
 * - Verifies the part ID of the sensor.
 * - Sets the default mode of operation.
 * - Sets the default pulse width for LEDs.
 * - Sets the default sampling rate.
 * - Sets the default current for IR and red LEDs.
 * - Enables high-resolution mode.
 * 
 * @return bool True if the initialization is successful, false otherwise.
 */
bool MAX30100_begin(MAX30100 *sensor, const struct device *dev) {
    // Verify part ID
    if (MAX30100_getPartId(sensor, dev) != EXPECTED_PART_ID) {
        return false;
    }

    // Set Mode
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION, DEFAULT_MODE);
    uint8_t x = spo2_read(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION);

    // Set LedsPulseWidth
    uint8_t previous = spo2_read(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION);
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xfc) | DEFAULT_PULSE_WIDTH);

    // Set SamplingRate
    previous = spo2_read(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION);
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xe3) | (DEFAULT_SAMPLING_RATE << 2));

    // Set LedsCurrent
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_LED_CONFIGURATION, DEFAULT_RED_LED_CURRENT << 4 | DEFAULT_RED_LED_CURRENT);

    // Set HighresMode
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, previous | MAX30100_SPC_SPO2_HI_RES_EN);

    return true;
}


/** 
 * @fn update
 * 
 * @brief Reads FIFO data from the MAX30100 sensor.
 * 
 * @details This function periodically updates the MAX30100 sensor by reading FIFO data.
 */
void MAX30100_update(MAX30100 *sensor, const struct device *dev) {
    // Delay for 10ms before sampling every value. Used as sampling rate is way too high. (Optional)
    // delayms(10);
    MAX30100_readFifoData(sensor, dev);
}

/** 
 * @fn getRawValues
 * 
 * @brief Retrieves raw IR and red values from the MAX30100 sensor.
 * 
 * @details This function retrieves the raw IR and red values from the MAX30100 sensor.
 * 
 * @param ir Pointer to store the retrieved raw IR value.
 * @param red Pointer to store the retrieved raw red value.
 * 
 * @return bool True if raw values are successfully retrieved, false if the buffer is empty.
 */
bool MAX30100_getRawValues(MAX30100 *sensor, uint16_t *ir, uint16_t *red) {
    // Check if buffer is not empty
    if (!(CBuffer_isEmpty(&irBuffer) || CBuffer_isEmpty(&redBuffer))) {
        // Dequeue the oldest value from the buffer
        uint16_t pop_ir = CBuffer_pop(&irBuffer);
        uint16_t pop_red = CBuffer_pop(&redBuffer);
        // printf("\nm    %u",millis(get_mcycle_stop()));
        printf("\nIR    %u", pop_ir);
        printf("\nRed    %u", pop_red);
        *ir = pop_ir;
        *red = pop_red;
        return true;
    } else {
        return false;
    }
}
/** 
 * @fn resetFifo
 * 
 * @brief Resets the FIFO registers of the MAX30100 sensor.
 * 
 * @details This function resets the FIFO write pointer, FIFO read pointer, and FIFO overflow counter
 * registers of the MAX30100 sensor to zero, effectively clearing the FIFO buffer.
 */
void MAX30100_resetFifo(MAX30100 *sensor, const struct device *dev) {
    // Write 0 to FIFO write pointer register
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_WRITE_POINTER, 0);
    // Write 0 to FIFO read pointer register
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_READ_POINTER, 0);
    // Write 0 to FIFO overflow counter register
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_OVERFLOW_COUNTER, 0);
}
/** 
 * @fn readFifoData
 * 
 * @brief Reads data from the FIFO buffer of the MAX30100 sensor.
 * 
 * @details This function reads data from the FIFO buffer of the MAX30100 sensor.
 * It calculates the number of data points available in the FIFO buffer, then reads 
 * the data from the sensor in burst mode. The read data is then parsed and stored 
 * in the buffer for further processing.
 */
void MAX30100_readFifoData(MAX30100 *sensor, const struct device *dev) {
    // Buffer to store FIFO data
    uint8_t buffer[MAX30100_FIFO_DEPTH * 4];
    // Number of data points to read from FIFO
    uint8_t toRead;

    // Calculate the number of data points available in the FIFO buffer
    uint8_t x = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_WRITE_POINTER);
    uint8_t y = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_READ_POINTER);

    toRead = (x - y) & (MAX30100_FIFO_DEPTH - 1);
    // printf("\nx %u",x);
    // printf("\ny %u",y);
    if (x == y) {
        toRead = x;
    }
    // printf("\ntr %u",toRead);

    // Read data from the FIFO buffer in burst mode
    spo2_burstread(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA, buffer, toRead);

    // Parse and store the read data in the buffer
    for (int j = 0; j < toRead; ++j) {
        uint16_t push_ir = ((buffer[j * 4] << 8) | buffer[j * 4 + 1]);
        uint16_t push_red = ((buffer[j * 4 + 2] << 8) | buffer[j * 4 + 3]);
        CBuffer_push(&irBuffer, push_ir);
        CBuffer_push(&redBuffer, push_red);
    }
}

/** 
 * @fn startTemperatureSampling
 * 
 * @brief Starts temperature sampling on the MAX30100 sensor.
 * 
 * @details This function enables temperature sampling on the MAX30100 sensor by
 * setting the appropriate configuration bit in the mode configuration register.
 */
void MAX30100_startTemperatureSampling(MAX30100 *sensor, const struct device *dev) {
    // Read the current mode configuration
    uint8_t modeConfig = spo2_read(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION);
    // Enable temperature sampling by setting the appropriate bit
    modeConfig |= MAX30100_MC_TEMP_EN;
    // Write the updated mode configuration back to the sensor
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION, modeConfig);
}

/** 
 * @fn isTemperatureReady
 * 
 * @brief Checks if temperature sampling is ready on the MAX30100 sensor.
 * 
 * @details This function checks if temperature sampling is ready on the MAX30100 sensor
 * by reading the mode configuration register and checking the temperature 
 * sampling enable bit.
 * 
 * @return bool True if temperature sampling is not in progress, false otherwise.
 */
bool MAX30100_isTemperatureReady(MAX30100 *sensor, const struct device *dev) {
    // Read the mode configuration register and check the temperature sampling enable bit
    return !(spo2_read(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION) & MAX30100_MC_TEMP_EN);
}

/** 
 * @fn retrieveTemperature
 * 
 * @brief Retrieves the temperature reading from the MAX30100 sensor.
 * 
 * @details @details This function retrieves the temperature reading from the MAX30100 sensor
 * by reading the temperature data registers and combining the integer and 
 * fractional parts to calculate the temperature in degrees Celsius.
 * 
 * @return float The temperature reading in degrees Celsius.
 */
float MAX30100_retrieveTemperature(MAX30100 *sensor, const struct device *dev) {
    // Read the integer part of the temperature
    int8_t tempInteger = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_INT);
    // Read the fractional part of the temperature
    float tempFrac = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_FRAC);

    // Calculate the temperature in degrees Celsius and return it
    return tempFrac * 0.0625 + tempInteger;
}

/** 
 * @fn getPartId
 * 
 * @brief Retrieves the part ID of the MAX30100 sensor.
 * 
 * @details This function retrieves the part ID of the MAX30100 sensor by reading 
 * the corresponding register via I2C communication.
 * 
 * @return uint8_t The part ID of the MAX30100 sensor.
 */
uint8_t MAX30100_getPartId(MAX30100 *sensor,const struct device *dev) {
    // Read the part ID register and return the value
    return spo2_read(dev, I2C0, 0X57, 0xff);
}

/** 
 * @fn addSample
 * 
 * @brief Adds a sample to the BeatDetector for analysis.
 * 
 * @details This function adds a sample to the BeatDetector for analysis. It passes the
 * sample to the checkForBeat function to determine if a beat is detected.
 * 
 * @param sample The sample value to be added to the BeatDetector for analysis.
 * @return bool True if a beat is detected, otherwise false.
 */
bool BeatDetector_addSample(BeatDetector *detector, float sample) 
{
	return checkForBeat(detector,sample);
}

/** 
 * @fn getRate
 * 
 * @brief Retrieves the heart rate calculated by the BeatDetector.
 * 
 * @details This function calculates and retrieves the heart rate (beats per minute) 
 * based on the beat period stored in the BeatDetector. If no beat period 
 * is available (i.e., beatPeriod is 0), it returns 0.
 * 
 * @return uint8_t The heart rate calculated by the BeatDetector in beats per minute.
 */
uint8_t BeatDetector_getRate(BeatDetector *detector) {
    if (detector->beatPeriod != 0) {
        return (uint8_t)(1 / detector->beatPeriod * 1000 * 60);
    } else {
        return 0;
    }
}

/** 
 * @fn getCurrentThreshold
 * 
 * @brief Retrieves the current threshold value used by the BeatDetector.
 * 
 * @details This function retrieves the current threshold value used by the BeatDetector 
 * for beat detection.
 * 
 * @return float The current threshold value used by the BeatDetector.
 */
float BeatDetector_getCurrentThreshold(BeatDetector *detector) {
    return detector->threshold;
}

/** 
 * @fn decreaseThreshold
 * 
 * @brief Decreases the threshold value used by the BeatDetector.
 * 
 * @details This function decreases the threshold value used by the BeatDetector for
 * beat detection. It adjusts the threshold based on the last maximum value
 * detected and the beat period. If no beat period or maximum value is available,
 * it applies an asymptotic decay to gradually decrease the threshold value.
 */
static void decreaseThreshold(BeatDetector *detector) {
	// Adjust threshold based on last maximum value and beat period
    if (detector->lastMaxValue > 0 && detector->beatPeriod > 0) {
        detector->threshold -= detector->lastMaxValue * (1 - BEATDETECTOR_THRESHOLD_FALLOFF_TARGET) / (detector->beatPeriod / BEATDETECTOR_SAMPLES_PERIOD);
    } 
    // Apply asymptotic decay if no beat period or maximum value is available
    else {
        detector->threshold *= BEATDETECTOR_THRESHOLD_DECAY_FACTOR;
    }

	// Ensure threshold does not fall below the minimum threshold
    if (detector->threshold < BEATDETECTOR_MIN_THRESHOLD) {
        detector->threshold = BEATDETECTOR_MIN_THRESHOLD;
    }
}

/** 
 * @fn checkForBeat
 * 
 * @brief Analyzes a sample to determine if a beat is detected.
 * 
 * @details This function analyzes a sample to determine if a beat is detected based on
 * the current state and threshold of the BeatDetector. It implements a state
 * machine to track the beat detection process, adjusting the threshold and
 * updating internal states accordingly.
 * 
 * @param sample The sample value to be analyzed for beat detection.
 * @return bool True if a beat is detected, otherwise false.
 */
static bool checkForBeat(BeatDetector *detector, float sample) {
    bool beatDetected = false;
    uint32_t total_cycles = get_mcycle_stop();

    switch (detector->state) {
        case BEATDETECTOR_STATE_INIT:
            if (millis(total_cycles) > BEATDETECTOR_INIT_HOLDOFF) {
                detector->state = BEATDETECTOR_STATE_WAITING;
            }
            break;
        case BEATDETECTOR_STATE_WAITING:
            if (sample > detector->threshold) {
                detector->threshold = min(sample, BEATDETECTOR_MAX_THRESHOLD);
                detector->state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
            }
            if (millis(total_cycles) - detector->tsLastBeat > BEATDETECTOR_INVALID_READOUT_DELAY) {
                detector->beatPeriod = 0;
                detector->lastMaxValue = 0;
            }
            decreaseThreshold(detector);
            break;
        case BEATDETECTOR_STATE_FOLLOWING_SLOPE:
            if (sample < detector->threshold) {
                detector->state = BEATDETECTOR_STATE_MAYBE_DETECTED;
            } else {
                detector->threshold = min(sample, BEATDETECTOR_MAX_THRESHOLD);
            }
            break;
        case BEATDETECTOR_STATE_MAYBE_DETECTED:
            if (sample + BEATDETECTOR_STEP_RESILIENCY < detector->threshold) {
                beatDetected = true;
                detector->lastMaxValue = sample;
                detector->state = BEATDETECTOR_STATE_MASKING;
                total_cycles = get_mcycle_stop();
                float delta = millis(total_cycles) - detector->tsLastBeat;
                if (delta) {
                    detector->beatPeriod = BEATDETECTOR_BPFILTER_ALPHA * delta + ((1 - BEATDETECTOR_BPFILTER_ALPHA) * detector->beatPeriod);
                }
                total_cycles = get_mcycle_stop();
                detector->tsLastBeat = millis(total_cycles);
            } else {
                detector->state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
            }
            break;
        case BEATDETECTOR_STATE_MASKING:
            if (millis(total_cycles) - detector->tsLastBeat > BEATDETECTOR_MASKING_HOLDOFF) {
                detector->state = BEATDETECTOR_STATE_WAITING;
            }
            decreaseThreshold(detector);
            break;
    }

    return beatDetected;
}
// bool BeatDetector::checkForBeat(float sample)
// {
// 	bool beatDetected = false;
// 	total_cycles = get_mcycle_stop();
// 	switch (state) 
// 	{
// 		// Initialization state
// 		case BEATDETECTOR_STATE_INIT:
// 			// Transition to waiting state after initialization hold-off time
// 			if (millis(total_cycles) > BEATDETECTOR_INIT_HOLDOFF) 
// 			{
// 				state = BEATDETECTOR_STATE_WAITING;
// 			}
// 			break;
// 		// Waiting state
// 		case BEATDETECTOR_STATE_WAITING:
// 			// Adjust threshold if sample exceeds it
// 			if (sample > threshold) 
// 			{
// 				threshold = min(sample, BEATDETECTOR_MAX_THRESHOLD);
// 				state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
// 			}

// 			// Reset if tracking lost
// 			if (millis(total_cycles) - tsLastBeat > BEATDETECTOR_INVALID_READOUT_DELAY) 
// 			{
// 				beatPeriod = 0;
// 				lastMaxValue = 0;
// 			}

// 			decreaseThreshold();
// 			break;
// 		// Following slope state
// 		case BEATDETECTOR_STATE_FOLLOWING_SLOPE:
// 			if (sample < threshold) 
// 			{
// 				state = BEATDETECTOR_STATE_MAYBE_DETECTED;
// 			} 
// 			else 
// 			{
// 				threshold = min(sample, BEATDETECTOR_MAX_THRESHOLD);
// 			}
// 			break;
// 		// Maybe detected state
// 		case BEATDETECTOR_STATE_MAYBE_DETECTED:
// 			if (sample + BEATDETECTOR_STEP_RESILIENCY < threshold) 
// 			{
// 				// Found a beat
// 				beatDetected = true;
// 				lastMaxValue = sample;
// 				state = BEATDETECTOR_STATE_MASKING;
// 				total_cycles = get_mcycle_stop();   
// 				float delta = millis(total_cycles) - tsLastBeat;
// 				// float delta = 700;
// 				// printf("\ndelta %f",delta);
// 				// printf("\nbeatperiod before %f",beatPeriod);
// 				if (delta) 
// 				{
// 					beatPeriod = BEATDETECTOR_BPFILTER_ALPHA * delta +((1 - BEATDETECTOR_BPFILTER_ALPHA) * beatPeriod);
// 				}
// 				// printf("\nbeatperiod %f",beatPeriod);

// 				total_cycles = get_mcycle_stop();
// 				tsLastBeat = millis(total_cycles);
// 			} 
// 			else 
// 			{
// 				state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
// 			}
// 			break;
// 		// Masking state
// 		case BEATDETECTOR_STATE_MASKING:
// 			if (millis(total_cycles) - tsLastBeat > BEATDETECTOR_MASKING_HOLDOFF) 
// 			{
// 				state = BEATDETECTOR_STATE_WAITING;
// 			}
// 			decreaseThreshold();
// 			break;
// 	}

// 	return beatDetected;
// }

/** 
 * @brief Constructor for the PulseOximeter structure.
 * 
 * @details Initializes the PulseOximeter object with default values for its internal
 * state variables. 
 */
void PulseOximeter_init(PulseOximeter *ox) {
    ox->state = PULSEOXIMETER_STATE_INIT;
    ox->tsFirstBeatDetected = 0;
    ox->tsLastBeatDetected = 0;
    ox->tsLastBiasCheck = 0;
    ox->tsLastCurrentAdjustment = 0;
    ox->redLedCurrentIndex = RED_LED_CURRENT_START;
    ox->onBeatDetected = NULL;
    // Initialize other components if necessary
}

/** 
 * @fn begin
 * 
 * @brief Initializes the PulseOximeter object and the underlying hardware.
 * 
 * @details This function initializes the PulseOximeter object and the underlying
 * hardware components.
 * 
 * @param debuggingMode_ The debugging mode to be set for the PulseOximeter object.
 * 
 * @return bool True if initialization is successful, otherwise false.
 */
bool PulseOximeter_begin(PulseOximeter *ox, const struct device *dev, PulseOximeterDebuggingMode debuggingMode_) {
    ox->debuggingMode = debuggingMode_;
    bool ready = MAX30100_begin(&ox->hrm, dev);
    if (!ready) {
        if (ox->debuggingMode != PULSEOXIMETER_DEBUGGINGMODE_NONE) {
            printf("\nFailed to initialize the HRM sensor");
        }
        return false;
    }

    spo2_write(dev, I2C0, 0x57, MAX30100_REG_MODE_CONFIGURATION, MAX30100_MODE_SPO2_HR);
    // Set LED currents
    ox->irDCRemover = DC_REMOVER_ALPHA;
    ox->redDCRemover = DC_REMOVER_ALPHA;
    ox->state = PULSEOXIMETER_STATE_IDLE;
    return true;
}

/** 
 * @fn update
 * 
 * @brief Updates the PulseOximeter object.
 * 
 * @details This function updates the PulseOximeter object by calling the `update` function
 * of the heart rate monitor (HRM) sensor to retrieve new samples. It then performs
 * sample analysis and current bias checking.
 */
void PulseOximeter_update(PulseOximeter *ox, const struct device *dev) {
    MAX30100_update(&ox->hrm, dev);
    PulseOximeter_checkSample(ox);
    PulseOximeter_checkCurrentBias(ox, dev);
}

// void PulseOximeter::update(const struct device *dev)
// {
// 	// Update the heart rate monitor sensor
// 	hrm.update(dev);
// 	// Perform sample analysis
// 	checkSample();
// 	// Perform current bias checking
// 	checkCurrentBias(dev);
// }

/** 
 * @fn getHeartRate
 * 
 * @brief Retrieves the heart rate calculated by the PulseOximeter.
 * 
 * @details This function retrieves the heart rate calculated by the PulseOximeter
 * using the BeatDetector object associated with it.
 * 
 * @return uint8_t The heart rate calculated by the PulseOximeter in beats per minute.
 */
uint8_t PulseOximeter_getHeartRate(const PulseOximeter *ox) {
    return BeatDetector_getRate(&ox->beatDetector);
}

/**
 * @details The function `getSpO2` returns the SpO2 value calculated by `SpO2Calculator_getSpO2`.
 * 
 * @return The `getSpO2` function is returning an 8-bit unsigned integer value representing the SpO2
 * (blood oxygen saturation level) obtained from the `SpO2Calculator_getSpO2` function.
 */
uint8_t PulseOximeter_getSpO2(const PulseOximeter *ox) {
    return SpO2Calculator_getSpO2();
}
/** 
 * @fn getSpO2
 * 
 * @brief Retrieves the SpO2 (blood oxygen saturation) value calculated by the PulseOximeter.
 * 
 * @details This function retrieves the SpO2 value calculated by the
 * PulseOximeter using the SpO2Calculator associated with it.
 * 
 * @return uint8_t The SpO2 value calculated by the PulseOximeter.
 */
uint8_t PulseOximeter_getRedLedCurrentBias(const PulseOximeter *ox) {
    return ox->redLedCurrentIndex;
}

/** 
 * @fn setOnBeatDetectedCallback
 * 
 * @brief Sets a callback function to be called when a beat is detected by the PulseOximeter.
 * 
 * @details This function sets a callback function to be called when a beat is detected by the
 * PulseOximeter. The provided callback function `cb` is invoked whenever a beat is
 * detected during the operation of the PulseOximeter.
 * 
 * @param cb A pointer to the callback function to be called when a beat is detected.
 */
void PulseOximeter_setOnBeatDetectedCallback(PulseOximeter *ox, void (*cb)()) {
    ox->onBeatDetected = cb;
}

// void PulseOximeter::setIRLedCurrent(LEDCurrent irLedNewCurrent)
// {
//     irLedCurrent = irLedNewCurrent;
//     // hrm.setLedsCurrent(irLedCurrent, (LEDCurrent)redLedCurrentIndex);
//     spo2_write(const struct device *dev, I2C0, 0x57, irLedCurrent, (LEDCurrent)redLedCurrentIndex);
// }

/** 
 * @fn shutdown
 * 
 * @brief Shuts down the PulseOximeter by shutting down the heart rate monitor sensor.
 * 
 * This typically involves putting the sensor into a low-power or standby mode to conserve energy.
 */
void PulseOximeter_shutdown(PulseOximeter *ox, const struct device *dev) {
    MAX30100_shutdown(&ox->hrm, dev);
}
/** 
 * @fn resume
 * 
 * @brief Resumes operation of the PulseOximeter by resuming the heart rate monitor sensor.
 * 
 * @details This function is typically used after a previous shutdown operation to restore normal operation of the sensor.
 */
void PulseOximeter_resume(PulseOximeter *ox, const struct device *dev) {
    MAX30100_resume(&ox->hrm, dev);
}

/** 
 * @fn checkSample
 * 
 * @brief Processes raw sensor samples to detect beats and update SpO2 values.
 */

void PulseOximeter_checkSample(PulseOximeter *ox) {
    uint16_t rawIRValue, rawRedValue;
    while (MAX30100_getRawValues(&ox->hrm, &rawIRValue, &rawRedValue)) {
        float irACValue = DCRemover_step(&ox->irDCRemover, rawIRValue);
        float redACValue = DCRemover_step(&ox->redDCRemover, rawRedValue);
        float filteredPulseValue = FilterBuLp1_step(&ox->lpf, -irACValue);
        bool beatDetected = BeatDetector_addSample(&ox->beatDetector, filteredPulseValue);
        if (BeatDetector_getRate(&ox->beatDetector) > 0) {
            ox->state = PULSEOXIMETER_STATE_DETECTING;
            SpO2Calculator_update(irACValue, redACValue, beatDetected);
        } else if (ox->state == PULSEOXIMETER_STATE_DETECTING) {
            ox->state = PULSEOXIMETER_STATE_IDLE;
            SpO2Calculator_reset();
        }
    }
}

/** 
 * @fn checkCurrentBias
 * 
 * @brief Adjusts the red LED current to maintain comparable DC baselines between red and IR LEDs.
 */
void PulseOximeter_checkCurrentBias(PulseOximeter *ox, const struct device *dev) {
    uint32_t total_cycles = get_mcycle_stop();
    if (millis(total_cycles) - ox->tsLastBiasCheck > CURRENT_ADJUSTMENT_PERIOD_MS) {
        bool changed = false;
        if (DCRemover_getDCW(&ox->irDCRemover) - DCRemover_getDCW(&ox->redDCRemover) > 70000 && ox->redLedCurrentIndex < MAX30100_LED_CURR_50MA) {
            ++ox->redLedCurrentIndex;
            changed = true;
        } else if (DCRemover_getDCW(&ox->redDCRemover) - DCRemover_getDCW(&ox->irDCRemover) > 70000 && ox->redLedCurrentIndex > 0) {
            --ox->redLedCurrentIndex;
            changed = true;
        }
        if (changed) {
            spo2_write(dev, I2C0, 0x57, DEFAULT_IR_LED_CURRENT, (LEDCurrent)ox->redLedCurrentIndex);
            if (ox->debuggingMode != PULSEOXIMETER_DEBUGGINGMODE_NONE) {
                printf("\nI: %u", ox->redLedCurrentIndex);
            }
        }
        total_cycles = get_mcycle_stop();
        ox->tsLastBiasCheck = millis(total_cycles);
    }
}

/** 
 * @fn SpO2Calculator_update
 * 
 * @brief Updates SpO2 value based on IR and red AC values and beat detection.
 * 
 * @param irACValue The AC (alternating current) value of the infrared LED signal.
 * @param redACValue The AC (alternating current) value of the red LED signal.
 * @param beatDetected A boolean indicating whether a beat was detected.
 */

void SpO2Calculator_update(float irACValue, float redACValue, bool beatDetected)
{
	irACValueSqSum += irACValue * irACValue;
	redACValueSqSum += redACValue * redACValue;
	++samplesRecorded;

	if (beatDetected) 
	{
		// printf("\nInside spo2calculator");
		++beatsDetectedNum;
		if (beatsDetectedNum == CALCULATE_EVERY_N_BEATS) 
		{
			float acSqRatio = 100.0 * log(redACValueSqSum/samplesRecorded) / log(irACValueSqSum/samplesRecorded);
			uint8_t index = 0;

			// printf("\nacSqratio %f",acSqRatio);
			if (acSqRatio > 66) 
			{
				index = (uint8_t)acSqRatio - 66;
			} 
			else if (acSqRatio > 50) 
			{
				index = (uint8_t)acSqRatio - 50;
			}
			SpO2Calculator_reset();
			// printf("\nindex %u",index);

			spO2 = spO2LUT[index];
		}
	}
}

/** 
 * @fn SpO2Calculator_reset
 * 
 * @brief Resets internal variables of the SpO2Calculator.
 * 
 * @details This function resets the internal variables of the SpO2Calculator,
 * including the number of recorded samples, sum of squared AC values
 * for both IR and red LEDs, the number of detected beats, and the
 * calculated SpO2 value.
 */
void SpO2Calculator_reset()
{
	samplesRecorded = 0;
	redACValueSqSum = 0;
	irACValueSqSum = 0;
	beatsDetectedNum = 0;
	spO2 = 0;
}

/** 
 * @fn SpO2Calculator_getSpO2
 * 
 * @brief Retrieves the calculated SpO2 value.
 * 
 * @return uint8_t The calculated SpO2 value.
 */
uint8_t SpO2Calculator_getSpO2()
{
	return spO2;
}

// Callback (registered below) fired when a pulse is detected
/** 
 * @fn onBeatDetected
 * 
 * @brief Prints a message indicating a beat detection event.
 */
void onBeatDetected()
{
	printf("\nBeat!");
}

void delayms(long delay)
{
  for(int i=0;i<(3334*delay*(DELAY_FREQ_BASE/40000000));i++){
    // asm volatile("NOP");
  }
}

// MAX30100_Minimal code
int main() {

    const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	uint8_t cycle_at_start = get_mcycle_start();
    // CircularBuffer redBuffer, irBuffer;
    PulseOximeter pox;
    PulseOximeter_init(&pox);

    // Initialize both buffers
    CBuffer_init(&redBuffer);
    CBuffer_init(&irBuffer);

    spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA, DEFAULT_MODE);

    // Initialize the PulseOximeter instance
    // Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip
    if (!PulseOximeter_begin(&pox, dev,PULSEOXIMETER_DEBUGGINGMODE_NONE)) {
        printf("\nFAILED");
        for(;;);
    } else {
        printf("\nSUCCESS");
    }
    // printf("\nTime	IR	irAC	LPF");

    uint32_t total_cycles, tsLastReport = 0;

    while(1) {
        // Make sure to call update as fast as possible
        PulseOximeter_update(&pox, dev);

        // Asynchronously dump heart rate and oxidation levels to the serial
        // For both, a value of 0 means "invalid"
        total_cycles = get_mcycle_stop();
        if (millis(total_cycles) - tsLastReport > REPORTING_PERIOD_MS) {
            uint8_t heartrate = PulseOximeter_getHeartRate(&pox);
            uint8_t spo2 = PulseOximeter_getSpO2(&pox);
            printf("Heart Rate: %u, SpO2: %u\n", heartrate, spo2);
            tsLastReport = millis(total_cycles);
        }
    }
    return 0;
}


/*TODO
setIRLedCurrent
*/