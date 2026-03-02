/* Define for CH5XX         */
/* Website:  http://wch.cn  */
/* Email:    tech@wch.cn    */
/* Author:   W.ch 2020.05   */
/* V0.2 SpecialFunctionRegister */

// multi-blocks: __BASE_TYPE__, __CH583SFR_H__, __CH583USBSFR_H__, __USB_TYPE__...

#ifndef __BASE_TYPE__
#define __BASE_TYPE__

#ifdef __cplusplus
extern "C" {
#endif

/* ********************************************************************************************************************* */
/* Base types & constants */

#ifndef TRUE
#define TRUE                    1
#define FALSE                   0
#endif
#ifndef NULL
#define NULL                    0
#endif

#ifndef VOID
#define VOID                    void
#endif
#ifndef CONST
#define CONST                   const
#endif
#ifndef BOOL
typedef unsigned char           BOOL;
#endif
#ifndef BOOLEAN
typedef unsigned char           BOOLEAN;
#endif
#ifndef CHAR
typedef char                    CHAR;
#endif
#ifndef INT8
typedef char                    INT8;
#endif
#ifndef INT16
typedef short                   INT16;
#endif
#ifndef INT32
typedef long                    INT32;
#endif
#ifndef UINT8
typedef unsigned char           UINT8;
#endif
#ifndef UINT16
typedef unsigned short          UINT16;
#endif
#ifndef UINT32
typedef unsigned long           UINT32;
#endif
#ifndef UINT64
typedef unsigned long long      UINT64;
#endif
#ifndef UINT8V
typedef unsigned char volatile  UINT8V;
#endif
#ifndef UINT16V
typedef unsigned short volatile UINT16V;
#endif
#ifndef UINT32V
typedef unsigned long volatile  UINT32V;
#endif
#ifndef UINT64V
typedef unsigned long long volatile  UINT64V;
#endif

#ifndef PVOID
typedef void                    *PVOID;
#endif
#ifndef PCHAR
typedef char                    *PCHAR;
#endif
#ifndef PCHAR
typedef const char              *PCCHAR;
#endif
#ifndef PINT8
typedef char                    *PINT8;
#endif
#ifndef PINT16
typedef short                   *PINT16;
#endif
#ifndef PINT32
typedef long                    *PINT32;
#endif
#ifndef PUINT8
typedef unsigned char           *PUINT8;
#endif
#ifndef PUINT16
typedef unsigned short          *PUINT16;
#endif
#ifndef PUINT32
typedef unsigned long           *PUINT32;
#endif
#ifndef PUINT8V
typedef volatile unsigned char  *PUINT8V;
#endif
#ifndef PUINT16V
typedef volatile unsigned short *PUINT16V;
#endif
#ifndef PUINT32V
typedef volatile unsigned long  *PUINT32V;
#endif
#ifndef PUINT64V
typedef volatile unsigned long long  *PUINT64V;
#endif

/* ********************************************************************************************************************* */
/* Base macros */

#ifndef min
#define min(a,b)                (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)                (((a) > (b)) ? (a) : (b))
#endif

#ifdef  DEBUG
#define PRINT(X...) printf(X)
#else
#define PRINT(X...)
#endif

/* Calculate the byte offset of a field in a structure of type */
#define FIELD_OFFSET(Type, Field)    ((UINT16)&(((Type *)0)->Field))

/* Calculate the size of a field in a structure of type */
#define FIELD_SIZE(Type, Field)      (sizeof(((Type *)0)->Field))

/* An expression that yields the type of a field in a struct */
#define FIELD_TYPE(Type, Field)      (((Type *)0)->Field)

/* Return the number of elements in a statically sized array */
#define NUMBER_OF(Array)             (sizeof(Array)/sizeof((Array)[0]))
#define NUMBER_OF_FIELD(Type, Field) (NUMBER_OF(FIELD_TYPE(Type, Field)))

#ifdef __cplusplus
}
#endif

#endif  // __BASE_TYPE__


#ifndef __CH5XXSFR_H__
#define __CH5XXSFR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ********************************************************************************************************************* */

// Address Space
//    CODE:   00000000H - 0007FFFFH   512K
//    DATA:   20000000H - 20007FFFH   32KB
//    SFR:    40000000H - 4000FFFFH   64KB
//
//    SFR:    40000000H - 4000FFFFH,  64KB
//      SYS:     +1000H - 1BFFH, include base configuration, interrupt, GPIO, etc...
//      TMR0:    +2000H - 23FFH
//      TMR1:    +2400H - 27FFH
//      TMR2:    +2800H - 2BFFH
//      TMR3:    +2C00H - 2FFFH
//      UART0:   +3000H - 33FFH
//      UART1:   +3400H - 37FFH
//      UART2:   +3800H - 3BFFH
//      UART3:   +3C00H - 3FFFH
//      SPI0:    +4000H - 43FFH
//      SPI1:    +4400H - 47FFH
//      I2C:     +4800H - 4BFFH
//      PWMx:    +5000H - 53FFH
//      LCD:     +6000H - 63FFH
//      USB:     +8000H - 83FFH
//      USB2:    +8400H - 87FFH
//      BLE:     +C000H - D3FFH

// Register Bit Attribute / Bit Access Type
//   RF:    Read only for Fixed value
//   RO:    Read Only (internal change)
//   RZ:    Read only with auto clear Zero
//   WO:    Write Only (read zero or different)
//   WA:    Write only under safe Accessing mode (read zero or different)
//   WZ:    Write only with auto clear Zero
//   RW:    Read / Write
//   RWA:   Read / Write under safe Accessing mode
//   RW1:   Read / Write 1 to Clear

/* Register name rule:
   R32_* for 32 bits register (UINT32,ULONG)
   R16_* for 16 bits register (UINT16,USHORT)
   R8_*  for  8 bits register (UINT8,UCHAR)
   RB_*  for bit or bit mask of 8 bit register
   BA_*  for base address point
   b*    for GPIO bit mask
   Others for register address offset */

/* ********************************************************************************************************************* */

/* Independent watch-dog register */
#define R32_IWDG_KR         (*((PUINT32V)0x40001000)) // WO, watch-dog key register, must enable LSI
#define R32_IWDG_CFG        (*((PUINT32V)0x40001004)) // RW, watch-dog configuration
#define  RB_RLR             0x0FFF                    // RW, watch-dog counter reload (write protect)
#define  RB_PR              0x7000                    // PR, prescale (write protect)
#define  RB_PVU             0x8000                    // RO, register update flag (write protect)
#define  RB_COUNT           0xFF0000                  // RO, watch-dog down counter
#define  RB_STOP_EN         0x20000000                // RW, watch-dog stop enable (write protect)
#define  RB_WR_PROTECT      0x40000000                // RO, write protect
#define  RB_IWDG_EN         0x80000000                // RO, watch-dog enable

/* System: safe accessing register */
#define R32_SAFE_ACCESS     (*((PUINT32V)0x40001040)) // RW, safe accessing
#define R8_SAFE_ACCESS_SIG  (*((PUINT8V)0x40001040))  // WO, safe accessing sign register, must write SAFE_ACCESS_SIG1 then SAFE_ACCESS_SIG2 to enter safe accessing mode
#define  RB_SAFE_ACC_MODE   0x03                      // RO, current safe accessing mode: 11=safe/unlocked (SAM), other=locked (00..01..10..11)
#define  RB_CHIP_ID0        0x04                      // RO, chip id0
#define  RB_SAFE_ACC_ACT    0x08                      // RO, indicate safe accessing status now: 0=locked, read only, 1=safe/unlocked (SAM), write enabled
#define  RB_SAFE_ACC_TIMER  0x70                      // RO, safe accessing timer bit mask (16*clock number)
#define  RB_CHIP_ID1        0x80                      // RO, chip id1
#define SAFE_ACCESS_SIG1    0x57                      // WO: safe accessing sign value step 1
#define SAFE_ACCESS_SIG2    0xA8                      // WO: safe accessing sign value step 2
#define SAFE_ACCESS_SIG0    0x00                      // WO: safe accessing sign value for disable
#define R8_CHIP_ID          (*((PUINT8V)0x40001041))  // RF, chip ID register, always is ID_CH58*
#define R8_SAFE_ACCESS_ID   (*((PUINT8V)0x40001042))  // RF, safe accessing ID register, always 0x0C
#define R8_WDOG_COUNT       (*((PUINT8V)0x40001043))  // RW, watch-dog count, count by clock frequency Fsys/131072

/* System: global configuration register */
#define R32_GLOBAL_CONFIG   (*((PUINT32V)0x40001044)) // RW, global configuration
#define R8_RESET_STATUS     (*((PUINT8V)0x40001044))  // RO, reset status
#define  RB_RESET_FLAG      0x07                      // RO: recent reset flag
#define  RST_FLAG_SW        0x00
#define  RST_FLAG_RPOR      0x01
#define  RST_FLAG_WTR       0x02
#define  RST_FLAG_MR        0x03
//#define  RST_FLAG_GPWSM     0x04                      // RO, power on reset flag during sleep/shutdown: 0=no power on reset during sleep/shutdown, 1=power on reset occurred during sleep/shutdown
#define  RST_FLAG_GPWSM     0x05
// RB_RESET_FLAG: recent reset flag
//   000 - SR, software reset, by RB_SOFTWARE_RESET=1 @RB_WDOG_RST_EN=0
//   001 - RPOR, real power on reset
//   010 - WTR, watch-dog timer-out reset
//   011 - MR, external manual reset by RST pin input low
//   101 - GRWSM, global reset by waking under shutdown mode
//   1?? - LRW, power on reset occurred during sleep
#define R8_GLOB_ROM_CFG     R8_RESET_STATUS           // RWA, flash ROM configuration, SAM
#define  RB_ROM_CODE_OFS    0x10                      // RWA, code offset address selection in Flash ROM: 0=start address 0x000000, 1=start address 0x040000
#define  RB_ROM_CTRL_EN     0x20                      // RWA, enable flash ROM control interface enable: 0=disable access, 1=enable access control register
#define  RB_ROM_DATA_WE     0x40                      // RWA, enable flash ROM data & code area being erase/write: 0=all writing protect, 1=enable data area program and erase
#define  RB_ROM_CODE_WE     0x80                      // RWA, enable flash ROM code area being erase/write: 0=code writing protect, 1=enable code area program and erase
#define R8_GLOB_CFG_INFO    (*((PUINT8V)0x40001045))  // RO, global configuration information and status
#define  RB_CFG_ROM_READ    0x01                      // RO, indicate protected status of Flash ROM code and data: 0=reading protect, 1=enable read by external programmer
#define  RB_CFG_RESET_EN    0x04                      // RO, manual reset input enable status
#define  RB_CFG_BOOT_EN     0x08                      // RO, boot-loader enable status
#define  RB_CFG_DEBUG_EN    0x10                      // RO, debug enable status
#define  RB_BOOT_LOADER     0x20                      // RO, indicate boot loader status: 0=application status (by software reset), 1=boot loader status
#define R8_RST_WDOG_CTRL    (*((PUINT8V)0x40001046))  // RWA, reset and watch-dog control, SAM
#define  RB_SOFTWARE_RESET  0x01                      // WA/WZ, global software reset, high action, auto clear
#define  RB_WDOG_RST_EN     0x02                      // RWA, enable watch-dog reset if watch-dog timer overflow: 0=as timer only, 1=enable reset if timer overflow
#define  RB_WDOG_INT_EN     0x04                      // RWA, watch-dog timer overflow interrupt enable: 0=disable, 1=enable
#define  RB_WDOG_INT_FLAG   0x10                      // RW1, watch-dog timer overflow interrupt flag, cleared by RW1 or reload watch-dog count or __SEV(Send-Event)
#define R8_GLOB_RESET_KEEP  (*((PUINT8V)0x40001047))  // RW, value keeper during global reset

/* System: clock configuration register */
#define R32_CLOCK_CONFIG    (*((PUINT32V)0x40001008)) // RWA, clock configuration, SAM
#define R16_CLK_SYS_CFG     (*((PUINT16V)0x40001008)) // RWA, system clock configuration, SAM
#define  RB_CLK_PLL_DIV     0x1F                      // RWA, output clock divider from PLL or CK32M
#define  RB_CLK_SYS_MOD     0xC0                      // RWA, system clock source mode: 00=divided from 32MHz, 01=divided from PLL-480MHz, 10=directly from 32MHz, 11=directly from 32KHz
#define R8_HFCK_PWR_CTRL    (*((PUINT8V)0x4000100A))  // RWA, high frequency clock module power control, SAM
#define  RB_CLK_XT32M_PON   0x04                      // RWA, external 32MHz oscillator power control: 0=power down, 1-power on
#define  RB_CLK_XT32M_KEEP  0x08                      // RWA, external 32MHz oscillator power keep under halt mode: 0=auto stop, 1=keep running
#define  RB_CLK_PLL_PON     0x10                      // RWA, PLL power control: 0=power down, 1-power on
// Fck32k = RB_CLK_OSC32K_XT ? XT_32KHz : RC_32KHz
// Fpll = XT_32MHz * 15 = 480MHz
// Fsys = RB_CLK_SYS_MOD==3 ? Fck32k : ( ( RB_CLK_SYS_MOD[0] ? Fpll : XT_32MHz ) / RB_CLK_PLL_DIV )
// default: Fsys = XT_32MHz / RB_CLK_PLL_DIV = 32MHz / 5 = 6.4MHz
//   range: 32KHz, 2MHz~10MHz, 15MHz~80MHz

/* System: sleep control register */
#define R32_SLEEP_CONTROL   (*((PUINT32V)0x4000100C)) // RWA, sleep control, SAM
#define R8_SLP_CLK_OFF0     (*((PUINT8V)0x4000100C))  // RWA, sleep clock off control byte 0, SAM
#define  RB_SLP_CLK_TMR0    0x01                      // RWA, close TMR0 clock
#define  RB_SLP_CLK_TMR1    0x02                      // RWA, close TMR1 clock
#define  RB_SLP_CLK_TMR2    0x04                      // RWA, close TMR2 clock
#define  RB_SLP_CLK_TMR3    0x08                      // RWA, close TMR3 clock
#define  RB_SLP_CLK_UART0   0x10                      // RWA, close UART0 clock
#define  RB_SLP_CLK_UART1   0x20                      // RWA, close UART1 clock
#define  RB_SLP_CLK_UART2   0x40                      // RWA, close UART2 clock
#define  RB_SLP_CLK_UART3   0x80                      // RWA, close UART3 clock
#define R8_SLP_CLK_OFF1     (*((PUINT8V)0x4000100D))  // RWA, sleep clock off control byte 1, SAM
#define  RB_SLP_CLK_SPI0    0x01                      // RWA, close SPI0 clock
#define  RB_SLP_CLK_SPI1    0x02                      // RWA, close SPI1 clock
#define  RB_SLP_CLK_PWMX    0x04                      // RWA, close PWMx clock
#define  RB_SLP_CLK_I2C     0x08                      // RWA, close I2C clock
#define  RB_SLP_CLK_USB     0x10                      // RWA, close USB clock
#define  RB_SLP_CLK_USB2    0x20                      // RWA, close USB2 clock
#define  RB_SLP_CLK_LCD     0x40                      // RWA, close LCD clock
#define  RB_SLP_CLK_BLE     0x80                      // RWA, close BLE clock
#define R8_SLP_WAKE_CTRL    (*((PUINT8V)0x4000100E))  // RWA, wake control, SAM
#define  RB_SLP_USB_WAKE    0x01                      // RWA, enable USB waking
#define  RB_SLP_USB2_WAKE   0x02                      // RWA, enable USB2 waking
//#define  RB_SLP_BLE_WAKE    0x04                      // RWA, enable BLE waking
#define  RB_GPIO_EDGE_WAKE  0x04                      // RWA, enable GPIO edge waking,no mater rising or falling
#define  RB_SLP_RTC_WAKE    0x08                      // RWA, enable RTC waking
#define  RB_SLP_GPIO_WAKE   0x10                      // RWA, enable GPIO waking
#define  RB_SLP_BAT_WAKE    0x20                      // RWA, enable BAT waking
#define  RB_WAKE_EV_MODE    0x40                      // RWA, event wakeup mode: 0=event keep valid for long time, 1=short pulse event
#define  RB_GPIO_WAKE_MODE  0x80                      // RWA, enable GPIO wake up system 
#define R8_SLP_POWER_CTRL   (*((PUINT8V)0x4000100F))  // RWA, peripherals power down control, SAM
#define  RB_WAKE_DLY_MOD    0x03                      // RWA, wakeup delay time selection
// RB_WAKE_DLY_MOD select wakeup delay
//   00: long time, 3590 cycles+TSUHSE
//   01: short time, 520 cycles+TSUHSE
//   10: shorter time, 70 cycles+TSUHSE
//   11: no delay, 8 cycles+TSUHSE
//#define  RB_SLP_USB_PWR_DN  0x01                      // RWA, enable USB power down
//#define  RB_SLP_BLE_PWR_DN  0x04                      // RWA, enable BLE power down
#define  RB_SLP_CLK_RAMX    0x10                      // RWA, close main SRAM clock
#define  RB_SLP_CLK_RAM2K   0x20                      // RWA, close retention 2KB SRAM clock
#define  RB_RAM_RET_LV      0x40                      // RWA, SRAM retention voltage selection: 0=normal, 1=low voltage for low power

/* System: I/O pin configuration register */
#define R32_PIN_CONFIG      (*((PUINT32V)0x40001018)) // RW, I/O pin configuration
#define R16_PIN_ALTERNATE   (*((PUINT16V)0x40001018)) // RW, function pin alternate configuration
#define  RB_PIN_TMR0        0x01                      // RW, TMR0 alternate pin enable: 0=TMR0/PWM0/CAP0 on PA[9], 1=TMR0_/PWM0_/CAP0_ on PB[23]
#define  RB_PIN_TMR1        0x02                      // RW, TMR1 alternate pin enable: 0=TMR1/PWM1/CAP1 on PA[10], 1=TMR1_/PWM1_/CAP1_ on PB[10]
#define  RB_PIN_TMR2        0x04                      // RW, TMR2 alternate pin enable: 0=TMR2/PWM2/CAP2 on PA[11], 1=TMR2_/PWM2_/CAP2_ on PB[11]
#define  RB_PIN_TMR3        0x08                      // RW, TMR3 alternate pin enable: 0=TMR3/PWM3/CAP3 on PA[2], 1=TMR3_/PWM3_/CAP3_ on PB[22]
#define  RB_PIN_UART0       0x10                      // RW, RXD0/TXD0 alternate pin enable: 0=RXD0/TXD0 on PB[4]/PB[7], 1=RXD0_/TXD0_ on PA[15]/PA[14]
#define  RB_PIN_UART1       0x20                      // RW, RXD1/TXD1 alternate pin enable: 0=RXD1/TXD1 on PA[8]/PA[9], 1=RXD1_/TXD1_ on PB[12]/PB[13]
#define  RB_PIN_UART2       0x40                      // RW, RXD2/TXD2 alternate pin enable: 0=RXD2/TXD2 on PA[6]/PA[7], 1=RXD2_/TXD2_ on PB[22]/PB[23]
#define  RB_PIN_UART3       0x80                      // RW, RXD3/TXD3 alternate pin enable: 0=RXD3/TXD3 on PA[4]/PA[5], 1=RXD3_/TXD3_ on PB[20]/PB[21]
#define  RB_PIN_SPI0        0x100                     // RW, SCS/SCK0/MOSI/MISO alternate pin enable: 0=SCS/SCK0/MOSI/MISO on PA[12]/PA[13]/PA[14]/PA[15], 1=SCS_/SCK0_/MOSI_/MISO_ on PB[12]/PB[13]/PB[14]/PB[15]
#define  RB_PIN_PWMX        0x400                     // RW, PWM4/PWM5/PWM7/PWM8/PWM9 alternate pin enable: 0=PWM4/5/7/8/9 on PA[12]/PA[13]/PB[4]/PB[6]/PB[7], 1=PWM4/5/7/8/9 on PA[6]/PA[7]/PB[1]/PB[2]/P[3]
#define  RB_PIN_I2C         0x800                     // RW, SCL/SDA alternate pin enable: 0=SCL/SDA on PB[13]/PB[12], 1=SCL_/SDA_ on PB[21]/PB[20]
#define  RB_PIN_MODEM       0x1000                    // RW, DSR/DTR alternate pin enable: 0=DSR/DTR on PB[1]/PB[5], 1=DSR_/DTR_ on PB[14]/PB[15]
#define  RB_PIN_INTX        0x2000                    // RW, interrupt INT24/INT25 alternate pin enable: 0=INT24/INT25 on PB[8]/PB[9], 1=INT24_/INT25_ on PB[22]/PB[23]
#define  RB_PIN_U0_INV      0x4000                    // RW, RXD0/RXD0_/TXD0/TXD0_ invert input/output enable: 0=normal input/output, 1=RXD invert input, TXD invert output
#define  RB_RF_ANT_SW_EN    0x8000                    // RW, RF antenna switch control output enable: 0=disable output, 1=output on PB[16]/PB[17]/PB[18]/PB[19]/PB[20]/PB[21]
#define R16_PIN_ANALOG_IE   (*((PUINT16V)0x4000101A)) // RW, analog pin enable and digital input disable
#define  RB_PIN_ADC8_9_IE   0x01                      // RW, ADC/TouchKey channel 9/8 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC6_7_IE   0x02                      // RW, ADC/TouchKey channel 7/6 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC10_IE    0x04                      // RW, ADC/TouchKey channel 10 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC11_IE    0x08                      // RW, ADC/TouchKey channel 11 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_USB2_DP_PU  0x10                      // RW, USB2 UDP internal pullup resistance enable: 0=enable/disable by RB_UC_DEV_PU_EN, 1=enable pullup, replace RB_UC_DEV_PU_EN under sleep mode
#define  RB_PIN_USB2_IE     0x20                      // RW, USB2 analog I/O enable: 0=analog I/O disable, 1=analog I/O enable
#define  RB_PIN_USB_DP_PU   0x40                      // RW, USB UDP internal pullup resistance enable: 0=enable/disable by RB_UC_DEV_PU_EN, 1=enable pullup, replace RB_UC_DEV_PU_EN under sleep mode
#define  RB_PIN_USB_IE      0x80                      // RW, USB analog I/O enable: 0=analog I/O disable, 1=analog I/O enable
#define  RB_PIN_ADC0_IE     0x0200                    // RW, ADC/TouchKey channel 0 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC1_IE     0x0400                    // RW, ADC/TouchKey channel 1 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC12_IE    0x0800                    // RW, ADC/TouchKey channel 12 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC13_IE    0x1000                    // RW, ADC/TouchKey channel 13 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_XT32K_IE    0x2000                    // RW, external 32KHz oscillator digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC2_3_IE   0x4000                    // RW, ADC/TouchKey channel 2/3 digital input disable: 0=digital input enable, 1=digital input disable
#define  RB_PIN_ADC4_5_IE   0x8000                    // RW, ADC/TouchKey channel 4/5 digital input disable: 0=digital input enable, 1=digital input disable

