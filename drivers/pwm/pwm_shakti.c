/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : pwm_shakti.c
 * Brief Description of file         : This is a zephyr rtos PWM Driver file for Mindgrove Silicon's PWM Peripheral.
 * Name of Author                    : Harini Sree.S
 * Email ID                          : harini@mindgrovetech.in
 * 
 * @file pwm_shakti.c
 * @author Harini Sree.S (harini@mindgrovetech.in)
 * @brief This is a zephyr rtos PWM Driver file for Mindgrove Silicon's PWM Peripheral.
 * @version 0.1
 * @date 2024-05-03
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 * 
 * @copyright Copyright (c) 2017 Google LLC.
 * @copyright Copyright (c) 2018 qianfan Zhao.
 * @copyright Copyright (c) 2023 Gerson Fernando Budke.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT shakti_pwm

#include <stdint.h>
#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/pwm.h>
#include<stdbool.h>
#include <stdio.h>


/* Macros */
/* Register Offsets */
#define PERIOD_REGISTER_MAX   0xFFFFFFFF
#define DUTY_REGISTER_MAX         0xFFFFFFFF
#define CONTROL_REGISTER_MAX          0x0000FFFF
#define DEADBAND_DELAY_REGISTER_MAX   0x0000FFFF

/* pwmcfg Bit Offsets */
#define PWM_0 0
#define PWM_1 1
#define PWM_2 2
#define PWM_3 3
#define PWM_4 4
#define PWM_5 5
#define PWM_6 6
#define PWM_7 7

// Control Register Individual Bits
#define PWM_ENABLE                      0x00000001
#define PWM_START                       0x00000002
#define PWM_OUTPUT_ENABLE               0x00000004
#define PWM_OUTPUT_POLARITY             0x00000008
#define PWM_COUNTER_RESET               0x00000010
#define PWM_HALFPERIOD_INTERRUPT_ENABLE 0x00000040
#define PWM_FALL_INTERRUPT_ENABLE       0x00000080
#define PWM_RISE_INTERRUPT_ENABLE       0x00000100
#define PWM_HALFPERIOD_INTERRUPT        0x00000200
#define PWM_FALL_INTERRUPT              0x00000400
#define PWM_RISE_INTERRUPT              0x00000800
#define PWM_UPDATE_ENABLE               0x00001000 

// Clock frequency
#define CLOCK_FREQUENCY 40000000

/*!Pulse Width Modulation Start Offsets */
#define PWM_MAX_COUNT 8				/*Maximum number of PWM*/
#define PWM_BASE_ADDRESS 0x00030000 /*PWM Base address*/
#define PWM_END_ADDRESS 0x000307FF  /*PWM End address*/

#define PWM_MODULE_OFFSET 0x00000100 /*Offset value to be incremented for each interface*/

/*!Pulse Width Modulation Offsets */
#define PWM_START_0 0x00030000 	/* Pulse Width Modulation 0 */
#define PWM_START_1 0x00030100 	/* Pulse Width Modulation 1 */
#define PWM_START_2 0x00030200 	/* Pulse Width Modulation 2 */
#define PWM_START_3 0x00030300 	/* Pulse Width Modulation 3 */
#define PWM_START_4 0x00030400 	/* Pulse Width Modulation 4 */
#define PWM_START_5 0x00030500	/* Pulse Width Modulation 5 */
#define PWM_START_6 0x00030600 	/* Pulse Width Modulation 6 */
#define PWM_START_7 0x00030700 	/* Pulse Width Modulation 7 */

/*pinmux*/
#define PINMUX_START 0x40300 			/*Pinmux start address*/
#define PINMUX_CONFIGURE_REG 0x40300	/*Pinmux configuration register*/

#define PWM_BASE                   0x00030000UL
#define PWM_OFFSET                 0x00000100UL

/*Interrupt enum*/
typedef enum
{
	rise_interrupt,				//Enable interrupt only on rise
	fall_interrupt,				//Enable interrupt only on fall
	halfperiod_interrupt,		//Enable interrupt only on halfperiod
	no_interrupt				//Disable interrupts
}pwm_interrupt_modes;

/* Structure Declarations */
/* PWM registers*/
typedef struct
{
  uint16_t clock;       	/*pwm clock register 16 bits*/
  uint16_t reserved0;		/*reserved for future use and currently has no defined purpose 16 bits*/
  uint16_t control;     	/*pwm control register 16 bits*/
  uint16_t reserved1;   	/*reserved for future use and currently has no defined purpose 16 bits*/
  uint32_t period;      	/*pwm period register 32 bits */
  uint32_t duty;        	/*pwm duty cycle register 32 bits*/
  uint16_t deadband_delay;	/*pwm deadband delay register 16 bits */
  uint16_t reserved2;		/*reserved for future use and currently has no defined purpose 16 bits*/
}PWM_Type;

/*Data structure*/
struct pwm_shakti_data {};

/*Zephyr configuration structure*/
struct pwm_shakti_cfg {
	uint32_t base;
	uint32_t f_sys;
	uint32_t cmpwidth;
	const struct pinctrl_dev_config *pcfg;
};

