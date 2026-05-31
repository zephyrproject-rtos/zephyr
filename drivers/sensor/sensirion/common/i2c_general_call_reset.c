int sensirion_i2c_general_call_reset(const struct i2c_dt_spec *i2c_spec)
{
	const uint8_t data = 0x06;
	const uint8_t i2c_address = 0x00;

	return i2c_write(i2c_spec->bus, &data, sizeof(data), i2c_address);
}
