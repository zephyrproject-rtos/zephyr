// #include "spo2.h"

// #define REPORTING_PERIOD_MS     1000
// #define DELAY_FREQ_BASE 40000000

// /** 
//  * @fn spo2_write
//  * 
//  * @brief Writes data to a register of the MAX30100 sensor.
//  * 
//  * @details This function writes data to a register of the MAX30100 sensor via I2C communication.
//  * 
//  * @param i2c_number The I2C number used for communication
//  * @param slave_address The address of the module MAX30100
//  * @param reg_addr The address of the register to read.
//  * @param data The data to write to the register.
//  */
// void spo2_write(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr, uint8_t data)
// {
// 	// printf("\nBegin");
// 	uint8_t msg_arr[2];
// 	struct i2c_msg msg_obj;
// 	msg_arr[0] = reg_addr;
// 	msg_arr[1] = data;
// 	msg_obj.buf = msg_arr;
// 	msg_obj.len = 2;
// 	msg_obj.flags = I2C_MSG_WRITE;
// 	i2c_transfer(dev,&msg_obj,1,slave_address);
// }

// /** 
//  * @fn spo2_read
//  * 
//  * @brief Reads a register from the MAX30100 sensor.
//  * 
//  * @details This function reads a register from the MAX30100 sensor via I2C communication.
//  * 
//  * @param i2c_number The I2C number used for communication
//  * @param slave_address The address of the module MAX30100
//  * @param reg_addr The address of the register to read.
//  * 
//  * @return uint8_t The value read from the specified register.
//  */
// uint8_t spo2_read(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr)
// {
// 	uint8_t msg_arr[1];
// 	struct i2c_msg msg_obj;

// 	msg_arr[0] = reg_addr;
// 	msg_obj.buf = msg_arr;
// 	msg_obj.len = 1;
// 	msg_obj.flags = I2C_MSG_WRITE;
// 	i2c_transfer(dev,&msg_obj,1,slave_address);

// 	msg_obj.flags = I2C_MSG_READ;
// 	i2c_transfer(dev,&msg_obj,1,slave_address);
// 	return msg_arr[0];
// }

// /** 
//  * @fn spo2_burstread
//  * 
//  * @brief Reads multiple registers from the MAX30100 sensor in burst mode.
//  * 
//  * @details This function reads multiple registers from the MAX30100 sensor in burst mode via I2C communication.
//  * 
//  * @param i2c_number The I2C number used for communication
//  * @param slave_address The address of the module MAX30100
//  * @param reg_addr The address of the register to read.
//  * @param buffer Pointer to a buffer to store the read data.
//  * @param toRead The number of registers to read.
//  */
// void spo2_burstread(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr, uint8_t *buffer, uint8_t toRead)
// {
// 	// uint64_t check;
// 	// while(1)
// 	// check = 0;
// 	for (uint8_t i=0; i< toRead*4; i++)
// 	{
// 		buffer[i] = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA);
// 	}
// }

// /** 
//  * @fn readFifoData
//  * 
//  * @brief Reads data from the FIFO buffer of the MAX30100 sensor.
//  * 
//  * @details This function reads data from the FIFO buffer of the MAX30100 sensor.
//  * It calculates the number of data points available in the FIFO buffer, then reads 
//  * the data from the sensor in burst mode. The read data is then parsed and stored 
//  * in the buffer for further processing.
//  */
// void readFifoData(const struct device *dev)
// {
// 	// Buffer to store FIFO data
// 	uint8_t buffer[MAX30100_FIFO_DEPTH*4];
// 	// Number of data points to read from FIFO
// 	uint8_t toRead;

// 	// Calculate the number of data points available in the FIFO buffer
// 	uint8_t x = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_WRITE_POINTER);
// 	uint8_t y = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_READ_POINTER);

// 	toRead = (x - y) & (MAX30100_FIFO_DEPTH-1);
// 	// printf("\nx %u",x);
// 	// printf("\ny %u",y);
// 	if(x == y)
// 	{
// 		toRead = x;
// 	}
// 	if(toRead == 0)
// 	{
// 		toRead = 1;
// 	}
// 	// printf("\n %u",toRead);