#define PWM_REG(x)      ((PWM_Type*)(PWM_BASE + (x)*PWM_OFFSET))

volatile unsigned int* pinmux_config_reg = (volatile unsigned int* ) PINMUX_CONFIGURE_REG;

/* Functions */

/** @fn  pwm_init
 * @brief Function to initialize all pwm modules
 * @details This function will be called to initialize all pwm modules
 * @param[in] dev The device declared in devicetree
 * @param[Out] No output parameter
 */
static int pwm_shakti_init(const struct device *dev)
{
	const struct pwm_shakti_cfg *cfg = dev->config;
	char *pwm_no;
	pwm_no = dev->name;
	int channel = pwm_no[6] - '0';
	printk("\nInit of PWM %d",channel);
}

/** @fn  pwm_set_control
 * @brief Function to set the control register of the selected pwm module
 * @details This function will be called to set the value of the control register for the selected module
 * @param[in] uint32_t (channel- specifies the pwm smodule to be selected)
 * @param[in] uint32_t (value - value to be set between 0x0000 to 0xffff.)
 * @param[Out] uint32_t (returns 1 on success, 0 on failure.)
 */
int pwm_set_control(int channel, uint32_t value)
{
	PWM_REG(channel)->control |= value;
	return 1;
}

/** @fn pwm_set_prescalar_value
 * @brief Function to set the prescalar value of a specific pwm cluster		
 * @details This function will set the prescalar value of a specific pwm cluster
 * @param[in] uint32_t (channel-  the pwm channel to be selected)
 * @param[in] uint32_t (prescalar_value-  value of prescalar values which is used to divide the clock frequency.)
 * @param[Out] No output parameter
 */
void pwm_set_prescalar_value(int channel, uint16_t prescalar_value)
{
	if( 32768 < prescalar_value )
	{
		return;
	}
	PWM_REG(channel)->clock = (prescalar_value << 1);
}

/** @fn  configure_control_mode
 * @brief Function to set value of control register based on parameteres
 * @details This function will set value of control register based on parameters
 * @param[in]  bool          (update                      - specifies if the module is to be updated)
 *           interrupt_mode  (interrupt_mode              - it specifes the mode of the interrupt)
 *           bool            (change_output_polarity      - it specifies if the output polarity should be changed) 
 * @param[Out] uint32_t (returns value to be set in the control register.)
 */
inline int configure_control(bool update, pwm_interrupt_modes interrupt_mode, bool change_output_polarity)
{
	int value = 0x0;

	if(update==1)
	{
		value |=PWM_UPDATE_ENABLE;
	}

	if(interrupt_mode==0)
	{
		value |= PWM_RISE_INTERRUPT_ENABLE;
	}

	if(interrupt_mode==1)
	{
		value |= PWM_FALL_INTERRUPT_ENABLE;
	}

	if(interrupt_mode==2)
	{
		value |= PWM_HALFPERIOD_INTERRUPT_ENABLE;
	}

	if(change_output_polarity)
	{
		value |= PWM_OUTPUT_POLARITY;
	}

	return value;
}

/** @fn  pwm_configure
 * @brief Function to configure the pwm module with all the values required like channel, period, duty, interrupt_mode, deadband_delay, change_output_polarity_
 * @details This function configure the pwm module
 * @param[in] uint32_t (channel - the pwm module to be selected)
 * @param[in] uint32_t(period - value of periodic cycle to be used. the signal resets after every count of the periodic cycle)
 * @param[in] uint32_t(duty - value of duty cycle. It specifies how many cycles the signal is active out of the periodic cycle )
 * @param[in] pwm_interrupt_modes(interrupt_mode - value of interrupt mode. It specifies the interrupt mode to be used)
 * @param[in] uint32_t(deadband_delay - value of deadband_delay. It specifies the deadband_delay to be used.)
 * @param[in] bool (change_output_polarity - value of change_output_polarity. It specifies if output polarity is to be changed.)
 * @param[Out] No output parameter
 */
void pwm_configure(int channel, uint32_t period, uint32_t duty, pwm_interrupt_modes interrupt_mode, uint32_t deadband_delay, bool change_output_polarity)
{
	PWM_REG(channel)->duty=duty;                    
	PWM_REG(channel)->period=period;
	PWM_REG(channel)->deadband_delay = deadband_delay;

	int control = configure_control( false, interrupt_mode, change_output_polarity);

	PWM_REG(channel)->control=control;
}

/** @fn pwm_start
 * @brief Function to start a specific pwm module
 * @details This function will start the specific pwm module
 * @param[in] uint32_t (channel-  the pwm module to be selected)
 * @param[Out] No output parameter
 */
void pwm_start(int channel)
{
	int value= 0x0;
	value = PWM_REG(channel)->control ;

	value |= (PWM_UPDATE_ENABLE | PWM_ENABLE | PWM_START);

	PWM_REG(channel)->control = value;
}

/** @fn pinmux_enable_pwm
 * @brief Function to configure the pin as pwm module
 * @details This function will be called to configure the pin as pwm module
 * @param[in] uint32_t (num - specifies the pwm module to be selected)
 * @param[Out] No output parameter
 */