/* System: power management register */
#define R32_POWER_MANAG     (*((PUINT32V)0x40001020)) // RWA, power management register, SAM
#define R16_POWER_PLAN      (*((PUINT16V)0x40001020)) // RWA, power plan before sleep instruction, SAM
#define  RB_PWR_XROM        0x01                      // RWA, power for flash ROM
#define  RB_PWR_RAMSYS      0x02                      // RWA, power for retention 2KB SRAM
#define  RB_PWR_CORE        0x04                      // RWA, power retention for core and base peripherals
#define  RB_PWR_EXTEND      0x08                      // RWA, power retention for USB and BLE
#define  RB_PWR_RAMUSER     0x10                      // RWA, power for main SRAM
#define  RB_PWR_SYS_EN      0x80                      // RWA, power for system
#define  RB_PWR_LDO_EN      0x0100                    // RWA, LDO enable 
#define  RB_PWR_DCDC_EN     0x0200                    // RWA, DC/DC converter enable: 0=DC/DC disable and bypass, 1=DC/DC enable
#define  RB_PWR_DCDC_PRE    0x0400                    // RWA, DC/DC converter pre-enable
#define  RB_PWR_MUST_0010   0x1000                    // RWA, must write 0010
#define  RB_PWR_MUST_0      0x2000                    // RWA, must write 0
#define  RB_XT_PRE_EN       0x4000                    // RWA, extern 32MHz HSE early wake up enable, must be used with LSI/LSE
#define  RB_PWR_PLAN_EN     0x8000                    // RWA/WZ, power plan enable, auto clear after sleep executed
#define R16_AUX_POWER_ADJ   (*((PUINT16V)0x40001022))  // RWA, aux power adjust control, SAM
#define  RB_ULPLDO_ADJ      0x0007                    // RWA, Ultra-Low-Power LDO voltage adjust
#define  RB_DCDC_CHARGE     0x0080                    // RWA, DC/DC aux charge enable

/* System: battery detector register */
#define R32_BATTERY_CTRL    (*((PUINT32V)0x40001024)) // RWA, battery voltage detector, SAM
#define R8_BAT_DET_CTRL     (*((PUINT8V)0x40001024))  // RWA, battery voltage detector control, SAM
#define  RB_BAT_DET_EN      0x01                      // RWA, battery voltage detector enable if RB_BAT_MON_EN=0
#define  RB_BAT_LOW_VTHX    0x01                      // RWA, select monitor threshold voltage if RB_BAT_MON_EN=1
#define  RB_BAT_MON_EN      0x02                      // RWA, battery voltage monitor enable under sleep mode
#define  RB_BAT_LOWER_IE    0x04                      // RWA, interrupt enable for battery lower voltage
#define  RB_BAT_LOW_IE      0x08                      // RWA, interrupt enable for battery low voltage
// request NMI interrupt if both RB_BAT_LOWER_IE and RB_BAT_LOW_IE enabled
#define R8_BAT_DET_CFG      (*((PUINT8V)0x40001025))  // RWA, battery voltage detector configuration, SAM
#define  RB_BAT_LOW_VTH     0x03                      // RWA, select detector/monitor threshold voltage of battery voltage low
#define R8_BAT_STATUS       (*((PUINT8V)0x40001026))  // RO, battery status
#define  RB_BAT_STAT_LOWER  0x01                      // RO, battery lower voltage status for detector, high action
#define  RB_BAT_STAT_LOW    0x02                      // RO, battery low voltage status for detector/monitor, high action

/* System: 32KHz oscillator control register */
#define R32_OSC32K_CTRL     (*((PUINT32V)0x4000102C)) // RWA, 32KHz oscillator control, SAM
#define R16_INT32K_TUNE     (*((PUINT16V)0x4000102C)) // RWA, internal 32KHz oscillator tune control, SAM
#define  RB_INT32K_TUNE     0x1FFF                    // RWA, internal 32KHz oscillator frequency tune
#define R8_XT32K_TUNE       (*((PUINT8V)0x4000102E))  // RWA, external 32KHz oscillator tune control, SAM
#define  RB_XT32K_I_TUNE    0x03                      // RWA, external 32KHz oscillator current tune: 00=70% current, 01=standard current, 10=140% current, 11=200% current for startup
#define  RB_RC32K_I_TUNE    0x0C                      // RWA, internal 32KHz oscillator current tune: 00=70% current, 01=standard current, 10=140% current, 11=200% current for startup
#define  RB_XT32K_C_LOAD    0xF0                      // RWA, external 32KHz oscillator load capacitor tune: Cap = RB_XT32K_C_LOAD + 12pF
#define R8_CK32K_CONFIG     (*((PUINT8V)0x4000102F))  // RWA, 32KHz oscillator configure
#define  RB_CLK_XT32K_PON   0x01                      // RWA, external 32KHz oscillator power on
#define  RB_CLK_INT32K_PON  0x02                      // RWA, internal 32KHz oscillator power on
#define  RB_CLK_OSC32K_XT   0x04                      // RWA, 32KHz oscillator source selection: 0=RC, 1=XT
#define  RB_CLK_OSC32K_FILT 0x08                      // RWA, internal 32KHz oscillator low noise mode disable: 1=enable, 0=disable
#define  RB_32K_CLK_PIN     0x80                      // RO, 32KHz oscillator clock pin status

/* System: real-time clock register */
#define R32_RTC_CTRL        (*((PUINT32V)0x40001030)) // RWA, RTC control, SAM
#define R8_RTC_FLAG_CTRL    (*((PUINT8V)0x40001030))  // RW, RTC flag and clear control
#define  RB_RTC_TMR_CLR     0x10                      // RW, set 1 to clear RTC timer action flag, auto clear
#define  RB_RTC_TRIG_CLR    0x20                      // RW, set 1 to clear RTC trigger action flag, auto clear
#define  RB_RTC_TMR_FLAG    0x40                      // RO, RTC timer action flag
#define  RB_RTC_TRIG_FLAG   0x80                      // RO, RTC trigger action flag
#define R8_RTC_MODE_CTRL    (*((PUINT8V)0x40001031))  // RWA, RTC mode control, SAM
#define  RB_RTC_TMR_MODE    0x07                      // RWA, RTC timer mode: 000=0.125S, 001=0.25S, 010=0.5S, 011=1S, 100=2S, 101=4S, 110=8S, 111=16S
#define  RB_RTC_IGNORE_B0   0x08                      // RWA, force ignore bit0 for trigger mode: 0=compare bit0, 1=ignore bit0
#define  RB_RTC_TMR_EN      0x10                      // RWA, RTC timer mode enable
#define  RB_RTC_TRIG_EN     0x20                      // RWA, RTC trigger mode enable
#define  RB_RTC_LOAD_LO     0x40                      // RWA, set 1 to load RTC count low word R32_RTC_CNT_32K, auto clear after loaded
#define  RB_RTC_LOAD_HI     0x80                      // RWA, set 1 to load RTC count high word R32_RTC_CNT_DAY, auto clear after loaded
#define R32_RTC_TRIG        (*((PUINT32V)0x40001034)) // RWA, RTC trigger value, SAM
#define R32_RTC_CNT_32K     (*((PUINT32V)0x40001038)) // RO, RTC count based 32KHz
#define R16_RTC_CNT_32K     (*((PUINT16V)0x40001038)) // RO, RTC count based 32KHz
#define R16_RTC_CNT_2S      (*((PUINT16V)0x4000103A)) // RO, RTC count based 2 second
#define R32_RTC_CNT_DAY     (*((PUINT32V)0x4000103C)) // RO, RTC count based one day, only low 14 bit

/*System: Miscellaneous Control register */
#define R32_MISC_CTRL       (*((PUINT32V)0x40001048)) // RWA, miscellaneous control register
#define R8_PLL_CONFIG       (*((PUINT8V)0x4000104B))  // RWA, PLL configuration control, SAM
#define  RB_FLASH_IO_MOD    0x80                      // RWA, flash ROM interface mode, SAM
#define  RB_PLL_CFG_DAT     0x7F                      // RWA, PLL configuration control, SAM

/* System: 32MHz oscillator control register */
#define R32_OSC32M_CTRL     (*((PUINT32V)0x4000104C)) // RWA, 32MHz oscillator control, SAM
#define R8_XT32M_TUNE       (*((PUINT8V)0x4000104E))  // RWA, external 32MHz oscillator tune control, SAM
#define  RB_XT32M_I_BIAS    0x03                      // RWA, external 32MHz oscillator bias current tune: 00=75% current, 01=standard current, 10=125% current, 11=150% current
#define  RB_XT32M_C_LOAD    0x70                      // RWA, external 32MHz oscillator load capacitor tune: Cap = RB_XT32M_C_LOAD * 2 + 10pF

/* System: oscillator frequency calibration register */
#define R32_OSC_CALIB       (*((PUINT32V)0x40001050)) // RWA, oscillator frequency calibration, SAM
#define R16_OSC_CAL_CNT     (*((PUINT16V)0x40001050)) // RO, system clock count value for 32KHz multi-cycles
#define  RB_OSC_CAL_CNT     0x3FFF                    // RO, system clock count value for 32KHz multi-cycles
#define  RB_OSC_CAL_OV_CLR  0x4000                    // RW1, indicate R8_OSC_CAL_OV_CNT not zero, set 1 to clear R8_OSC_CAL_OV_CNT
#define  RB_OSC_CAL_IF      0x8000                    // RW1, interrupt flag for oscillator capture end, set 1 to clear
#define R8_OSC_CAL_OV_CNT   (*((PUINT8V)0x40001052))  // RO, oscillator frequency calibration overflow times
#define R8_OSC_CAL_CTRL     (*((PUINT8V)0x40001053))  // RWA, oscillator frequency calibration control, SAM
#define  RB_OSC_CNT_TOTAL   0x07                      // RWA, total cycles mode for oscillator capture
// RB_OSC_CNT_TOTAL: select total cycles for oscillator capture
//    000: 1
//    001: 2
//    010: 4
//    011: 32
//    100: 64
//    101: 128
//    110: 1024
//    111: 2047
#define  RB_OSC_CNT_HALT    0x08                      // RO, calibration counter halt status: 0=counting, 1=halt for reading count value
#define  RB_OSC_CAL_IE      0x10                      // RWA, interrupt enable for oscillator capture end
#define  RB_OSC_CNT_EN      0x20                      // RWA, calibration counter enable
#define  RB_OSC_CNT_END     0x40                      // RWA, select oscillator capture end mode: 0=normal, 1=append 2 cycles

/* System: ADC and Touch-key register */
#define R32_ADC_CTRL        (*((PUINT32V)0x40001058)) // RW, ADC control
#define R8_ADC_CHANNEL      (*((PUINT8V)0x40001058))  // RW, ADC input channel selection
#define  RB_ADC_CH_INX      0x0F                      // RW, ADC input channel index
#define R8_ADC_CFG          (*((PUINT8V)0x40001059))  // RW, ADC configure
#define  RB_ADC_POWER_ON    0x01                      // RW, ADC power control: 0=power down, 1=power on
#define  RB_ADC_BUF_EN      0x02                      // RW, ADC input buffer enable 
#define  RB_ADC_DIFF_EN     0x04                      // RW, ADC input channel mode: 0=single-end, 1=differnetial
#define  RB_ADC_OFS_TEST    0x08                      // RW, enable ADC offset test mode: 0=normal mode, 1=short to test offset
#define  RB_ADC_PGA_GAIN    0x30                      // RW, set ADC input PGA gain: 00=-12dB, 01=-6dB, 10=0dB, 11=6dB
#define  RB_ADC_CLK_DIV     0xC0                      // RW, select ADC clock frequency: 00=3.2MHz, 01=8MHz, 10=5.33MHz, 11=4MHz
#define R8_ADC_CONVERT      (*((PUINT8V)0x4000105A))  // RW, ADC convert control
#define  RB_ADC_START       0x01                      // RW, ADC convert start control: 0=stop ADC convert, 1=start an ADC convert, auto clear
#define  RB_ADC_PGA_GAIN2   0x02                      // RW, ADC gain direction, must be 0 when using TS
#define  RB_ADC_SAMPLE_TIME 0x30                      // RW, ADC sample time delay,Tadc = Tsys/clk_div, 00=4*Tadc, 01=5*Tadc, 10=6*Tadc, 11=7*Tadc
#define  RB_ADC_EOC_X       0x80                      // RO, end of ADC conversion flag
#define R8_TEM_SENSOR       (*((PUINT8V)0x4000105B))  // RW, temperature sensor control
#define  RB_TEM_SEN_PWR_ON  0x80                      // RW, temperature sensor power control: 0=power down, 1=power on
#define R32_ADC_DATA        (*((PUINT32V)0x4000105C)) // RO, ADC data and status
#define R16_ADC_DATA        (*((PUINT16V)0x4000105C)) // RO, ADC data
#define  RB_ADC_DATA        0x0FFF                    // RO, ADC conversion data
#define R8_ADC_INT_FLAG     (*((PUINT8V)0x4000105E))  // RO, ADC interrupt flag register
#define  RB_ADC_IF_EOC      0x80                      // RO, ADC conversion interrupt flag: 0=free or converting, 1=end of conversion, interrupt action, auto ADC or write R8_ADC_CONVERT or write R8_TKEY_CONVERT to clear flag
#define R32_TKEY_CTRL       (*((PUINT32V)0x40001054)) // RW, Touchkey control
#define R8_TKEY_COUNT        (*((PUINT8V)0x40001054)) // RW, touch-key charge and discharge time register
#define  RB_TKEY_DISCH_CNT   0xE0                     // RW, number of touch-key discharge cycles, calculation method:  discharge time=(RB_TKEY_DISCH_CNT+1)*Tadc
#define  RB_TKEY_CHARG_CNT   0x1F                     // RW, number of touch-key charge cycles, calculation method:  charge time=(RB_TKEY_CHARG_CNT+3)*Tadc  
#define R8_TKEY_CONVERT      (*((PUINT8V)0x40001056)) // RW, touch-key detection control register
#define  RB_TKEY_START       0x01                     // RW, touch-key detect control and status, 0=stop the conversion, 1=start detecting or conversion
#define R8_TKEY_CFG          (*((PUINT8V)0x40001057)) // RW, touch-key configure register
#define  RB_TKEY_DMA_EN      0x40                     // RW, enable touch-key DMA
#define  RB_TKEY_AUTO_EN     0x20                     // RW, enable touch-key auto trigger   
#define  RB_TKEY_RAND_EN     0x10                     // RW, enable touch-key rand trigger
#define  RB_TKEY_PGA_ADJ     0x08                     // RW, select work speed of PGA which in ADC, 1=high speed, 0=nomarl speed 
#define  RB_TKEY_DRV_EN      0x04                     // RW, enable touch-key drive shielding
#define  RB_TKEY_CURRENT     0x02                     // RW, select touch-key charge current, 1=60%, 0=current rating 
#define  RB_TKEY_PWR_ON      0x01                     // RW, enable touch-key power control   
#define R32_TKEY_SEL	    (*((PUINT32V)0x4000107C)) // RW, Touchkey sel
#define  RB_TKEY_DRV_OUTEN 	0xFFFC0000                   // RW, Touchkey multi_drive shields each channel,high enable
#define R32_ADC_DMA_CTRL    (*((PUINT32V)0x40001060)) // RW, ADC DMA control
#define R8_ADC_CTRL_DMA     (*((PUINT8V)0x40001061))  // RW, ADC DMA control
#define  RB_ADC_DMA_ENABLE  0x01                      // RW, ADC DMA enable
#define  RB_ADC_DMA_LOOP    0x04                      // RW, ADC DMA address loop enable
#define  RB_ADC_IE_DMA_END  0x08                      // RW, enable interrupt for ADC DMA completion
#define  RB_ADC_IE_EOC      0x10                      // RW, enable interrupt for end of ADC conversion
#define  RB_SCAN_AUTO_TYPE  0x20                      // RW, ADC scan mode, 1=contineous trig, 0=periodicity trig
#define  RB_ADC_CONT_EN     0x40                      // RW, enable contineous conversion ADC
#define  RB_ADC_AUTO_EN     0x80                      // RW, enable auto continuing ADC for DMA
#define R8_ADC_DMA_IF       (*((PUINT8V)0x40001062))  // RW1, ADC interrupt flag
#define  RB_ADC_IF_DMA_END  0x08                      // RW1, interrupt flag for ADC DMA completion
#define  RB_ADC_IF_END_ADC  0x10                      // RW1, interrupt flag for end of ADC conversion, DMA for auto ADC or write R8_ADC_CONVERT to clear flag
#define R8_ADC_AUTO_CYCLE   (*((PUINT8V)0x40001063))  // RW, auto ADC cycle value, unit is 16 Fsys
#define R32_ADC_DMA_NOW     (*((PUINT32V)0x40001064)) // RW, ADC DMA current address
#define R16_ADC_DMA_NOW     (*((PUINT16V)0x40001064)) // RW, ADC DMA current address
#define R32_ADC_DMA_BEG     (*((PUINT32V)0x40001068)) // RW, ADC DMA begin address
#define R16_ADC_DMA_BEG     (*((PUINT16V)0x40001068)) // RW, ADC DMA begin address
#define R32_ADC_DMA_END     (*((PUINT32V)0x4000106C)) // RW, ADC DMA end address
#define R16_ADC_DMA_END     (*((PUINT16V)0x4000106C)) // RW, ADC DMA end address
#define R32_ADC_SCAN_CFG1	(*((PUINT32V)0x40001070)) // RW, ADC channel scan config 
#define  RB_ADC_SCAN_CH1     0x000F                   // RW, channel1 analog input channel select  
#define  RB_ADC_SCAN_CH2     0x00F0                   // RW, channel2 analog input channel select   
#define  RB_ADC_SCAN_CH3     0x0F00                   // RW, channel3 analog input channel select   
#define  RB_ADC_SCAN_CH4     0xF000                   // RW, channel4 analog input channel select   
#define  RB_ADC_SCAN_CH5     0x000F0000               // RW, channel5 analog input channel select       
#define  RB_ADC_SCAN_CH6     0x00F00000               // RW, channel6 analog input channel select       
#define  RB_ADC_SCAN_CH7     0x0F000000               // RW, channel7 analog input channel select       
#define  RB_ADC_SCAN_CH8     0xF0000000               // RW, channel8 analog input channel select       
#define R32_ADC_SCAN_CFG2	(*((PUINT32V)0x40001074)) // RW, ADC channel scan config 
#define  RB_ADC_SCAN_CH9         0x000F               // RW, channel9 analog input channel select   
#define  RB_ADC_SCAN_CH10        0x00F0               // RW, channel10 analog input channel select    
#define  RB_ADC_SCAN_CH11        0x0F00               // RW, channel11 analog input channel select   
#define  RB_ADC_SCAN_CH12        0xF000               // RW, channel12 analog input channel select    
#define  RB_ADC_SCAN_CH13        0x000F0000              // RW, channel13 analog input channel select   
#define  RB_ADC_SCAN_NUM         0x0F000000           // RW, ADC scan number  
#define  RB_ADC_SCAN_SEL         0x10000000           // RW, ADC scan select, 0=TKEY, 1=ADC  
#define  RB_ADC_IE_SCAN_END      0x20000000           // RW, enable ADC scan end interupt     
#define  RB_ADC_SCAN_MASK_DIS    0x40000000           // RW, disble ADC channel when sample end         
#define  RB_ADC_IF_SCAN_END      0x80000000           // RW, clear when RB_ADC_IF_SCAN_END = 1 

/* System: Flash ROM control register */
#define R32_FLASH_DATA      (*((PUINT32V)0x40001800)) // RO/WO, flash ROM data
#define R32_FLASH_CONTROL   (*((PUINT32V)0x40001804)) // RW, flash ROM control
#define R8_FLASH_DATA       (*((PUINT8V)0x40001804))  // RO/WO, flash ROM data buffer
#define R8_FLASH_SCK        (*((PUINT8V)0x40001805))  // RW, flash ROM sck time config
#define R8_FLASH_CTRL       (*((PUINT8V)0x40001806))  // RW, flash ROM access control
#define R8_FLASH_CFG        (*((PUINT8V)0x40001807))  // RW, flash ROM access config, SAM

/* System: GPIO interrupt control register */
#define R32_GPIO_INT_EN     (*((PUINT32V)0x40001090)) // RW, GPIO interrupt enable
#define R16_PA_INT_EN       (*((PUINT16V)0x40001090)) // RW, GPIO PA interrupt enable
#define R16_PB_INT_EN       (*((PUINT16V)0x40001092)) // RW, GPIO PB interrupt enable
#define R32_GPIO_INT_MODE   (*((PUINT32V)0x40001094)) // RW, GPIO interrupt mode: 0=level action, 1=edge action
#define R16_PA_INT_MODE     (*((PUINT16V)0x40001094)) // RW, GPIO PA interrupt mode: 0=level action, 1=edge action
#define R16_PB_INT_MODE     (*((PUINT16V)0x40001096)) // RW, GPIO PB interrupt mode: 0=level action, 1=edge action
#define R32_GPIO_INT_IF     (*((PUINT32V)0x4000109C)) // RW1, GPIO interrupt flag
#define R16_PA_INT_IF       (*((PUINT16V)0x4000109C)) // RW1, GPIO PA interrupt flag
#define R16_PB_INT_IF       (*((PUINT16V)0x4000109E)) // RW1, GPIO PB interrupt flag

