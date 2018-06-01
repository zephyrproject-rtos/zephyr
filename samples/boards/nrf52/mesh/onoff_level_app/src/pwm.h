#ifndef _PWM_H
#define _PWM_H

#define ZERO_INTENSITY 125

extern uint16_t pwm_seq[4];

void pwm_init(int, int);
void pwm_enable(void);
void pwm_disable(void);
void pwm_power_colour_smooth(unsigned char, unsigned char);
void pwm_power_colour(unsigned char, unsigned char);

#endif
