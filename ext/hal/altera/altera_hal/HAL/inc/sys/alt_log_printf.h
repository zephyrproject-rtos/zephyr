/*  alt_log_printf.h
 *
 *  ALT_LOG is designed to provide extra logging/debugging messages from HAL
 *  through a different port than stdout.  It is enabled by the ALT_LOG_ENABLE
 *  define, which needs to supplied at compile time.  When logging is turned off, 
 *  code size is unaffected.  Thus, this should be transparent to the user
 *  when it is not actively turned on, and should not affect projects in any way.
 *
 *  There are macros sprinkled within different components, such as the jtag uart 
 *  and timer, in the HAL code.  They are always named ALT_LOG_<name>, and can be
 *  safely ignored if ALT_LOG is turned off. 
 *
 *  To turn on ALT_LOG, ALT_LOG_ENABLE must be defined, and ALT_LOG_PORT_TYPE and
 *  ALT_LOG_PORT_BASE must be set in system.h.  This is done through editing 
 *  <project>.ptf, by editing the alt_log_port_type & alt_log_port_base settings.
 *  See the documentation html file for examples.
 *
 *  When it is turned on, it will output extra HAL messages to a port specified
 *  in system.h.  This can be a UART or JTAG UART port.  By default it will 
 *  output boot messages, detailing every step of the boot process.  
 *
 *  Extra logging is designed to be enabled by flags, which are defined in 
 *  alt_log_printf.c.  The default value is that all flags are off, so only the
 *  boot up logging messages show up.  ALT_LOG_FLAGS can be set to enable certain
 *  groupings of flags, and that grouping is done in this file.  Each flag can 
 *  also be overridden with a -D at compile time.
 *
 *  This header file includes the necessary prototypes for using the alt_log
 *  functions.  It also contains all the macros that are used to remove the code
 *  from alt log is turned off.  Also, the macros in other HAL files are defined 
 *  here at the bottom.  These macros all call some C function that is in 
 *  alt_log_printf.c.
 *
 *  The logging has functions for printing in C (ALT_LOG_PRINTF) and in assembly
 *  (ALT_LOG_PUTS).  This was needed because the assembly printing occurs before
 *  the device is initialized.  The assembly function corrupts register R4-R7, 
 *  which are not used in the normal boot process.  For this reason, do not call
 *  the assembly function in C.
 *
 *  author: gkwan 
 */


#ifndef __ALT_LOG_PRINTF_H__
#define __ALT_LOG_PRINTF_H__

#include <system.h>