/* GPIO PA register */
#define R32_PA_DIR          (*((PUINT32V)0x400010A0)) // RW, GPIO PA I/O direction: 0=in, 1=out
#define R8_PA_DIR_0         (*((PUINT8V)0x400010A0))  // RW, GPIO PA I/O direction byte 0
#define R8_PA_DIR_1         (*((PUINT8V)0x400010A1))  // RW, GPIO PA I/O direction byte 1
#define R32_PA_PIN          (*((PUINT32V)0x400010A4)) // RO, GPIO PA input
#define R8_PA_PIN_0         (*((PUINT8V)0x400010A4))  // RO, GPIO PA input byte 0
#define R8_PA_PIN_1         (*((PUINT8V)0x400010A5))  // RO, GPIO PA input byte 1
#define R32_PA_OUT          (*((PUINT32V)0x400010A8)) // RW, GPIO PA output
#define R8_PA_OUT_0         (*((PUINT8V)0x400010A8))  // RW, GPIO PA output byte 0
#define R8_PA_OUT_1         (*((PUINT8V)0x400010A9))  // RW, GPIO PA output byte 1
#define R32_PA_CLR          (*((PUINT32V)0x400010AC)) // WZ, GPIO PA clear output: 0=keep, 1=clear
#define R8_PA_CLR_0         (*((PUINT8V)0x400010AC))  // WZ, GPIO PA clear output byte 0
#define R8_PA_CLR_1         (*((PUINT8V)0x400010AD))  // WZ, GPIO PA clear output byte 1
#define R32_PA_PU           (*((PUINT32V)0x400010B0)) // RW, GPIO PA pullup resistance enable
#define R8_PA_PU_0          (*((PUINT8V)0x400010B0))  // RW, GPIO PA pullup resistance enable byte 0
#define R8_PA_PU_1          (*((PUINT8V)0x400010B1))  // RW, GPIO PA pullup resistance enable byte 1
#define R32_PA_PD_DRV       (*((PUINT32V)0x400010B4)) // RW, PA pulldown for input or PA driving capability for output
#define R8_PA_PD_DRV_0      (*((PUINT8V)0x400010B4))  // RW, PA pulldown for input or PA driving capability for output byte 0
#define R8_PA_PD_DRV_1      (*((PUINT8V)0x400010B5))  // RW, PA pulldown for input or PA driving capability for output byte 1
#define R32_PA_SET          (*((PUINT32V)0x400010B8)) // WZ, GPIO PA set output: 0=keep, 1=set
#define R8_PA_SET_0         (*((PUINT8V)0x400010B8))  // WZ, GPIO PA_0~7 set output: 0=keep, 1=set
#define R8_PA_SET_1         (*((PUINT8V)0x400010B9))  // WZ, GPIO PA_8~15 set output: 0=keep, 1=set
/* GPIO PB register */
#define R32_PB_DIR          (*((PUINT32V)0x400010C0)) // RW, GPIO PB I/O direction: 0=in, 1=out
#define R8_PB_DIR_0         (*((PUINT8V)0x400010C0))  // RW, GPIO PB I/O direction byte 0
#define R8_PB_DIR_1         (*((PUINT8V)0x400010C1))  // RW, GPIO PB I/O direction byte 1
#define R8_PB_DIR_2         (*((PUINT8V)0x400010C2))  // RW, GPIO PB I/O direction byte 2
#define R32_PB_PIN          (*((PUINT32V)0x400010C4)) // RO, GPIO PB input
#define R8_PB_PIN_0         (*((PUINT8V)0x400010C4))  // RO, GPIO PB input byte 0
#define R8_PB_PIN_1         (*((PUINT8V)0x400010C5))  // RO, GPIO PB input byte 1
#define R8_PB_PIN_2         (*((PUINT8V)0x400010C6))  // RO, GPIO PB input byte 2
#define R32_PB_OUT          (*((PUINT32V)0x400010C8)) // RW, GPIO PB output
#define R8_PB_OUT_0         (*((PUINT8V)0x400010C8))  // RW, GPIO PB output byte 0
#define R8_PB_OUT_1         (*((PUINT8V)0x400010C9))  // RW, GPIO PB output byte 1
#define R8_PB_OUT_2         (*((PUINT8V)0x400010CA))  // RW, GPIO PB output byte 2
#define R32_PB_CLR          (*((PUINT32V)0x400010CC)) // WZ, GPIO PB clear output: 0=keep, 1=clear
#define R8_PB_CLR_0         (*((PUINT8V)0x400010CC))  // WZ, GPIO PB clear output byte 0
#define R8_PB_CLR_1         (*((PUINT8V)0x400010CD))  // WZ, GPIO PB clear output byte 1
#define R8_PB_CLR_2         (*((PUINT8V)0x400010CE))  // WZ, GPIO PB clear output byte 2
#define R32_PB_PU           (*((PUINT32V)0x400010D0)) // RW, GPIO PB pullup resistance enable
#define R8_PB_PU_0          (*((PUINT8V)0x400010D0))  // RW, GPIO PB pullup resistance enable byte 0
#define R8_PB_PU_1          (*((PUINT8V)0x400010D1))  // RW, GPIO PB pullup resistance enable byte 1
#define R8_PB_PU_2          (*((PUINT8V)0x400010D2))  // RW, GPIO PB pullup resistance enable byte 2
#define R32_PB_PD_DRV       (*((PUINT32V)0x400010D4)) // RW, PB pulldown for input or PB driving capability for output
#define R8_PB_PD_DRV_0      (*((PUINT8V)0x400010D4))  // RW, PB pulldown for input or PB driving capability for output byte 0
#define R8_PB_PD_DRV_1      (*((PUINT8V)0x400010D5))  // RW, PB pulldown for input or PB driving capability for output byte 1
#define R8_PB_PD_DRV_2      (*((PUINT8V)0x400010D6))  // RW, PB pulldown for input or PB driving capability for output byte 2
#define R32_PB_SET          (*((PUINT32V)0x400010D8)) // WZ, GPIO PB set output: 0=keep, 1=set 
#define R8_PB_SET_0         (*((PUINT8V)0x400010D8))  // WZ, GPIO PB_0~7 set output: 0=keep, 1=set
#define R8_PB_SET_1         (*((PUINT8V)0x400010D9))  // WZ, GPIO PB_8~15 set output: 0=keep, 1=set
#define R8_PB_SET_2         (*((PUINT8V)0x400010DA))  // WZ, GPIO PB_16~23 set output: 0=keep, 1=set
/* GPIO register address offset and bit define */
#define BA_PA               ((PUINT8V)0x400010A0)     // point GPIO PA base address
#define BA_PB               ((PUINT8V)0x400010C0)     // point GPIO PB base address
#define GPIO_DIR            0x00
#define GPIO_DIR_0          0x00
#define GPIO_DIR_1          0x01
#define GPIO_DIR_2          0x02
#define GPIO_PIN            0x04
#define GPIO_PIN_0          0x04
#define GPIO_PIN_1          0x05
#define GPIO_PIN_2          0x06
#define GPIO_OUT            0x08
#define GPIO_OUT_0          0x08
#define GPIO_OUT_1          0x09
#define GPIO_OUT_2          0x0A
#define GPIO_CLR            0x0C
#define GPIO_CLR_0          0x0C
#define GPIO_CLR_1          0x0D
#define GPIO_CLR_2          0x0E
#define GPIO_PU             0x10
#define GPIO_PU_0           0x10
#define GPIO_PU_1           0x11
#define GPIO_PU_2           0x12
#define GPIO_PD_DRV         0x14
#define GPIO_PD_DRV_0       0x14
#define GPIO_PD_DRV_1       0x15
#define GPIO_PD_DRV_2       0x16

/* GPIO alias name */
#define  bAIN9              (1<<0)                    // PA0
#define  bSCK1              (1<<0)                    // PA0

#define  bAIN8              (1<<1)                    // PA1
#define  bMOSI1             (1<<1)

#define  bAIN7              (1<<2)                    // PA2
#define  bMISO1             (1<<2)
#define  bTMR3_             (1<<2)
#define  bCAP3_             bTMR3_
#define  bPWM3_             bTMR3_

#define  bAIN6              (1<<3)                    // PA3

#define  bAIN0              (1<<4)                    // PA4
#define  bRXD3              (1<<4)                    // PA4

#define  bAIN1              (1<<5)                    // PA5
#define  bTXD3              (1<<5)                    // PA5

#define  bAIN10             (1<<6)                    // PA6
#define  bRXD2              (1<<6)                    // PA6
#define  bPWM4_             (1<<6)                    // PA6

#define  bAIN11             (1<<7)                    // PA7
#define  bTXD2              (1<<7)                    // PA7
#define  bPWM5_             (1<<7)                    // PA7

#define  bAIN12             (1<<8)                    // PA8
#define  bRXD1              (1<<8)                    // PA8
#define  bTIO_              (1<<8)                    // PA8

#define  bAIN13             (1<<9)                    // PA9
#define  bTMR0              (1<<9)                    // PA9
#define  bCAP0              bTMR0
#define  bPWM0              bTMR0
#define  bTXD1              (1<<9)                    // PA9
#define  bTCK_              (1<<9)                    // PA9

#define  bX32KI             (1<<10)                   // PA10
#define  bTMR1              (1<<10)                   // PA10
#define  bCAP1              bTMR1
#define  bPWM1              bTMR1

#define  bX32KO             (1<<11)                   // PA11
#define  bTMR2              (1<<11)                   // PA11
#define  bCAP2              bTMR2
#define  bPWM2              bTMR2

#define  bAIN2              (1<<12)                   // PA12
#define  bPWM4              (1<<12)                   // PA12
#define  bSCS               (1<<12)                   // PA12

#define  bAIN3              (1<<13)                   // PA13
#define  bSCK0              (1<<13)                   // PA13
#define  bPWM5              (1<<13)                   // PA13

#define  bAIN4              (1<<14)                   // PA14
#define  bMOSI0             (1<<14)                   // PA14
#define  bTXD0_             (1<<14)                   // PA14

#define  bAIN5              (1<<15)                   // PA15
#define  bMISO0             (1<<15)                   // PA15
#define  bRXD0_             (1<<15)                   // PA15

#define  bPWM6              (1<<0)                    // PB0
#define  bCTS               (1<<0)                    // PB0

#define  bDSR               (1<<1)                    // PB1
#define  bPWM7_             (1<<1)                    // PB1

#define  bRI                (1<<2)                    // PB2
#define  bPWM8_             (1<<2)                    // PB2

#define  bDCD               (1<<3)                    // PB3
#define  bPWM9_             (1<<3)                    // PB3

#define  bPWM7              (1<<4)                    // PB4
#define  bRXD0              (1<<4)                    // PB4

#define  bDTR               (1<<5)                    // PB5

#define  bRTS               (1<<6)                    // PB6
#define  bPWM8              (1<<6)                    // PB6

#define  bTXD0              (1<<7)                    // PB7
#define  bPWM9              (1<<7)                    // PB7

#define  bUDM               (1<<10)                   // PB10
#define  bTMR1_             (1<<10)                   // PB10
#define  bCAP1_             bTMR1_
#define  bPWM1_             bTMR1_

#define  bUDP               (1<<11)                   // PB11
#define  bTMR2_             (1<<11)                   // PB11
#define  bCAP2_             bTMR2_
#define  bPWM2_             bTMR2_

#define  bU2DM              (1<<12)                   // PB12
#define  bSCS0_             (1<<12)                   // PB12
#define  bRXD1_             (1<<12)                   // PB12
#define  bSDA               (1<<12)                   // PB12

#define  bU2DP              (1<<13)                   // PB13
#define  bSCK0_             (1<<13)                   // PB13
#define  bTXD1_             (1<<13)                   // PB13
#define  bSCL               (1<<13)                   // PB13

#define  bTIO               (1<<14)                   // PB14
#define  bDSR_              (1<<14)                   // PB14
#define  bMOSI0_            (1<<14)                   // PB14
#define  bPWM10             (1<<14)                   // PB14

#define  bTCK               (1<<15)                   // PB15
#define  bMISO0_            (1<<15)                   // PB15
#define  bDTR_              (1<<15)                   // PB15

#define  bRXD3_             (1<<20)                   // PB20
#define  bSDA_              (1<<20)                   // PB20

#define  bTXD3_             (1<<21)                   // PB21
#define  bSCL_              (1<<21)                   // PB21

#define  bRXD2_             (1<<22)                   // PB22
#define  bTMR3              (1<<22)                   // PB22
#define  bCAP3              bTMR3
#define  bPWM3              bTMR3

#define  bRST               (1<<23)                   // PB23
#define  bTMR0_             (1<<23)                   // PB23
#define  bCAP0_             bTMR0_
#define  bPWM0_             bTMR0_
#define  bTXD2_             (1<<23)                   // PB23
#define  bPWM11             (1<<23)                   // PB23

/* Timer0 register */
#define R32_TMR0_CONTROL    (*((PUINT32V)0x40002000)) // RW, TMR0 control
#define R8_TMR0_CTRL_MOD    (*((PUINT8V)0x40002000))  // RW, TMR0 mode control
#define R8_TMR0_CTRL_DMA    (*((PUINT8V)0x40002001))  // RW, TMR0 DMA control
#define R8_TMR0_INTER_EN    (*((PUINT8V)0x40002002))  // RW, TMR0 interrupt enable
#define R32_TMR0_STATUS     (*((PUINT32V)0x40002004)) // RW, TMR0 status
#define R8_TMR0_INT_FLAG    (*((PUINT8V)0x40002006))  // RW1, TMR0 interrupt flag
#define R8_TMR0_FIFO_COUNT  (*((PUINT8V)0x40002007))  // RO, TMR0 FIFO count status
#define R32_TMR0_COUNT      (*((PUINT32V)0x40002008)) // RO, TMR0 current count
#define R16_TMR0_COUNT      (*((PUINT16V)0x40002008)) // RO, TMR0 current count
#define R8_TMR0_COUNT       (*((PUINT8V)0x40002008))  // RO, TMR0 current count
#define R32_TMR0_CNT_END    (*((PUINT32V)0x4000200C)) // RW, TMR0 end count value, only low 26 bit
#define R32_TMR0_FIFO       (*((PUINT32V)0x40002010)) // RO/WO, TMR0 FIFO register, only low 26 bit
#define R16_TMR0_FIFO       (*((PUINT16V)0x40002010)) // RO/WO, TMR0 FIFO register
#define R8_TMR0_FIFO        (*((PUINT8V)0x40002010))  // RO/WO, TMR0 FIFO register
#define R32_TMR0_DMA_NOW    (*((PUINT32V)0x40002014)) // RW, TMR0 DMA current address
#define R32_TMR0_DMA_BEG    (*((PUINT32V)0x40002018)) // RW, TMR0 DMA begin address
#define R32_TMR0_DMA_END    (*((PUINT32V)0x4000201C)) // RW, TMR0 DMA end address

/* Timer1 register */
#define R32_TMR1_CONTROL    (*((PUINT32V)0x40002400)) // RW, TMR1 control
#define R8_TMR1_CTRL_MOD    (*((PUINT8V)0x40002400))  // RW, TMR1 mode control
#define R8_TMR1_CTRL_DMA    (*((PUINT8V)0x40002401))  // RW, TMR1 DMA control
#define R8_TMR1_INTER_EN    (*((PUINT8V)0x40002402))  // RW, TMR1 interrupt enable
#define R32_TMR1_STATUS     (*((PUINT32V)0x40002404)) // RW, TMR1 status
#define R8_TMR1_INT_FLAG    (*((PUINT8V)0x40002406))  // RW1, TMR1 interrupt flag
#define R8_TMR1_FIFO_COUNT  (*((PUINT8V)0x40002407))  // RO, TMR1 FIFO count status
#define R32_TMR1_COUNT      (*((PUINT32V)0x40002408)) // RO, TMR1 current count
#define R16_TMR1_COUNT      (*((PUINT16V)0x40002408)) // RO, TMR1 current count
#define R8_TMR1_COUNT       (*((PUINT8V)0x40002408))  // RO, TMR1 current count
#define R32_TMR1_CNT_END    (*((PUINT32V)0x4000240C)) // RW, TMR1 end count value, only low 26 bit
#define R32_TMR1_FIFO       (*((PUINT32V)0x40002410)) // RO/WO, TMR1 FIFO register, only low 26 bit
#define R16_TMR1_FIFO       (*((PUINT16V)0x40002410)) // RO/WO, TMR1 FIFO register
#define R8_TMR1_FIFO        (*((PUINT8V)0x40002410))  // RO/WO, TMR1 FIFO register
#define R32_TMR1_DMA_NOW    (*((PUINT32V)0x40002414)) // RW, TMR1 DMA current address
#define R16_TMR1_DMA_NOW    (*((PUINT16V)0x40002414)) // RW, TMR1 DMA current address
#define R32_TMR1_DMA_BEG    (*((PUINT32V)0x40002418)) // RW, TMR1 DMA begin address
#define R16_TMR1_DMA_BEG    (*((PUINT16V)0x40002418)) // RW, TMR1 DMA begin address
#define R32_TMR1_DMA_END    (*((PUINT32V)0x4000241C)) // RW, TMR1 DMA end address
#define R16_TMR1_DMA_END    (*((PUINT16V)0x4000241C)) // RW, TMR1 DMA end address

/* Timer2 register */
#define R32_TMR2_CONTROL    (*((PUINT32V)0x40002800)) // RW, TMR2 control
#define R8_TMR2_CTRL_MOD    (*((PUINT8V)0x40002800))  // RW, TMR2 mode control
#define R8_TMR2_CTRL_DMA    (*((PUINT8V)0x40002801))  // RW, TMR2 DMA control
#define R8_TMR2_INTER_EN    (*((PUINT8V)0x40002802))  // RW, TMR2 interrupt enable
#define R32_TMR2_STATUS     (*((PUINT32V)0x40002804)) // RW, TMR2 status
#define R8_TMR2_INT_FLAG    (*((PUINT8V)0x40002806))  // RW1, TMR2 interrupt flag
#define R8_TMR2_FIFO_COUNT  (*((PUINT8V)0x40002807))  // RO, TMR2 FIFO count status
#define R32_TMR2_COUNT      (*((PUINT32V)0x40002808)) // RO, TMR2 current count
#define R16_TMR2_COUNT      (*((PUINT16V)0x40002808)) // RO, TMR2 current count
#define R8_TMR2_COUNT       (*((PUINT8V)0x40002808))  // RO, TMR2 current count
#define R32_TMR2_CNT_END    (*((PUINT32V)0x4000280C)) // RW, TMR2 end count value, only low 26 bit
#define R32_TMR2_FIFO       (*((PUINT32V)0x40002810)) // RO/WO, TMR2 FIFO register, only low 26 bit
#define R16_TMR2_FIFO       (*((PUINT16V)0x40002810)) // RO/WO, TMR2 FIFO register
#define R8_TMR2_FIFO        (*((PUINT8V)0x40002810))  // RO/WO, TMR2 FIFO register
#define R32_TMR2_DMA_NOW    (*((PUINT32V)0x40002814)) // RW, TMR2 DMA current address
#define R16_TMR2_DMA_NOW    (*((PUINT16V)0x40002814)) // RW, TMR2 DMA current address
#define R32_TMR2_DMA_BEG    (*((PUINT32V)0x40002818)) // RW, TMR2 DMA begin address
#define R16_TMR2_DMA_BEG    (*((PUINT16V)0x40002818)) // RW, TMR2 DMA begin address
#define R32_TMR2_DMA_END    (*((PUINT32V)0x4000281C)) // RW, TMR2 DMA end address
#define R16_TMR2_DMA_END    (*((PUINT16V)0x4000281C)) // RW, TMR2 DMA end address

/* Timer3 register */
#define R32_TMR3_CONTROL    (*((PUINT32V)0x40002C00)) // RW, TMR3 control
#define R8_TMR3_CTRL_MOD    (*((PUINT8V)0x40002C00))  // RW, TMR3 mode control
#define R8_TMR3_CTRL_DMA    (*((PUINT8V)0x40002C01))  // RW, TMR3 DMA control
#define R8_TMR3_INTER_EN    (*((PUINT8V)0x40002C02))  // RW, TMR3 interrupt enable
#define R32_TMR3_STATUS     (*((PUINT32V)0x40002C04)) // RW, TMR3 status
#define R8_TMR3_INT_FLAG    (*((PUINT8V)0x40002C06))  // RW1, TMR3 interrupt flag
#define R8_TMR3_FIFO_COUNT  (*((PUINT8V)0x40002C07))  // RO, TMR3 FIFO count status
#define R32_TMR3_COUNT      (*((PUINT32V)0x40002C08)) // RO, TMR3 current count
#define R16_TMR3_COUNT      (*((PUINT16V)0x40002C08)) // RO, TMR3 current count
#define R8_TMR3_COUNT       (*((PUINT8V)0x40002C08))  // RO, TMR3 current count
#define R32_TMR3_CNT_END    (*((PUINT32V)0x40002C0C)) // RW, TMR3 end count value, only low 26 bit
#define R32_TMR3_FIFO       (*((PUINT32V)0x40002C10)) // RO/WO, TMR3 FIFO register, only low 26 bit
#define R16_TMR3_FIFO       (*((PUINT16V)0x40002C10)) // RO/WO, TMR3 FIFO register
#define R8_TMR3_FIFO        (*((PUINT8V)0x40002C10))  // RO/WO, TMR3 FIFO register
#define R32_TMR3_DMA_NOW    (*((PUINT32V)0x40002C14)) // RW, TMR3 DMA current address
#define R32_TMR3_DMA_BEG    (*((PUINT32V)0x40002C18)) // RW, TMR3 DMA begin address
#define R32_TMR3_DMA_END    (*((PUINT32V)0x40002C1C)) // RW, TMR3 DMA end address

/* Timer register address offset and bit define */
#define TMR_FIFO_SIZE       8                         // timer FIFO size (depth)
#define BA_TMR0             ((PUINT8V)0x40002000)     // point TMR0 base address
#define BA_TMR1             ((PUINT8V)0x40002400)     // point TMR1 base address
#define BA_TMR2             ((PUINT8V)0x40002800)     // point TMR2 base address
#define BA_TMR3             ((PUINT8V)0x40002C00)     // point TMR3 base address
#define TMR_CTRL_MOD        0
#define  RB_TMR_MODE_IN     0x01                      // RW, timer in mode: 0=timer/PWM, 1=capture/count
#define  RB_TMR_ALL_CLEAR   0x02                      // RW, force clear timer FIFO and count
#define  RB_TMR_COUNT_EN    0x04                      // RW, timer count enable
#define  RB_TMR_OUT_EN      0x08                      // RW, timer output enable
#define  RB_TMR_OUT_POLAR   0x10                      // RW, timer PWM output polarity: 0=default low and high action, 1=default high and low action
#define  RB_TMR_CAP_COUNT   0x10                      // RW, count sub-mode if RB_TMR_MODE_IN=1: 0=capture, 1=count
#define  RB_TMR_FREQ_13_56  0x20                      // RW, count with 13.56Mhz only s_clk >=42Mhz,only tmr0 and tmr3 new add_on
#define  RB_TMR_PWM_REPEAT  0xC0                      // RW, timer PWM repeat mode: 00=1, 01=4, 10=8, 11-16
#define  RB_TMR_CAP_EDGE    0xC0                      // RW, timer capture edge mode: 00=disable, 01=edge change, 10=fall to fall, 11-rise to rise
#define TMR_CTRL_DMA        1
#define  RB_TMR_DMA_ENABLE  0x01                      // RW, timer1/2 DMA enable
#define  RB_TMR_DMA_LOOP    0x04                      // RW, timer1/2 DMA address loop enable
#define TMR_INTER_EN        2
#define  RB_TMR_IE_CYC_END  0x01                      // RW, enable interrupt for timer capture count timeout or PWM cycle end
#define  RB_TMR_IE_DATA_ACT 0x02                      // RW, enable interrupt for timer capture input action or PWM trigger
#define  RB_TMR_IE_FIFO_HF  0x04                      // RW, enable interrupt for timer FIFO half (capture fifo >=4 or PWM fifo <=3)
#define  RB_TMR_IE_DMA_END  0x08                      // RW, enable interrupt for timer1/2 DMA completion
#define  RB_TMR_IE_FIFO_OV  0x10                      // RW, enable interrupt for timer FIFO overflow
#define TMR_INT_FLAG        6
#define  RB_TMR_IF_CYC_END  0x01                      // RW1, interrupt flag for timer capture count timeout or PWM cycle end
#define  RB_TMR_IF_DATA_ACT 0x02                      // RW1, interrupt flag for timer capture input action or PWM trigger
#define  RB_TMR_IF_FIFO_HF  0x04                      // RW1, interrupt flag for timer FIFO half (capture fifo >=4 or PWM fifo <=3)
#define  RB_TMR_IF_DMA_END  0x08                      // RW1, interrupt flag for timer1/2 DMA completion
#define  RB_TMR_IF_FIFO_OV  0x10                      // RW1, interrupt flag for timer FIFO overflow
#define TMR_FIFO_COUNT      7
#define TMR_COUNT           0x08
#define TMR_CNT_END         0x0C
#define TMR_FIFO            0x10
#define TMR_DMA_NOW         0x14
#define TMR_DMA_BEG         0x18
#define TMR_DMA_END         0x1C

/* UART0 register */
#define R32_UART0_CTRL      (*((PUINT32V)0x40003000)) // RW, UART0 control
#define R8_UART0_MCR        (*((PUINT8V)0x40003000))  // RW, UART0 modem control
#define R8_UART0_IER        (*((PUINT8V)0x40003001))  // RW, UART0 interrupt enable
#define R8_UART0_FCR        (*((PUINT8V)0x40003002))  // RW, UART0 FIFO control
#define R8_UART0_LCR        (*((PUINT8V)0x40003003))  // RW, UART0 line control
#define R32_UART0_STAT      (*((PUINT32V)0x40003004)) // RO, UART0 status
#define R8_UART0_IIR        (*((PUINT8V)0x40003004))  // RO, UART0 interrupt identification
#define R8_UART0_LSR        (*((PUINT8V)0x40003005))  // RO, UART0 line status
#define R8_UART0_MSR        (*((PUINT8V)0x40003006))  // RO, UART0 modem status
#define R32_UART0_FIFO      (*((PUINT32V)0x40003008)) // RW, UART0 data or FIFO port
#define R8_UART0_RBR        (*((PUINT8V)0x40003008))  // RO, UART0 receiver buffer, receiving byte
#define R8_UART0_THR        (*((PUINT8V)0x40003008))  // WO, UART0 transmitter holding, transmittal byte
#define R8_UART0_RFC        (*((PUINT8V)0x4000300A))  // RO, UART0 receiver FIFO count
#define R8_UART0_TFC        (*((PUINT8V)0x4000300B))  // RO, UART0 transmitter FIFO count
#define R32_UART0_SETUP     (*((PUINT32V)0x4000300C)) // RW, UART0 setup
#define R16_UART0_DL        (*((PUINT16V)0x4000300C)) // RW, UART0 divisor latch
#define R8_UART0_DLL        (*((PUINT8V)0x4000300C))  // RW, UART0 divisor latch LSB byte
#define R8_UART0_DLM        (*((PUINT8V)0x4000300D))  // RW, UART0 divisor latch MSB byte
#define R8_UART0_DIV        (*((PUINT8V)0x4000300E))  // RW, UART0 pre-divisor latch byte, only low 7 bit, from 1 to 0/128
#define R8_UART0_ADR        (*((PUINT8V)0x4000300F))  // RW, UART0 slave address: 0xFF=disable, other=enable