// 	// Read data from the FIFO buffer in burst mode
// 	spo2_burstread(dev, I2C0, 0X57,MAX30100_REG_FIFO_DATA ,buffer,toRead);

// 	// Parse and store the read data in the buffer
// 	for(int j=0; j<toRead ; ++j)
// 	{
// 		uint16_t push_ir = ((buffer[j*4] << 8) | buffer[j*4 + 1]);
// 		uint16_t push_red = ((buffer[j*4 + 2] << 8) | buffer[j*4 + 3]);
// 		printf("\nIR	%u",push_ir);
// 		printf("	Red	%u",push_red);
// 		// buffer_ir.push(push_ir);
// 		// buffer_red.push(push_red);
// 	}
// }

// /** 
//  * @fn getPartId
//  * 
//  * @brief Retrieves the part ID of the MAX30100 sensor.
//  * 
//  * @details This function retrieves the part ID of the MAX30100 sensor by reading 
//  * the corresponding register via I2C communication.
//  * 
//  * @return uint8_t The part ID of the MAX30100 sensor.
//  */
// uint8_t getPartId(const struct device *dev) {
//     // Read the part ID register and return the value
//     return spo2_read(dev, I2C0, 0X57, 0xff);
// }

// /** 
//  * @fn begin
//  * 
//  * @brief Initializes the MAX30100 sensor.
//  * 
//  * @details This function initializes the MAX30100 sensor by performing the following steps:
//  * - Initializes the I2C communication.
//  * - Sets the I2C clock speed.
//  * - Verifies the part ID of the sensor.
//  * - Sets the default mode of operation.
//  * - Sets the default pulse width for LEDs.
//  * - Sets the default sampling rate.
//  * - Sets the default current for IR and red LEDs.
//  * - Enables high-resolution mode.
//  * 
//  * @return bool True if the initialization is successful, false otherwise.
//  */
// bool hrm_begin(const struct device *dev)
// {
// 	// Verify part ID
// 	if (getPartId(dev) != EXPECTED_PART_ID) 
// 	{
// 		return false;
// 	}

// 	// Set Mode
// 	uint8_t modeConfig = MAX30100_MC_TEMP_EN | DEFAULT_MODE;	// To initialise temperature detection
// 	// uint8_t modeConfig = DEFAULT_MODE;	// Default mode
// 	spo2_write(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION, modeConfig);
// 	uint8_t x = spo2_read(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION);

// 	// Set LedsPulseWidth
// 	uint8_t previous = spo2_read(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION);
// 	spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xfc) | DEFAULT_PULSE_WIDTH);

// 	// Set SamplingRate
// 	previous = spo2_read(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION);
// 	spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xe3) | (DEFAULT_SAMPLING_RATE << 2));

// 	// Set LedsCurrent
// 	spo2_write(dev, I2C0, 0X57, MAX30100_REG_LED_CONFIGURATION, DEFAULT_RED_LED_CURRENT << 4 | DEFAULT_RED_LED_CURRENT);

// 	// Set HighresMode
// 	spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, previous | MAX30100_SPC_SPO2_HI_RES_EN);

// 	return true;
// }

// /** 
//  * @fn begin
//  * 
//  * @brief Initializes the PulseOximeter object and the underlying hardware.
//  * 
//  * @details This function initializes the PulseOximeter object and the underlying
//  * hardware components.
//  * 
//  * @param debuggingMode_ The debugging mode to be set for the PulseOximeter object.
//  * 
//  * @return bool True if initialization is successful, otherwise false.
//  */
// bool begin(const struct device *dev)
// {
// 	bool ready = hrm_begin(dev);
// 	spo2_write(dev, I2C0, 0x57, MAX30100_REG_MODE_CONFIGURATION, MAX30100_MODE_SPO2_HR);
// 	return true;
// }

// /** 
//  * @fn retrieveTemperature
//  * 
//  * @brief Retrieves the temperature reading from the MAX30100 sensor.
//  * 
//  * @details @details This function retrieves the temperature reading from the MAX30100 sensor
//  * by reading the temperature data registers and combining the integer and 
//  * fractional parts to calculate the temperature in degrees Celsius.
//  * 
//  * @return float The temperature reading in degrees Celsius.
//  */
// float retrieveTemperature(const struct device *dev) 
// {
//     // Read the integer part of the temperature
//     int8_t tempInteger = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_INT);
//     // Read the fractional part of the temperature
//     float tempFrac = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_FRAC);

