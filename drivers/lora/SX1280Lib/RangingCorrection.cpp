#include "RangingCorrection.h"

#include "rangingCorrection/rangingCorrectionSF5BW0400.h"
#include "rangingCorrection/rangingCorrectionSF6BW0400.h"
#include "rangingCorrection/rangingCorrectionSF7BW0400.h"
#include "rangingCorrection/rangingCorrectionSF8BW0400.h"
#include "rangingCorrection/rangingCorrectionSF9BW0400.h"
#include "rangingCorrection/rangingCorrectionSF10BW0400.h"
#include "rangingCorrection/rangingCorrectionSF5BW0800.h"
#include "rangingCorrection/rangingCorrectionSF6BW0800.h"
#include "rangingCorrection/rangingCorrectionSF7BW0800.h"
#include "rangingCorrection/rangingCorrectionSF8BW0800.h"
#include "rangingCorrection/rangingCorrectionSF9BW0800.h"
#include "rangingCorrection/rangingCorrectionSF10BW0800.h"
#include "rangingCorrection/rangingCorrectionSF5BW1600.h"
#include "rangingCorrection/rangingCorrectionSF6BW1600.h"
#include "rangingCorrection/rangingCorrectionSF7BW1600.h"
#include "rangingCorrection/rangingCorrectionSF8BW1600.h"
#include "rangingCorrection/rangingCorrectionSF9BW1600.h"
#include "rangingCorrection/rangingCorrectionSF10BW1600.h"

const double* RangingCorrectionPerSfBwGain[6][3] = {
    { &RangingCorrectionSF5BW0400[0],  &RangingCorrectionSF5BW0800[0],  &RangingCorrectionSF5BW1600[0] },
    { &RangingCorrectionSF6BW0400[0],  &RangingCorrectionSF6BW0800[0],  &RangingCorrectionSF6BW1600[0] },
    { &RangingCorrectionSF7BW0400[0],  &RangingCorrectionSF7BW0800[0],  &RangingCorrectionSF7BW1600[0] },
    { &RangingCorrectionSF8BW0400[0],  &RangingCorrectionSF8BW0800[0],  &RangingCorrectionSF8BW1600[0] },
    { &RangingCorrectionSF9BW0400[0],  &RangingCorrectionSF9BW0800[0],  &RangingCorrectionSF9BW1600[0] },
    { &RangingCorrectionSF10BW0400[0], &RangingCorrectionSF10BW0800[0], &RangingCorrectionSF10BW1600[0] },
};

const RangingCorrectionPolynomes_t* RangingCorrectionPolynomesPerSfBw[6][3] = {
    { &correctionRangingPolynomeSF5BW0400,  &correctionRangingPolynomeSF5BW0800,  &correctionRangingPolynomeSF5BW1600 },
    { &correctionRangingPolynomeSF6BW0400,  &correctionRangingPolynomeSF6BW0800,  &correctionRangingPolynomeSF6BW1600 },
    { &correctionRangingPolynomeSF7BW0400,  &correctionRangingPolynomeSF7BW0800,  &correctionRangingPolynomeSF7BW1600 },
    { &correctionRangingPolynomeSF8BW0400,  &correctionRangingPolynomeSF8BW0800,  &correctionRangingPolynomeSF8BW1600 },
    { &correctionRangingPolynomeSF9BW0400,  &correctionRangingPolynomeSF9BW0800,  &correctionRangingPolynomeSF9BW1600 },
    { &correctionRangingPolynomeSF10BW0400, &correctionRangingPolynomeSF10BW0800, &correctionRangingPolynomeSF10BW1600 },
};

double Sx1280RangingCorrection::GetRangingCorrectionPerSfBwGain( const RadioLoRaSpreadingFactors_t sf, const RadioLoRaBandwidths_t bw, const int8_t gain){
    uint8_t sf_index, bw_index;
    
    switch(sf){
        case LORA_SF5:
            sf_index = 0;
            break;
        case LORA_SF6:
            sf_index = 1;
            break;
        case LORA_SF7:
            sf_index = 2;
            break;
        case LORA_SF8:
            sf_index = 3;
            break;
        case LORA_SF9:
            sf_index = 4;
            break;
        case LORA_SF10:
            sf_index = 5;
            break;
    }
    switch(bw){
        case LORA_BW_0400:
            bw_index = 0;
            break;
        case LORA_BW_0800:
            bw_index = 1;
            break;
        case LORA_BW_1600:
            bw_index = 2;
            break;
    }
    
    double correction = RangingCorrectionPerSfBwGain[sf_index][bw_index][gain];
    return correction;
}

double Sx1280RangingCorrection::ComputeRangingCorrectionPolynome(const RadioLoRaSpreadingFactors_t sf, const RadioLoRaBandwidths_t bw, const double median){
    uint8_t sf_index, bw_index;

    switch(sf){
        case LORA_SF5:
            sf_index = 0;
            break;
        case LORA_SF6:
            sf_index = 1;
            break;
        case LORA_SF7:
            sf_index = 2;
            break;
        case LORA_SF8:
            sf_index = 3;
            break;
        case LORA_SF9:
            sf_index = 4;
            break;
        case LORA_SF10:
            sf_index = 5;
            break;
    }
    switch(bw){
        case LORA_BW_0400:
            bw_index = 0;
            break;
        case LORA_BW_0800:
            bw_index = 1;
            break;
        case LORA_BW_1600:
            bw_index = 2;
            break;
    }
    const RangingCorrectionPolynomes_t *polynome = RangingCorrectionPolynomesPerSfBw[sf_index][bw_index];
    double correctedValue = 0.0;
    double correctionCoeff = 0;
    for(uint8_t order = 0; order < polynome->order; order++){
        correctionCoeff = polynome->coefficients[order] * pow(median, polynome->order - order - 1);
        correctedValue += correctionCoeff;
    }
    return correctedValue;
}