/* Global switch to turn on logging functions */
#ifdef ALT_LOG_ENABLE

    /* ALT_LOG_PORT_TYPE values as defined in system.h.  They are defined as
     * numbers here first becasue the C preprocessor does not handle string
     * comparisons.  */
    #define ALTERA_AVALON_JTAG_UART 1
    #define ALTERA_AVALON_UART 0

    /* If this .h file is included by an assembly file, skip over include files
     * that won't compile in assembly. */
    #ifndef ALT_ASM_SRC
        #include <stdarg.h>
        #include "sys/alt_alarm.h"
        #include "sys/alt_dev.h"
        #ifdef __ALTERA_AVALON_JTAG_UART 
            #include "altera_avalon_jtag_uart.h"
        #endif
    #endif /* ALT_ASM_SRC */

    /* These are included for the port register offsets and masks, needed
     * to write to the port.  Only include if the port type is set correctly,
     * otherwise error.  If alt_log is turned on and the port to output to is
     * incorrect or does not exist, then should exit. */
    #if ALT_LOG_PORT_TYPE == ALTERA_AVALON_JTAG_UART 
        #ifdef __ALTERA_AVALON_JTAG_UART
            #include <altera_avalon_jtag_uart_regs.h>
        #else
            #error ALT_LOG: JTAG_UART port chosen, but no JTAG_UART in system. 
        #endif
    #elif ALT_LOG_PORT_TYPE == ALTERA_AVALON_UART 
        #ifdef __ALTERA_AVALON_UART
            #include <altera_avalon_uart_regs.h>
        #else
            #error ALT_LOG: UART Port chosen, but no UART in system.
        #endif
    #else
        #error ALT_LOG: alt_log_port_type declaration invalid!
    #endif

    /* ALT_LOG_ENABLE turns on the basic printing function */
    #define ALT_LOG_PRINTF(...) do {alt_log_printf_proc(__VA_ARGS__);} while (0)
 
    /* Assembly macro for printing in assembly, calls tx_log_str
     * which is in alt_log_macro.S.
     * If alt_log_boot_on_flag is 0, skips the printing */
    #define ALT_LOG_PUTS(str) movhi r4, %hiadj(alt_log_boot_on_flag) ; \
         addi r4, r4, %lo(alt_log_boot_on_flag) ; \
         ldwio r5, 0(r4) ; \
         beq r0, r5, 0f ; \
         movhi r4, %hiadj(str) ; \
         addi r4, r4, %lo(str) ; \
	 call tx_log_str ; \
       0:
    
    /* These defines are here to faciliate the use of one output function 
     * (alt_log_txchar) to print to both the JTAG UART or the UART. Depending
     * on the port type, the status register, read mask, and output register 
     * are set to the appropriate value for the port.  */
    #if ALT_LOG_PORT_TYPE == ALTERA_AVALON_JTAG_UART 
      #define ALT_LOG_PRINT_REG_RD IORD_ALTERA_AVALON_JTAG_UART_CONTROL
      #define ALT_LOG_PRINT_MSK ALTERA_AVALON_JTAG_UART_CONTROL_WSPACE_MSK
      #define ALT_LOG_PRINT_TXDATA_WR IOWR_ALTERA_AVALON_JTAG_UART_DATA
      #define ALT_LOG_PRINT_REG_OFFSET (ALTERA_AVALON_JTAG_UART_CONTROL_REG*0x4)
      #define ALT_LOG_PRINT_TXDATA_REG_OFFSET (ALTERA_AVALON_JTAG_UART_DATA_REG*0x4)
    #elif ALT_LOG_PORT_TYPE == ALTERA_AVALON_UART 
      #define ALT_LOG_PRINT_REG_RD IORD_ALTERA_AVALON_UART_STATUS
      #define ALT_LOG_PRINT_MSK ALTERA_AVALON_UART_STATUS_TRDY_MSK
      #define ALT_LOG_PRINT_TXDATA_WR IOWR_ALTERA_AVALON_UART_TXDATA
      #define ALT_LOG_PRINT_REG_OFFSET (ALTERA_AVALON_UART_STATUS_REG*0x4)
      #define ALT_LOG_PRINT_TXDATA_REG_OFFSET (ALTERA_AVALON_UART_TXDATA_REG*0x4)
    #endif /* ALT_LOG_PORT */

    /* Grouping of flags via ALT_LOG_FLAGS.  Each specific flag can be set via 
     * -D at compile time, or else they'll be set to a default value according
     * to ALT_LOG_FLAGS.  ALT_LOG_FLAGS = 0 or not set is the default, where 
     * only the boot messages will be printed. As ALT_LOG_FLAGS increase, they
     * increase in intrusiveness to the program, and will affect performance.
     *
     * Flag Level 1 - turns on system clock and JTAG UART startup status
     *            2 - turns on write echo and JTAG_UART alarm (periodic report)
     *            3 - turns on JTAG UART ISR logging - will slow performance
     *                significantly. 
     *           -1 - All logging output is off, but if ALT_LOG_ENABLE is 
     *                defined all logging function is built and code size 
     *                remains constant 
     *
     * Flag settings - 1 = on, 0 = off. */

    /* This flag turns on "boot" messages for printing.  This includes messages
     * during crt0.S, then alt_main, and finally alt_exit. */
    #ifndef ALT_LOG_BOOT_ON_FLAG_SETTING
        #if ALT_LOG_FLAGS == 1
            #define ALT_LOG_BOOT_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 2
            #define ALT_LOG_BOOT_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 3
            #define ALT_LOG_BOOT_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == -1 /* silent mode */
            #define ALT_LOG_BOOT_ON_FLAG_SETTING 0x0
        #else /* default setting */
            #define ALT_LOG_BOOT_ON_FLAG_SETTING 0x1
        #endif
    #endif /* ALT_LOG_BOOT_ON_FLAG_SETTING */

    #ifndef ALT_LOG_SYS_CLK_ON_FLAG_SETTING
        #if ALT_LOG_FLAGS == 1
            #define ALT_LOG_SYS_CLK_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 2
            #define ALT_LOG_SYS_CLK_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 3
            #define ALT_LOG_SYS_CLK_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == -1 /* silent mode */ 
            #define ALT_LOG_SYS_CLK_ON_FLAG_SETTING 0x0
        #else /* default setting */
            #define ALT_LOG_SYS_CLK_ON_FLAG_SETTING 0x0
        #endif
    #endif /* ALT_LOG_SYS_CLK_ON_FLAG_SETTING */

    #ifndef ALT_LOG_WRITE_ON_FLAG_SETTING
        #if ALT_LOG_FLAGS == 1
            #define ALT_LOG_WRITE_ON_FLAG_SETTING 0x0
        #elif ALT_LOG_FLAGS == 2
            #define ALT_LOG_WRITE_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 3
            #define ALT_LOG_WRITE_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == -1 /* silent mode */
            #define ALT_LOG_WRITE_ON_FLAG_SETTING 0x0
        #else /* default setting */
            #define ALT_LOG_WRITE_ON_FLAG_SETTING 0x0
        #endif
    #endif /* ALT_LOG_WRITE_ON_FLAG_SETTING */

    #ifndef ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING
        #ifndef __ALTERA_AVALON_JTAG_UART 
            #define ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING 0x0
        #elif ALT_LOG_FLAGS == 1
            #define ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING 0x0
        #elif ALT_LOG_FLAGS == 2
            #define ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 3
            #define ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == -1 /* silent mode */
            #define ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING 0x0
        #else /* default setting */
            #define ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING 0x0
        #endif
    #endif /* ALT_LOG_JTAG_UART_ALARM_ON_FLAG_SETTING */

    #ifndef ALT_LOG_JTAG_UART_STARTUP_INFO_ON_FLAG_SETTING
        #ifndef __ALTERA_AVALON_JTAG_UART 
            #define ALT_LOG_JTAG_UART_STARTUP_INFO_ON_FLAG_SETTING 0x0
        #elif ALT_LOG_FLAGS == 1
            #define ALT_LOG_JTAG_UART_STARTUP_INFO_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 2
            #define ALT_LOG_JTAG_UART_STARTUP_INFO_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == 3
            #define ALT_LOG_JTAG_UART_STARTUP_INFO_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == -1 /* silent mode */
            #define ALT_LOG_JTAG_UART_STARTUP_INFO_ON_FLAG_SETTING 0x0
        #else /* default setting */
            #define ALT_LOG_JTAG_UART_STARTUP_INFO_ON_FLAG_SETTING 0x0
        #endif
    #endif /* ALT_LOG_JTAG_UART_STARTUP_INFO_FLAG_SETTING */

    #ifndef ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING
        #ifndef __ALTERA_AVALON_JTAG_UART 
            #define ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING 0x0
        #elif ALT_LOG_FLAGS == 1
            #define ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING 0x0
        #elif ALT_LOG_FLAGS == 2
            #define ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING 0x0
        #elif ALT_LOG_FLAGS == 3
            #define ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING 0x1
        #elif ALT_LOG_FLAGS == -1 /* silent mode */
            #define ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING 0x0
        #else /* default setting */
            #define ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING 0x0
        #endif
    #endif /* ALT_LOG_JTAG_UART_ISR_ON_FLAG_SETTING */