//     // Calculate the temperature in degrees Celsius and return it
//     return tempFrac * 0.0625 + tempInteger;
// }

// int main()
// {
// 	const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
// 	spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA, DEFAULT_MODE);

// 	// Initialize the PulseOximeter instance
// 	// Failures are generally due to an improper I2C wiring, missing power supply
// 	// or wrong target chip
// 	if (!begin(dev)) 
// 	{
// 		printf("\nFAILED");
// 		for(;;);
// 	} 
// 	else 
// 	{
// 		printf("\nSUCCESS");
// 	}
// 	while(1)
// 	{
// 		readFifoData(dev);
// 	}
// }











/**
 * Project                           : Smartwatch
 * Name of the file                  : spo2_polling.cpp
 * Brief Description of file         : This is a Baremetal SpO2 sensor (using polling) Driver file for Mindgrove Silicon.
 * Name of Author                    : Harini Sree.S
 * Email ID                          : harini@mindgrovetech.in
 *
 * @file spo2_polling.cpp
 * @author Harini Sree. S (harini@mindgrovetech.in)
 * @brief This is a Baremetal SpO2 sensor Driver file for Mindgrove Silicon.
 * @date 2024-01-29
 *
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 *
 */

#include "spo2.h"

#define REPORTING_PERIOD_MS     1000
#define DELAY_FREQ_BASE 40000000
// using namespace std; 

uint64_t total_cycles  = 0; // Used to store the value of mcycle.
int tsLastReport = 0;		// Used to store previous millis value.


circularbuffer buffer_ir; 	// Object of class circularbuffer to store IR values.
circularbuffer buffer_red;	// Object of class circularbuffer to store RED values.
PulseOximeter pox;			// Object of class PulseOximeter.

/** 
 * @brief Constructor for the BeatDetector class.
 * 
 * Sets the initial state to BEATDETECTOR_STATE_INIT,
 * the threshold to the minimum threshold value, the beat period to 0,
 * the last maximum value to 0, and the timestamp of the last beat to 0.
 */
BeatDetector::BeatDetector() :
	state(BEATDETECTOR_STATE_INIT),
	threshold(BEATDETECTOR_MIN_THRESHOLD),
	beatPeriod(0),
	lastMaxValue(0),
	tsLastBeat(0)
{
}

/** 
 * @brief Initializes the circular buffer by setting the head and tail pointers
 * to the beginning of the buffer, and the count of elements in the buffer
 * to 0.
 */
void circularbuffer:: init()
{
	head = buffer;
	tail = buffer;
	count = 0;
}

/** @fn push
 * 
 * @brief The function `push` in the circular buffer class is used to add a value to the buffer while handling
 * circular buffer logic.
 * 
 * @param unsigned value The value to be added in the circular buffer
 * 
 * @return bool Returns 0 when Circular buffer is full
 * 				Returns 1 when Circular buffer is not full 
 */
bool circularbuffer::push(uint16_t value)
{
    // Increment the tail pointer and wrap around if necessary
    if (++tail == buffer + S) 
	{
        tail = buffer;
    }
    
    // Store the value at the tail position
    *tail = value;
    
    // If the buffer is full, adjust the head pointer
    if (count == S) 
	{
        // Increment head and wrap around if necessary
        if (++head == buffer + S) 
		{
            head = buffer;
        }
        
        // Return 0 as the buffer is full
        return 0;
    } 
    else {
        // If the buffer is not full, increment count and set head if it's the first element
        if (count++ == 0) 
		{
            head = tail;
        }
        
        // Return 1 as the push operation was successful
        return 1;
    }
}

/** 
 * @fn pop
 * 
 * @brief The function `pop` in the circular buffer class is used to retrieve and remove the oldest value from the buffer.
 * 
 * @return uint16_t The value retrieved from the circular buffer.
 * 
 * @remarks If the buffer is empty, the function causes a crash.
 */
