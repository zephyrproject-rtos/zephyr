## WCH CH5XX Bluetooth SOCs

These SOCs are structured and named slightly differently than the CH32 family of chips.

Some examples:

* Non-volatile flash memory is typically divided into 4 sections:
  * CodeFlash - user code/rodata
  * DataFlash - user non-volatile storage, typically 32 KB
  * BootLoader - system code/rodata, typically 24 KB
  * InfoFlash - system non-volatile storage, typically 8 KB
* Volatile SRAM memory is typically divided into 2 sections:
  * RAM16K, RAM24K, RAM30K, RAM96K, or "RAMHP" - main memory region
  * RAM2K, RAM32K, or "RAMLP" - a lower power memory region that can be maintaned while sleeping
* User peripheral addresses are 0x40000000-0x4000FFFF, instead of 0x40000000-0x4001FFFF
* Most register names begin a zero instead of 1
  * Timer registers are TMR0-TMR3 instead of TIM1-TIM4
  * UART registers are UART0-UART3 instead of USART1-USART4
  * SPI registers are SPI0-SPI1 instead of SPI1-SPI2
    * SPI0 is typically assigned to 0x40004000 instead of 0x40013000

## References

* <https://www.wch-ic.com/products/productsCenter/mcuInterface?categoryId=63>
