/* This file is a temporary workaround for mapping of the generated information
 * to the current driver definitions.  This will be removed when the drivers
 * are modified to handle the generated information, or the mapping of
 * generated data matches the driver definitions.
 */


#define CONFIG_NUM_IRQ_PRIO_BITS	ARM_V6M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS
#define CONFIG_USART_GECKO_0_NAME	SILABS_EFM32_USART_4000C000_LABEL
#define CONFIG_USART_GECKO_1_NAME	SILABS_EFM32_USART_4000C400_LABEL

#define CONFIG_USART_GECKO_0_BAUD_RATE	SILABS_EFM32_USART_4000C000_CURRENT_SPEED
#define CONFIG_USART_GECKO_0_IRQ_PRI	SILABS_EFM32_USART_4000C000_IRQ_0_PRIORITY
#define CONFIG_USART_GECKO_1_BAUD_RATE	SILABS_EFM32_USART_4000C400_CURRENT_SPEED
#define CONFIG_USART_GECKO_1_IRQ_PRI	SILABS_EFM32_USART_4000C400_IRQ_0_PRIORITY
