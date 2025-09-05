i2c ram test
############

Tests an i2c controller driver against a (s/f/nv)ram module doing a simple write and readback.

Exercises most of the i2c controller APIs in the process.

Hardware Setup
==============
- Target supporting I2C (with RTIO for the I2C_RTIO test-cases), e.g: mimxrt1010_evk.
- External I2C RAM chipset connected to the I2C bus, e.g: Fujitsu's MB85 FeRAM.
