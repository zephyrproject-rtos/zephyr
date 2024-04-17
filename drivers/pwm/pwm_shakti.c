#define DT_DRV_COMPAT shakti_pwm

#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
// #include <zephyr/kernel.h>
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

#define CLOCK_FREQUENCY 40000000

/*!Pulse Width Modulation Start Offsets */
#define PWM_MAX_COUNT 8
#define PWM_BASE_ADDRESS 0x00030000 /*PWM Base address*/
#define PWM_END_ADDRESS 0x000307FF /*PWM End address*/

#define PWM_MODULE_OFFSET 0x00000100 /*Offset value to be incremented for each interface*/

/*!Pulse Width Modulation Offsets */
#define PWM_START_0 0x00030000 /* Pulse Width Modulation 0 */
#define PWM_START_1 0x00030100 /* Pulse Width Modulation 1 */
#define PWM_START_2 0x00030200 /* Pulse Width Modulation 2 */
#define PWM_START_3 0x00030300 /* Pulse Width Modulation 3 */
#define PWM_START_4 0x00030400 /* Pulse Width Modulation 4 */
#define PWM_START_5 0x00030500 /* Pulse Width Modulation 5 */
#define PWM_START_6 0x00030600 /* Pulse Width Modulation 6 */
#define PWM_START_7 0x00030700 /* Pulse Width Modulation 7 */

typedef enum
{
	rise_interrupt,				//Enable interrupt only on rise
	fall_interrupt,				//Enable interrupt only on fall
	halfperiod_interrupt,			//Enable interrupt only on halfperiod
	no_interrupt				//Disable interrupts
}pwm_interrupt_modes;

/* Structure Declarations */
typedef struct
{
  uint8_t n;
  uint16_t clock;       /*! pwm clock register 16 bits*/
  uint16_t reserved0;
  uint16_t control;     /*! pwm control register 16 bits*/
  uint16_t reserved1;   
  uint32_t period;      /*! pwm period register 32 bits */
  uint32_t duty;        /*! pwm duty cycle register 32 bits*/
  uint16_t deadband_delay;    /*! pwm deadband delay register 16 bits */
  uint16_t reserved2;
}pwm_struct;

struct pwm_shakti_data {};

struct pwm_shakti_cfg {
	uint32_t base;
	uint32_t f_sys;
	uint32_t cmpwidth;
	const struct pinctrl_dev_config *pcfg;
};

volatile pwm_struct *pwm_instance[PWM_MAX_COUNT];

int db_config[2] = DT_PROP(DT_NODELABEL(pwm0), db_configure);

/* Functions */
static int pwm_shakti_init(const struct device *dev,int pwm_number)
{
	const struct pwm_shakti_cfg *cfg = dev->config;
	pwm_instance[pwm_number] = (pwm_struct *) (PWM_BASE_ADDRESS + (pwm_number * PWM_MODULE_OFFSET) );
	// printf("\n pwm_instance[%x]: %x",  i, pwm_instance[i]);
	//log_info("\n Initilization Done");
}

// void pwm_set_cycles(const struct device *dev, int pwm_number, uint32_t period, uint32_t pulse)

// int pwm_set_cycles(const struct device *dev, uint32_t pwm_number, uint32_t period, uint32_t pulse)
// {
// 	const struct pwm_shakti_cfg *cfg = dev->config;
// 	uint32_t duty = pulse/period *100;
// 	pwm_instance[pwm_number]->duty=duty;

// 	log_debug("\n Duty Register of module number %d set to %x", pwm_number, duty);
// 	pwm_instance[pwm_number]->period=period;

// 	log_debug("\n Period Register of module number %d set to %x", pwm_number, period);
// 	return 0;
// }

int pwm_set_control(const struct device *dev,int pwm_number, uint32_t value)
{
	printk("\n4");
	const struct pwm_shakti_cfg *cfg = dev->config;
	pwm_instance[pwm_number]->control |= value;

	//log_debug("\n Control Register of module number %d set to %x", pwm_number, value);

	return 1;
}

void pwm_set_prescalar_value(const struct device *dev,int pwm_number, uint16_t prescalar_value)
{
	printk("\n2");
	const struct pwm_shakti_cfg *cfg = dev->config;
	if( 32768 < prescalar_value )
	{
		//log_error("Prescaler value should be less than 32768");
		return;
	}
	pwm_instance[pwm_number]->clock = (prescalar_value << 1);
}

inline int configure_control(bool update, pwm_interrupt_modes interrupt_mode, bool change_output_polarity)
{
	int value = 0x0;

	if(update==1)
	{
		value |=PWM_UPDATE_ENABLE;
	}

	if(interrupt_mode==0 || interrupt_mode==3 || interrupt_mode==5 || interrupt_mode==6)
	{
		value |= PWM_RISE_INTERRUPT_ENABLE;
	}

	if(interrupt_mode==1 || interrupt_mode==3 || interrupt_mode==4 || interrupt_mode==6)
	{
		value |= PWM_FALL_INTERRUPT_ENABLE;
	}

	if(interrupt_mode==2 || interrupt_mode==4 || interrupt_mode==5 || interrupt_mode==6)
	{
		value |= PWM_HALFPERIOD_INTERRUPT_ENABLE;
	}

	if(change_output_polarity)
	{
		value |= PWM_OUTPUT_POLARITY;
	}

	return value;
}

void pwm_configure(const struct device *dev,int pwm_number, uint32_t period, uint32_t duty, pwm_interrupt_modes interrupt_mode, uint32_t deadband_delay, bool change_output_polarity)
{
	printk("\n3");
	const struct pwm_shakti_cfg *cfg = dev->config;
	pwm_instance[pwm_number]->duty=duty;
	pwm_instance[pwm_number]->period=period;
	pwm_instance[pwm_number]->deadband_delay = deadband_delay;

	int control = configure_control( false, interrupt_mode, change_output_polarity);

	pwm_instance[pwm_number]->control=control;

	//log_debug("PWM %d succesfully configured with %x",pwm_number, pwm_instance[pwm_number]->control);
}

void pwm_start(const struct device *dev,int pwm_number)
{
	printk("\n5");
	const struct pwm_shakti_cfg *cfg = dev->config;
	int value= 0x0;
	value = pwm_instance[pwm_number]->control ;

	value |= (PWM_UPDATE_ENABLE | PWM_ENABLE | PWM_START);

	pwm_instance[pwm_number]->control = value;
}

void pwm_stop(const struct device *dev,int pwm_number)
{
	const struct pwm_shakti_cfg *cfg = dev->config;
	int value = 0xfff8;  //it will set pwm_enable,pwm_start,pwm_output_enable  to zero
	pwm_instance[pwm_number]->control &= value;
	//log_debug("\n PWM module number %d has been stopped", pwm_number);
}

static int pwm_shakti_set_cycles(const struct device *dev, uint32_t channel,
				 uint32_t period_cycles, uint32_t pulse_cycles)
{
	//db_configure(deadband_delay,polarity,control_reg,prescale)
	printk("\n1");
	uint32_t deadband_delay = db_config[0];
	uint32_t control_reg = db_config[1];
	uint16_t prescale = db_config[2];
	bool polarity = 0;

	pwm_set_prescalar_value(dev,PWM_0, prescale);
    pwm_configure(dev,PWM_0, period_cycles, pulse_cycles, no_interrupt, deadband_delay, polarity);
    pwm_set_control(dev,PWM_0, control_reg);
    pwm_start(dev,PWM_0);
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
