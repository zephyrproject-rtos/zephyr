I2C Test For Microchip board
############################

This application test is used to test the I2C driver on Microchip
mec15xxevb_assy6853 board.


Board Hardware Setup
####################

As the I2C slave device NXP pca95xx on mec15xxevb_assy6853 is connected
to I2C00 port, however, I2C00 port is shared with UART2 RS232 to TTL
converter used to catch serial log, so it's not possible to use UART2
and I2C00 port simultaneously.
We need to change to use I2C01 port by making some jumpers setting as below:

 * JP99 	1-2 	Connected	Connect I2C01_SDA from CPU to header J5
 * JP99 	13-14 	Connected	Connect I2C01_SCL from CPU to header J5
 * JP25 	21-22 	Connected	External pull-up for I2C01_SDA
 * JP25 	23-24 	Connected	External pull-up for I2C01_SCL
 *
 * JP44.1 	J5.1 	Connected	Connect NXP PCA95xx to I2C01
 * JP44.3 	J5.3	Connected	Connect NXP PCA95xx to I2C01