#ifndef ALT_ASM_SRC
    /* Function Prototypes */
    void alt_log_txchar(int c,char *uartBase);
    void alt_log_private_printf(const char *fmt,int base,va_list args);
    void alt_log_repchar(char c,int r,int base);
    int alt_log_printf_proc(const char *fmt, ... );
    void alt_log_system_clock();
    #ifdef __ALTERA_AVALON_JTAG_UART 
        alt_u32 altera_avalon_jtag_uart_report_log(void * context);
        void alt_log_jtag_uart_startup_info(altera_avalon_jtag_uart_state* dev, int base);
        void alt_log_jtag_uart_print_control_reg(altera_avalon_jtag_uart_state* dev, \
             int base, const char* header);
        void alt_log_jtag_uart_isr_proc(int base, altera_avalon_jtag_uart_state* dev);
    #endif
    void alt_log_write(const void *ptr, size_t len);
    
    /* extern all global variables */
    /* CASE:368514 - The boot message flag is linked into the sdata section
     * because if it is zero, it would otherwise be placed in the bss section.
     * alt_log examines this variable before the BSS is cleared in the boot-up
     * process. 
     */
    extern volatile alt_u32 alt_log_boot_on_flag __attribute__ ((section (".sdata")));
    extern volatile alt_u8 alt_log_write_on_flag;
    extern volatile alt_u8 alt_log_sys_clk_on_flag;
    extern volatile alt_u8 alt_log_jtag_uart_alarm_on_flag;
    extern volatile alt_u8 alt_log_jtag_uart_isr_on_flag;
    extern volatile alt_u8 alt_log_jtag_uart_startup_info_on_flag;
    extern volatile int alt_log_sys_clk_count;
    extern volatile int alt_system_clock_in_sec;
    extern alt_alarm alt_log_jtag_uart_alarm_1;
