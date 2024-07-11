#include "spo2.h"

#define REPORTING_PERIOD_MS     1000
#define DELAY_FREQ_BASE 40000000
// using namespace std; 

uint64_t total_cycles  = 0; // Used to store the value of mcycle.
int tsLastReport = 0;		// Used to store previous millis value.


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
	// printf("\nBegin");
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

// /** 
//  * @fn getRawValues
//  * 
//  * @brief Retrieves raw IR and red values from the MAX30100 sensor.
//  * 
//  * @details This function retrieves the raw IR and red values from the MAX30100 sensor.
//  * 
//  * @param ir Pointer to store the retrieved raw IR value.
//  * @param red Pointer to store the retrieved raw red value.
//  * 
//  * @return bool True if raw values are successfully retrieved, false if the buffer is empty.
//  */
// void getRawValues()
// {
// 	// Check if buffer is not empty
// 	if (!(buffer_red.isEmpty() || buffer_ir.isEmpty())) 
// 	{
// 		// Dequeue the oldest value from the buffer
// 		uint16_t pop_ir = buffer_ir.pop();
// 		uint16_t pop_red = buffer_red.pop();
// 		// printf("\nm	%u",millis(get_mcycle_stop()));
// 		printf("\nIR	%u",pop_ir);
// 		printf("\nRed	%u",pop_red);
// 		// *ir = pop_ir;
// 		// *red = pop_red;
// 	}
// }

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
void readFifoData(const struct device *dev)
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
	if(toRead == 0)
	{
		toRead = 1;
	}
	// printf("\n %u",toRead);

	// Read data from the FIFO buffer in burst mode
	spo2_burstread(dev, I2C0, 0X57,MAX30100_REG_FIFO_DATA ,buffer,toRead);

	// Parse and store the read data in the buffer
	for(int j=0; j<toRead ; ++j)
	{
		uint16_t push_ir = ((buffer[j*4] << 8) | buffer[j*4 + 1]);
		uint16_t push_red = ((buffer[j*4 + 2] << 8) | buffer[j*4 + 3]);
		printf("\nIR	%u",push_ir);
		printf("	Red	%u",push_red);
		// buffer_ir.push(push_ir);
		// buffer_red.push(push_red);
	}
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
uint8_t getPartId(const struct device *dev) {
    // Read the part ID register and return the value
    return spo2_read(dev, I2C0, 0X57, 0xff);
}

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
bool hrm_begin(const struct device *dev)
{
	// Verify part ID
	if (getPartId(dev) != EXPECTED_PART_ID) 
	{
		return false;
	}

	// Set Mode
	uint8_t modeConfig = MAX30100_MC_TEMP_EN | DEFAULT_MODE;	// To initialise temperature detection
	// uint8_t modeConfig = DEFAULT_MODE;	// Default mode
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION, modeConfig);
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
bool begin(const struct device *dev)
{
	// debuggingMode = debuggingMode_;

	bool ready = hrm_begin(dev);
	// if (!ready) 
	// {
	// 	if (debuggingMode != PULSEOXIMETER_DEBUGGINGMODE_NONE) 
	// 	{
	// 		//printf("\nFailed to initialize the HRM sensor");
	// 	}
	// 	return false;
	// }

	// hrm.setMode(MAX30100_MODE_SPO2_HR);
	spo2_write(dev, I2C0, 0x57, MAX30100_REG_MODE_CONFIGURATION, MAX30100_MODE_SPO2_HR);
	// hrm.setLedsCurrent(irLedCurrent, (LEDCurrent)redLedCurrentIndex);
	// spo2_write(dev, I2C0, 0X57, MAX30100_REG_LED_CONFIGURATION, irLedCurrent << 4 | (LEDCurrent)redLedCurrentIndex);

	// irDCRemover = DCRemover(DC_REMOVER_ALPHA);
	// redDCRemover = DCRemover(DC_REMOVER_ALPHA);

	// state = PULSEOXIMETER_STATE_IDLE;

	return true;
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
float retrieveTemperature(const struct device *dev) 
{
    // Read the integer part of the temperature
    int8_t tempInteger = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_INT);
    // Read the fractional part of the temperature
    float tempFrac = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_FRAC);

    // Calculate the temperature in degrees Celsius and return it
    return tempFrac * 0.0625 + tempInteger;
}

int main()
{
   const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	// uint8_t cycle_at_start = get_mcycle_start();

	// buffer_ir.init();
	// buffer_red.init();

	spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA, DEFAULT_MODE);

	// Initialize the PulseOximeter instance
	// Failures are generally due to an improper I2C wiring, missing power supply
	// or wrong target chip
	printf("\nBegin");
	if (!begin(dev)) 
	{
		printf("\nFAILED");
		for(;;);
	} 
	else 
	{
		printf("\nSUCCESS");
	}
	while(1)
	{
		readFifoData(dev);
		printf("	Temp %f",retrieveTemperature(dev));
		// getRawValues();
	}
}