/* UART1 register */
#define R32_UART1_CTRL      (*((PUINT32V)0x40003400)) // RW, UART1 control
#define R8_UART1_MCR        (*((PUINT8V)0x40003400))  // RW, UART1 modem control
#define R8_UART1_IER        (*((PUINT8V)0x40003401))  // RW, UART1 interrupt enable
#define R8_UART1_FCR        (*((PUINT8V)0x40003402))  // RW, UART1 FIFO control
#define R8_UART1_LCR        (*((PUINT8V)0x40003403))  // RW, UART1 line control
#define R32_UART1_STAT      (*((PUINT32V)0x40003404)) // RO, UART1 status
#define R8_UART1_IIR        (*((PUINT8V)0x40003404))  // RO, UART1 interrupt identification
#define R8_UART1_LSR        (*((PUINT8V)0x40003405))  // RO, UART1 line status
#define R32_UART1_FIFO      (*((PUINT32V)0x40003408)) // RW, UART1 data or FIFO port
#define R8_UART1_RBR        (*((PUINT8V)0x40003408))  // RO, UART1 receiver buffer, receiving byte
#define R8_UART1_THR        (*((PUINT8V)0x40003408))  // WO, UART1 transmitter holding, transmittal byte
#define R8_UART1_RFC        (*((PUINT8V)0x4000340A))  // RO, UART1 receiver FIFO count
#define R8_UART1_TFC        (*((PUINT8V)0x4000340B))  // RO, UART1 transmitter FIFO count
#define R32_UART1_SETUP     (*((PUINT32V)0x4000340C)) // RW, UART1 setup
#define R16_UART1_DL        (*((PUINT16V)0x4000340C)) // RW, UART1 divisor latch
#define R8_UART1_DLL        (*((PUINT8V)0x4000340C))  // RW, UART1 divisor latch LSB byte
#define R8_UART1_DLM        (*((PUINT8V)0x4000340D))  // RW, UART1 divisor latch MSB byte
#define R8_UART1_DIV        (*((PUINT8V)0x4000340E))  // RW, UART1 pre-divisor latch byte, only low 7 bit, from 1 to 0/128

/* UART2 register */
#define R32_UART2_CTRL      (*((PUINT32V)0x40003800)) // RW, UART2 control
#define R8_UART2_MCR        (*((PUINT8V)0x40003800))  // RW, UART2 modem control
#define R8_UART2_IER        (*((PUINT8V)0x40003801))  // RW, UART2 interrupt enable
#define R8_UART2_FCR        (*((PUINT8V)0x40003802))  // RW, UART2 FIFO control
#define R8_UART2_LCR        (*((PUINT8V)0x40003803))  // RW, UART2 line control
#define R32_UART2_STAT      (*((PUINT32V)0x40003804)) // RO, UART2 status
#define R8_UART2_IIR        (*((PUINT8V)0x40003804))  // RO, UART2 interrupt identification
#define R8_UART2_LSR        (*((PUINT8V)0x40003805))  // RO, UART2 line status
#define R32_UART2_FIFO      (*((PUINT32V)0x40003808)) // RW, UART2 data or FIFO port
#define R8_UART2_RBR        (*((PUINT8V)0x40003808))  // RO, UART2 receiver buffer, receiving byte
#define R8_UART2_THR        (*((PUINT8V)0x40003808))  // WO, UART2 transmitter holding, transmittal byte
#define R8_UART2_RFC        (*((PUINT8V)0x4000380A))  // RO, UART2 receiver FIFO count
#define R8_UART2_TFC        (*((PUINT8V)0x4000380B))  // RO, UART2 transmitter FIFO count
#define R32_UART2_SETUP     (*((PUINT32V)0x4000380C)) // RW, UART2 setup
#define R16_UART2_DL        (*((PUINT16V)0x4000380C)) // RW, UART2 divisor latch
#define R8_UART2_DLL        (*((PUINT8V)0x4000380C))  // RW, UART2 divisor latch LSB byte
#define R8_UART2_DLM        (*((PUINT8V)0x4000380D))  // RW, UART2 divisor latch MSB byte
#define R8_UART2_DIV        (*((PUINT8V)0x4000380E))  // RW, UART2 pre-divisor latch byte, only low 7 bit, from 1 to 0/128

/* UART3 register */
#define R32_UART3_CTRL      (*((PUINT32V)0x40003C00)) // RW, UART3 control
#define R8_UART3_MCR        (*((PUINT8V)0x40003C00))  // RW, UART3 modem control
#define R8_UART3_IER        (*((PUINT8V)0x40003C01))  // RW, UART3 interrupt enable
#define R8_UART3_FCR        (*((PUINT8V)0x40003C02))  // RW, UART3 FIFO control
#define R8_UART3_LCR        (*((PUINT8V)0x40003C03))  // RW, UART3 line control
#define R32_UART3_STAT      (*((PUINT32V)0x40003C04)) // RO, UART3 status
#define R8_UART3_IIR        (*((PUINT8V)0x40003C04))  // RO, UART3 interrupt identification
#define R8_UART3_LSR        (*((PUINT8V)0x40003C05))  // RO, UART3 line status
#define R32_UART3_FIFO      (*((PUINT32V)0x40003C08)) // RW, UART3 data or FIFO port
#define R8_UART3_RBR        (*((PUINT8V)0x40003C08))  // RO, UART3 receiver buffer, receiving byte
#define R8_UART3_THR        (*((PUINT8V)0x40003C08))  // WO, UART3 transmitter holding, transmittal byte
#define R8_UART3_RFC        (*((PUINT8V)0x40003C0A))  // RO, UART3 receiver FIFO count
#define R8_UART3_TFC        (*((PUINT8V)0x40003C0B))  // RO, UART3 transmitter FIFO count
#define R32_UART3_SETUP     (*((PUINT32V)0x40003C0C)) // RW, UART3 setup
#define R16_UART3_DL        (*((PUINT16V)0x40003C0C)) // RW, UART3 divisor latch
#define R8_UART3_DLL        (*((PUINT8V)0x40003C0C))  // RW, UART3 divisor latch LSB byte
#define R8_UART3_DLM        (*((PUINT8V)0x40003C0D))  // RW, UART3 divisor latch MSB byte
#define R8_UART3_DIV        (*((PUINT8V)0x40003C0E))  // RW, UART3 pre-divisor latch byte, only low 7 bit, from 1 to 0/128

/* UART register address offset and bit define */
#define UART_FIFO_SIZE      8                         // UART FIFO size (depth)
#define UART_RECV_RDY_SZ    7                         // the max FIFO trigger level for UART receiver data available
#define BA_UART0            ((PUINT8V)0x40003000)     // point UART0 base address
#define BA_UART1            ((PUINT8V)0x40003400)     // point UART1 base address
#define BA_UART2            ((PUINT8V)0x40003800)     // point UART2 base address
#define BA_UART3            ((PUINT8V)0x40003C00)     // point UART3 base address
#define UART_MCR            0
#define  RB_MCR_DTR         0x01                      // RW, UART0 control DTR
#define  RB_MCR_RTS         0x02                      // RW, UART0 control RTS
#define  RB_MCR_OUT1        0x04                      // RW, UART0 control OUT1
#define  RB_MCR_OUT2        0x08                      // RW, UART control OUT2
#define  RB_MCR_INT_OE      0x08                      // RW, UART interrupt output enable
#define  RB_MCR_LOOP        0x10                      // RW, UART0 enable local loop back
#define  RB_MCR_AU_FLOW_EN  0x20                      // RW, UART0 enable autoflow control
#define  RB_MCR_TNOW        0x40                      // RW, UART0 enable TNOW output on DTR pin
#define  RB_MCR_HALF        0x80                      // RW, UART0 enable half-duplex
#define UART_IER            1
#define  RB_IER_RECV_RDY    0x01                      // RW, UART interrupt enable for receiver data ready
#define  RB_IER_THR_EMPTY   0x02                      // RW, UART interrupt enable for THR empty
#define  RB_IER_LINE_STAT   0x04                      // RW, UART interrupt enable for receiver line status
#define  RB_IER_MODEM_CHG   0x08                      // RW, UART0 interrupt enable for modem status change
#define  RB_IER_DTR_EN      0x10                      // RW, UART0 DTR/TNOW output pin enable
#define  RB_IER_RTS_EN      0x20                      // RW, UART0 RTS output pin enable
#define  RB_IER_TXD_EN      0x40                      // RW, UART TXD pin enable
#define  RB_IER_RESET       0x80                      // WZ, UART software reset control, high action, auto clear
#define UART_FCR            2
#define  RB_FCR_FIFO_EN     0x01                      // RW, UART FIFO enable
#define  RB_FCR_RX_FIFO_CLR 0x02                      // WZ, clear UART receiver FIFO, high action, auto clear
#define  RB_FCR_TX_FIFO_CLR 0x04                      // WZ, clear UART transmitter FIFO, high action, auto clear
#define  RB_FCR_FIFO_TRIG   0xC0                      // RW, UART receiver FIFO trigger level: 00-1byte, 01-2bytes, 10-4bytes, 11-7bytes
#define UART_LCR            3
#define  RB_LCR_WORD_SZ     0x03                      // RW, UART word bit length: 00-5bit, 01-6bit, 10-7bit, 11-8bit
#define  RB_LCR_STOP_BIT    0x04                      // RW, UART stop bit length: 0-1bit, 1-2bit
#define  RB_LCR_PAR_EN      0x08                      // RW, UART parity enable
#define  RB_LCR_PAR_MOD     0x30                      // RW, UART parity mode: 00-odd, 01-even, 10-mark, 11-space
#define  RB_LCR_BREAK_EN    0x40                      // RW, UART break control enable
#define  RB_LCR_DLAB        0x80                      // RW, UART reserved bit
#define  RB_LCR_GP_BIT      0x80                      // RW, UART general purpose bit
#define UART_IIR            4
#define  RB_IIR_NO_INT      0x01                      // RO, UART no interrupt flag: 0=interrupt action, 1=no interrupt
#define  RB_IIR_INT_MASK    0x0F                      // RO, UART interrupt flag bit mask
#define  RB_IIR_FIFO_ID     0xC0                      // RO, UART FIFO enabled flag
#define UART_LSR            5
#define  RB_LSR_DATA_RDY    0x01                      // RO, UART receiver fifo data ready status
#define  RB_LSR_OVER_ERR    0x02                      // RZ, UART receiver overrun error
#define  RB_LSR_PAR_ERR     0x04                      // RZ, UART receiver parity error
#define  RB_LSR_FRAME_ERR   0x08                      // RZ, UART receiver frame error
#define  RB_LSR_BREAK_ERR   0x10                      // RZ, UART receiver break error
#define  RB_LSR_TX_FIFO_EMP 0x20                      // RO, UART transmitter fifo empty status
#define  RB_LSR_TX_ALL_EMP  0x40                      // RO, UART transmitter all empty status
#define  RB_LSR_ERR_RX_FIFO 0x80                      // RO, indicate error in UART receiver fifo
#define UART_MSR            6
#define  RB_MSR_CTS_CHG     0x01                      // RZ, UART0 CTS changed status, high action
#define  RB_MSR_DSR_CHG     0x02                      // RZ, UART0 DSR changed status, high action
#define  RB_MSR_RI_CHG      0x04                      // RZ, UART0 RI changed status, high action
#define  RB_MSR_DCD_CHG     0x08                      // RZ, UART0 DCD changed status, high action
#define  RB_MSR_CTS         0x10                      // RO, UART0 CTS action status
#define  RB_MSR_DSR         0x20                      // RO, UART0 DSR action status
#define  RB_MSR_RI          0x40                      // RO, UART0 RI action status
#define  RB_MSR_DCD         0x80                      // RO, UART0 DCD action status
#define UART_RBR            8
#define UART_THR            8
#define UART_RFC            0x0A
#define UART_TFC            0x0B
#define UART_DLL            0x0C
#define UART_DLM            0x0D
#define UART_DIV            0x0E
#define UART_ADR            0x0F

/* UART interrupt identification values for IIR bits 3:0 */
#define UART_II_SLV_ADDR    0x0E                      // RO, UART0 slave address match
#define UART_II_LINE_STAT   0x06                      // RO, UART interrupt by receiver line status
#define UART_II_RECV_RDY    0x04                      // RO, UART interrupt by receiver data available
#define UART_II_RECV_TOUT   0x0C                      // RO, UART interrupt by receiver fifo timeout
#define UART_II_THR_EMPTY   0x02                      // RO, UART interrupt by THR empty
#define UART_II_MODEM_CHG   0x00                      // RO, UART0 interrupt by modem status change
#define UART_II_NO_INTER    0x01                      // RO, no UART interrupt is pending

/* SPI0 register */
#define R32_SPI0_CONTROL    (*((PUINT32V)0x40004000)) // RW, SPI0 control
#define R8_SPI0_CTRL_MOD    (*((PUINT8V)0x40004000))  // RW, SPI0 mode control
#define R8_SPI0_CTRL_CFG    (*((PUINT8V)0x40004001))  // RW, SPI0 configuration control
#define R8_SPI0_INTER_EN    (*((PUINT8V)0x40004002))  // RW, SPI0 interrupt enable
#define R8_SPI0_CLOCK_DIV   (*((PUINT8V)0x40004003))  // RW, SPI0 master clock divisor
#define R8_SPI0_SLAVE_PRE   (*((PUINT8V)0x40004003))  // RW, SPI0 slave preset value
#define R32_SPI0_STATUS     (*((PUINT32V)0x40004004)) // RW, SPI0 status
#define R8_SPI0_BUFFER      (*((PUINT8V)0x40004004))  // RO, SPI0 data buffer
#define R8_SPI0_RUN_FLAG    (*((PUINT8V)0x40004005))  // RO, SPI0 work flag
#define R8_SPI0_INT_FLAG    (*((PUINT8V)0x40004006))  // RW1, SPI0 interrupt flag
#define R8_SPI0_FIFO_COUNT  (*((PUINT8V)0x40004007))  // RO, SPI0 FIFO count status
#define R32_SPI0_TOTAL_CNT  (*((PUINT32V)0x4000400C)) // RW, SPI0 total byte count, only low 12 bit
#define R16_SPI0_TOTAL_CNT  (*((PUINT16V)0x4000400C)) // RW, SPI0 total byte count, only low 12 bit
#define R32_SPI0_FIFO       (*((PUINT32V)0x40004010)) // RW, SPI0 FIFO register
#define R8_SPI0_FIFO        (*((PUINT8V)0x40004010))  // RO/WO, SPI0 FIFO register
#define R8_SPI0_FIFO_COUNT1 (*((PUINT8V)0x40004013))  // RO, SPI0 FIFO count status
#define R32_SPI0_DMA_NOW    (*((PUINT32V)0x40004014)) // RW, SPI0 DMA current address
#define R16_SPI0_DMA_NOW    (*((PUINT16V)0x40004014)) // RW, SPI0 DMA current address
#define R32_SPI0_DMA_BEG    (*((PUINT32V)0x40004018)) // RW, SPI0 DMA begin address
#define R16_SPI0_DMA_BEG    (*((PUINT16V)0x40004018)) // RW, SPI0 DMA begin address
#define R32_SPI0_DMA_END    (*((PUINT32V)0x4000401C)) // RW, SPI0 DMA end address
#define R16_SPI0_DMA_END    (*((PUINT16V)0x4000401C)) // RW, SPI0 DMA end address

/* SPI1 register */
#define R32_SPI1_CONTROL    (*((PUINT32V)0x40004400)) // RW, SPI1 control
#define R8_SPI1_CTRL_MOD    (*((PUINT8V)0x40004400))  // RW, SPI1 mode control
#define R8_SPI1_CTRL_CFG    (*((PUINT8V)0x40004401))  // RW, SPI1 configuration control
#define R8_SPI1_INTER_EN    (*((PUINT8V)0x40004402))  // RW, SPI1 interrupt enable
#define R8_SPI1_CLOCK_DIV   (*((PUINT8V)0x40004403))  // RW, SPI1 master clock divisor
#define R32_SPI1_STATUS     (*((PUINT32V)0x40004404)) // RW, SPI1 status
#define R8_SPI1_BUFFER      (*((PUINT8V)0x40004404))  // RO, SPI1 data buffer
#define R8_SPI1_RUN_FLAG    (*((PUINT8V)0x40004405))  // RO, SPI1 work flag
#define R8_SPI1_INT_FLAG    (*((PUINT8V)0x40004406))  // RW1, SPI1 interrupt flag
#define R8_SPI1_FIFO_COUNT  (*((PUINT8V)0x40004407))  // RO, SPI1 FIFO count status
#define R32_SPI1_TOTAL_CNT  (*((PUINT32V)0x4000440C)) // RW, SPI1 total byte count, only low 12 bit
#define R16_SPI1_TOTAL_CNT  (*((PUINT16V)0x4000440C)) // RW, SPI1 total byte count, only low 12 bit
#define R32_SPI1_FIFO       (*((PUINT32V)0x40004410)) // RW, SPI1 FIFO register
#define R8_SPI1_FIFO        (*((PUINT8V)0x40004410))  // RO/WO, SPI1 FIFO register
#define R8_SPI1_FIFO_COUNT1 (*((PUINT8V)0x40004413))  // RO, SPI1 FIFO count status

/* SPI register address offset and bit define */
#define SPI_FIFO_SIZE       8                         // SPI FIFO size (depth)
#define BA_SPI0             ((PUINT8V)0x40004000)     // point SPI0 base address
#define BA_SPI1             ((PUINT8V)0x40004400)     // point SPI1 base address
#define SPI_CTRL_MOD        0
#define  RB_SPI_MODE_SLAVE  0x01                      // RW, SPI slave mode: 0=master/host, 1=slave/device
#define  RB_SPI_ALL_CLEAR   0x02                      // RW, force clear SPI FIFO and count
#define  RB_SPI_2WIRE_MOD   0x04                      // RW, SPI enable 2 wire mode for slave: 0=3wire(SCK0,MOSI,MISO), 1=2wire(SCK0,MISO=MXSX)
#define  RB_SPI_MST_SCK_MOD 0x08                      // RW, SPI master clock mode: 0=mode 0, 1=mode 3
#define  RB_SPI_SLV_CMD_MOD 0x08                      // RW, SPI slave command mode: 0=byte stream, 1=first byte command
#define  RB_SPI_FIFO_DIR    0x10                      // RW, SPI FIFO direction: 0=out(write @master mode), 1=in(read @master mode)
#define  RB_SPI_SCK_OE      0x20                      // RW, SPI SCK output enable
#define  RB_SPI_MOSI_OE     0x40                      // RW, SPI MOSI output enable
#define  RB_SPI1_SDO_OE     0x40                      // RW, SPI1 SDO output enable
#define  RB_SPI_MISO_OE     0x80                      // RW, SPI MISO output enable
#define  RB_SPI1_SDI_OE     0x80                      // RW, SPI1 SDI output enable, SPI1 enable 2 wire mode: 0=3wire(SCK1,SDO,SDI), 1=2wire(SCK1,SDI=SDX)
#define SPI_CTRL_CFG        1
#define  RB_SPI_DMA_ENABLE  0x01                      // RW, SPI DMA enable
#define  RB_SPI_DMA_LOOP    0x04                      // RW, SPI DMA address loop enable
#define  RB_SPI_AUTO_IF     0x10                      // RW, enable buffer/FIFO accessing to auto clear RB_SPI_IF_BYTE_END interrupt flag
#define  RB_SPI_BIT_ORDER   0x20                      // RW, SPI bit data order: 0=MSB first, 1=LSB first
#define  RB_SPI_MST_DLY_EN  0x40                      // RW, SPI master input delay enable
#define SPI_INTER_EN        2
#define  RB_SPI_IE_CNT_END  0x01                      // RW, enable interrupt for SPI total byte count end
#define  RB_SPI_IE_BYTE_END 0x02                      // RW, enable interrupt for SPI byte exchanged
#define  RB_SPI_IE_FIFO_HF  0x04                      // RW, enable interrupt for SPI FIFO half
#define  RB_SPI_IE_DMA_END  0x08                      // RW, enable interrupt for SPI DMA completion
#define  RB_SPI_IE_FIFO_OV  0x10                      // RW, enable interrupt for SPI FIFO overflow
#define  RB_SPI_IE_FST_BYTE 0x80                      // RW, enable interrupt for SPI slave mode first byte received
#define SPI_CLOCK_DIV       3
#define SPI_SLAVE_PRESET    3
#define SPI_BUFFER          4
#define SPI_RUN_FLAG        5
#define  RB_SPI_SLV_CMD_ACT 0x10                      // RO, SPI slave first byte / command flag
#define  RB_SPI_FIFO_READY  0x20                      // RO, SPI FIFO ready status
#define  RB_SPI_SLV_CS_LOAD 0x40                      // RO, SPI slave chip-select loading status
#define  RB_SPI_SLV_SELECT  0x80                      // RO, SPI slave selection status
#define SPI_INT_FLAG        6
#define  RB_SPI_IF_CNT_END  0x01                      // RW1, interrupt flag for SPI total byte count end
#define  RB_SPI_IF_BYTE_END 0x02                      // RW1, interrupt flag for SPI byte exchanged
#define  RB_SPI_IF_FIFO_HF  0x04                      // RW1, interrupt flag for SPI FIFO half (RB_SPI_FIFO_DIR ? >=4bytes : <4bytes)
#define  RB_SPI_IF_DMA_END  0x08                      // RW1, interrupt flag for SPI DMA completion
#define  RB_SPI_IF_FIFO_OV  0x10                      // RW1, interrupt flag for SPI FIFO overflow
#define  RB_SPI_FREE        0x40                      // RO, current SPI free status
#define  RB_SPI_IF_FST_BYTE 0x80                      // RW1, interrupt flag for SPI slave mode first byte received
#define SPI_FIFO_COUNT      7
#define SPI_TOTAL_CNT       0x0C
#define SPI_FIFO            0x10
#define SPI_DMA_NOW         0x14
#define SPI_DMA_BEG         0x18
#define SPI_DMA_END         0x1C

/* I2C register */
#define R16_I2C_CTRL1       (*((PUINT16V)0x40004800)) // RW, I2C control 1
#define R16_I2C_CTRL2       (*((PUINT16V)0x40004804)) // RW, I2C control 2
#define R16_I2C_OADDR1      (*((PUINT16V)0x40004808)) // RW, I2C own address register 1
#define R16_I2C_OADDR2      (*((PUINT16V)0x4000480C)) // RW, I2C own address register 2
#define R16_I2C_DATAR       (*((PUINT16V)0x40004810)) // RW, I2C data register
#define R16_I2C_STAR1       (*((PUINT16V)0x40004814)) // R0, I2C stauts register 1
#define R16_I2C_STAR2       (*((PUINT16V)0x40004818)) // R0, I2C status register 2
#define R8_I2C_PEC          (*((PUINT8V) 0x40004819)) // R0, I2C Packet error checking register 
#define R16_I2C_CKCFGR      (*((PUINT16V)0x4000481C)) // RW, I2C clock control register
#define R16_I2C_RTR         (*((PUINT16V)0x40004820)) // RW, I2C trise register

