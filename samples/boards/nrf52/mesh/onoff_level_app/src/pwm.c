#include "common.h"

uint16_t pwm_seq[4] = {ZERO_INTENSITY, ZERO_INTENSITY, 0, 0};

void pwm_init(int led0, int led1)
{

  NRF_P0->DIR |= (1 << led0) | (1 << led1);

  CLRB(NRF_P0->OUT,13);
  CLRB(NRF_P0->OUT,14);

  NRF_PWM0->PSEL.OUT[0] = led0;
  NRF_PWM0->PSEL.OUT[1] = led1;
  
  NRF_PWM0->MODE = 0x00000000;

  NRF_PWM0->PRESCALER = 0x00000007; //125 Khz

  NRF_PWM0->COUNTERTOP = ZERO_INTENSITY;  //1 Khz

  NRF_PWM0->DECODER = 0x00000002;
  
  NRF_PWM0->LOOP = 0x00000000;
  
  NRF_PWM0->SEQ[0].REFRESH = 0;
  
  NRF_PWM0->SEQ[0].ENDDELAY = 0;

  NRF_PWM0->SEQ[0].PTR = (uint32_t)(pwm_seq);
  
  NRF_PWM0->SEQ[0].CNT = 4;

  NRF_PWM0->ENABLE = 0x00000001;
  
  NRF_PWM0->TASKS_SEQSTART[0] = 1;

}

void pwm_enable(void)
{
	NRF_PWM0->ENABLE = 0x00000001;

	NRF_PWM0->TASKS_SEQSTART[0] = 1;
}

void pwm_disable(void)
{
	CLRB(NRF_P0->OUT,13);
	CLRB(NRF_P0->OUT,14);

	NRF_PWM0->ENABLE = 0x00000000;
}

void pwm_power_colour_smooth(unsigned char power, unsigned char colour)
{
	static unsigned char last_power = 0, last_colour = 0;
	unsigned char tmp = 0;

	//printk("\n\rpower = %d, colour = %d\n\r", power, colour);

	if(power < 0)
	{
		pwm_seq[0] = ZERO_INTENSITY;
		pwm_seq[1] = ZERO_INTENSITY;
		
		NRF_PWM0->TASKS_SEQSTART[0] = 1;

		last_power = 0;
	}
	else
	{	
		if(power < last_power)
		{
			while(power < last_power)
			{
				last_power--;

				tmp = (u8_t)(colour * last_power * 0.01);

				pwm_seq[0] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * (last_power - tmp) * 0.01));

				pwm_seq[1] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * tmp * 0.01));

				NRF_PWM0->TASKS_SEQSTART[0] = 1;
		
				k_sleep(1);
			}

			last_colour = tmp;
		}
		else if(power > last_power)
		{
			while(power > last_power)
			{
				last_power++;

				tmp = (u8_t)(colour * last_power * 0.01);

				pwm_seq[0] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * (last_power - tmp) * 0.01));

				pwm_seq[1] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * tmp * 0.01));	

				NRF_PWM0->TASKS_SEQSTART[0] = 1;

				k_sleep(1);
			}

			last_colour = tmp;
		}
		else
		{
			tmp = (u8_t)(colour * power * 0.01);

			if(tmp < last_colour)
			{
				while(tmp < last_colour)
				{
					last_colour--;

					pwm_seq[0] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * (power - last_colour) * 0.01));

					pwm_seq[1] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * last_colour * 0.01));	

					NRF_PWM0->TASKS_SEQSTART[0] = 1;

					k_sleep(1);
				}
			}
			else if(tmp > last_colour)
			{
				while(tmp > last_colour)
				{
					last_colour++;
					
					pwm_seq[0] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * (power - last_colour) * 0.01));

					pwm_seq[1] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * last_colour * 0.01));	

					NRF_PWM0->TASKS_SEQSTART[0] = 1;
					
					k_sleep(1);
				}
			}
			else
			{
				pwm_seq[0] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * (power - last_colour) * 0.01));

				pwm_seq[1] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * last_colour * 0.01));	

				NRF_PWM0->TASKS_SEQSTART[0] = 1;
			}	
		}
	}

}

void pwm_power_colour(unsigned char power, unsigned char colour)
{
	unsigned char tmp;
	
	if(power < 0)
	{
		pwm_seq[0] = ZERO_INTENSITY;
		pwm_seq[1] = ZERO_INTENSITY;
		
		NRF_PWM0->TASKS_SEQSTART[0] = 1;

	}
	else
	{
		tmp = (u8_t)(colour * power * 0.01);

		pwm_seq[0] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * (power - tmp) * 0.01));

		pwm_seq[1] = (u16_t)(ZERO_INTENSITY - (ZERO_INTENSITY * tmp * 0.01));	

		NRF_PWM0->TASKS_SEQSTART[0] = 1;
	}

}

