#ifndef __RANGING_CORRECTION_DEFINES_H__
#define __RANGING_CORRECTION_DEFINES_H__

#include <stdint.h>

#define NUMBER_OF_FACTORS_PER_SFBW 160
#define MAX_POLYNOME_ORDER         10

typedef struct{
    const uint8_t order;
    const double coefficients[MAX_POLYNOME_ORDER];
}RangingCorrectionPolynomes_t;

#endif // __RANGING_CORRECTION_DEFINES_H__