/* I2C register address offset and bit define */
#define BA_I2C              ((PUINT8V)0x40004800)     // point I2C base address
#define I2C_CTRL1           0
#define  RB_I2C_PE          0x0001                    // RW, Peripheral enable
#define  RB_I2C_SMBUS       0x0002                    // RW, SMBUS mode: 0=I2C mode, 1=SMBUS mode
#define  RB_I2C_SMBTYPE     0x0008                    // RW, SMBus type: 0=Device, 1=Host
#define  RB_I2C_EBARP       0x0010                    // RW, ARP enable
#define  RB_I2C_ENPEC       0x0020                    // RW, PEC ebable
#define  RB_I2C_ENGC        0x0040                    // RW, General call enable
#define  RB_I2C_NOSTRETCH   0x0080                    // RW, Clock stretching disable (Slave mode)
#define  RB_I2C_START       0x0100                    // RW, Start generation: master mode: 0=no start, 1=repeated start; slave mode: 0=no start, 1=start at bus free
#define  RB_I2C_STOP        0x0200                    // RW, Stop generation: master mode: 0=no stop, 1=stop after the current byte transfer or after the current Start condition is sent; slave mode: 0=no stop, 1=Release the SCL and SDA lines after the current byte transfer
#define  RB_I2C_ACK         0x0400                    // RW, Acknowledge enable
#define  RB_I2C_POS         0x0800                    // RW, Acknowledge/PEC Position (for data reception)
#define  RB_I2C_PEC         0x1000                    // RW, Packet error checking: 0=No PEC transfer, 1=PEC transfer (in Tx or Rx mode)
#define  RB_I2C_ALERT       0x2000                    // RW, SMBus alert: 0=Releases SMBA pin high, 1=Drives SMBA pin low.
#define  RB_I2C_SWRST       0x8000                    // RW, Software reset
#define I2C_CTRL2           4
#define  RB_I2C_FREQ        0x003F                    // RW, Peripheral clock frequency, The minimum allowed frequency is 2 MHz,the maximum frequency is 36 MHz
#define  RB_I2C_ITERREN     0x0100                    // RW, Error interrupt enable
#define  RB_I2C_ITEVTEN     0x0200                    // RW, Event interrupt enable
#define  RB_I2C_ITBUFEN     0x0400                    // RW, Buffer interrupt enable
#define I2C_OADDR1          8
#define  RB_I2C_ADD0        0x0001                    // RW, bit0 of address in 10-bit addressing mode
#define  RB_I2C_ADD7_1      0x00FE                    // RW, bit[7:1] of address
#define  RB_I2C_ADD9_8      0x0300                    // RW, bit[9:8] of address in 10-bit addressing mode
#define  RB_I2C_MUST1       0x4000                    // RW, Should always be kept at 1
#define  RB_I2C_ADDMODE     0x8000                    // RW, Addressing mode (slave mode): 0=7-bit slave address, 1=10-bit slave address
#define I2C_OADDR2          12
#define  RB_I2C_ENDUAL      0x0001                    // RW, Dual addressing mode enable
#define  RB_I2C_ADD2        0x00FE                    // RW, bit[7:1] of address2
#define I2C_DATAR           16              
#define I2C_STAR1           20
#define  RB_I2C_SB          0x0001                    // RW0, Start bit flag (Master mode)
#define  RB_I2C_ADDR        0x0002                    // RW0, Address sent (master mode)/matched (slave mode) flag
#define  RB_I2C_BTF         0x0004                    // RO, Byte transfer finished flag
#define  RB_I2C_ADD10       0x0008                    // RO, 10-bit header sent flag (Master mode)
#define  RB_I2C_STOPF       0x0010                    // RO, Stop detection flag (slave mode)
#define  RB_I2C_RxNE        0x0040                    // RO, Data register not empty flag (receivers)
#define  RB_I2C_TxE         0x0080                    // RO, Data register empty flag (transmitters)
#define  RB_I2C_BERR        0x0100                    // RW0, Bus error flag
#define  RB_I2C_ARLO        0x0200                    // RW0, Arbitration lost flag (master mode)
#define  RB_I2C_AF          0x0400                    // RW0, Acknowledge failure flag
#define  RB_I2C_OVR         0x0800                    // RW0, Overrun/Underrun flag
#define  RB_I2C_PECERR      0x1000                    // RW0, PEC Error flag in reception
#define  RB_I2C_TIMEOUT     0x4000                    // RW0, Timeout or Tlow error flag
#define  RB_I2C_SMBALERT    0x8000                    // RW0, SMBus alert flag
#define I2C_STAR2           24
#define  RB_I2C_MSL         0x0001                    // RO, Mode statu: 0=Slave mode, 1=Master mode
#define  RB_I2C_BUSY        0x0002                    // RO, Bus busy flag
#define  RB_I2C_TRA         0x0004                    // RO, Trans flag: 0=data bytes received, 1=data bytes transmitted
#define  RB_I2C_GENCALL     0x0010                    // RO, General call address (Slave mode) received flag
#define  RB_I2C_SMBDEFAULT  0x0020                    // RO, SMBus device default address (Slave mode) received flag
#define  RB_I2C_SMBHOST     0x0040                    // RO, SMBus host header (Slave mode) received flag
#define  RB_I2C_DUALF       0x0080                    // RO, Dual flag (Slave mode): 0=Received address matched with OAR1, 1=Received address matched with OAR2
#define  RB_I2C_PECX        0xFF00                    // RO, Packet error checking register
#define I2C_CKCFGR          28
#define  RB_I2C_CCR         0x0FFF                    // RW, Controls the SCL clock in Fm/Sm mode (Master mode)
#define  RB_I2C_DUTY        0x4000                    // RW, Fm mode duty cycle: 0=L/H=2, 1=L/H=16/9
#define  RB_I2C_F_S         0x8000                    // RW, I2C master mode selection: 0=standard mode, 1=fast mode
#define I2C_RTR             32
#define  RB_I2C_TRISE       0x003F                    // RW, Maximum rise time in Fm/Sm mode (Master mode)

/* PWM4/5/6/7/8/9/10/11 register */
#define R32_PWM_CONTROL     (*((PUINT32V)0x40005000)) // RW, PWM control
#define R8_PWM_OUT_EN       (*((PUINT8V)0x40005000))  // RW, PWM output enable control
#define R8_PWM_POLAR        (*((PUINT8V)0x40005001))  // RW, PWM output polarity control
#define R8_PWM_CONFIG       (*((PUINT8V)0x40005002))  // RW, PWM configuration
#define R8_PWM_CLOCK_DIV    (*((PUINT8V)0x40005003))  // RW, PWM clock divisor
#define R32_PWM4_7_DATA     (*((PUINT32V)0x40005004)) // RW, PWM4-7 data holding
#define R16_PWM4_DATA       (*((PUINT16V)0x40005004)) // RW, PWM4 16bit data holding
#define R16_PWM5_DATA       (*((PUINT16V)0x40005006)) // RW, PWM5 16bit data holding
#define R8_PWM4_DATA        (*((PUINT8V)0x40005004))  // RW, PWM4 data holding
#define R8_PWM5_DATA        (*((PUINT8V)0x40005005))  // RW, PWM5 data holding
#define R8_PWM6_DATA        (*((PUINT8V)0x40005006))  // RW, PWM6 data holding
#define R8_PWM7_DATA        (*((PUINT8V)0x40005007))  // RW, PWM7 data holding
#define R32_PWM8_11_DATA    (*((PUINT32V)0x40005008)) // RW, PWM8-11 data holding
#define R16_PWM6_DATA       (*((PUINT16V)0x40005008)) // RW, PWM6 16bit data holding
#define R16_PWM7_DATA       (*((PUINT16V)0x4000500A)) // RW, PWM7 16bit data holding
#define R8_PWM8_DATA        (*((PUINT8V)0x40005008))  // RW, PWM8 data holding
#define R8_PWM9_DATA        (*((PUINT8V)0x40005009))  // RW, PWM9 data holding
#define R8_PWM10_DATA       (*((PUINT8V)0x4000500A))  // RW, PWM10 data holding
#define R8_PWM11_DATA       (*((PUINT8V)0x4000500B))  // RW, PWM11 data holding
#define R8_PWM_INT_CTRL     (*((PUINT8V)0x4000500C))  // RW, PWM interrupt control
#define  RB_PWM_IE_CYC      0x01                      // RW, enable interrupt for PWM cycle end
#define  RB_PWM_CYC_PRE     0x02                      // RW, select PWM cycle interrupt point: 0=after count 0xFE (0x7E for 7 bits mode...), 1=after count 0xF0 (0x70 for 7 bits mode...)
#define  RB_PWM_IF_CYC      0x80                      // RW1, interrupt flag for PWM cycle end
#define R32_PWM_REG_DATA8   (*((PUINT32V)0x40005010)) // RW, PWM8-9 data register
#define R16_PWM8_DATA       (*((PUINT16V)0x40005010)) // RW, PWM8 16bit data holding
#define R16_PWM9_DATA       (*((PUINT16V)0x40005012)) // RW, PWM9 16bit data holding
#define R32_PWM_REG_CYCLE   (*((PUINT32V)0x40005014)) // RW, PWM cycle value

/* PWM4/5/6/7/8/9/10/11 register address offset and bit define */
#define BA_PWMX             ((PUINT8V)0x40005000)     // point PWM4/5/6/7/8/9/10/11 base address
#define PWM_OUT_EN          0
#define  RB_PWM4_OUT_EN     0x01                      // RW, PWM4 output enable
#define  RB_PWM5_OUT_EN     0x02                      // RW, PWM5 output enable
#define  RB_PWM6_OUT_EN     0x04                      // RW, PWM6 output enable
#define  RB_PWM7_OUT_EN     0x08                      // RW, PWM7 output enable
#define  RB_PWM8_OUT_EN     0x10                      // RW, PWM8 output enable
#define  RB_PWM9_OUT_EN     0x20                      // RW, PWM9 output enable
#define  RB_PWM10_OUT_EN    0x40                      // RW, PWM10 output enable
#define  RB_PWM11_OUT_EN    0x80                      // RW, PWM11 output enable
#define PWM_POLAR           1
#define  RB_PWM4_POLAR      0x01                      // RW, PWM4 output polarity: 0=default low and high action, 1=default high and low action
#define  RB_PWM5_POLAR      0x02                      // RW, PWM5 output polarity: 0=default low and high action, 1=default high and low action
#define  RB_PWM6_POLAR      0x04                      // RW, PWM6 output polarity: 0=default low and high action, 1=default high and low action
#define  RB_PWM7_POLAR      0x08                      // RW, PWM7 output polarity: 0=default low and high action, 1=default high and low action
#define  RB_PWM8_POLAR      0x10                      // RW, PWM8 output polarity: 0=default low and high action, 1=default high and low action
#define  RB_PWM9_POLAR      0x20                      // RW, PWM9 output polarity: 0=default low and high action, 1=default high and low action
#define  RB_PWM10_POLAR     0x40                      // RW, PWM10 output polarity: 0=default low and high action, 1=default high and low action
#define  RB_PWM11_POLAR     0x80                      // RW, PWM11 output polarity: 0=default low and high action, 1=default high and low action
#define PWM_CONFIG          2
#define  RB_PWM_CYCLE_SEL   0x01                      // RW, PWM cycle selection: 0=256/128/64/32 clocks, 1=255/127/63/31 clocks
#define  RB_PWM_STAG_ST     0x02                      // RO, PWM stagger cycle status
#define  RB_PWM_CYC_MOD     0x0C                      // RW, PWM data width mode: 00=8 bits data, 01=7 bits data, 10=6 bits data, 11=5 bits data
#define  RB_PWM4_5_STAG_EN  0x10                      // RW, PWM4/5 stagger output enable: 0=independent output, 1=stagger output
#define  RB_PWM6_7_STAG_EN  0x20                      // RW, PWM6/7 stagger output enable: 0=independent output, 1=stagger output
#define  RB_PWM8_9_STAG_EN  0x40                      // RW, PWM8/9 stagger output enable: 0=independent output, 1=stagger output
#define  RB_PWM10_11_STAG_EN 0x80                     // RW, PWM10/11 stagger output enable: 0=independent output, 1=stagger output
#define PWM_CLOCK_DIV       3
#define PWM4_DATA_HOLD      4
#define PWM5_DATA_HOLD      5
#define PWM6_DATA_HOLD      6
#define PWM7_DATA_HOLD      7
#define PWM8_DATA_HOLD      8
#define PWM9_DATA_HOLD      9
#define PWM10_DATA_HOLD     10
#define PWM11_DATA_HOLD     11
/* LED register */
#define R8_LED_CTRL_MOD	    (*((PUINT8V)0x4000F000))  // RW, LED mode config register  
#define  RB_LED_BIT_ORDER   0x01                      // RW, LED serial bit director select 0=hign first, 1=low first      
#define  RB_LED_ALL_CLEAR   0x02                      // RW, force LED FIFO/count/interupt flag clear      
#define  RB_LED_OUT_POLAR   0x04                      // RW, LED data output inverted      
#define  RB_LED_OUT_EN      0x08                      // RW, enable LED output    
#define  RB_LED_DMA_EN      0x10                      // RW, enable LED DMA and DMA interupt  
#define  RB_LED_IE_FIFO     0x20                      // RW, enable LED fifo count <=2 interupt    
#define  RB_LED_CHAN_MOD    0xC0                      // RW, LED channel mode select, 00=LED0, 01=LED0/1, 10=LED0~3, 11=LED0~7  
#define R8_LED_CLOCK_DIV	(*((PUINT8V)0x4000F001))  // RW, LED serial clk div register   
#define  RB_LED_CLOCK_DIV   0xFF                      // RW, paramater of LED output clk divider
#define R8_LED_CTRL_MOD1	(*((PUINT8V)0x4000F002))  // RW, LED mode config1 register  
#define  RB_LED_DMA_LOOP    0x01                      // RW, enable LED DMA data loop
#define  RB_IE_SEND_END     0x02                      // RW, enable DMA data transfer interupt 
#define R16_LED_STATUS	    (*((PUINT16V)0x4000F004)) // RW, LED status register    
#define  RB_LED_FIFO_COUNT  0x0F                      // RO, LED fifo current count   
#define  RB_LED_CLOCK       0x20                      // RO, LED clock current status
#define  RB_LED_IF_FIFO     0x40                      // RW1, FIFO count <=2 flag, clear RB_LED_IF_FIFO 
#define  RB_LED_LOAD_FAIL   0x80                      // R0, result of LED data loading   
#define  RB_LED_IF_DMA_END  0x0100                    // RW1, LED DMA complete flag, clear RB_LED_IF_DMA_END  
#define  RB_LED_IF_DMA_INT  0x0200                    // RW1, LED DMA transfer complete flag, clear RB_LED_IF_DMA_INT   
#define R32_LED_FIFO	    (*((PUINT32V)0x4000F008)) // RW, LED data fifo register    
#define  RB_LED_FIFO        0xFFFFFFFF                // WO, LED fifo data input
#define R16_LED_DMA_LEN	    (*((PUINT16V)0x4000F010)) // RW, LED DMA send length  
#define  RB_LED_DMA_LEN     0x0FFF                    // WO, LED DMA send length
#define R16_LED_DMA_CNT	    (*((PUINT16V)0x4000F014)) // RW, LED DMA remain count  
#define  RB_LED_DMA_CNT     0x0FFF                    // RO, LED DMA main buffer remain data cnt
#define R32_LED_DMA_BEG	    (*((PUINT32V)0x4000F018)) // RW, LED DMA begin address  
#define  RB_LED_DMA_BEG     0x01FFFC                  // RW, LED DMA start adress
#define R32_LED_DMA_CUR	    (*((PUINT32V)0x4000F01C)) // RW, LED DMA current address  
#define  RB_LED_DMA_CUR     0x01FFFC                  // RW, LED DMA now address

/* Address space define */
#define BA_CODE             ((PUINT32)0x00000000)     // point code base address
#define SZ_CODE             0x00080000                // code size
#define BA_SFR              ((PUINT32)0x40000000)     // point SFR base address
#define SZ_SFR              0x00010000                // SFR size
#define BA_RAM              ((PUINT32)0x20000000)     // point RAM base address
#define SZ_RAM              0x00008000                // RAM size

/* Special Program Space */
#define DATA_FLASH_ADDR     0x70000                   // start address of Data-Flash
#define DATA_FLASH_SIZE     0x8000                    // size of Data-Flash
#define BOOT_LOAD_ADDR      0x78000                   // start address of boot loader program
#define BOOT_LOAD_SIZE      0x6000                    // size of boot loader program
#define BOOT_LOAD_CFG       0x7E000                   // start address of configuration information for boot loader program
#define ROM_CFG_ADDR        0x7F000                   // chip configuration information address

/*----- Reference Information --------------------------------------------*/
#define ID_CH592            0x92                      // chip ID
#define ID_CH591            0x91                      // chip ID
#define ID_CH583            0x83                      // chip ID
#define ID_CH582            0x82                      // chip ID
#define ID_CH581            0x81                      // chip ID

/* Interrupt routine address and interrupt number */
#define INT_ID_TMR0         16                        // interrupt number for Timer0
#define INT_ID_GPIO_A       17                        // interrupt number for GPIO port A
#define INT_ID_GPIO_B       18                        // interrupt number for GPIO port B
#define INT_ID_SPI0         19                        // interrupt number for SPI0
#define INT_ID_BLEB         20                        // interrupt number for BLEBB
#define INT_ID_BLEL         21                        // interrupt number for BLELLE
#define INT_ID_USB          22                        // interrupt number for USB
#define INT_ID_USB2         23                        // interrupt number for USB2
#define INT_ID_TMR1         24                        // interrupt number for Timer1
#define INT_ID_TMR2         25                        // interrupt number for Timer2
#define INT_ID_UART0        26                        // interrupt number for UART0
#define INT_ID_UART1        27                        // interrupt number for UART1
#define INT_ID_RTC          28                        // interrupt number for RTC
#define INT_ID_ADC          29                        // interrupt number for ADC and TouchKey
#define INT_ID_I2C          30                        // interrupt number for I2C
#define INT_ID_PWMX_SPI1    31                        // interrupt number for PWM4~11 and SPI1
#define INT_ID_TMR3         32                        // interrupt number for Timer3
#define INT_ID_UART2        33                        // interrupt number for UART2
#define INT_ID_UART3        34                        // interrupt number for UART3
#define INT_ID_WDOG_BAT     35                        // interrupt number for Watch-Dog timer and Battery low voltage
#define INT_VEC_ENTRY_SZ    4                         // size of each interrupt vector entry
#define INT_ADDR_TMR0       (INT_ID_TMR0*INT_VEC_ENTRY_SZ)    // interrupt vector address for Timer0
#define INT_ADDR_GPIO_A     (INT_ID_GPIO_A*INT_VEC_ENTRY_SZ)  // interrupt vector address for GPIO port A
#define INT_ADDR_GPIO_B     (INT_ID_GPIO_B*INT_VEC_ENTRY_SZ)  // interrupt vector address for GPIO port B
#define INT_ADDR_SPI0       (INT_ID_SPI0*INT_VEC_ENTRY_SZ)    // interrupt vector address for SPI0
#define INT_ADDR_BLEB       (INT_ID_BLEB*INT_VEC_ENTRY_SZ)    // interrupt vector address for BLEBB
#define INT_ADDR_BLEL       (INT_ID_BLEL*INT_VEC_ENTRY_SZ)    // interrupt vector address for BLELLE
#define INT_ADDR_USB        (INT_ID_USB*INT_VEC_ENTRY_SZ)     // interrupt vector address for USB
#define INT_ADDR_USB2       (INT_ID_USB2*INT_VEC_ENTRY_SZ)    // interrupt vector address for USB2
#define INT_ADDR_TMR1       (INT_ID_TMR1*INT_VEC_ENTRY_SZ)    // interrupt vector address for Timer1
#define INT_ADDR_TMR2       (INT_ID_TMR2*INT_VEC_ENTRY_SZ)    // interrupt vector address for Timer2
#define INT_ADDR_UART0      (INT_ID_UART0*INT_VEC_ENTRY_SZ)   // interrupt vector address for UART0
#define INT_ADDR_UART1      (INT_ID_UART1*INT_VEC_ENTRY_SZ)   // interrupt vector address for UART1
#define INT_ADDR_RTC        (INT_ID_RTC*INT_VEC_ENTRY_SZ)     // interrupt vector address for RTC
#define INT_ADDR_ADC        (INT_ID_ADC*INT_VEC_ENTRY_SZ)     // interrupt vector address for ADC and TouchKey
#define INT_ADDR_I2C        (INT_ID_I2C*INT_VEC_ENTRY_SZ)     // interrupt vector address for I2C
#define INT_ADDR_PWMX_SPI1  (INT_ID_PWMX_SPI1*INT_VEC_ENTRY_SZ) // interrupt vector address for PWM4~11 and SPI1
#define INT_ADDR_TMR3       (INT_ID_TMR3*INT_VEC_ENTRY_SZ)    // interrupt vector address for Timer3
#define INT_ADDR_UART2      (INT_ID_UART2*INT_VEC_ENTRY_SZ)   // interrupt vector address for UART2
#define INT_ADDR_UART3      (INT_ID_UART3*INT_VEC_ENTRY_SZ)   // interrupt vector address for UART3
#define INT_ADDR_WDOG_BAT   (INT_ID_WDOG_BAT*INT_VEC_ENTRY_SZ) // interrupt vector address for Watch-Dog timer and Battery low voltage

#ifndef TABLE_IRQN
#define __PFIC_PRIO_BITS          2 /*!< uses 8 Bits for the Priority Levels    */
#define __Vendor_SysTickConfig    0 /*!< Set to 1 if different SysTick Config is used */
typedef enum IRQn
{
  Reset_IRQn                    = 1,
  NMI_IRQn                      = 2,      /*!<  Non Maskable Interrupt   */
  EXC_IRQn                      = 3,      /*!<  Exceptions Interrupt     */
  SysTick_IRQn                  = 12,	  /*!<  System timer Interrupt  */
  SWI_IRQn                      = 14,     /*!<  software Interrupt */
  TMR0_IRQn                     = 16,
  GPIO_A_IRQn                   = 17,
  GPIO_B_IRQn                   = 18,
  SPI0_IRQn                     = 19,
  BLEB_IRQn                     = 20,
  BLEL_IRQn                     = 21,
  USB_IRQn                      = 22,
  USB2_IRQn                     = 23,
  TMR1_IRQn                     = 24,
  TMR2_IRQn                     = 25,
  UART0_IRQn                    = 26,
  UART1_IRQn                    = 27,
  RTC_IRQn                      = 28,
  ADC_IRQn                      = 29,
  I2C_IRQn                      = 30,
  PWMX_SPI1_IRQn                = 31,
  TMR3_IRQn                     = 32,
  UART2_IRQn                    = 33,
  UART3_IRQn                    = 34,
  WDOG_BAT_IRQn                 = 35,
  LED_IRQn                      = 39
} IRQn_Type;
#endif


#ifdef __cplusplus
}
#endif

#endif  // __CH5XXSFR_H__


