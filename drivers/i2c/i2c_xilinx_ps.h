#ifndef ZEPHYR_DRIVERS_I2C_I2C_XILINX_PS_H_
#define ZEPHYR_DRIVERS_I2C_I2C_XILINX_PS_H_

enum xilinx_ps_i2c_register {
    REG_CR          = 0x00, /**< 32-bit Control */
    REG_SR          = 0x04, /**< Status */
    REG_ADDR        = 0x08, /**< IIC Address */
    REG_DATA        = 0x0C, /**< IIC FIFO Data */
    REG_ISR         = 0x10, /**< Interrupt Status */
    REG_TRANS_SIZE  = 0x14, /**< Transfer Size */     
    REG_SLV_PAUSE   = 0x18, /**< Slave monitor pause */    
    REG_TIME_OUT    = 0x1C, /**< Time Out */ 
    REG_IMR         = 0x20, /**< Interrupt Enabled Mask */
    REG_IER         = 0x24, /**< Interrupt Enable */
    REG_IDR         = 0x28, /**< Interrupt Disable */
};

enum xilinxps_i2c_isr_bits {
    ISR_ARB_LOST = BIT(9),
	ISR_RX_UNF = BIT(7),
	ISR_TX_OVF = BIT(6),	 
	ISR_RX_OVF = BIT(5),	 
	ISR_SLV_RDY = BIT(4),
	ISR_TIMEOUT = BIT(3),	 
	ISR_NACK = BIT(2),	 
	ISR_DATA = BIT(1),
	ISR_TX_COMP = BIT(0),
};

/************************** Constant Definitions *****************************/

/** @name Register Map
 *
 * Register offsets for the IIC.
 * @{
 */
#define XIICPS_CR_DIV_A_MASK	0x0000C000U /**< Clock Divisor A */
#define XIICPS_CR_DIV_A_SHIFT			14U /**< Clock Divisor A shift */
#define XIICPS_DIV_A_MAX				 4U /**< Maximum value of Divisor A */
#define XIICPS_CR_DIV_B_MASK	0x00003F00U /**< Clock Divisor B */
#define XIICPS_CR_DIV_B_SHIFT			 8U /**< Clock Divisor B shift */
#define XIICPS_CR_CLR_FIFO_MASK	0x00000040U /**< Clear FIFO, auto clears*/
#define XIICPS_CR_SLVMON_MASK	0x00000020U /**< Slave monitor mode */
#define XIICPS_CR_HOLD_MASK		0x00000010U /**<  Hold bus 1=Hold scl,
												0=terminate transfer */
#define XIICPS_CR_ACKEN_MASK	0x00000008U /**< Enable TX of ACK when
												Master receiver*/
#define XIICPS_CR_NEA_MASK		0x00000004U /**< Addressing Mode 1=7 bit,
												0=10 bit */
#define XIICPS_CR_MS_MASK		0x00000002U /**< Master mode bit 1=Master,
												0=Slave */
#define XIICPS_CR_RD_WR_MASK	0x00000001U /**< Read or Write Master
												transfer  0=Transmitter,
												1=Receiver*/
#define XIICPS_CR_RESET_VALUE			 0U /**< Reset value of the Control
												register */
/** @} */

/** @name IIC Time Out Register
*
* The value of time out register represents the time out interval in number of
* sclk cycles minus one.
*
* When the accessed slave holds the sclk line low for longer than the time out
* period, thus prohibiting the I2C interface in master mode to complete the
* current transfer, an interrupt is generated and TO interrupt flag is set.
*
* The reset value of the register is 0x1f.
* Read/Write
* @{
 */
#define XIICPS_TIME_OUT_MASK    0x0000001FU    /**< IIC Time Out mask */
#define XIICPS_TO_RESET_VALUE   0x0000001FU    /**< IIC Time Out reset value */
/** @} */
/** @name IIC Interrupt Registers
 *
 * <b>IIC Interrupt Status Register</b>
 *
 * This register holds the interrupt status flags for the IIC controller. Some
 * of the flags are level triggered
 * - i.e. are set as long as the interrupt condition exists.  Other flags are
 *   edge triggered, which means they are set one the interrupt condition occurs
 *   then remain set until they are cleared by software.
 *   The interrupts are cleared by writing a one to the interrupt bit position
 *   in the Interrupt Status Register. Read/Write.
 *
 * <b>IIC Interrupt Enable Register</b>
 *
 * This register is used to enable interrupt sources for the IIC controller.
 * Writing a '1' to a bit in this register clears the corresponding bit in the
 * IIC Interrupt Mask register.  Write only.
 *
 * <b>IIC Interrupt Disable Register </b>
 *
 * This register is used to disable interrupt sources for the IIC controller.
 * Writing a '1' to a bit in this register sets the corresponding bit in the
 * IIC Interrupt Mask register. Write only.
 *
 * <b>IIC Interrupt Mask Register</b>
 *
 * This register shows the enabled/disabled status of each IIC controller
 * interrupt source. A bit set to 1 will ignore the corresponding interrupt in
 * the status register. A bit set to 0 means the interrupt is enabled.
 * All mask bits are set and all interrupts are disabled after reset. Read only.
 *
 * All four registers have the same bit definitions. They are only defined once
 * for each of the Interrupt Enable Register, Interrupt Disable Register,
 * Interrupt Mask Register, and Interrupt Status Register
 * @{
 */

#define XIICPS_IXR_ARB_LOST_MASK  0x00000200U	 /**< Arbitration Lost Interrupt
													mask */
#define XIICPS_IXR_RX_UNF_MASK    0x00000080U	 /**< FIFO Receive Underflow
													Interrupt mask */
#define XIICPS_IXR_TX_OVR_MASK    0x00000040U	 /**< Transmit Overflow
													Interrupt mask */
#define XIICPS_IXR_RX_OVR_MASK    0x00000020U	 /**< Receive Overflow Interrupt
													mask */
#define XIICPS_IXR_SLV_RDY_MASK   0x00000010U	 /**< Monitored Slave Ready
													Interrupt mask */
#define XIICPS_IXR_TO_MASK        0x00000008U	 /**< Transfer Time Out
													Interrupt mask */
#define XIICPS_IXR_NACK_MASK      0x00000004U	 /**< NACK Interrupt mask */
#define XIICPS_IXR_DATA_MASK      0x00000002U	 /**< Data Interrupt mask */
#define XIICPS_IXR_COMP_MASK      0x00000001U	 /**< Transfer Complete
													Interrupt mask */
#define XIICPS_IXR_DEFAULT_MASK   0x000002FFU	 /**< Default ISR Mask */
#define XIICPS_IXR_ALL_INTR_MASK  0x000002FFU	 /**< All ISR Mask */
/** @} */


/** @name IIC Status Register
 *
 * This register is used to indicate status of the IIC controller. Read only
 * @{
 */
#define XIICPS_SR_BA_MASK		0x00000100U  /**< Bus Active Mask */
#define XIICPS_SR_RXOVF_MASK	0x00000080U  /**< Receiver Overflow Mask */
#define XIICPS_SR_TXDV_MASK		0x00000040U  /**< Transmit Data Valid Mask */
#define XIICPS_SR_RXDV_MASK		0x00000020U  /**< Receiver Data Valid Mask */
#define XIICPS_SR_RXRW_MASK		0x00000008U  /**< Receive read/write Mask */
/** @} */

#define XIICPS_MAX_TRANSFER_SIZE	(uint32_t)(255U - 3U) /**< Max transfer size */
#define FIFO_SIZE 16
#endif