uint16_t circularbuffer::pop() {
    // Pointer to function for crashing if the buffer is empty
    void(* crash) (void) = 0;
    
    // Check if the buffer is empty, and crash if so
    if (count <= 0) 
	{
        crash();
    }
    
    // Retrieve the value from the tail position
    uint16_t result = *tail--;
    
    // Wrap around if tail goes below the buffer
    if (tail < buffer) 
	{
        tail = buffer + S - 1;
    }
    
    // Decrease the count as an element is removed
    count--;
    
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
bool circularbuffer::isEmpty() 
{
	return count == 0;
}


/**
 * The above function is a constructor for the MAX30100 class.
 */
MAX30100::MAX30100()
{
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
bool MAX30100::begin(const struct device *dev)
{
	// Verify part ID
	if (getPartId(dev) != EXPECTED_PART_ID) 
	{
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
void MAX30100::update(const struct device *dev)
{
	// Delay for 10ms before sampling every value.Used as samplingrate is way too high.(Optional)
	// delayms(10);
	readFifoData(dev);
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
bool MAX30100::getRawValues(uint16_t *ir, uint16_t *red)
{
	// Check if buffer is not empty
	if (!(buffer_red.isEmpty() || buffer_ir.isEmpty())) 
	{
		// Dequeue the oldest value from the buffer
		uint16_t pop_ir = buffer_ir.pop();
		uint16_t pop_red = buffer_red.pop();
		// printf("\nm	%u",millis(get_mcycle_stop()));
		printf("\nIR	%u",pop_ir);
		printf("\nRed	%u",pop_red);
		*ir = pop_ir;
		*red = pop_red;
		return true;
	}
	else
	{
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
void MAX30100::resetFifo(const struct device *dev)
{
	// Write 0 to FIFO write pointer register
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_WRITE_POINTER, 0);
	// Write 0 to FIFO read pointer register
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_READ_POINTER, 0);
	// Write 0 to FIFO overflow counter register
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_OVERFLOW_COUNTER, 0);
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
 * @fn readFifoData
 * 
 * @brief Reads data from the FIFO buffer of the MAX30100 sensor.
 * 
 * @details This function reads data from the FIFO buffer of the MAX30100 sensor.
 * It calculates the number of data points available in the FIFO buffer, then reads 
 * the data from the sensor in burst mode. The read data is then parsed and stored 
 * in the buffer for further processing.
 */
void MAX30100::readFifoData(const struct device *dev)
{
	// Buffer to store FIFO data
	uint8_t buffer[MAX30100_FIFO_DEPTH*4];
	// Number of data points to read from FIFO
	uint8_t toRead;

	// Calculate the number of data points available in the FIFO buffer
	uint8_t x = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_WRITE_POINTER);
	uint8_t y = spo2_read(dev, I2C0, 0X57, MAX30100_REG_FIFO_READ_POINTER);

	toRead = (x - y) & (MAX30100_FIFO_DEPTH-1);
	// printf("\nx %u",x);
	// printf("\ny %u",y);
	if(x == y)
	{
		toRead = x;
	}
	// printf("\ntr %u",toRead);

	// Read data from the FIFO buffer in burst mode
	spo2_burstread(dev, I2C0, 0X57,MAX30100_REG_FIFO_DATA ,buffer,toRead);

	// Parse and store the read data in the buffer
	for(int j=0; j<toRead ; ++j)
	{
		uint16_t push_ir = ((buffer[j*4] << 8) | buffer[j*4 + 1]);
		uint16_t push_red = ((buffer[j*4 + 2] << 8) | buffer[j*4 + 3]);
		buffer_ir.push(push_ir);
		buffer_red.push(push_red);
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
void MAX30100::startTemperatureSampling(const struct device *dev)
{
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
bool MAX30100::isTemperatureReady(const struct device *dev)
{
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
float MAX30100::retrieveTemperature(const struct device *dev) 
{
    // Read the integer part of the temperature
    int8_t tempInteger = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_INT);
    // Read the fractional part of the temperature
    float tempFrac = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_FRAC);

    // Calculate the temperature in degrees Celsius and return it
    return tempFrac * 0.0625 + tempInteger;
}

/** 
 * @fn shutdown
 * 
 * @brief Shuts down the MAX30100 sensor.
 * 
 * @details This function shuts down the MAX30100 sensor by setting the shutdown bit
 * in the mode configuration register.
 */
void MAX30100::shutdown(const struct device *dev) {
    // Read the current mode configuration
    uint8_t modeConfig = spo2_read(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION);
    // Set the shutdown bit
    modeConfig |= MAX30100_MC_SHDN;
    // Write the updated mode configuration back to the sensor
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION, modeConfig);
}

/** 
 * @fn resume
 * 
 * @brief Resumes normal operation of the MAX30100 sensor.
 * 
 * @details This function resumes normal operation of the MAX30100 sensor by clearing 
 * the shutdown bit in the mode configuration register.
 */
void MAX30100::resume(const struct device *dev) {
    // Read the current mode configuration
    uint8_t modeConfig = spo2_read(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION);
    // Clear the shutdown bit
    modeConfig &= ~MAX30100_MC_SHDN;
    // Write the updated mode configuration back to the sensor
    spo2_write(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION, modeConfig);
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
uint8_t MAX30100::getPartId(const struct device *dev) {
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
bool BeatDetector::addSample(float sample)
{
	return checkForBeat(sample);
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
uint8_t BeatDetector::getRate()
{
	if (beatPeriod != 0) 
	{
		return 1 / beatPeriod * 1000 * 60;
	} 
	else 
	{
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
float BeatDetector::getCurrentThreshold()
{
	return threshold;
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
bool BeatDetector::checkForBeat(float sample)
{
	bool beatDetected = false;
	total_cycles = get_mcycle_stop();
	switch (state) 
	{
		// Initialization state
		case BEATDETECTOR_STATE_INIT:
			// Transition to waiting state after initialization hold-off time
			if (millis(total_cycles) > BEATDETECTOR_INIT_HOLDOFF) 
			{
				state = BEATDETECTOR_STATE_WAITING;
			}
			break;
		// Waiting state
		case BEATDETECTOR_STATE_WAITING:
			// Adjust threshold if sample exceeds it
			if (sample > threshold) 
			{
				threshold = min(sample, BEATDETECTOR_MAX_THRESHOLD);
				state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
			}

			// Reset if tracking lost
			if (millis(total_cycles) - tsLastBeat > BEATDETECTOR_INVALID_READOUT_DELAY) 
			{
				beatPeriod = 0;
				lastMaxValue = 0;
			}

			decreaseThreshold();
			break;
		// Following slope state
		case BEATDETECTOR_STATE_FOLLOWING_SLOPE:
			if (sample < threshold) 
			{
				state = BEATDETECTOR_STATE_MAYBE_DETECTED;
			} 
			else 
			{
				threshold = min(sample, BEATDETECTOR_MAX_THRESHOLD);
			}
			break;
		// Maybe detected state
		case BEATDETECTOR_STATE_MAYBE_DETECTED:
			if (sample + BEATDETECTOR_STEP_RESILIENCY < threshold) 
			{
				// Found a beat
				beatDetected = true;
				lastMaxValue = sample;
				state = BEATDETECTOR_STATE_MASKING;
				total_cycles = get_mcycle_stop();   
				float delta = millis(total_cycles) - tsLastBeat;
				// float delta = 700;
				// printf("\ndelta %f",delta);
				// printf("\nbeatperiod before %f",beatPeriod);
				if (delta) 
				{
					beatPeriod = BEATDETECTOR_BPFILTER_ALPHA * delta +((1 - BEATDETECTOR_BPFILTER_ALPHA) * beatPeriod);
				}
				// printf("\nbeatperiod %f",beatPeriod);

				total_cycles = get_mcycle_stop();
				tsLastBeat = millis(total_cycles);
			} 
			else 
			{
				state = BEATDETECTOR_STATE_FOLLOWING_SLOPE;
			}
			break;
		// Masking state
		case BEATDETECTOR_STATE_MASKING:
			if (millis(total_cycles) - tsLastBeat > BEATDETECTOR_MASKING_HOLDOFF) 
			{
				state = BEATDETECTOR_STATE_WAITING;
			}
			decreaseThreshold();
			break;
	}

	return beatDetected;
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
void BeatDetector::decreaseThreshold()
{
	// Adjust threshold based on last maximum value and beat period
	if (lastMaxValue > 0 && beatPeriod > 0) 
	{
		threshold -= lastMaxValue * (1 - BEATDETECTOR_THRESHOLD_FALLOFF_TARGET) / (beatPeriod / BEATDETECTOR_SAMPLES_PERIOD);
	} 
	else 
	{
        // Apply asymptotic decay if no beat period or maximum value is available
		threshold *= BEATDETECTOR_THRESHOLD_DECAY_FACTOR;
	}

	// Ensure threshold does not fall below the minimum threshold
	if (threshold < BEATDETECTOR_MIN_THRESHOLD) 
	{
		threshold = BEATDETECTOR_MIN_THRESHOLD;
	}
}

/** 
 * @brief Constructor for the PulseOximeter class.
 * 
 * @details Initializes the PulseOximeter object with default values for its internal
 * state variables. 
 */
PulseOximeter::PulseOximeter() :
	state(PULSEOXIMETER_STATE_INIT),
	tsFirstBeatDetected(0),
	tsLastBeatDetected(0),
	tsLastBiasCheck(0),
	tsLastCurrentAdjustment(0),
	redLedCurrentIndex((uint8_t)RED_LED_CURRENT_START),
	// irLedCurrent(DEFAULT_IR_LED_CURRENT),
	onBeatDetected(NULL)
{
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
bool PulseOximeter::begin(const struct device *dev, PulseOximeterDebuggingMode debuggingMode_)
{
	debuggingMode = debuggingMode_;

	bool ready = hrm.begin(dev);
	if (!ready) 
	{
		if (debuggingMode != PULSEOXIMETER_DEBUGGINGMODE_NONE) 
		{
			//printf("\nFailed to initialize the HRM sensor");
		}
		return false;
	}

	// hrm.setMode(MAX30100_MODE_SPO2_HR);
	spo2_write(dev, I2C0, 0x57, MAX30100_REG_MODE_CONFIGURATION, MAX30100_MODE_SPO2_HR);
	// hrm.setLedsCurrent(irLedCurrent, (LEDCurrent)redLedCurrentIndex);
	// spo2_write(dev, I2C0, 0X57, MAX30100_REG_LED_CONFIGURATION, irLedCurrent << 4 | (LEDCurrent)redLedCurrentIndex);

	irDCRemover = DCRemover(DC_REMOVER_ALPHA);
	redDCRemover = DCRemover(DC_REMOVER_ALPHA);

	state = PULSEOXIMETER_STATE_IDLE;

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
void PulseOximeter::update(const struct device *dev)
{
	// Update the heart rate monitor sensor
	hrm.update(dev);
	// Perform sample analysis
	checkSample();
	// Perform current bias checking
	checkCurrentBias(dev);
}

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
uint8_t PulseOximeter::getHeartRate()
{
	return beatDetector.getRate();
}

/**
 * @details The function `getSpO2` returns the SpO2 value calculated by `SpO2Calculator_getSpO2`.
 * 
 * @return The `getSpO2` function is returning an 8-bit unsigned integer value representing the SpO2
 * (blood oxygen saturation level) obtained from the `SpO2Calculator_getSpO2` function.
 */
uint8_t PulseOximeter::getSpO2()
{
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
uint8_t PulseOximeter::getRedLedCurrentBias()
{
	return redLedCurrentIndex;
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
void PulseOximeter::setOnBeatDetectedCallback(void (*cb)())
{
	onBeatDetected = cb;
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
void PulseOximeter::shutdown(const struct device *dev)
{
	hrm.shutdown(dev);
}

/** 
 * @fn resume
 * 
 * @brief Resumes operation of the PulseOximeter by resuming the heart rate monitor sensor.
 * 
 * @details This function is typically used after a previous shutdown operation to restore normal operation of the sensor.
 */
void PulseOximeter::resume(const struct device *dev)
{
	hrm.resume(dev);
}

/** 
 * @fn checkSample
 * 
 * @brief Processes raw sensor samples to detect beats and update SpO2 values.
 */

void PulseOximeter::checkSample()
{
	uint16_t rawIRValue, rawRedValue, c;

	// Dequeue all available samples, they're properly timed by the HRM
	
	while (hrm.getRawValues(&rawIRValue, &rawRedValue)) 
	{
		//printf("\npoint rawirvalue %u",rawIRValue);
		//printf("\npoint rawredvalue %u",rawRedValue);

		float irACValue = irDCRemover.step(rawIRValue);
		float redACValue = redDCRemover.step(rawRedValue);

		//printf("\npoint redACvalue %f",redACValue);

		// The signal fed to the beat detector is mirrored since the cleanest monotonic spike is below zero
		float filteredPulseValue = lpf.step(-irACValue);
		// printf("	%f",irACValue);
		// printf("	%f",filteredPulseValue);
		bool beatDetected = beatDetector.addSample(filteredPulseValue);
		//printf("\nbeatDetected %d",beatDetected);
		// printf("\nstate %d",state);

		if (beatDetector.getRate() > 0) 
		{
			state = PULSEOXIMETER_STATE_DETECTING;
			SpO2Calculator_update(irACValue, redACValue, beatDetected);
		}
		else if (state == PULSEOXIMETER_STATE_DETECTING) 
		{
			state = PULSEOXIMETER_STATE_IDLE;
			SpO2Calculator_reset();
		}
	}
}

/** 
 * @fn checkCurrentBias
 * 
 * @brief Adjusts the red LED current to maintain comparable DC baselines between red and IR LEDs.
 */
void PulseOximeter::checkCurrentBias(const struct device *dev)
{
	// Follower that adjusts the red led current in order to have comparable DC baselines between
	// red and IR leds. The numbers are really magic: the less possible to avoid oscillations
	total_cycles = get_mcycle_stop();
	if (millis(total_cycles) - tsLastBiasCheck > CURRENT_ADJUSTMENT_PERIOD_MS) 
	{
		bool changed = false;
		if (irDCRemover.getDCW() - redDCRemover.getDCW() > 70000 && redLedCurrentIndex < MAX30100_LED_CURR_50MA) 
		{
			++redLedCurrentIndex;
			changed = true;
		} 
		else if (redDCRemover.getDCW() - irDCRemover.getDCW() > 70000 && redLedCurrentIndex > 0) 
		{
			--redLedCurrentIndex;
			changed = true;
		}

		if (changed) 
		{
			// hrm.setLedsCurrent(irLedCurrent, (LEDCurrent)redLedCurrentIndex);
			spo2_write(dev, I2C0, 0X57, DEFAULT_IR_LED_CURRENT, (LEDCurrent)redLedCurrentIndex); 

			if (debuggingMode != PULSEOXIMETER_DEBUGGINGMODE_NONE)
			{
				printf("\nI: %u",redLedCurrentIndex);
			}
		}
		total_cycles = get_mcycle_stop();
		tsLastBiasCheck = millis(total_cycles);
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
    asm volatile("NOP");
  }
}








// MAX30100_Minimal code
int main()
{
   const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	uint8_t cycle_at_start = get_mcycle_start();

	buffer_ir.init();
	buffer_red.init();

	spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA, DEFAULT_MODE);

	// Initialize the PulseOximeter instance
	// Failures are generally due to an improper I2C wiring, missing power supply
	// or wrong target chip
	if (!pox.begin(dev)) 
	{
		printf("\nFAILED");
		for(;;);
	} 
	else 
	{
		printf("\nSUCCESS");
	}
	// printf("\nTime	IR	irAC	LPF");

	while(1)
	{
		// delayms(1);
		// Make sure to call update as fast as possible
		pox.update(dev);

		// Asynchronously dump heart rate and oxidation levels to the serial
		// For both, a value of 0 means "invalid"
		total_cycles = get_mcycle_stop();
		if (millis(total_cycles) - tsLastReport > REPORTING_PERIOD_MS) 
		{
			uint8_t spo2 = pox.getSpO2();
			uint8_t heartrate = pox.getHeartRate();
			// printf("	SPO2 : %u%",spo2);

			tsLastReport = millis(total_cycles);
		}
	}
	return 0;    
}


/*TODO
setIRLedCurrent
*/