#ifndef __CH5XXUSBSFR_H__
#define __CH5XXUSBSFR_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/* usb addresses         
//      USB:     +8000H - 83FFH                                                    */
#define USB_BASE_ADDR              (0x40008000)
#define USB2_BASE_ADDR             (0x40008400)
#define BA_USB              ((PUINT8V)0x40008000)     // point USB base address
#define BA_USB2             ((PUINT8V)0x40008400)     // point USB2 base address

/*       USB  */
#define R32_USB_CONTROL     (*((PUINT32V)0x40008000)) // USB control & interrupt enable & device address
#define R8_USB_CTRL         (*((PUINT8V)0x40008000))  // USB base control
#define  RB_UC_HOST_MODE    0x80      // enable USB host mode: 0=device mode, 1=host mode
#define  RB_UC_LOW_SPEED    0x40      // enable USB low speed: 0=12Mbps, 1=1.5Mbps
#define  RB_UC_DEV_PU_EN    0x20      // USB device enable and internal pullup resistance enable
#define  RB_UC_SYS_CTRL1    0x20      // USB system control high bit
#define  RB_UC_SYS_CTRL0    0x10      // USB system control low bit
#define  MASK_UC_SYS_CTRL   0x30      // bit mask of USB system control
// bUC_HOST_MODE & bUC_SYS_CTRL1 & bUC_SYS_CTRL0: USB system control
//   0 00: disable USB device and disable internal pullup resistance
//   0 01: enable USB device and disable internal pullup resistance, need RB_PIN_USB_DP_PU=1 or need external pullup resistance
//   0 1x: enable USB device and enable internal pullup resistance
//   1 00: enable USB host and normal status
//   1 01: enable USB host and force UDP/UDM output SE0 state
//   1 10: enable USB host and force UDP/UDM output J state
//   1 11: enable USB host and force UDP/UDM output resume or K state
#define  RB_UC_INT_BUSY     0x08      // enable automatic responding busy for device mode or automatic pause for host mode during interrupt flag UIF_TRANSFER valid
#define  RB_UC_RESET_SIE    0x04      // force reset USB SIE, need software clear
#define  RB_UC_CLR_ALL      0x02      // force clear FIFO and count of USB
#define  RB_UC_DMA_EN       0x01      // DMA enable and DMA interrupt enable for USB

#define R8_UDEV_CTRL        (*((PUINT8V)0x40008001))  // USB device physical prot control
#define  RB_UD_PD_DIS       0x80      // disable USB UDP/UDM pulldown resistance: 0=enable pulldown, 1=disable
#define  RB_UD_DP_PIN       0x20      // ReadOnly: indicate current UDP pin level
#define  RB_UD_DM_PIN       0x10      // ReadOnly: indicate current UDM pin level
#define  RB_UD_LOW_SPEED    0x04      // enable USB physical port low speed: 0=full speed, 1=low speed
#define  RB_UD_GP_BIT       0x02      // general purpose bit
#define  RB_UD_PORT_EN      0x01      // enable USB physical port I/O: 0=disable, 1=enable

#define R8_UHOST_CTRL       R8_UDEV_CTRL              // USB host physical prot control
#define  RB_UH_PD_DIS       0x80      // disable USB UDP/UDM pulldown resistance: 0=enable pulldown, 1=disable
#define  RB_UH_DP_PIN       0x20      // ReadOnly: indicate current UDP pin level
#define  RB_UH_DM_PIN       0x10      // ReadOnly: indicate current UDM pin level
#define  RB_UH_LOW_SPEED    0x04      // enable USB port low speed: 0=full speed, 1=low speed
#define  RB_UH_BUS_RESET    0x02      // control USB bus reset: 0=normal, 1=force bus reset
#define  RB_UH_PORT_EN      0x01      // enable USB port: 0=disable, 1=enable port, automatic disabled if USB device detached

#define R8_USB_INT_EN       (*((PUINT8V)0x40008002))  // USB interrupt enable
#define  RB_UIE_DEV_NAK     0x40      // enable interrupt for NAK responded for USB device mode
// #define  RB_MOD_1_WIRE      0x20      // enable single wire mode
#define  RB_UIE_FIFO_OV     0x10      // enable interrupt for FIFO overflow
#define  RB_UIE_HST_SOF     0x08      // enable interrupt for host SOF timer action for USB host mode
#define  RB_UIE_SUSPEND     0x04      // enable interrupt for USB suspend or resume event
#define  RB_UIE_TRANSFER    0x02      // enable interrupt for USB transfer completion
#define  RB_UIE_DETECT      0x01      // enable interrupt for USB device detected event for USB host mode
#define  RB_UIE_BUS_RST     0x01      // enable interrupt for USB bus reset event for USB device mode

#define R8_USB_DEV_AD       (*((PUINT8V)0x40008003))  // USB device address
#define  RB_UDA_GP_BIT      0x80      // general purpose bit
#define  MASK_USB_ADDR      0x7F      // bit mask for USB device address

#define R32_USB_STATUS      (*((PUINT32V)0x40008004)) // USB miscellaneous status & interrupt flag & interrupt status
#define R8_USB_MIS_ST       (*((PUINT8V)0x40008005))  // USB miscellaneous status
#define  RB_UMS_SOF_PRES    0x80      // RO, indicate host SOF timer presage status
#define  RB_UMS_SOF_ACT     0x40      // RO, indicate host SOF timer action status for USB host
#define  RB_UMS_SIE_FREE    0x20      // RO, indicate USB SIE free status
#define  RB_UMS_R_FIFO_RDY  0x10      // RO, indicate USB receiving FIFO ready status (not empty)
#define  RB_UMS_BUS_RESET   0x08      // RO, indicate USB bus reset status
#define  RB_UMS_SUSPEND     0x04      // RO, indicate USB suspend status
#define  RB_UMS_DM_LEVEL    0x02      // RO, indicate UDM level saved at device attached to USB host
#define  RB_UMS_DEV_ATTACH  0x01      // RO, indicate device attached status on USB host

#define R8_USB_INT_FG       (*((PUINT8V)0x40008006))  // USB interrupt flag
#define  RB_U_IS_NAK        0x80    // RO, indicate current USB transfer is NAK received
#define  RB_U_TOG_OK        0x40    // RO, indicate current USB transfer toggle is OK
#define  RB_U_SIE_FREE      0x20    // RO, indicate USB SIE free status
#define  RB_UIF_FIFO_OV     0x10    // FIFO overflow interrupt flag for USB, direct bit address clear or write 1 to clear
#define  RB_UIF_HST_SOF     0x08    // host SOF timer interrupt flag for USB host, direct bit address clear or write 1 to clear
#define  RB_UIF_SUSPEND     0x04    // USB suspend or resume event interrupt flag, direct bit address clear or write 1 to clear
#define  RB_UIF_TRANSFER    0x02    // USB transfer completion interrupt flag, direct bit address clear or write 1 to clear
#define  RB_UIF_DETECT      0x01    // device detected event interrupt flag for USB host mode, direct bit address clear or write 1 to clear
#define  RB_UIF_BUS_RST     0x01    // bus reset event interrupt flag for USB device mode, direct bit address clear or write 1 to clear

#define R8_USB_INT_ST       (*((PUINT8V)0x40008007))  // USB interrupt status
#define  RB_UIS_SETUP_ACT   0x80      // RO, indicate SETUP token & 8 bytes setup request received for USB device mode
#define  RB_UIS_TOG_OK      0x40      // RO, indicate current USB transfer toggle is OK
#define  RB_UIS_TOKEN1      0x20      // RO, current token PID code bit 1 received for USB device mode
#define  RB_UIS_TOKEN0      0x10      // RO, current token PID code bit 0 received for USB device mode
#define  MASK_UIS_TOKEN     0x30      // RO, bit mask of current token PID code received for USB device mode
#define  UIS_TOKEN_OUT      0x00
#define  UIS_TOKEN_IN       0x20
#define  UIS_TOKEN_SETUP    0x30
// bUIS_TOKEN1 & bUIS_TOKEN0: current token PID code received for USB device mode, keep last status during SETUP token, clear RB_UIF_TRANSFER ( RB_UIF_TRANSFER from 1 to 0 ) to set free
//   00: OUT token PID received
//   01: SOF token PID received
//   10: IN token PID received
//   11: free
#define  MASK_UIS_ENDP      0x0F      // RO, bit mask of current transfer endpoint number for USB device mode
#define  MASK_UIS_H_RES     0x0F      // RO, bit mask of current transfer handshake response for USB host mode: 0000=no response, time out from device, others=handshake response PID received

#define R8_USB_RX_LEN       (*((PUINT8V)0x40008008))  // USB receiving length
#define R32_USB_BUF_MODE    (*((PUINT32V)0x4000800C)) // USB endpoint buffer mode
#define R8_UEP4_1_MOD       (*((PUINT8V)0x4000800C))  // endpoint 4/1 mode
#define  RB_UEP1_RX_EN      0x80      // enable USB endpoint 1 receiving (OUT)
#define  RB_UEP1_TX_EN      0x40      // enable USB endpoint 1 transmittal (IN)
#define  RB_UEP1_BUF_MOD    0x10      // buffer mode of USB endpoint 1
// bUEPn_RX_EN & bUEPn_TX_EN & bUEPn_BUF_MOD: USB endpoint 1/2/3 buffer mode, buffer start address is UEPn_DMA
//   0 0 x:  disable endpoint and disable buffer
//   1 0 0:  64 bytes buffer for receiving (OUT endpoint)
//   1 0 1:  dual 64 bytes buffer by toggle bit bUEP_R_TOG selection for receiving (OUT endpoint), total=128bytes
//   0 1 0:  64 bytes buffer for transmittal (IN endpoint)
//   0 1 1:  dual 64 bytes buffer by toggle bit bUEP_T_TOG selection for transmittal (IN endpoint), total=128bytes
//   1 1 0:  64 bytes buffer for receiving (OUT endpoint) + 64 bytes buffer for transmittal (IN endpoint), total=128bytes
//   1 1 1:  dual 64 bytes buffer by bUEP_R_TOG selection for receiving (OUT endpoint) + dual 64 bytes buffer by bUEP_T_TOG selection for transmittal (IN endpoint), total=256bytes
#define  RB_UEP4_RX_EN      0x08      // enable USB endpoint 4 receiving (OUT)
#define  RB_UEP4_TX_EN      0x04      // enable USB endpoint 4 transmittal (IN)
// bUEP4_RX_EN & bUEP4_TX_EN: USB endpoint 4 buffer mode, buffer start address is UEP0_DMA
//   0 0:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint)
//   1 0:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint) + 64 bytes buffer for endpoint 4 receiving (OUT endpoint), total=128bytes
//   0 1:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint) + 64 bytes buffer for endpoint 4 transmittal (IN endpoint), total=128bytes
//   1 1:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint)
//           + 64 bytes buffer for endpoint 4 receiving (OUT endpoint) + 64 bytes buffer for endpoint 4 transmittal (IN endpoint), total=192bytes

#define R8_UEP2_3_MOD       (*((PUINT8V)0x4000800D))   // endpoint 2/3 mode
#define  RB_UEP3_RX_EN      0x80      // enable USB endpoint 3 receiving (OUT)
#define  RB_UEP3_TX_EN      0x40      // enable USB endpoint 3 transmittal (IN)
#define  RB_UEP3_BUF_MOD    0x10      // buffer mode of USB endpoint 3
#define  RB_UEP2_RX_EN      0x08      // enable USB endpoint 2 receiving (OUT)
#define  RB_UEP2_TX_EN      0x04      // enable USB endpoint 2 transmittal (IN)
#define  RB_UEP2_BUF_MOD    0x01      // buffer mode of USB endpoint 2

#define R8_UEP567_MOD       (*((PUINT8V)0x4000800E))   // endpoint 5/6/7 mode
#define  RB_UEP7_RX_EN      0x20      // enable USB endpoint 7 receiving (OUT)
#define  RB_UEP7_TX_EN      0x10      // enable USB endpoint 7 transmittal (IN)
#define  RB_UEP6_RX_EN      0x08      // enable USB endpoint 6 receiving (OUT)
#define  RB_UEP6_TX_EN      0x04      // enable USB endpoint 6 transmittal (IN)
#define  RB_UEP5_RX_EN      0x02      // enable USB endpoint 5 receiving (OUT)
#define  RB_UEP5_TX_EN      0x01      // enable USB endpoint 5 transmittal (IN)
// bUEPn_RX_EN & bUEPn_TX_EN: USB endpoint 5/6/7 buffer mode, buffer start address is UEPn_DMA
//   0 0:  disable endpoint and disable buffer
//   1 0:  64 bytes buffer for receiving (OUT endpoint)
//   0 1:  64 bytes buffer for transmittal (IN endpoint)
//   1 1:  64 bytes buffer for receiving (OUT endpoint) + 64 bytes buffer for transmittal (IN endpoint), total=128bytes

#define R8_UH_EP_MOD        R8_UEP2_3_MOD             //host endpoint mode
#define  RB_UH_EP_TX_EN     0x40      // enable USB host OUT endpoint transmittal
#define  RB_UH_EP_TBUF_MOD  0x10      // buffer mode of USB host OUT endpoint
// bUH_EP_TX_EN & bUH_EP_TBUF_MOD: USB host OUT endpoint buffer mode, buffer start address is UH_TX_DMA
//   0 x:  disable endpoint and disable buffer
//   1 0:  64 bytes buffer for transmittal (OUT endpoint)
//   1 1:  dual 64 bytes buffer by toggle bit bUH_T_TOG selection for transmittal (OUT endpoint), total=128bytes
#define  RB_UH_EP_RX_EN     0x08      // enable USB host IN endpoint receiving
#define  RB_UH_EP_RBUF_MOD  0x01      // buffer mode of USB host IN endpoint
// bUH_EP_RX_EN & bUH_EP_RBUF_MOD: USB host IN endpoint buffer mode, buffer start address is UH_RX_DMA
//   0 x:  disable endpoint and disable buffer
//   1 0:  64 bytes buffer for receiving (IN endpoint)
//   1 1:  dual 64 bytes buffer by toggle bit bUH_R_TOG selection for receiving (IN endpoint), total=128bytes

#define R16_UEP0_DMA        (*((PUINT16V)0x40008010)) // endpoint 0 DMA buffer address
#define R16_UEP1_DMA        (*((PUINT16V)0x40008014)) // endpoint 1 DMA buffer address
#define R16_UEP2_DMA        (*((PUINT16V)0x40008018)) // endpoint 2 DMA buffer address
#define R16_UH_RX_DMA       R16_UEP2_DMA              // host rx endpoint buffer address
#define R16_UEP3_DMA        (*((PUINT16V)0x4000801C)) // endpoint 3 DMA buffer address
#define R16_UH_TX_DMA       R16_UEP3_DMA              // host tx endpoint buffer address
#define R16_UEP5_DMA        (*((PUINT16V)0x40008054)) // endpoint 5 DMA buffer address
#define R16_UEP6_DMA        (*((PUINT16V)0x40008058)) // endpoint 6 DMA buffer address
#define R16_UEP7_DMA        (*((PUINT16V)0x4000805C)) // endpoint 7 DMA buffer address
#define R32_USB_EP0_CTRL    (*((PUINT32V)0x40008020)) // endpoint 0 control & transmittal length
#define R8_UEP0_T_LEN       (*((PUINT8V)0x40008020))  // endpoint 0 transmittal length
#define R8_UEP0_CTRL        (*((PUINT8V)0x40008022))  // endpoint 0 control
#define R32_USB_EP1_CTRL    (*((PUINT32V)0x40008024)) // endpoint 1 control & transmittal length
#define R8_UEP1_T_LEN       (*((PUINT8V)0x40008024))  // endpoint 1 transmittal length
#define R8_UEP1_CTRL        (*((PUINT8V)0x40008026))  // endpoint 1 control
#define  RB_UEP_R_TOG       0x80      // expected data toggle flag of USB endpoint X receiving (OUT): 0=DATA0, 1=DATA1
#define  RB_UEP_T_TOG       0x40      // prepared data toggle flag of USB endpoint X transmittal (IN): 0=DATA0, 1=DATA1
#define  RB_UEP_AUTO_TOG    0x10      // enable automatic toggle after successful transfer completion on endpoint 1/2/3: 0=manual toggle, 1=automatic toggle
#define  RB_UEP_R_RES1      0x08      // handshake response type high bit for USB endpoint X receiving (OUT)
#define  RB_UEP_R_RES0      0x04      // handshake response type low bit for USB endpoint X receiving (OUT)
#define  MASK_UEP_R_RES     0x0C      // bit mask of handshake response type for USB endpoint X receiving (OUT)
#define  UEP_R_RES_ACK      0x00
#define  UEP_R_RES_TOUT     0x04
#define  UEP_R_RES_NAK      0x08
#define  UEP_R_RES_STALL    0x0C
// RB_UEP_R_RES1 & RB_UEP_R_RES0: handshake response type for USB endpoint X receiving (OUT)
//   00: ACK (ready)
//   01: no response, time out to host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)
#define  RB_UEP_T_RES1      0x02      // handshake response type high bit for USB endpoint X transmittal (IN)
#define  RB_UEP_T_RES0      0x01      // handshake response type low bit for USB endpoint X transmittal (IN)
#define  MASK_UEP_T_RES     0x03      // bit mask of handshake response type for USB endpoint X transmittal (IN)
#define  UEP_T_RES_ACK      0x00
#define  UEP_T_RES_TOUT     0x01
#define  UEP_T_RES_NAK      0x02
#define  UEP_T_RES_STALL    0x03
// bUEP_T_RES1 & bUEP_T_RES0: handshake response type for USB endpoint X transmittal (IN)
//   00: DATA0 or DATA1 then expecting ACK (ready)
//   01: DATA0 or DATA1 then expecting no response, time out from host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)

#define R8_UH_SETUP         R8_UEP1_CTRL              // host aux setup
#define  RB_UH_PRE_PID_EN   0x80      // USB host PRE PID enable for low speed device via hub
#define  RB_UH_SOF_EN       0x40      // USB host automatic SOF enable

#define R32_USB_EP2_CTRL    (*((PUINT32V)0x40008028)) // endpoint 2 control & transmittal length
#define R8_UEP2_T_LEN       (*((PUINT8V)0x40008028))  // endpoint 2 transmittal length
#define R8_UEP2_CTRL        (*((PUINT8V)0x4000802A))  // endpoint 2 control

#define R8_UH_EP_PID        R8_UEP2_T_LEN             // host endpoint and PID
#define  MASK_UH_TOKEN      0xF0      // bit mask of token PID for USB host transfer
#define  MASK_UH_ENDP       0x0F      // bit mask of endpoint number for USB host transfer

#define R8_UH_RX_CTRL       R8_UEP2_CTRL              // host receiver endpoint control
#define  RB_UH_R_TOG        0x80      // expected data toggle flag of host receiving (IN): 0=DATA0, 1=DATA1
#define  RB_UH_R_AUTO_TOG   0x10      // enable automatic toggle after successful transfer completion: 0=manual toggle, 1=automatic toggle
#define  RB_UH_R_RES        0x04      // prepared handshake response type for host receiving (IN): 0=ACK (ready), 1=no response, time out to device, for isochronous transactions

#define R32_USB_EP3_CTRL    (*((PUINT32V)0x4000802c)) // endpoint 3 control & transmittal length
#define R8_UEP3_T_LEN       (*((PUINT8V)0x4000802c))  // endpoint 3 transmittal length
#define R8_UEP3_CTRL        (*((PUINT8V)0x4000802e))  // endpoint 3 control
#define R8_UH_TX_LEN        R8_UEP3_T_LEN             // host transmittal endpoint transmittal length

#define R8_UH_TX_CTRL       R8_UEP3_CTRL              // host transmittal endpoint control
#define  RB_UH_T_TOG        0x40      // prepared data toggle flag of host transmittal (SETUP/OUT): 0=DATA0, 1=DATA1
#define  RB_UH_T_AUTO_TOG   0x10      // enable automatic toggle after successful transfer completion: 0=manual toggle, 1=automatic toggle
#define  RB_UH_T_RES        0x01      // expected handshake response type for host transmittal (SETUP/OUT): 0=ACK (ready), 1=no response, time out from device, for isochronous transactions

#define R32_USB_EP4_CTRL    (*((PUINT32V)0x40008030)) // endpoint 4 control & transmittal length
#define R8_UEP4_T_LEN       (*((PUINT8V)0x40008030))  // endpoint 4 transmittal length
#define R8_UEP4_CTRL        (*((PUINT8V)0x40008032))  // endpoint 4 control

#define R32_USB_EP5_CTRL    (*((PUINT32V)0x40008064)) // endpoint 5 control & transmittal length
#define R8_UEP5_T_LEN       (*((PUINT8V)0x40008064))  // endpoint 5 transmittal length
#define R8_UEP5_CTRL        (*((PUINT8V)0x40008066))  // endpoint 5 control

#define R32_USB_EP6_CTRL    (*((PUINT32V)0x40008068)) // endpoint 6 control & transmittal length
#define R8_UEP6_T_LEN       (*((PUINT8V)0x40008068))  // endpoint 6 transmittal length
#define R8_UEP6_CTRL        (*((PUINT8V)0x4000806A))  // endpoint 6 control

#define R32_USB_EP7_CTRL    (*((PUINT32V)0x4000806C)) // endpoint 7 control & transmittal length
#define R8_UEP7_T_LEN       (*((PUINT8V)0x4000806C))  // endpoint 7 transmittal length
#define R8_UEP7_CTRL        (*((PUINT8V)0x4000806E))  // endpoint 7 control
#define R32_EPX_MODE        (*((PUINT32V)0x40008070)) // endpoint 8-15 control 

