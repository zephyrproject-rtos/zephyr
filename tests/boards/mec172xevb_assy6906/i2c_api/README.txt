I2C Test For Microchip board
############################

This application test is used to test the I2C driver on Microchip
mec172xevb_assy6906 board.


Board Hardware Setup
####################

As the I2C slave device NXP pca95xx on mec172xevb_assy6906 is connected
to I2C00 port, however, I2C00 port is shared with UART2 RS232 to TTL
converter used to catch serial log, so it's not possible to use UART2
and I2C00 port simultaneously.
We need to change to use I2C01 port by making some jumpers setting as below:

 * JP49 	1-2 	Connected 	Connect PCA9555 VCC to +3.3V_STBY
 * JP53 	1-2 	Connected 	Select address 0100b, which means 0x26
 * JP12 	13-14 	Connected 	Connect I2C01_SDA from CPU to header J20
 * JP12 	4-5 	Connected 	Connect I2C01_SCL from CPU to header J20

 * JP77 	7-8 	Connected 	External pull-up for I2C01_SDA
 * JP77 	9-10 	Connected 	External pull-up for I2C01_SCL

 * JP58.1 	J20.1 	Connected 	Connect NXP PCA95xx to I2C01
 * JP58.3 	J20.3 	Connected 	Connect NXP PCA95xx to I2C01
