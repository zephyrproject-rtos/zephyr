
#ifndef _ATMOSIC_ATM_IRQ_H_
#define _ATMOSIC_ATM_IRQ_H_

/* IRQ Numbers */
#define IRQ_UART0RX             0  /* UART 0 RX Interrupt                   */
#define IRQ_UART0TX             1  /* UART 0 TX Interrupt                   */
#define IRQ_UART1RX             2  /* UART 1 RX Interrupt                   */
#define IRQ_UART1TX             3  /* UART 1 TX Interrupt                   */
#define IRQ_GADC                4  /* GADC Interrupt                        */
#define IRQ_PORT0_ALL           6  /* GPIO Port 0 combined Interrupt        */
#define IRQ_RADIO               6  /* Radio Interrupt                       */
#define IRQ_PMU                 6  /* Power Management Unit Interrupt       */
#define IRQ_I2C0                6  /* I2C Port 0 Interrupt                  */
#define IRQ_I2C1                6  /* I2C Port 1 Interrupt                  */
#define IRQ_PORT1_ALL           7  /* GPIO Port 1 combined Interrupt        */
#define IRQ_SPI1                7  /* SPI Port 1 Interrupt                  */
#define IRQ_TIMER0              8  /* TIMER 0 Interrupt                     */
#define IRQ_TIMER1              9  /* TIMER 1 Interrupt                     */
#define IRQ_DUALTIMER           10 /* Dual Timer Interrupt                  */
#define IRQ_SPI0                11 /* SPI Port 0 Interrupt                  */
#define IRQ_UART0OVF            12 /* UART 0 Overflow Interrupt             */
#define IRQ_UART1OVF            13 /* UART 1 Overflow Interrupt             */
#define IRQ_KSM                 14 /* Keyboard Interrupt                    */
#define IRQ_SLWTIMER            15 /* Slow Timer Interrupt                  */
#define IRQ_BLE                 15 /* Bluetooth Core Interrupt              */
#define IRQ_PORT0_0             16 /* All P0 I/O pins used as irq source    */
#define IRQ_PORT0_1             17 /* There are 16 pins in total            */
#define IRQ_PORT0_2             18
#define IRQ_PORT0_3             19
#define IRQ_PORT0_4             20
#define IRQ_PORT0_5             21
#define IRQ_PORT0_6             22
#define IRQ_PORT0_7             23
#define IRQ_PORT0_8             24
#define IRQ_PORT0_9             25
#define IRQ_PORT0_10            26
#define IRQ_PORT0_11            27
#define IRQ_PORT0_12            28
#define IRQ_PORT0_13            29
#define IRQ_PORT0_14            30
#define IRQ_PORT0_15            31

#endif // _ATMOSIC_ATM_IRQ_H_