/*      USB2  */
#define R32_USB2_CONTROL    (*((PUINT32V)0x40008400)) // USB2 control & interrupt enable & device address
#define R8_USB2_CTRL        (*((PUINT8V)0x40008400))  // USB2 base control
#define R8_U2DEV_CTRL       (*((PUINT8V)0x40008401))  // USB2 device physical prot control
#define R8_U2HOST_CTRL      R8_U2DEV_CTRL             // USB2 host physical prot control
#define R8_USB2_INT_EN      (*((PUINT8V)0x40008402))  // USB2 interrupt enable
#define R8_USB2_DEV_AD      (*((PUINT8V)0x40008403))  // USB2 device address
#define R32_USB2_STATUS     (*((PUINT32V)0x40008404)) // USB2 miscellaneous status & interrupt flag & interrupt status
#define R8_USB2_MIS_ST      (*((PUINT8V)0x40008405))  // USB2 miscellaneous status
#define R8_USB2_INT_FG      (*((PUINT8V)0x40008406))  // USB2 interrupt flag
#define R8_USB2_INT_ST      (*((PUINT8V)0x40008407))  // USB2 interrupt status
#define R8_USB2_RX_LEN      (*((PUINT8V)0x40008408))  // USB2 receiving length
#define R32_USB2_BUF_MODE   (*((PUINT32V)0x4000840C)) // USB2 endpoint buffer mode
#define R8_U2EP4_1_MOD      (*((PUINT8V)0x4000840C))  // USB2 endpoint 4/1 mode
#define R8_U2EP2_3_MOD      (*((PUINT8V)0x4000840D))  // USB2 endpoint 2/3 mode
#define R8_U2EP567_MOD      (*((PUINT8V)0x4000840E))  // USB2 endpoint 5/6/7 mode
#define R8_U2H_EP_MOD       R8_U2EP2_3_MOD            // USB2 host endpoint mode
#define R16_U2EP0_DMA       (*((PUINT16V)0x40008410)) // USB2 endpoint 0 DMA buffer address
#define R16_U2EP1_DMA       (*((PUINT16V)0x40008414)) // USB2 endpoint 1 DMA buffer address
#define R16_U2EP2_DMA       (*((PUINT16V)0x40008418)) // USB2 endpoint 2 DMA buffer address
#define R16_U2H_RX_DMA      R16_U2EP2_DMA             // USB2 host rx endpoint buffer address
#define R16_U2EP3_DMA       (*((PUINT16V)0x4000841C)) // USB2 endpoint 3 DMA buffer address
#define R16_U2H_TX_DMA      R16_U2EP3_DMA             // USB2 host tx endpoint buffer address
#define R16_U2EP5_DMA       (*((PUINT16V)0x40008454)) // USB2 endpoint 5 DMA buffer address
#define R16_U2EP6_DMA       (*((PUINT16V)0x40008458)) // USB2 endpoint 6 DMA buffer address
#define R16_U2EP7_DMA       (*((PUINT16V)0x4000845C)) // USB2 endpoint 7 DMA buffer address
#define R32_USB2_EP0_CTRL   (*((PUINT32V)0x40008420)) // USB2 endpoint 0 control & transmittal length
#define R8_U2EP0_T_LEN      (*((PUINT8V)0x40008420))  // USB2 endpoint 0 transmittal length
#define R8_U2EP0_CTRL       (*((PUINT8V)0x40008422))  // USB2 endpoint 0 control
#define R32_USB2_EP1_CTRL   (*((PUINT32V)0x40008424)) // USB2 endpoint 1 control & transmittal length
#define R8_U2EP1_T_LEN      (*((PUINT8V)0x40008424))  // USB2 endpoint 1 transmittal length
#define R8_U2EP1_CTRL       (*((PUINT8V)0x40008426))  // USB2 endpoint 1 control
#define R8_U2H_SETUP        R8_U2EP1_CTRL             // USB2 host aux setup
#define R32_USB2_EP2_CTRL   (*((PUINT32V)0x40008428)) // USB2 endpoint 2 control & transmittal length
#define R8_U2EP2_T_LEN      (*((PUINT8V)0x40008428))  // USB2 endpoint 2 transmittal length
#define R8_U2EP2_CTRL       (*((PUINT8V)0x4000842A))  // USB2 endpoint 2 control
#define R8_U2H_EP_PID       R8_U2EP2_T_LEN            // USB2 host endpoint and PID
#define R8_U2H_RX_CTRL      R8_U2EP2_CTRL             // USB2 host receiver endpoint control
#define R32_USB2_EP3_CTRL   (*((PUINT32V)0x4000842c)) // USB2 endpoint 3 control & transmittal length
#define R8_U2EP3_T_LEN      (*((PUINT8V)0x4000842c))  // USB2 endpoint 3 transmittal length
#define R8_U2EP3_CTRL       (*((PUINT8V)0x4000842e))  // USB2 endpoint 3 control
#define R8_U2H_TX_LEN       R8_U2EP3_T_LEN            // USB2 host transmittal endpoint transmittal length
#define R8_U2H_TX_CTRL      R8_U2EP3_CTRL             // USB2 host transmittal endpoint control
#define R32_USB2_EP4_CTRL   (*((PUINT32V)0x40008430)) // USB2 endpoint 4 control & transmittal length
#define R8_U2EP4_T_LEN      (*((PUINT8V)0x40008430))  // USB2 endpoint 4 transmittal length
#define R8_U2EP4_CTRL       (*((PUINT8V)0x40008432))  // USB2 endpoint 4 control
#define R32_USB2_EP5_CTRL   (*((PUINT32V)0x40008464)) // USB2 endpoint 5 control & transmittal length
#define R8_U2EP5_T_LEN      (*((PUINT8V)0x40008464))  // USB2 endpoint 5 transmittal length
#define R8_U2EP5_CTRL       (*((PUINT8V)0x40008466))  // USB2 endpoint 5 control
#define R32_USB2_EP6_CTRL   (*((PUINT32V)0x40008468)) // USB2 endpoint 6 control & transmittal length
#define R8_U2EP6_T_LEN      (*((PUINT8V)0x40008468))  // USB2 endpoint 6 transmittal length
#define R8_U2EP6_CTRL       (*((PUINT8V)0x4000846A))  // USB2 endpoint 6 control
#define R32_USB2_EP7_CTRL   (*((PUINT32V)0x4000846C)) // USB2 endpoint 7 control & transmittal length
#define R8_U2EP7_T_LEN      (*((PUINT8V)0x4000846C))  // USB2 endpoint 7 transmittal length
#define R8_U2EP7_CTRL       (*((PUINT8V)0x4000846E))  // USB2 endpoint 7 control

#ifdef __cplusplus
}
#endif

#endif //__CH5XXUSBSFR_H__


#ifndef __USB_TYPE__
#define __USB_TYPE__

#ifdef __cplusplus
extern "C" {
#endif

/*----- USB constant and structure define --------------------------------*/

/* USB PID */
#ifndef USB_PID_SETUP
#define USB_PID_NULL            0x00    /* reserved PID */
#define USB_PID_SOF             0x05
#define USB_PID_SETUP           0x0D
#define USB_PID_IN              0x09
#define USB_PID_OUT             0x01
#define USB_PID_ACK             0x02
#define USB_PID_NYET            0x06
#define USB_PID_NAK             0x0A
#define USB_PID_STALL           0x0E
#define USB_PID_DATA0           0x03
#define USB_PID_DATA1           0x0B
#define USB_PID_PRE             0x0C
#endif

/* USB standard device request code */
#ifndef USB_GET_DESCRIPTOR
#define USB_GET_STATUS          0x00
#define USB_CLEAR_FEATURE       0x01
#define USB_SET_FEATURE         0x03
#define USB_SET_ADDRESS         0x05
#define USB_GET_DESCRIPTOR      0x06
#define USB_SET_DESCRIPTOR      0x07
#define USB_GET_CONFIGURATION   0x08
#define USB_SET_CONFIGURATION   0x09
#define USB_GET_INTERFACE       0x0A
#define USB_SET_INTERFACE       0x0B
#define USB_SYNCH_FRAME         0x0C
#endif

#define DEF_STRING_DESC_LANG        0x00
#define DEF_STRING_DESC_MANU        0x01
#define DEF_STRING_DESC_PROD        0x02
#define DEF_STRING_DESC_SERN        0x03

/* USB hub class request code */
#ifndef HUB_GET_DESCRIPTOR
#define HUB_GET_STATUS          0x00
#define HUB_CLEAR_FEATURE       0x01
#define HUB_GET_STATE           0x02
#define HUB_SET_FEATURE         0x03
#define HUB_GET_DESCRIPTOR      0x06
#define HUB_SET_DESCRIPTOR      0x07
#endif

/* USB HID class request code */
#ifndef HID_GET_REPORT
#define HID_GET_REPORT          0x01
#define HID_GET_IDLE            0x02
#define HID_GET_PROTOCOL        0x03
#define HID_SET_REPORT          0x09
#define HID_SET_IDLE            0x0A
#define HID_SET_PROTOCOL        0x0B
#endif

/* USB CDC Class request code */
#ifndef CDC_GET_LINE_CODING
#define CDC_GET_LINE_CODING         0x21                                      /* This request allows the host to find out the currently configured line coding */
#define CDC_SET_LINE_CODING         0x20                                      /* Configures DTE rate, stop-bits, parity, and number-of-character */
#define CDC_SET_LINE_CTLSTE         0x22                                      /* This request generates RS-232/V.24 style control signals */
#define CDC_SEND_BREAK              0x23                                      /* Sends special carrier modulation used to specify RS-232 style break */
#endif


/* Bit define for USB request type */
#ifndef USB_REQ_TYP_MASK
#define USB_REQ_TYP_IN          0x80            /* control IN, device to host */
#define USB_REQ_TYP_OUT         0x00            /* control OUT, host to device */
#define USB_REQ_TYP_READ        0x80            /* control read, device to host */
#define USB_REQ_TYP_WRITE       0x00            /* control write, host to device */
#define USB_REQ_TYP_MASK        0x60            /* bit mask of request type */
#define USB_REQ_TYP_STANDARD    0x00
#define USB_REQ_TYP_CLASS       0x20
#define USB_REQ_TYP_VENDOR      0x40
#define USB_REQ_TYP_RESERVED    0x60
#define USB_REQ_RECIP_MASK      0x1F            /* bit mask of request recipient */
#define USB_REQ_RECIP_DEVICE    0x00
#define USB_REQ_RECIP_INTERF    0x01
#define USB_REQ_RECIP_ENDP      0x02
#define USB_REQ_RECIP_OTHER     0x03
#define USB_REQ_FEAT_REMOTE_WAKEUP  0x01
#define USB_REQ_FEAT_ENDP_HALT      0x00
#endif

/* USB request type for hub class request */
#ifndef HUB_GET_HUB_DESCRIPTOR
#define HUB_CLEAR_HUB_FEATURE   0x20
#define HUB_CLEAR_PORT_FEATURE  0x23
#define HUB_GET_BUS_STATE       0xA3
#define HUB_GET_HUB_DESCRIPTOR  0xA0
#define HUB_GET_HUB_STATUS      0xA0
#define HUB_GET_PORT_STATUS     0xA3
#define HUB_SET_HUB_DESCRIPTOR  0x20
#define HUB_SET_HUB_FEATURE     0x20
#define HUB_SET_PORT_FEATURE    0x23
#endif

/* Hub class feature selectors */
#ifndef HUB_PORT_RESET
#define HUB_C_HUB_LOCAL_POWER   0
#define HUB_C_HUB_OVER_CURRENT  1
#define HUB_PORT_CONNECTION     0
#define HUB_PORT_ENABLE         1
#define HUB_PORT_SUSPEND        2
#define HUB_PORT_OVER_CURRENT   3
#define HUB_PORT_RESET          4
#define HUB_PORT_POWER          8
#define HUB_PORT_LOW_SPEED      9
#define HUB_C_PORT_CONNECTION   16
#define HUB_C_PORT_ENABLE       17
#define HUB_C_PORT_SUSPEND      18
#define HUB_C_PORT_OVER_CURRENT 19
#define HUB_C_PORT_RESET        20
#endif

/* USB descriptor type */
#ifndef USB_DESCR_TYP_DEVICE
#define USB_DESCR_TYP_DEVICE    0x01
#define USB_DESCR_TYP_CONFIG    0x02
#define USB_DESCR_TYP_STRING    0x03
#define USB_DESCR_TYP_INTERF    0x04
#define USB_DESCR_TYP_ENDP      0x05
#define USB_DESCR_TYP_QUALIF    0x06
#define USB_DESCR_TYP_SPEED     0x07
#define USB_DESCR_TYP_OTG       0x09
#define USB_DESCR_TYP_BOS       0X0F
#define USB_DESCR_TYP_HID       0x21
#define USB_DESCR_TYP_REPORT    0x22
#define USB_DESCR_TYP_PHYSIC    0x23
#define USB_DESCR_TYP_CS_INTF   0x24
#define USB_DESCR_TYP_CS_ENDP   0x25
#define USB_DESCR_TYP_HUB       0x29
#endif

/* USB device class */
#ifndef USB_DEV_CLASS_HUB
#define USB_DEV_CLASS_RESERVED  0x00
#define USB_DEV_CLASS_AUDIO     0x01
#define USB_DEV_CLASS_COMMUNIC  0x02
#define USB_DEV_CLASS_HID       0x03
#define USB_DEV_CLASS_MONITOR   0x04
#define USB_DEV_CLASS_PHYSIC_IF 0x05
#define USB_DEV_CLASS_POWER     0x06
#define USB_DEV_CLASS_IMAGE     0x06
#define USB_DEV_CLASS_PRINTER   0x07
#define USB_DEV_CLASS_STORAGE   0x08
#define USB_DEV_CLASS_HUB       0x09
#define USB_DEV_CLASS_VEN_SPEC  0xFF
#endif

/* USB endpoint type and attributes */
#ifndef USB_ENDP_TYPE_MASK
#define USB_ENDP_DIR_MASK       0x80
#define USB_ENDP_ADDR_MASK      0x0F
#define USB_ENDP_TYPE_MASK      0x03
#define USB_ENDP_TYPE_CTRL      0x00
#define USB_ENDP_TYPE_ISOCH     0x01
#define USB_ENDP_TYPE_BULK      0x02
#define USB_ENDP_TYPE_INTER     0x03
#endif

#ifndef USB_DEVICE_ADDR
#define USB_DEVICE_ADDR         0x02    /*default addr of USB */
#endif
#ifndef DEFAULT_ENDP0_SIZE
#define DEFAULT_ENDP0_SIZE      8       /* default maximum packet size for endpoint 0 */
#endif
#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE         64      /* maximum packet size */
#endif
#ifndef USB_BO_CBW_SIZE
#define USB_BO_CBW_SIZE         0x1F    /* total length of CBW command block */
#define USB_BO_CSW_SIZE         0x0D    /* total length of CSW command state block */
#endif
#ifndef USB_BO_CBW_SIG
#define USB_BO_CBW_SIG          0x43425355  /* identification mark of CBW command block 'USBC' */
#define USB_BO_CSW_SIG          0x53425355  /* identification mark of CSW command state block 'USBC'USBS' */
#endif

#ifndef __PACKED
#define __PACKED               __attribute__((packed))
#endif

typedef struct __PACKED _USB_SETUP_REQ {
    UINT8 bRequestType;
    UINT8 bRequest;
    UINT16 wValue;
    UINT16 wIndex;
    UINT16 wLength;
} USB_SETUP_REQ, *PUSB_SETUP_REQ;

typedef struct __PACKED _USB_DEVICE_DESCR {
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT16 bcdUSB;
    UINT8 bDeviceClass;
    UINT8 bDeviceSubClass;
    UINT8 bDeviceProtocol;
    UINT8 bMaxPacketSize0;
    UINT16 idVendor;
    UINT16 idProduct;
    UINT16 bcdDevice;
    UINT8 iManufacturer;
    UINT8 iProduct;
    UINT8 iSerialNumber;
    UINT8 bNumConfigurations;
} USB_DEV_DESCR, *PUSB_DEV_DESCR;

typedef struct __PACKED _USB_CONFIG_DESCR {
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT16 wTotalLength;
    UINT8 bNumInterfaces;
    UINT8 bConfigurationValue;
    UINT8 iConfiguration;
    UINT8 bmAttributes;
    UINT8 MaxPower;
} USB_CFG_DESCR, *PUSB_CFG_DESCR;

typedef struct __PACKED _USB_INTERF_DESCR {
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bInterfaceNumber;
    UINT8 bAlternateSetting;
    UINT8 bNumEndpoints;
    UINT8 bInterfaceClass;
    UINT8 bInterfaceSubClass;
    UINT8 bInterfaceProtocol;
    UINT8 iInterface;
} USB_ITF_DESCR, *PUSB_ITF_DESCR;

typedef struct __PACKED _USB_ENDPOINT_DESCR {
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bEndpointAddress;
    UINT8 bmAttributes;
    UINT16 wMaxPacketSize;
    UINT8 bInterval;
} USB_ENDP_DESCR, *PUSB_ENDP_DESCR;

typedef struct __PACKED _USB_CONFIG_DESCR_LONG {
    USB_CFG_DESCR   cfg_descr;
    USB_ITF_DESCR   itf_descr;
    USB_ENDP_DESCR  endp_descr[1];
} USB_CFG_DESCR_LONG, *PUSB_CFG_DESCR_LONG;

typedef USB_CFG_DESCR_LONG *PXUSB_CFG_DESCR_LONG;

typedef struct __PACKED  _USB_HUB_DESCR {
    UINT8 bDescLength;
    UINT8 bDescriptorType;
    UINT8 bNbrPorts;
    UINT8 wHubCharacteristicsL;
    UINT8 wHubCharacteristicsH;
    UINT8 bPwrOn2PwrGood;
    UINT8 bHubContrCurrent;
    UINT8 DeviceRemovable;
    UINT8 PortPwrCtrlMask;
} USB_HUB_DESCR, *PUSB_HUB_DESCR;

typedef USB_HUB_DESCR  *PXUSB_HUB_DESCR;

typedef struct __PACKED  _USB_HID_DESCR {
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT16 bcdHID;
    UINT8 bCountryCode;
    UINT8 bNumDescriptors;
    UINT8 bDescriptorTypeX;
    UINT8 wDescriptorLengthL;
    UINT8 wDescriptorLengthH;
} USB_HID_DESCR, *PUSB_HID_DESCR;

typedef USB_HID_DESCR *PXUSB_HID_DESCR;

typedef struct __PACKED  _UDISK_BOC_CBW {         /* command of BulkOnly USB-FlashDisk */
    UINT32 mCBW_Sig;
    UINT32 mCBW_Tag;
    UINT32 mCBW_DataLen;                /* uppest byte of data length, always is 0 */
    UINT8 mCBW_Flag;                    /* transfer direction and etc. */
    UINT8 mCBW_LUN;
    UINT8 mCBW_CB_Len;                  /* length of command block */
    UINT8 mCBW_CB_Buf[16];              /* command block buffer */
} UDISK_BOC_CBW, *PUDISK_BOC_CBW;

typedef UDISK_BOC_CBW  *PXUDISK_BOC_CBW;

typedef struct __PACKED  _UDISK_BOC_CSW {         /* status of BulkOnly USB-FlashDisk */
    UINT32 mCSW_Sig;
    UINT32 mCSW_Tag;
    UINT32 mCSW_Residue;                /* return: remainder bytes */
    UINT8 mCSW_Status;                  /* return: result status */
} UDISK_BOC_CSW, *PUDISK_BOC_CSW;

typedef UDISK_BOC_CSW  *PXUDISK_BOC_CSW;

#ifdef __cplusplus
}
#endif

#endif  // __USB_TYPE__

/**********************************/
/*********USB high speed**********/
/**********************************/

/* USB high speed device register */
#define R8_USB2_CTRL                (*((PUINT8V)0x40009000))  // RW, USB_high_speed control register
#define  USBHS_UD_LPM_EN            0x80                      // RW, enable LPM
#define  USBHS_UD_DEV_EN            0x20                      // RW, enable USB equipment
#define  USBHS_UD_DMA_EN            0x10                      // RW, enable DMA transmit
#define  USBHS_UD_PHY_SUSPENDM      0x08                      // RW, suspeng USB PHY
#define  USBHS_UD_CLR_ALL           0x04                      // RW, clear all interupt flag
#define  USBHS_UD_RST_SIE           0x02                      // RW, reset USB protocol processor,including end point register
#define  USBHS_UD_RST_LINK          0x01                      // RW, enable LNK layer reset
#define R8_USB2_BASE_MODE           (*((PUINT8V)0x40009001))  // RW, USB_high_speed mode control register
#define  USBHS_UD_SPEED_FULL        0x00
#define  USBHS_UD_SPEED_HIGH        0x01
#define  USBHS_UD_SPEED_LOW         0x02
#define  USBHS_UD_SPEED_TYPE        0x03                      // RW, speed mode excpeted by the equipment,00:full speed, 01:high speed, 10:low speed
#define R8_USB2_INT_EN              (*((PUINT8V)0x40009002))  // RW, USB_high_speed intreurpt enable register
#define  USBHS_UDIE_FIFO_OVER       0x80                      // RW, enable fifo overflow interupt
#define  USBHS_UDIE_LINK_RDY        0x40                      // RW, enable USB conect interupt
#define  USBHS_UDIE_SOF_ACT         0x20                      // RW, enable SOF package received interupt
#define  USBHS_UDIE_TRANSFER        0x10                      // RW, enable USB transmit end interupt
#define  USBHS_UDIE_LPM_ACT         0x08                      // RW, enable lpm transmit end interupt
#define  USBHS_UDIE_BUS_SLEEP       0x04                      // RW, enable usb bus sleep interupt
#define  USBHS_UDIE_SUSPEND         0x02                      // RW, enable usb bus suspend interupt
#define  USBHS_UDIE_BUS_RST         0x01                      // RW, enable usb bus reset interupt
#define R8_USB2_DEV_AD              (*((PUINT8V)0x40009003))  // RW, USB_high_speed device adress register
#define  USBHS_UD_DEV_ADDR          0x7F                      // RW, adress of usb equipment
#define R8_USB2_WAKE_CTRL           (*((PUINT8V)0x40009004))  // RW, USB_high_speed wake up remotely register
#define  USBHS_UD_UD_REMOTE_WKUP    0x01                      // RW1, wake up remotely and auto reset hardware
#define R8_USB2_TEST_MODE           (*((PUINT8V)0x40009005))  // RW, USB_high_speed test mode register
#define  USBHS_UD_TEST_EN           0x80                      // RW, enable test mode
#define  USBHS_UD_TEST_SE0NAK       0x08                      // RW, output SE0 when in test mode
#define  USBHS_UD_TEST_PKT          0x04                      // RW, output one package(including DATA0,data and length of end pont4) when in test mode,not work on virtual equipment
#define  USBHS_UD_TEST_K            0x02                      // RW, output K when in test mode
#define  USBHS_UD_TEST_J            0x01                      // RW, output J when in test mode
#define R16_USB2_LPM_DATA           (*((PUINT16V)0x40009006)) // RW, USB_high_speed power control register
#define  USBHS_UD_LPM_BUSY          0x8000                    // RW, power control busy
#define  USBHS_UD_LPM_DATA          0x07FF                    // RO, power control data
#define R8_USB2_INT_FG              (*((PUINT8V)0x40009008))  // RW, USB_high_speed interupt flag register
#define  USBHS_UDIF_FIFO_OV         0x80                      // RW1, clear fifo overflow interupt flag
#define  USBHS_UDIF_LINK_RDY        0x40                      // RW1, clear USB conect interupt flag
#define  USBHS_UDIF_RX_SOF          0x20                      // RW1, clear SOF package received interupt flag
#define  USBHS_UDIF_TRANSFER        0x10                      // RO,  USB transmit end interupt flag,cleared by USBHS_UDMS_HS_MOD
#define  USBHS_UDIF_LPM_ACT         0x08                      // RW1, clear lpm transmit end interupt flag
#define  USBHS_UDIF_BUS_SLEEP       0x04                      // RW1, clear usb bus sleep interupt flag
#define  USBHS_UDIF_SUSPEND         0x02                      // RW1, clear usb bus suspend interupt flag
#define  USBHS_UDIF_BUS_RST         0x01                      // RW1, clear usb bus reset interupt flag
#define R8_USB2_INT_ST              (*((PUINT8V)0x40009009))  // RW, USB_high_speed interupt status register
#define  USBHS_UDIS_EP_DIR          0x10                      // RO, end point tranfer diector of data
#define  USBHS_UDIS_EP_ID_MASK      0x07                      // RO, number of end point which data transmission occured
#define R8_USB2_MIS_ST              (*((PUINT8V)0x4000900A))  // RW, USB_high_speed miscellaneous register
#define  USBHS_UDMS_HS_MOD          0x80                      // RO, host with high speed
#define  USBHS_UDMS_SUSP_REQ        0x10                      // RO, requirment of suspending USB
#define  USBHS_UDMS_SIE_FREE        0x08                      // RO, USB free state
#define  USBHS_UDMS_SLEEP           0x04                      // RO, USB sleep state
#define  USBHS_UDMS_SUSPEND         0x02                      // RO, USB in suspend state
#define  USBHS_UDMS_READY           0x01                      // RO, USB in connected state
#define R16_USB2_FRAME_NO           (*((PUINT16V)0x4000900C)) // RW, USB_high_speed frame number register
#define  USBHS_UD_MFRAME_NO         0xE000
#define  USBHS_UD_FRAME_NO          0x07FF
#define R16_USB2_BUS                (*((PUINT16V)0x4000900E)) // RW, USB_high_speed bus status register
#define  USBHS_USB_DM_ST            0x08
#define  USBHS_USB_DP_ST            0x04
#define  USB_WAKEUP                 0x01
#define R16_U2EP_TX_EN              (*((PUINT16V)0x40009010)) // RW, USB_high_speed end point transmit enable register
/* Bit definition for R16_U2EP_TX_EN & R16_U2EP_RX_EN register */
#define RB_EP0_EN                   0x0001
#define RB_EP1_EN                   0x0002
#define RB_EP2_EN                   0x0004
#define RB_EP3_EN                   0x0008
#define RB_EP4_EN                   0x0010
#define RB_EP5_EN                   0x0020
#define RB_EP6_EN                   0x0040
#define RB_EP7_EN                   0x0080
#define RB_EP8_EN                   0x0100
#define RB_EP9_EN                   0x0200
#define RB_EP10_EN                  0x0400
#define RB_EP11_EN                  0x0800
#define RB_EP12_EN                  0x1000
#define RB_EP13_EN                  0x2000
#define RB_EP14_EN                  0x4000
#define RB_EP15_EN                  0x8000

