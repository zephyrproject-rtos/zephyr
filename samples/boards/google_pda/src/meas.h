#ifndef __MEAS_H__
#define __MEAS_H__

int meas_init(void);
void meas_vbus(int32_t *v);
void meas_cbus(int32_t *c);
void meas_vcc1(int32_t *v);
void meas_vcc2(int32_t *v);
void meas_ccon(int32_t *c);

#endif