void pinmux_enable_pwm(int num)
{
	if (num < 8)
	{
		*(pinmux_config_reg + num) = 1;
	}
	else
		printf("Max pinmuxed PWMs are 8");
}

/** @fn pwm_clear
 * @brief Function to clear all registers in a specific pwm module
 * @details This function will be called to clear all registers in a specific pwm module
 * @param[in] uint32_t (pwm_number- specifies the pwm module to be selected)
 * @param[Out] No output parameter
 */
void pwm_clear(const struct device *dev, int channel)
{
	PWM_REG(channel)->control=0;
	PWM_REG(channel)->duty=0;
	PWM_REG(channel)->period=0;
	PWM_REG(channel)->deadband_delay=0;
}

/** @fn pwm_shakti_set_cycles
 * @brief Set the period and pulse width for a specific pwm module
 * @details This function will set the period and pulse width for a specific pwm module
 * @param[in] dev The device declared in devicetree
 * 			uint32_t (channel-  the pwm module to be selected)
 * 			uint32_t (period_cycles- value of periodic cycle to be used)
 * 			uint32_t (pulse_cycles- value of duty cycle)
 * 			pwm_flags_t (flags- polarity of a PWM channel.)
 * @param[Out] No output parameter
 */
static int pwm_shakti_set_cycles(const struct device *dev, uint32_t channel,uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_shakti_cfg *cfg = dev->config;
	uint32_t deadband_delay;	/*32bit field to store deadband delay value*/ 
	uint32_t control_reg;		/*32bit field to store control register value*/
	uint16_t prescale;			/*32bit field to store prescale value*/

	/*Switch case to obtain deadband delay, control register value and prescale from the devicetree for the given channel*/
	switch (channel)
	{
	case PWM_0:
		int db_config0[] = DT_PROP(DT_NODELABEL(pwm0), db_configure);
		deadband_delay = db_config0[0];
		control_reg = db_config0[1];
		prescale = db_config0[2];
		break;
	case PWM_1:
		int db_config1[] = DT_PROP(DT_NODELABEL(pwm1), db_configure);
		deadband_delay = db_config1[0];
		control_reg = db_config1[1];
		prescale = db_config1[2];
		break;
	case PWM_2:
		int db_config2[] = DT_PROP(DT_NODELABEL(pwm2), db_configure);
		deadband_delay = db_config2[0];
		control_reg = db_config2[1];
		prescale = db_config2[2];
		break;
	case PWM_3:
		int db_config3[] = DT_PROP(DT_NODELABEL(pwm3), db_configure);
		deadband_delay = db_config3[0];
		control_reg = db_config3[1];
		prescale = db_config3[2];
		break;
	case PWM_4:
		int db_config4[] = DT_PROP(DT_NODELABEL(pwm4), db_configure);
		deadband_delay = db_config4[0];
		control_reg = db_config4[1];
		prescale = db_config4[2];
		break;
	case PWM_5:
		int db_config5[] = DT_PROP(DT_NODELABEL(pwm5), db_configure);
		deadband_delay = db_config5[0];
		control_reg = db_config5[1];
		prescale = db_config5[2];
		break;
	case PWM_6:
		int db_config6[] = DT_PROP(DT_NODELABEL(pwm6), db_configure);
		deadband_delay = db_config6[0];
		control_reg = db_config6[1];
		prescale = db_config6[2];
		break;
	case PWM_7:
		int db_config7[] = DT_PROP(DT_NODELABEL(pwm7), db_configure);
		deadband_delay = db_config7[0];
		control_reg = db_config7[1];
		prescale = db_config7[2];
		break;
	default:
		break;
	}

	// printf("\nDeadband_delay %u",deadband_delay);
	// printf("\nControl_reg %u",control_reg);
	// printf("\nPrescale value %u",prescale);

	pinmux_enable_pwm(channel);
	pwm_set_prescalar_value(channel, prescale);
    pwm_configure(channel, period_cycles, pulse_cycles, no_interrupt, deadband_delay, flags);
    pwm_set_control(channel, control_reg);
    pwm_start(channel);
}

/* Device Instantiation */

static const struct pwm_driver_api pwm_shakti_api = {
	.set_cycles = pwm_shakti_set_cycles,
};
// .cmpwidth = DT_INST_PROP(n, shakti_compare_width), \

#define PWM_SHAKTI_INIT(n)	\
	static struct pwm_shakti_data pwm_shakti_data_##n;	\
	static const struct pwm_shakti_cfg pwm_shakti_cfg_##n = {	\
			.base = PWM_START_##n ,	\
			.f_sys = CLOCK_FREQUENCY,  \
		};	\
	DEVICE_DT_INST_DEFINE(n,	\
			    pwm_shakti_init,	\
			    NULL,	\
			    &pwm_shakti_data_##n,	\
			    &pwm_shakti_cfg_##n,	\
			    POST_KERNEL,	\
			    CONFIG_PWM_INIT_PRIORITY,	\
			    &pwm_shakti_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SHAKTI_INIT)