#define R16_U2EP_RX_EN              (*((PUINT16V)0x40009012))   // RW, USB_high_speed end point receive enableregister
#define  USBHS_UEP_RX_EN            0xFFFF
#define R16_U2EP_T_TOG_AUTO         (*((PUINT16V)0x40009014))   // RW, USB_high_speed end point transmit auto toggle enable register
#define  USBHS_UEP_T_TOG_AUTO       0xFF
#define R16_U2EP_R_TOG_AUTO         (*((PUINT16V)0x40009016))   // RW, USB_high_speed end point receive auto toggle enable register
#define  USBHS_UEP_R_TOG_AUTO       0xFF
#define R8_U2EP_T_BURST             (*((PUINT8V)0x40009018))    // RW, USB_high_speed end point transmit burst register
#define  USBHS_UEP_T_BURST_EN       0xFF
#define R8_U2EP_T_BURST_MODE        (*((PUINT8V)0x40009019))    // RW, USB_high_speed end point transmit burst mode register
#define  USBHS_UEP_T_BURST_MODE     0xFF
#define R8_U2EP_R_BURST             (*((PUINT8V)0x4000901A))    // RW, USB_high_speed end point receive burst register
#define  USBHS_UEP_R_BURST_EN       0xFF
#define R8_U2EP_R_RES_MODE          (*((PUINT8V)0x4000901B))    // RW, USB_high_speed end point transmit reply mode register
#define  USBHS_UEP_R_RES_MODE       0xFF
#define R32_U2EP_AF_MODE            (*((PUINT32V)0x4000901C))   // RW, USB_high_speed end point multiplexing register
#define  USBHS_UEP_T_AF             0xFE
#define R32_U2EP0_DMA               (*((PUINT32V)0x40009020))   // RW, USB_high_speed end point0 begin adress of DMA buffer register
#define  UEPn_DMA                   0x01FFFF
#define R32_U2EP1_RX_DMA            (*((PUINT32V)0x40009024))   // RW, USB_high_speed end point1 begin adress of DMA receive buffer register
#define R32_U2EP2_RX_DMA            (*((PUINT32V)0x40009028))   // RW, USB_high_speed end point2 begin adress of DMA receive buffer register
#define R32_U2EP3_RX_DMA            (*((PUINT32V)0x4000902C))   // RW, USB_high_speed end point3 begin adress of DMA receive buffer register
#define R32_U2EP4_RX_DMA            (*((PUINT32V)0x40009030))   // RW, USB_high_speed end point4 begin adress of DMA receive buffer register
#define R32_U2EP5_RX_DMA            (*((PUINT32V)0x40009034))   // RW, USB_high_speed end point5 begin adress of DMA receive buffer register
#define R32_U2EP6_RX_DMA            (*((PUINT32V)0x40009038))   // RW, USB_high_speed end point6 begin adress of DMA receive buffer register
#define R32_U2EP7_RX_DMA            (*((PUINT32V)0x4000903C))   // RW, USB_high_speed end point7 begin adress of DMA receive buffer register
#define  UEPn_RX_DMA                0x01FFFF
#define R32_U2EP1_TX_DMA            (*((PUINT32V)0x40009040))   // RW, USB_high_speed end point1 begin adress of DMA transmit buffer register
#define R32_U2EP2_TX_DMA            (*((PUINT32V)0x40009044))   // RW, USB_high_speed end point2 begin adress of DMA transmit buffer register
#define R32_U2EP3_TX_DMA            (*((PUINT32V)0x40009048))   // RW, USB_high_speed end point3 begin adress of DMA transmit buffer register
#define R32_U2EP4_TX_DMA            (*((PUINT32V)0x4000904C))   // RW, USB_high_speed end point4 begin adress of DMA transmit buffer register
#define R32_U2EP5_TX_DMA            (*((PUINT32V)0x40009050))   // RW, USB_high_speed end point5 begin adress of DMA transmit buffer register
#define R32_U2EP6_TX_DMA            (*((PUINT32V)0x40009054))   // RW, USB_high_speed end point6 begin adress of DMA transmit buffer register
#define R32_U2EP7_TX_DMA            (*((PUINT32V)0x40009058))   // RW, USB_high_speed end point7 begin adress of DMA transmit buffer register
#define  UEPn_TX_DMA                0x01FFFF
#define R32_U2EP0_MAX_LEN           (*((PUINT32V)0x4000905C))   // RW, USB_high_speed end point0 max length package register
#define R32_U2EP1_MAX_LEN           (*((PUINT32V)0x40009060))   // RW, USB_high_speed end point1 max length package register
#define R32_U2EP2_MAX_LEN           (*((PUINT32V)0x40009064))   // RW, USB_high_speed end point2 max length package register
#define R32_U2EP3_MAX_LEN           (*((PUINT32V)0x40009068))   // RW, USB_high_speed end point3 max length package register
#define R32_U2EP4_MAX_LEN           (*((PUINT32V)0x4000906C))   // RW, USB_high_speed end point4 max length package register
#define R32_U2EP5_MAX_LEN           (*((PUINT32V)0x40009070))   // RW, USB_high_speed end point5 max length package register
#define R32_U2EP6_MAX_LEN           (*((PUINT32V)0x40009074))   // RW, USB_high_speed end point6 max length package register
#define R32_U2EP7_MAX_LEN           (*((PUINT32V)0x40009078))   // RW, USB_high_speed end point7 max length package register
#define  UEPn_MAX_LEN               0x007F
#define R16_U2EP0_RX_LEN            (*((PUINT16V)0x4000907C))   // RW, USB_high_speed end point0 length of receive register
#define  UEP0_RX_LEN                0x007F
#define R16_U2EP1_RX_LEN            (*((PUINT16V)0x40009080))   // RW, USB_high_speed end point1 single received length register
#define R16_U2EP1_R_SIZE            (*((PUINT16V)0x40009082))   // RW, USB_high_speed end point1 total received length register
#define R16_U2EP2_RX_LEN            (*((PUINT16V)0x40009084))   // RW, USB_high_speed end point2 single received length register
#define R16_U2EP2_R_SIZE            (*((PUINT16V)0x40009086))   // RW, USB_high_speed end point2 total received length register
#define R16_U2EP3_RX_LEN            (*((PUINT16V)0x40009088))   // RW, USB_high_speed end point3 single received length register
#define R16_U2EP3_R_SIZE            (*((PUINT16V)0x4000908A))   // RW, USB_high_speed end point3 total received length register
#define R16_U2EP4_RX_LEN            (*((PUINT16V)0x4000908C))   // RW, USB_high_speed end point4 single received length register
#define R16_U2EP4_R_SIZE            (*((PUINT16V)0x4000908E))   // RW, USB_high_speed end point4 total received length register
#define R16_U2EP5_RX_LEN            (*((PUINT16V)0x40009090))   // RW, USB_high_speed end point5 single received length register
#define R16_U2EP5_R_SIZE            (*((PUINT16V)0x40009092))   // RW, USB_high_speed end point5 total received length register
#define R16_U2EP6_RX_LEN            (*((PUINT16V)0x40009094))   // RW, USB_high_speed end point6 single received length register
#define R16_U2EP6_R_SIZE            (*((PUINT16V)0x40009096))   // RW, USB_high_speed end point6 total received length register
#define R16_U2EP7_RX_LEN            (*((PUINT16V)0x40009098))   // RW, USB_high_speed end point7 single received length register
#define R16_U2EP7_R_SIZE            (*((PUINT16V)0x4000909A))   // RW, USB_high_speed end point7 total received length register
#define  UEPn_RX_LEN                0xFFFF
#define  UEPn_R_SIZE                0xFFFF
#define R16_U2EP0_T_LEN             (*((PUINT16V)0x4000909C))   // RW, USB_high_speed end point0 length of transmission register
#define  UEP0_T_LEN                 0x7F
#define R8_U2EP0_TX_CTRL            (*((PUINT8V)0x4000909E))    // RW, USB_high_speed end point0 transmit control register
#define R8_U2EP0_RX_CTRL            (*((PUINT8V)0x4000909F))    // RW, USB_high_speed end point0 receive control register
#define R16_U2EP1_T_LEN             (*((PUINT16V)0x400090A0))   // RW, USB_high_speed end point1 length of transmission register
#define R8_U2EP1_TX_CTRL            (*((PUINT8V)0x400090A2))    // RW, USB_high_speed end point1 transmit control register
#define R8_U2EP1_RX_CTRL            (*((PUINT8V)0x400090A3))    // RW, USB_high_speed end point1 receive control register
#define R16_U2EP2_T_LEN             (*((PUINT16V)0x400090A4))   // RW, USB_high_speed end point2 length of transmission register
#define R8_U2EP2_TX_CTRL            (*((PUINT8V)0x400090A6))    // RW, USB_high_speed end point2 transmit control register
#define R8_U2EP2_RX_CTRL            (*((PUINT8V)0x400090A7))    // RW, USB_high_speed end point2 receive control register
#define R16_U2EP3_T_LEN             (*((PUINT16V)0x400090A8))   // RW, USB_high_speed end point3 length of transmission register
#define R8_U2EP3_TX_CTRL            (*((PUINT8V)0x400090AA))    // RW, USB_high_speed end point3 transmit control register
#define R8_U2EP3_RX_CTRL            (*((PUINT8V)0x400090AB))    // RW, USB_high_speed end point3 receive control register
#define R16_U2EP4_T_LEN             (*((PUINT16V)0x400090AC))   // RW, USB_high_speed end point4 length of transmission register
#define R8_U2EP4_TX_CTRL            (*((PUINT8V)0x400090AE))    // RW, USB_high_speed end point4 transmit control register
#define R8_U2EP4_RX_CTRL            (*((PUINT8V)0x400090AF))    // RW, USB_high_speed end point4 receive control register
#define R16_U2EP5_T_LEN             (*((PUINT16V)0x400090B0))   // RW, USB_high_speed end point5 length of transmission register
#define R8_U2EP5_TX_CTRL            (*((PUINT8V)0x400090B2))    // RW, USB_high_speed end point5 transmit control register
#define R8_U2EP5_RX_CTRL            (*((PUINT8V)0x400090B3))    // RW, USB_high_speed end point5 receive control register
#define R16_U2EP6_T_LEN             (*((PUINT16V)0x400090B4))   // RW, USB_high_speed end point6 length of transmission register
#define R8_U2EP6_TX_CTRL            (*((PUINT8V)0x400090B6))    // RW, USB_high_speed end point6 transmit control register
#define R8_U2EP6_RX_CTRL            (*((PUINT8V)0x400090B7))    // RW, USB_high_speed end point6 receive control register
#define R16_U2EP7_T_LEN             (*((PUINT16V)0x400090B8))   // RW, USB_high_speed end point7 length of transmission register
#define R8_U2EP7_TX_CTRL            (*((PUINT8V)0x400090BA))    // RW, USB_high_speed end point7 transmit control register
#define R8_U2EP7_RX_CTRL            (*((PUINT8V)0x400090BB))    // RW, USB_high_speed end point7 receive control register
/**R16_UEPn_T_LEN**/ 
#define UEPn_T_LEN                  0xFFFF
/**R8_UEPn_TX_CTRL**/ 
#define USBHS_UEP_T_DONE            0x80
#define USBHS_UEP_T_NAK_ACT         0x40
#define USBHS_UEP_T_TOG_MASK        0x0C
#define USBHS_UEP_T_TOG_MDATA       0x0C
#define USBHS_UEP_T_TOG_DATA2       0x08
#define USBHS_UEP_T_TOG_DATA1       0x04
#define USBHS_UEP_T_TOG_DATA0       0x00
#define USBHS_UEP_T_RES_MASK        0x03
#define USBHS_UEP_T_RES_ACK         0x02
#define USBHS_UEP_T_RES_STALL       0x01
#define USBHS_UEP_T_RES_NAK         0x00
 
/**R8_UEPn_RX_CTRL**/ 
#define USBHS_UEP_R_DONE            0x80
#define USBHS_UEP_R_NAK_ACT         0x40
#define USBHS_UEP_R_NAK_TOG         0x20
#define USBHS_UEP_R_TOG_MATCH       0x10
#define USBHS_UEP_R_SETUP_IS        0x08
#define USBHS_UEP_R_TOG_MASK        0x04
//#define USBHS_UEP_R_TOG_MDATA       0x0C
//#define USBHS_UEP_R_TOG_DATA2       0x08
#define USBHS_UEP_R_TOG_DATA1       0x04
#define USBHS_UEP_R_TOG_DATA0       0x00
#define USBHS_UEP_R_RES_MASK        0x03
#define USBHS_UEP_R_RES_ACK         0x02
#define USBHS_UEP_R_RES_STALL       0x01
#define USBHS_UEP_R_RES_NAK         0x00
 
#define R16_U2EP_T_ISO              (*((PUINT16V)0x400090BC))   // RW, USB_high_speed end point transmit sync mode register
#define  USBHS_UEP1_T_ISO_EN        0x02
#define  USBHS_UEP2_T_ISO_EN        0x04
#define  USBHS_UEP3_T_ISO_EN        0x08
#define  USBHS_UEP4_T_ISO_EN        0x10
#define  USBHS_UEP5_T_ISO_EN        0x20
#define  USBHS_UEP6_T_ISO_EN        0x40
#define  USBHS_UEP7_T_ISO_EN        0x80
#define R16_U2EP_R_ISO              (*((PUINT16V)0x400090BE))   // RW, USB_high_speed end point receive sync mode register
#define  USBHS_UEP1_R_ISO_EN        0x02
#define  USBHS_UEP2_R_ISO_EN        0x04
#define  USBHS_UEP3_R_ISO_EN        0x08
#define  USBHS_UEP4_R_ISO_EN        0x10
#define  USBHS_UEP5_R_ISO_EN        0x20
#define  USBHS_UEP6_R_ISO_EN        0x40
#define  USBHS_UEP7_R_ISO_EN        0x80
 
/* USB high speed host register  */
#define R8_U2H_CFG                  (*((PUINT8V)0x40009100))   // RW, USB_high_speed register
#define  USBHS_UH_LPM_EN            0x80
#define  USBHS_UH_FORCE_FS          0x40
#define  USBHS_UH_SOF_EN            0x20
#define  USBHS_UH_DMA_EN            0x10
#define  USBHS_UH_PHY_SUSPENDM      0x08
#define  USBHS_UH_CLR_ALL           0x04
#define  USBHS_RST_SIE              0x02
#define  USBHS_RST_LINK             0x01
#define R8_U2H_INT_EN               (*((PUINT8V)0x40009102))   // RW, USB_high_speed register
#define  USBHS_UHIE_FIFO_OVER       0x80
#define  USBHS_UHIE_TX_HALT         0x40
#define  USBHS_UHIE_SOF_ACT         0x20
#define  USBHS_UHIE_TRANSFER        0x10
#define  USBHS_UHIE_RESUME_ACT      0x08
#define  USBHS_UHIE_WKUP_ACT        0x04
#define R8_U2H_DEV_AD             (*((PUINT8V)0x40009103))   // RW, USB_high_speed register
#define  USBHS_UH_DEV_ADDR          0xFF
#define R32_U2H_CONTROL             (*((PUINT32V)0x40009104))   // RW, USB_high_speed register
#define  USBHS_UH_RX_NO_RES         0x800000
#define  USBHS_UH_TX_NO_RES         0x400000
#define  USBHS_UH_RX_NO_DATA        0x200000
#define  USBHS_UH_TX_NO_DATA        0x100000
#define  USBHS_UH_PRE_PID_EN        0x080000
#define  USBHS_UH_SPLIT_VALID       0x040000
#define  USBHS_UH_LPM_VALID         0x020000
#define  USBHS_UH_HOST_ACTION       0x010000
#define  USBHS_UH_BUF_MODE          0x0400
#define  USBHS_UH_T_TOG_MASK        0x0300
#define  USBHS_UH_T_TOG_MDATA       0x0300
#define  USBHS_UH_T_TOG_DATA2       0x0200
#define  USBHS_UH_T_TOG_DATA1       0x0100
#define  USBHS_UH_T_TOG_DATA0       0x0000
#define  USBHS_UH_T_ENDP_MASK       0xF0
#define  USBHS_UH_T_TOKEN_MASK      0x0F

#define R8_U2H_INT_FLAG             (*((PUINT8V)0x40009108))   // RW, USB_high_speed register
#define  USBHS_UHIF_FIFO_OVER       0x80
#define  USBHS_UHIF_TX_HALT         0x40
#define  USBHS_UHIF_SOF_ACT         0x20
#define  USBHS_UHIF_TRANSFER        0x10
#define  USBHS_UHIF_RESUME_ACT      0x08
#define  USBHS_UHIF_WKUP_ACT        0x04
#define R8_U2H_INT_ST               (*((PUINT8V)0x40009109))   // RW, USB_high_speed register
#define  USBHS_UHIF_PORT_RX_RESUME  0x10
#define  USBHS_UH_R_TOKEN_MASK      0x0F
#define R8_U2H_MIS_ST               (*((PUINT8V)0x4000910A))   // RW, USB_high_speed register
#define  USBHS_UHMS_BUS_SE0         0x80
#define  USBHS_UHMS_BUS_J           0x40
#define  USBHS_UHMS_LINESTATE       0x30
#define  USBHS_UHMS_USB_WAKEUP      0x08
#define  USBHS_UHMS_SOF_ACT         0x04
#define  USBHS_UHMS_SOF_PRE         0x02
#define  USBHS_UHMS_SOF_FREE        0x01
#define R32_U2H_LPM_DATA            (*((PUINT32V)0x4000910C))   // RW, USB_high_speed register
#define  USBHS_UH_LPM_DATA          0x07FF
#define R32_U2H_SPLIT_DATA          (*((PUINT32V)0x40009110))   // RW, USB_high_speed register
#define  USBHS_UH_SPLIT_DATA        0x07FFFF
#define R32_U2H_FRAME               (*((PUINT32V)0x40009114))   // RW, USB_high_speed register
#define  USBHS_UH_SOF_CNT_CLR       0x02000000
#define  USBHS_UH_SOF_CNT_EN        0x01000000
#define  USBHS_UH_MFRAME_NO         0x070000
#define  USBHS_UH_FRAME_NO          0x07FF
#define R32_U2H_TX_LEN              (*((PUINT32V)0x40009118))   // RW, USB_high_speed register
#define  USBHS_UH_TX_LEN            0x07FF
#define R32_U2H_RX_LEN              (*((PUINT32V)0x4000911C))   // RW, USB_high_speed register
#define  USBHS_UH_RX_LEN            0x07FF
#define R32_U2H_RX_MAX_LEN          (*((PUINT32V)0x40009120))   // RW, USB_high_speed register
#define  USBHS_UH_RX_MAX_LEN        0x07FF
#define R32_U2H_RX_DMA              (*((PUINT32V)0x40009124))   // RW, USB_high_speed register
#define  USBHS_R32_UH_RX_DMA        0x01FFFF
#define R32_U2H_TX_DMA              (*((PUINT32V)0x40009128))   // RW, USB_high_speed register
#define  USBHS_R32_UH_TX_DMA        0x01FFFF
#define R32_U2H_PORT_CTRL           (*((PUINT32V)0x4000912C))   // RW, USB_high_speed register
#define  USBHS_UH_BUS_RST_LONG      0x010000
#define  USBHS_UH_PORT_SLEEP_BESL   0xF000
#define  USBHS_UH_CLR_PORT_SLEEP    0x0100
#define  USBHS_UH_CLR_PORT_CONNECT  0x20
#define  USBHS_UH_CLR_PORT_EN       0x10
#define  USBHS_UH_SET_PORT_SLEEP    0x08
#define  USBHS_UH_CLR_PORT_SUSP     0x04
#define  USBHS_UH_SET_PORT_SUSP     0x02
#define  USBHS_UH_SET_PORT_RESET    0x01
#define R8_U2H_PORT_CFG             (*((PUINT8V)0x40009130))   // RW, USB_high_speed register
#define  USBHS_UH_PD_EN             0x80
#define  USBHS_UH_HOST_EN           0x01
#define R8_U2H_PORT_INT_EN          (*((PUINT8V)0x40009132))   // RW, USB_high_speed register
#define  USBHS_UHIE_PORT_SLP        0x20
#define  USBHS_UHIE_PORT_RESET      0x10
#define  USBHS_UHIE_PORT_SUSP       0x04
#define  USBHS_UHIE_PORT_EN         0x02
#define  USBHS_UHIE_PORT_CONNECT    0x01
#define R8_U2H_PORT_TEST_CT         (*((PUINT8V)0x40009133))   // RW, USB_high_speed register
#define  USBHS_UH_TEST_FORCE_EN     0x04
#define  USBHS_UH_TEST_K            0x02
#define  USBHS_UH_TEST_J            0x01
#define R16_U2H_PORT_ST             (*((PUINT16V)0x40009134))  // RW, USB_high_speed register
#define  USBHS_UHIS_PORT_TEST       0x0800
#define  USBHS_UHIS_PORT_SPEED_MASK 0x0600
#define  USBHS_UHIS_PORT_HS         0x0400
#define  USBHS_UHIS_PORT_LS         0x0200
#define  USBHS_UHIS_PORT_FS         0x0000
#define  USBHS_UHIS_PORT_SLP        0x20
#define  USBHS_UHIS_PORT_RST        0x10
#define  USBHS_UHIS_PORT_SUSP       0x04
#define  USBHS_UHIS_PORT_EN         0x02
#define  USBHS_UHIS_PORT_CONNECT    0x01
#define R8_U2H_PORT_CHG             (*((PUINT8V)0x40009136))
#define  USBHS_UHIF_PORT_SLP        0x20
#define  USBHS_UHIF_PORT_RESET      0x10
#define  USBHS_UHIF_PORT_SUSP       0x04
#define  USBHS_UHIF_PORT_EN         0x02
#define  USBHS_UHIF_PORT_CONNECT    0x01
#define R32_U2H_BC_CTRL             (*((PUINT32V)0x4000913C))
#define  UDM_VSRC_ACT               0x0400
#define  UDM_BC_VSRC                0x0200
#define  UDP_BC_VSRC                0x0100
#define  BC_AUTO_MODE               0x40
#define  UDM_BC_CMPE                0x20
#define  UDP_BC_CMPE                0x10
#define  UDM_BC_CMPO                0x02
#define  UDP_BC_CMPO                0x01
#define R8_USBHS_PLL_CTRL           (*((PUINT8V)0x40009200))
#define  USBHS_PLL_EN               0x04
#define  USBHS_PLL_LOWPOW           0x02
#define  USBHS_PLL_CKSEL            0x01