#endif  /* ALT_ASM_SRC */


    /* Below are the MACRO defines used in various HAL files.  They check
     * if their specific flag is turned on; if it is, then it executes its 
     * code.
     *
     * To keep this file reasonable, most of these macros calls functions,
     * which are defined in alt_log_printf.c.  Look there for implementation
     * details. */
  
    /* Boot Messages Logging */
    #define ALT_LOG_PRINT_BOOT(...) \
       do { if (alt_log_boot_on_flag==1) {ALT_LOG_PRINTF(__VA_ARGS__);} \
          } while (0)

    /* JTAG UART Logging */
    /* number of ticks before alarm runs logging function */
    #ifndef ALT_LOG_JTAG_UART_TICKS_DIVISOR
    	#define ALT_LOG_JTAG_UART_TICKS_DIVISOR 10
    #endif
    #ifndef ALT_LOG_JTAG_UART_TICKS
        #define ALT_LOG_JTAG_UART_TICKS \
        	(alt_ticks_per_second()/ALT_LOG_JTAG_UART_TICKS_DIVISOR)
    #endif
  
    /* if there's a JTAG UART defined, then enable these macros */
    #ifdef __ALTERA_AVALON_JTAG_UART 

        /* Macro in altera_avalon_jtag_uart.c, to register the alarm function.
         * Also, the startup register info is also printed here, as this is
         * called within the device driver initialization.  */
        #define ALT_LOG_JTAG_UART_ALARM_REGISTER(dev, base) \
            do { if (alt_log_jtag_uart_alarm_on_flag==1) { \
                    alt_alarm_start(&alt_log_jtag_uart_alarm_1, \
                    ALT_LOG_JTAG_UART_TICKS, &altera_avalon_jtag_uart_report_log,\
                    dev);} \
                 if (alt_log_jtag_uart_startup_info_on_flag==1) {\
                    alt_log_jtag_uart_startup_info(dev, base);} \
               } while (0)
  
        /* JTAG UART IRQ Logging (when buffer is empty) 
         * Inserted in the ISR in altera_avalon_jtag_uart.c */
        #define ALT_LOG_JTAG_UART_ISR_FUNCTION(base, dev) \
            do { alt_log_jtag_uart_isr_proc(base, dev); } while (0) 
    /* else, define macros to nothing.  Or else the jtag_uart specific types
     * will throw compiler errors */
    #else
        #define ALT_LOG_JTAG_UART_ALARM_REGISTER(dev, base) 
        #define ALT_LOG_JTAG_UART_ISR_FUNCTION(base, dev) 
    #endif

    /* System clock logging
     * How often (in seconds) the system clock logging prints.
     * The default value is every 1 second */
    #ifndef ALT_LOG_SYS_CLK_INTERVAL_MULTIPLIER
	#define ALT_LOG_SYS_CLK_INTERVAL_MULTIPLIER 1
    #endif
    #ifndef ALT_LOG_SYS_CLK_INTERVAL
	#define ALT_LOG_SYS_CLK_INTERVAL \
	    (alt_ticks_per_second()*ALT_LOG_SYS_CLK_INTERVAL_MULTIPLIER)
    #endif

    /* System clock logging - prints a message every interval (set above) 
     * to show that the system clock is alive. 
     * This macro is used in altera_avalon_timer_sc.c */
    #define ALT_LOG_SYS_CLK_HEARTBEAT() \
    	do { alt_log_system_clock(); } while (0)

    /* alt_write_logging - echos a message every time write() is called,
     * displays the first ALT_LOG_WRITE_ECHO_LEN characters.
     * This macro is used in alt_write.c */
    #ifndef ALT_LOG_WRITE_ECHO_LEN
        #define ALT_LOG_WRITE_ECHO_LEN 15
    #endif

    #define ALT_LOG_WRITE_FUNCTION(ptr,len) \
        do { alt_log_write(ptr,len); } while (0)

#else /* ALT_LOG_ENABLE not defined */

    /* logging is off, set all relevant macros to null */
    #define ALT_LOG_PRINT_BOOT(...)
    #define ALT_LOG_PRINTF(...)
    #define ALT_LOG_JTAG_UART_ISR_FUNCTION(base, dev) 
    #define ALT_LOG_JTAG_UART_ALARM_REGISTER(dev, base) 
    #define ALT_LOG_SYS_CLK_HEARTBEAT()
    #define ALT_LOG_PUTS(str) 
    #define ALT_LOG_WRITE_FUNCTION(ptr,len) 

#endif /* ALT_LOG_ENABLE */

#endif /* __ALT_LOG_PRINTF_H__ */
