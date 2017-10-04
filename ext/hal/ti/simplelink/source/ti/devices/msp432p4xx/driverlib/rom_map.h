//*****************************************************************************
//
// rom_map.h - Macros to facilitate calling functions in the ROM when they are
//             available and in flash otherwise.
//
// Copyright (c) 2013 Texas Instruments Incorporated.  All rights reserved.
// TI Information - Selective Disclosure
//
//
//*****************************************************************************

#ifndef __ROM_MAP_H__
#define __ROM_MAP_H__

//*****************************************************************************
//
// Macros for the ADC14 API.
//
//*****************************************************************************
#ifdef ROM_ADC14_enableModule
#define MAP_ADC14_enableModule                                                \
        ROM_ADC14_enableModule
#else
#define MAP_ADC14_enableModule                                                \
        ADC14_enableModule
#endif
#ifdef ROM_ADC14_disableModule
#define MAP_ADC14_disableModule                                               \
        ROM_ADC14_disableModule
#else
#define MAP_ADC14_disableModule                                               \
        ADC14_disableModule
#endif
#ifdef ROM_ADC14_initModule
#define MAP_ADC14_initModule                                                  \
        ROM_ADC14_initModule
#else
#define MAP_ADC14_initModule                                                  \
        ADC14_initModule
#endif
#ifdef ROM_ADC14_setResolution
#define MAP_ADC14_setResolution                                               \
        ROM_ADC14_setResolution
#else
#define MAP_ADC14_setResolution                                               \
        ADC14_setResolution
#endif
#ifdef ROM_ADC14_getResolution
#define MAP_ADC14_getResolution                                               \
        ROM_ADC14_getResolution
#else
#define MAP_ADC14_getResolution                                               \
        ADC14_getResolution
#endif
#ifdef ROM_ADC14_setSampleHoldTrigger
#define MAP_ADC14_setSampleHoldTrigger                                        \
        ROM_ADC14_setSampleHoldTrigger
#else
#define MAP_ADC14_setSampleHoldTrigger                                        \
        ADC14_setSampleHoldTrigger
#endif
#ifdef ROM_ADC14_setSampleHoldTime
#define MAP_ADC14_setSampleHoldTime                                           \
        ROM_ADC14_setSampleHoldTime
#else
#define MAP_ADC14_setSampleHoldTime                                           \
        ADC14_setSampleHoldTime
#endif
#ifdef ROM_ADC14_configureMultiSequenceMode
#define MAP_ADC14_configureMultiSequenceMode                                  \
        ROM_ADC14_configureMultiSequenceMode
#else
#define MAP_ADC14_configureMultiSequenceMode                                  \
        ADC14_configureMultiSequenceMode
#endif
#ifdef ROM_ADC14_configureSingleSampleMode
#define MAP_ADC14_configureSingleSampleMode                                   \
        ROM_ADC14_configureSingleSampleMode
#else
#define MAP_ADC14_configureSingleSampleMode                                   \
        ADC14_configureSingleSampleMode
#endif
#ifdef ROM_ADC14_enableConversion
#define MAP_ADC14_enableConversion                                            \
        ROM_ADC14_enableConversion
#else
#define MAP_ADC14_enableConversion                                            \
        ADC14_enableConversion
#endif
#ifdef ROM_ADC14_disableConversion
#define MAP_ADC14_disableConversion                                           \
        ROM_ADC14_disableConversion
#else
#define MAP_ADC14_disableConversion                                           \
        ADC14_disableConversion
#endif
#ifdef ROM_ADC14_isBusy
#define MAP_ADC14_isBusy                                                      \
        ROM_ADC14_isBusy
#else
#define MAP_ADC14_isBusy                                                      \
        ADC14_isBusy
#endif
#ifdef ROM_ADC14_configureConversionMemory
#define MAP_ADC14_configureConversionMemory                                   \
        ROM_ADC14_configureConversionMemory
#else
#define MAP_ADC14_configureConversionMemory                                   \
        ADC14_configureConversionMemory
#endif
#ifdef ROM_ADC14_enableComparatorWindow
#define MAP_ADC14_enableComparatorWindow                                      \
        ROM_ADC14_enableComparatorWindow
#else
#define MAP_ADC14_enableComparatorWindow                                      \
        ADC14_enableComparatorWindow
#endif
#ifdef ROM_ADC14_disableComparatorWindow
#define MAP_ADC14_disableComparatorWindow                                     \
        ROM_ADC14_disableComparatorWindow
#else
#define MAP_ADC14_disableComparatorWindow                                     \
        ADC14_disableComparatorWindow
#endif
#ifdef ROM_ADC14_setComparatorWindowValue
#define MAP_ADC14_setComparatorWindowValue                                    \
        ROM_ADC14_setComparatorWindowValue
#else
#define MAP_ADC14_setComparatorWindowValue                                    \
        ADC14_setComparatorWindowValue
#endif
#ifdef ROM_ADC14_setResultFormat
#define MAP_ADC14_setResultFormat                                             \
        ROM_ADC14_setResultFormat
#else
#define MAP_ADC14_setResultFormat                                             \
        ADC14_setResultFormat
#endif
#ifdef ROM_ADC14_getResult
#define MAP_ADC14_getResult                                                   \
        ROM_ADC14_getResult
#else
#define MAP_ADC14_getResult                                                   \
        ADC14_getResult
#endif
#ifdef ROM_ADC14_getMultiSequenceResult
#define MAP_ADC14_getMultiSequenceResult                                      \
        ROM_ADC14_getMultiSequenceResult
#else
#define MAP_ADC14_getMultiSequenceResult                                      \
        ADC14_getMultiSequenceResult
#endif
#ifdef ROM_ADC14_getResultArray
#define MAP_ADC14_getResultArray                                              \
        ROM_ADC14_getResultArray
#else
#define MAP_ADC14_getResultArray                                              \
        ADC14_getResultArray
#endif
#ifdef ROM_ADC14_enableReferenceBurst
#define MAP_ADC14_enableReferenceBurst                                        \
        ROM_ADC14_enableReferenceBurst
#else
#define MAP_ADC14_enableReferenceBurst                                        \
        ADC14_enableReferenceBurst
#endif
#ifdef ROM_ADC14_disableReferenceBurst
#define MAP_ADC14_disableReferenceBurst                                       \
        ROM_ADC14_disableReferenceBurst
#else
#define MAP_ADC14_disableReferenceBurst                                       \
        ADC14_disableReferenceBurst
#endif
#ifdef ROM_ADC14_setPowerMode
#define MAP_ADC14_setPowerMode                                                \
        ROM_ADC14_setPowerMode
#else
#define MAP_ADC14_setPowerMode                                                \
        ADC14_setPowerMode
#endif
#ifdef ROM_ADC14_enableInterrupt
#define MAP_ADC14_enableInterrupt                                             \
        ROM_ADC14_enableInterrupt
#else
#define MAP_ADC14_enableInterrupt                                             \
        ADC14_enableInterrupt
#endif
#ifdef ROM_ADC14_disableInterrupt
#define MAP_ADC14_disableInterrupt                                            \
        ROM_ADC14_disableInterrupt
#else
#define MAP_ADC14_disableInterrupt                                            \
        ADC14_disableInterrupt
#endif
#ifdef ROM_ADC14_getInterruptStatus
#define MAP_ADC14_getInterruptStatus                                          \
        ROM_ADC14_getInterruptStatus
#else
#define MAP_ADC14_getInterruptStatus                                          \
        ADC14_getInterruptStatus
#endif
#ifdef ROM_ADC14_getEnabledInterruptStatus
#define MAP_ADC14_getEnabledInterruptStatus                                   \
        ROM_ADC14_getEnabledInterruptStatus
#else
#define MAP_ADC14_getEnabledInterruptStatus                                   \
        ADC14_getEnabledInterruptStatus
#endif
#ifdef ROM_ADC14_clearInterruptFlag
#define MAP_ADC14_clearInterruptFlag                                          \
        ROM_ADC14_clearInterruptFlag
#else
#define MAP_ADC14_clearInterruptFlag                                          \
        ADC14_clearInterruptFlag
#endif
#ifdef ROM_ADC14_toggleConversionTrigger
#define MAP_ADC14_toggleConversionTrigger                                     \
        ROM_ADC14_toggleConversionTrigger
#else
#define MAP_ADC14_toggleConversionTrigger                                     \
        ADC14_toggleConversionTrigger
#endif
#ifdef ROM_ADC14_enableSampleTimer
#define MAP_ADC14_enableSampleTimer                                           \
        ROM_ADC14_enableSampleTimer
#else
#define MAP_ADC14_enableSampleTimer                                           \
        ADC14_enableSampleTimer
#endif
#ifdef ROM_ADC14_disableSampleTimer
#define MAP_ADC14_disableSampleTimer                                          \
        ROM_ADC14_disableSampleTimer
#else
#define MAP_ADC14_disableSampleTimer                                          \
        ADC14_disableSampleTimer
#endif
#ifdef ROM_ADC14_registerInterrupt
#define MAP_ADC14_registerInterrupt                                           \
        ROM_ADC14_registerInterrupt
#else
#define MAP_ADC14_registerInterrupt                                           \
        ADC14_registerInterrupt
#endif
#ifdef ROM_ADC14_unregisterInterrupt
#define MAP_ADC14_unregisterInterrupt                                         \
        ROM_ADC14_unregisterInterrupt
#else
#define MAP_ADC14_unregisterInterrupt                                         \
        ADC14_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the AES256 API.
//
//*****************************************************************************
#ifdef ROM_AES256_setCipherKey
#define MAP_AES256_setCipherKey                                               \
        ROM_AES256_setCipherKey
#else
#define MAP_AES256_setCipherKey                                               \
        AES256_setCipherKey
#endif
#ifdef ROM_AES256_encryptData
#define MAP_AES256_encryptData                                                \
        ROM_AES256_encryptData
#else
#define MAP_AES256_encryptData                                                \
        AES256_encryptData
#endif
#ifdef ROM_AES256_decryptData
#define MAP_AES256_decryptData                                                \
        ROM_AES256_decryptData
#else
#define MAP_AES256_decryptData                                                \
        AES256_decryptData
#endif
#ifdef ROM_AES256_setDecipherKey
#define MAP_AES256_setDecipherKey                                             \
        ROM_AES256_setDecipherKey
#else
#define MAP_AES256_setDecipherKey                                             \
        AES256_setDecipherKey
#endif
#ifdef ROM_AES256_reset
#define MAP_AES256_reset                                                      \
        ROM_AES256_reset
#else
#define MAP_AES256_reset                                                      \
        AES256_reset
#endif
#ifdef ROM_AES256_startEncryptData
#define MAP_AES256_startEncryptData                                           \
        ROM_AES256_startEncryptData
#else
#define MAP_AES256_startEncryptData                                           \
        AES256_startEncryptData
#endif
#ifdef ROM_AES256_startDecryptData
#define MAP_AES256_startDecryptData                                           \
        ROM_AES256_startDecryptData
#else
#define MAP_AES256_startDecryptData                                           \
        AES256_startDecryptData
#endif
#ifdef ROM_AES256_startSetDecipherKey
#define MAP_AES256_startSetDecipherKey                                        \
        ROM_AES256_startSetDecipherKey
#else
#define MAP_AES256_startSetDecipherKey                                        \
        AES256_startSetDecipherKey
#endif
#ifdef ROM_AES256_getDataOut
#define MAP_AES256_getDataOut                                                 \
        ROM_AES256_getDataOut
#else
#define MAP_AES256_getDataOut                                                 \
        AES256_getDataOut
#endif
#ifdef ROM_AES256_isBusy
#define MAP_AES256_isBusy                                                     \
        ROM_AES256_isBusy
#else
#define MAP_AES256_isBusy                                                     \
        AES256_isBusy
#endif
#ifdef ROM_AES256_clearErrorFlag
#define MAP_AES256_clearErrorFlag                                             \
        ROM_AES256_clearErrorFlag
#else
#define MAP_AES256_clearErrorFlag                                             \
        AES256_clearErrorFlag
#endif
#ifdef ROM_AES256_getErrorFlagStatus
#define MAP_AES256_getErrorFlagStatus                                         \
        ROM_AES256_getErrorFlagStatus
#else
#define MAP_AES256_getErrorFlagStatus                                         \
        AES256_getErrorFlagStatus
#endif
#ifdef ROM_AES256_clearInterruptFlag
#define MAP_AES256_clearInterruptFlag                                         \
        ROM_AES256_clearInterruptFlag
#else
#define MAP_AES256_clearInterruptFlag                                         \
        AES256_clearInterruptFlag
#endif
#ifdef ROM_AES256_getInterruptStatus
#define MAP_AES256_getInterruptStatus                                         \
        ROM_AES256_getInterruptStatus
#else
#define MAP_AES256_getInterruptStatus                                         \
        AES256_getInterruptStatus
#endif
#ifdef ROM_AES256_enableInterrupt
#define MAP_AES256_enableInterrupt                                            \
        ROM_AES256_enableInterrupt
#else
#define MAP_AES256_enableInterrupt                                            \
        AES256_enableInterrupt
#endif
#ifdef ROM_AES256_disableInterrupt
#define MAP_AES256_disableInterrupt                                           \
        ROM_AES256_disableInterrupt
#else
#define MAP_AES256_disableInterrupt                                           \
        AES256_disableInterrupt
#endif
#ifdef ROM_AES256_registerInterrupt
#define MAP_AES256_registerInterrupt                                          \
        ROM_AES256_registerInterrupt
#else
#define MAP_AES256_registerInterrupt                                          \
        AES256_registerInterrupt
#endif
#ifdef ROM_AES256_unregisterInterrupt
#define MAP_AES256_unregisterInterrupt                                        \
        ROM_AES256_unregisterInterrupt
#else
#define MAP_AES256_unregisterInterrupt                                        \
        AES256_unregisterInterrupt
#endif
#ifdef ROM_AES256_getInterruptFlagStatus
#define MAP_AES256_getInterruptFlagStatus                                     \
        ROM_AES256_getInterruptFlagStatus
#else
#define MAP_AES256_getInterruptFlagStatus                                     \
        AES256_getInterruptFlagStatus
#endif

//*****************************************************************************
//
// Macros for the Comp API.
//
//*****************************************************************************
#ifdef ROM_COMP_E_initModule
#define MAP_COMP_E_initModule                                                 \
        ROM_COMP_E_initModule
#else
#define MAP_COMP_E_initModule                                                 \
        COMP_E_initModule
#endif
#ifdef ROM_COMP_E_setReferenceVoltage
#define MAP_COMP_E_setReferenceVoltage                                        \
        ROM_COMP_E_setReferenceVoltage
#else
#define MAP_COMP_E_setReferenceVoltage                                        \
        COMP_E_setReferenceVoltage
#endif
#ifdef ROM_COMP_E_setReferenceAccuracy
#define MAP_COMP_E_setReferenceAccuracy                                       \
        ROM_COMP_E_setReferenceAccuracy
#else
#define MAP_COMP_E_setReferenceAccuracy                                       \
        COMP_E_setReferenceAccuracy
#endif
#ifdef ROM_COMP_E_setPowerMode
#define MAP_COMP_E_setPowerMode                                               \
        ROM_COMP_E_setPowerMode
#else
#define MAP_COMP_E_setPowerMode                                               \
        COMP_E_setPowerMode
#endif
#ifdef ROM_COMP_E_enableModule
#define MAP_COMP_E_enableModule                                               \
        ROM_COMP_E_enableModule
#else
#define MAP_COMP_E_enableModule                                               \
        COMP_E_enableModule
#endif
#ifdef ROM_COMP_E_disableModule
#define MAP_COMP_E_disableModule                                              \
        ROM_COMP_E_disableModule
#else
#define MAP_COMP_E_disableModule                                              \
        COMP_E_disableModule
#endif
#ifdef ROM_COMP_E_shortInputs
#define MAP_COMP_E_shortInputs                                                \
        ROM_COMP_E_shortInputs
#else
#define MAP_COMP_E_shortInputs                                                \
        COMP_E_shortInputs
#endif
#ifdef ROM_COMP_E_unshortInputs
#define MAP_COMP_E_unshortInputs                                              \
        ROM_COMP_E_unshortInputs
#else
#define MAP_COMP_E_unshortInputs                                              \
        COMP_E_unshortInputs
#endif
#ifdef ROM_COMP_E_disableInputBuffer
#define MAP_COMP_E_disableInputBuffer                                         \
        ROM_COMP_E_disableInputBuffer
#else
#define MAP_COMP_E_disableInputBuffer                                         \
        COMP_E_disableInputBuffer
#endif
#ifdef ROM_COMP_E_enableInputBuffer
#define MAP_COMP_E_enableInputBuffer                                          \
        ROM_COMP_E_enableInputBuffer
#else
#define MAP_COMP_E_enableInputBuffer                                          \
        COMP_E_enableInputBuffer
#endif
#ifdef ROM_COMP_E_swapIO
#define MAP_COMP_E_swapIO                                                     \
        ROM_COMP_E_swapIO
#else
#define MAP_COMP_E_swapIO                                                     \
        COMP_E_swapIO
#endif
#ifdef ROM_COMP_E_outputValue
#define MAP_COMP_E_outputValue                                                \
        ROM_COMP_E_outputValue
#else
#define MAP_COMP_E_outputValue                                                \
        COMP_E_outputValue
#endif
#ifdef ROM_COMP_E_enableInterrupt
#define MAP_COMP_E_enableInterrupt                                            \
        ROM_COMP_E_enableInterrupt
#else
#define MAP_COMP_E_enableInterrupt                                            \
        COMP_E_enableInterrupt
#endif
#ifdef ROM_COMP_E_disableInterrupt
#define MAP_COMP_E_disableInterrupt                                           \
        ROM_COMP_E_disableInterrupt
#else
#define MAP_COMP_E_disableInterrupt                                           \
        COMP_E_disableInterrupt
#endif
#ifdef ROM_COMP_E_clearInterruptFlag
#define MAP_COMP_E_clearInterruptFlag                                         \
        ROM_COMP_E_clearInterruptFlag
#else
#define MAP_COMP_E_clearInterruptFlag                                         \
        COMP_E_clearInterruptFlag
#endif
#ifdef ROM_COMP_E_getInterruptStatus
#define MAP_COMP_E_getInterruptStatus                                         \
        ROM_COMP_E_getInterruptStatus
#else
#define MAP_COMP_E_getInterruptStatus                                         \
        COMP_E_getInterruptStatus
#endif
#ifdef ROM_COMP_E_getEnabledInterruptStatus
#define MAP_COMP_E_getEnabledInterruptStatus                                  \
        ROM_COMP_E_getEnabledInterruptStatus
#else
#define MAP_COMP_E_getEnabledInterruptStatus                                  \
        COMP_E_getEnabledInterruptStatus
#endif
#ifdef ROM_COMP_E_setInterruptEdgeDirection
#define MAP_COMP_E_setInterruptEdgeDirection                                  \
        ROM_COMP_E_setInterruptEdgeDirection
#else
#define MAP_COMP_E_setInterruptEdgeDirection                                  \
        COMP_E_setInterruptEdgeDirection
#endif
#ifdef ROM_COMP_E_toggleInterruptEdgeDirection
#define MAP_COMP_E_toggleInterruptEdgeDirection                               \
        ROM_COMP_E_toggleInterruptEdgeDirection
#else
#define MAP_COMP_E_toggleInterruptEdgeDirection                               \
        COMP_E_toggleInterruptEdgeDirection
#endif
#ifdef ROM_COMP_E_registerInterrupt
#define MAP_COMP_E_registerInterrupt                                          \
        ROM_COMP_E_registerInterrupt
#else
#define MAP_COMP_E_registerInterrupt                                          \
        COMP_E_registerInterrupt
#endif
#ifdef ROM_COMP_E_unregisterInterrupt
#define MAP_COMP_E_unregisterInterrupt                                        \
        ROM_COMP_E_unregisterInterrupt
#else
#define MAP_COMP_E_unregisterInterrupt                                        \
        COMP_E_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the CRC32 API.
//
//*****************************************************************************
#ifdef ROM_CRC32_setSeed
#define MAP_CRC32_setSeed                                                     \
        ROM_CRC32_setSeed
#else
#define MAP_CRC32_setSeed                                                     \
        CRC32_setSeed
#endif
#ifdef ROM_CRC32_set8BitData
#define MAP_CRC32_set8BitData                                                 \
        ROM_CRC32_set8BitData
#else
#define MAP_CRC32_set8BitData                                                 \
        CRC32_set8BitData
#endif
#ifdef ROM_CRC32_set16BitData
#define MAP_CRC32_set16BitData                                                \
        ROM_CRC32_set16BitData
#else
#define MAP_CRC32_set16BitData                                                \
        CRC32_set16BitData
#endif
#ifdef ROM_CRC32_set32BitData
#define MAP_CRC32_set32BitData                                                \
        ROM_CRC32_set32BitData
#else
#define MAP_CRC32_set32BitData                                                \
        CRC32_set32BitData
#endif
#ifdef ROM_CRC32_set8BitDataReversed
#define MAP_CRC32_set8BitDataReversed                                         \
        ROM_CRC32_set8BitDataReversed
#else
#define MAP_CRC32_set8BitDataReversed                                         \
        CRC32_set8BitDataReversed
#endif
#ifdef ROM_CRC32_set16BitDataReversed
#define MAP_CRC32_set16BitDataReversed                                        \
        ROM_CRC32_set16BitDataReversed
#else
#define MAP_CRC32_set16BitDataReversed                                        \
        CRC32_set16BitDataReversed
#endif
#ifdef ROM_CRC32_set32BitDataReversed
#define MAP_CRC32_set32BitDataReversed                                        \
        ROM_CRC32_set32BitDataReversed
#else
#define MAP_CRC32_set32BitDataReversed                                        \
        CRC32_set32BitDataReversed
#endif
#ifdef ROM_CRC32_getResult
#define MAP_CRC32_getResult                                                   \
        ROM_CRC32_getResult
#else
#define MAP_CRC32_getResult                                                   \
        CRC32_getResult
#endif
#ifdef ROM_CRC32_getResultReversed
#define MAP_CRC32_getResultReversed                                           \
        ROM_CRC32_getResultReversed
#else
#define MAP_CRC32_getResultReversed                                           \
        CRC32_getResultReversed
#endif

//*****************************************************************************
//
// Macros for the CS API.
//
//*****************************************************************************
#ifdef ROM_CS_initClockSignal
#define MAP_CS_initClockSignal                                                \
        ROM_CS_initClockSignal
#else
#define MAP_CS_initClockSignal                                                \
        CS_initClockSignal
#endif
#ifdef ROM_CS_setReferenceOscillatorFrequency
#define MAP_CS_setReferenceOscillatorFrequency                                \
        ROM_CS_setReferenceOscillatorFrequency
#else
#define MAP_CS_setReferenceOscillatorFrequency                                \
        CS_setReferenceOscillatorFrequency
#endif
#ifdef ROM_CS_enableClockRequest
#define MAP_CS_enableClockRequest                                             \
        ROM_CS_enableClockRequest
#else
#define MAP_CS_enableClockRequest                                             \
        CS_enableClockRequest
#endif
#ifdef ROM_CS_disableClockRequest
#define MAP_CS_disableClockRequest                                            \
        ROM_CS_disableClockRequest
#else
#define MAP_CS_disableClockRequest                                            \
        CS_disableClockRequest
#endif
#ifdef ROM_CS_setDCOCenteredFrequency
#define MAP_CS_setDCOCenteredFrequency                                        \
        ROM_CS_setDCOCenteredFrequency
#else
#define MAP_CS_setDCOCenteredFrequency                                        \
        CS_setDCOCenteredFrequency
#endif
#ifdef ROM_CS_tuneDCOFrequency
#define MAP_CS_tuneDCOFrequency                                               \
        ROM_CS_tuneDCOFrequency
#else
#define MAP_CS_tuneDCOFrequency                                               \
        CS_tuneDCOFrequency
#endif
#ifdef ROM_CS_enableDCOExternalResistor
#define MAP_CS_enableDCOExternalResistor                                      \
        ROM_CS_enableDCOExternalResistor
#else
#define MAP_CS_enableDCOExternalResistor                                      \
        CS_enableDCOExternalResistor
#endif
#ifdef ROM_CS_disableDCOExternalResistor
#define MAP_CS_disableDCOExternalResistor                                     \
        ROM_CS_disableDCOExternalResistor
#else
#define MAP_CS_disableDCOExternalResistor                                     \
        CS_disableDCOExternalResistor
#endif
#ifdef ROM_CS_enableInterrupt
#define MAP_CS_enableInterrupt                                                \
        ROM_CS_enableInterrupt
#else
#define MAP_CS_enableInterrupt                                                \
        CS_enableInterrupt
#endif
#ifdef ROM_CS_disableInterrupt
#define MAP_CS_disableInterrupt                                               \
        ROM_CS_disableInterrupt
#else
#define MAP_CS_disableInterrupt                                               \
        CS_disableInterrupt
#endif
#ifdef ROM_CS_getEnabledInterruptStatus
#define MAP_CS_getEnabledInterruptStatus                                      \
        ROM_CS_getEnabledInterruptStatus
#else
#define MAP_CS_getEnabledInterruptStatus                                      \
        CS_getEnabledInterruptStatus
#endif
#ifdef ROM_CS_getInterruptStatus
#define MAP_CS_getInterruptStatus                                             \
        ROM_CS_getInterruptStatus
#else
#define MAP_CS_getInterruptStatus                                             \
        CS_getInterruptStatus
#endif
#ifdef ROM_CS_setDCOFrequency
#define MAP_CS_setDCOFrequency                                                \
        ROM_CS_setDCOFrequency
#else
#define MAP_CS_setDCOFrequency                                                \
        CS_setDCOFrequency
#endif
#ifdef ROM_CS_getDCOFrequency
#define MAP_CS_getDCOFrequency                                                \
        ROM_CS_getDCOFrequency
#else
#define MAP_CS_getDCOFrequency                                                \
        CS_getDCOFrequency
#endif
#ifdef ROM_CS_enableFaultCounter
#define MAP_CS_enableFaultCounter                                             \
        ROM_CS_enableFaultCounter
#else
#define MAP_CS_enableFaultCounter                                             \
        CS_enableFaultCounter
#endif
#ifdef ROM_CS_disableFaultCounter
#define MAP_CS_disableFaultCounter                                            \
        ROM_CS_disableFaultCounter
#else
#define MAP_CS_disableFaultCounter                                            \
        CS_disableFaultCounter
#endif
#ifdef ROM_CS_resetFaultCounter
#define MAP_CS_resetFaultCounter                                              \
        ROM_CS_resetFaultCounter
#else
#define MAP_CS_resetFaultCounter                                              \
        CS_resetFaultCounter
#endif
#ifdef ROM_CS_startFaultCounter
#define MAP_CS_startFaultCounter                                              \
        ROM_CS_startFaultCounter
#else
#define MAP_CS_startFaultCounter                                              \
        CS_startFaultCounter
#endif
#ifdef ROM_CS_registerInterrupt
#define MAP_CS_registerInterrupt                                              \
        ROM_CS_registerInterrupt
#else
#define MAP_CS_registerInterrupt                                              \
        CS_registerInterrupt
#endif
#ifdef ROM_CS_unregisterInterrupt
#define MAP_CS_unregisterInterrupt                                            \
        ROM_CS_unregisterInterrupt
#else
#define MAP_CS_unregisterInterrupt                                            \
        CS_unregisterInterrupt
#endif
#ifdef ROM_CS_clearInterruptFlag
#define MAP_CS_clearInterruptFlag                                             \
        ROM_CS_clearInterruptFlag
#else
#define MAP_CS_clearInterruptFlag                                             \
        CS_clearInterruptFlag
#endif
#ifdef ROM_CS_getACLK
#define MAP_CS_getACLK                                                        \
        ROM_CS_getACLK
#else
#define MAP_CS_getACLK                                                        \
        CS_getACLK
#endif
#ifdef ROM_CS_getSMCLK
#define MAP_CS_getSMCLK                                                       \
        ROM_CS_getSMCLK
#else
#define MAP_CS_getSMCLK                                                       \
        CS_getSMCLK
#endif
#ifdef ROM_CS_getMCLK
#define MAP_CS_getMCLK                                                        \
        ROM_CS_getMCLK
#else
#define MAP_CS_getMCLK                                                        \
        CS_getMCLK
#endif
#ifdef ROM_CS_getBCLK
#define MAP_CS_getBCLK                                                        \
        ROM_CS_getBCLK
#else
#define MAP_CS_getBCLK                                                        \
        CS_getBCLK
#endif
#ifdef ROM_CS_getHSMCLK
#define MAP_CS_getHSMCLK                                                      \
        ROM_CS_getHSMCLK
#else
#define MAP_CS_getHSMCLK                                                      \
        CS_getHSMCLK
#endif
#ifdef ROM_CS_startHFXT
#define MAP_CS_startHFXT                                                      \
        ROM_CS_startHFXT
#else
#define MAP_CS_startHFXT                                                      \
        CS_startHFXT
#endif
#ifdef ROM_CS_startHFXTWithTimeout
#define MAP_CS_startHFXTWithTimeout                                           \
        ROM_CS_startHFXTWithTimeout
#else
#define MAP_CS_startHFXTWithTimeout                                           \
        CS_startHFXTWithTimeout
#endif
#ifdef ROM_CS_startLFXT
#define MAP_CS_startLFXT                                                      \
        ROM_CS_startLFXT
#else
#define MAP_CS_startLFXT                                                      \
        CS_startLFXT
#endif
#ifdef ROM_CS_startLFXTWithTimeout
#define MAP_CS_startLFXTWithTimeout                                           \
        ROM_CS_startLFXTWithTimeout
#else
#define MAP_CS_startLFXTWithTimeout                                           \
        CS_startLFXTWithTimeout
#endif
#ifdef ROM_CS_setExternalClockSourceFrequency
#define MAP_CS_setExternalClockSourceFrequency                                \
        ROM_CS_setExternalClockSourceFrequency
#else
#define MAP_CS_setExternalClockSourceFrequency                                \
        CS_setExternalClockSourceFrequency
#endif
#ifdef ROM_CS_setDCOExternalResistorCalibration
#define MAP_CS_setDCOExternalResistorCalibration                              \
        ROM_CS_setDCOExternalResistorCalibration
#else
#define MAP_CS_setDCOExternalResistorCalibration                              \
        CS_setDCOExternalResistorCalibration
#endif

//*****************************************************************************
//
// Macros for the DMA API.
//
//*****************************************************************************
#ifdef ROM_DMA_enableModule
#define MAP_DMA_enableModule                                                  \
        ROM_DMA_enableModule
#else
#define MAP_DMA_enableModule                                                  \
        DMA_enableModule
#endif
#ifdef ROM_DMA_disableModule
#define MAP_DMA_disableModule                                                 \
        ROM_DMA_disableModule
#else
#define MAP_DMA_disableModule                                                 \
        DMA_disableModule
#endif
#ifdef ROM_DMA_getErrorStatus
#define MAP_DMA_getErrorStatus                                                \
        ROM_DMA_getErrorStatus
#else
#define MAP_DMA_getErrorStatus                                                \
        DMA_getErrorStatus
#endif
#ifdef ROM_DMA_clearErrorStatus
#define MAP_DMA_clearErrorStatus                                              \
        ROM_DMA_clearErrorStatus
#else
#define MAP_DMA_clearErrorStatus                                              \
        DMA_clearErrorStatus
#endif
#ifdef ROM_DMA_enableChannel
#define MAP_DMA_enableChannel                                                 \
        ROM_DMA_enableChannel
#else
#define MAP_DMA_enableChannel                                                 \
        DMA_enableChannel
#endif
#ifdef ROM_DMA_disableChannel
#define MAP_DMA_disableChannel                                                \
        ROM_DMA_disableChannel
#else
#define MAP_DMA_disableChannel                                                \
        DMA_disableChannel
#endif
#ifdef ROM_DMA_isChannelEnabled
#define MAP_DMA_isChannelEnabled                                              \
        ROM_DMA_isChannelEnabled
#else
#define MAP_DMA_isChannelEnabled                                              \
        DMA_isChannelEnabled
#endif
#ifdef ROM_DMA_setControlBase
#define MAP_DMA_setControlBase                                                \
        ROM_DMA_setControlBase
#else
#define MAP_DMA_setControlBase                                                \
        DMA_setControlBase
#endif
#ifdef ROM_DMA_getControlBase
#define MAP_DMA_getControlBase                                                \
        ROM_DMA_getControlBase
#else
#define MAP_DMA_getControlBase                                                \
        DMA_getControlBase
#endif
#ifdef ROM_DMA_getControlAlternateBase
#define MAP_DMA_getControlAlternateBase                                       \
        ROM_DMA_getControlAlternateBase
#else
#define MAP_DMA_getControlAlternateBase                                       \
        DMA_getControlAlternateBase
#endif
#ifdef ROM_DMA_requestChannel
#define MAP_DMA_requestChannel                                                \
        ROM_DMA_requestChannel
#else
#define MAP_DMA_requestChannel                                                \
        DMA_requestChannel
#endif
#ifdef ROM_DMA_enableChannelAttribute
#define MAP_DMA_enableChannelAttribute                                        \
        ROM_DMA_enableChannelAttribute
#else
#define MAP_DMA_enableChannelAttribute                                        \
        DMA_enableChannelAttribute
#endif
#ifdef ROM_DMA_disableChannelAttribute
#define MAP_DMA_disableChannelAttribute                                       \
        ROM_DMA_disableChannelAttribute
#else
#define MAP_DMA_disableChannelAttribute                                       \
        DMA_disableChannelAttribute
#endif
#ifdef ROM_DMA_getChannelAttribute
#define MAP_DMA_getChannelAttribute                                           \
        ROM_DMA_getChannelAttribute
#else
#define MAP_DMA_getChannelAttribute                                           \
        DMA_getChannelAttribute
#endif
#ifdef ROM_DMA_setChannelControl
#define MAP_DMA_setChannelControl                                             \
        ROM_DMA_setChannelControl
#else
#define MAP_DMA_setChannelControl                                             \
        DMA_setChannelControl
#endif
#ifdef ROM_DMA_setChannelTransfer
#define MAP_DMA_setChannelTransfer                                            \
        ROM_DMA_setChannelTransfer
#else
#define MAP_DMA_setChannelTransfer                                            \
        DMA_setChannelTransfer
#endif
#ifdef ROM_DMA_setChannelScatterGather
#define MAP_DMA_setChannelScatterGather                                       \
        ROM_DMA_setChannelScatterGather
#else
#define MAP_DMA_setChannelScatterGather                                       \
        DMA_setChannelScatterGather
#endif
#ifdef ROM_DMA_getChannelSize
#define MAP_DMA_getChannelSize                                                \
        ROM_DMA_getChannelSize
#else
#define MAP_DMA_getChannelSize                                                \
        DMA_getChannelSize
#endif
#ifdef ROM_DMA_getChannelMode
#define MAP_DMA_getChannelMode                                                \
        ROM_DMA_getChannelMode
#else
#define MAP_DMA_getChannelMode                                                \
        DMA_getChannelMode
#endif
#ifdef ROM_DMA_assignChannel
#define MAP_DMA_assignChannel                                                 \
        ROM_DMA_assignChannel
#else
#define MAP_DMA_assignChannel                                                 \
        DMA_assignChannel
#endif
#ifdef ROM_DMA_requestSoftwareTransfer
#define MAP_DMA_requestSoftwareTransfer                                       \
        ROM_DMA_requestSoftwareTransfer
#else
#define MAP_DMA_requestSoftwareTransfer                                       \
        DMA_requestSoftwareTransfer
#endif
#ifdef ROM_DMA_assignInterrupt
#define MAP_DMA_assignInterrupt                                               \
        ROM_DMA_assignInterrupt
#else
#define MAP_DMA_assignInterrupt                                               \
        DMA_assignInterrupt
#endif
#ifdef ROM_DMA_enableInterrupt
#define MAP_DMA_enableInterrupt                                               \
        ROM_DMA_enableInterrupt
#else
#define MAP_DMA_enableInterrupt                                               \
        DMA_enableInterrupt
#endif
#ifdef ROM_DMA_disableInterrupt
#define MAP_DMA_disableInterrupt                                              \
        ROM_DMA_disableInterrupt
#else
#define MAP_DMA_disableInterrupt                                              \
        DMA_disableInterrupt
#endif
#ifdef ROM_DMA_getInterruptStatus
#define MAP_DMA_getInterruptStatus                                            \
        ROM_DMA_getInterruptStatus
#else
#define MAP_DMA_getInterruptStatus                                            \
        DMA_getInterruptStatus
#endif
#ifdef ROM_DMA_clearInterruptFlag
#define MAP_DMA_clearInterruptFlag                                            \
        ROM_DMA_clearInterruptFlag
#else
#define MAP_DMA_clearInterruptFlag                                            \
        DMA_clearInterruptFlag
#endif
#ifdef ROM_DMA_registerInterrupt
#define MAP_DMA_registerInterrupt                                             \
        ROM_DMA_registerInterrupt
#else
#define MAP_DMA_registerInterrupt                                             \
        DMA_registerInterrupt
#endif
#ifdef ROM_DMA_unregisterInterrupt
#define MAP_DMA_unregisterInterrupt                                           \
        ROM_DMA_unregisterInterrupt
#else
#define MAP_DMA_unregisterInterrupt                                           \
        DMA_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the Flash API.
//
//*****************************************************************************
#ifdef ROM_FlashCtl_enableReadParityCheck
#define MAP_FlashCtl_enableReadParityCheck                                    \
        ROM_FlashCtl_enableReadParityCheck
#else
#define MAP_FlashCtl_enableReadParityCheck                                    \
        FlashCtl_enableReadParityCheck
#endif
#ifdef ROM_FlashCtl_disableReadParityCheck
#define MAP_FlashCtl_disableReadParityCheck                                   \
        ROM_FlashCtl_disableReadParityCheck
#else
#define MAP_FlashCtl_disableReadParityCheck                                   \
        FlashCtl_disableReadParityCheck
#endif
#ifdef ROM_FlashCtl_enableReadBuffering
#define MAP_FlashCtl_enableReadBuffering                                      \
        ROM_FlashCtl_enableReadBuffering
#else
#define MAP_FlashCtl_enableReadBuffering                                      \
        FlashCtl_enableReadBuffering
#endif
#ifdef ROM_FlashCtl_disableReadBuffering
#define MAP_FlashCtl_disableReadBuffering                                     \
        ROM_FlashCtl_disableReadBuffering
#else
#define MAP_FlashCtl_disableReadBuffering                                     \
        FlashCtl_disableReadBuffering
#endif
#ifdef ROM_FlashCtl_unprotectSector
#define MAP_FlashCtl_unprotectSector                                          \
        ROM_FlashCtl_unprotectSector
#else
#define MAP_FlashCtl_unprotectSector                                          \
        FlashCtl_unprotectSector
#endif
#ifdef ROM_FlashCtl_protectSector
#define MAP_FlashCtl_protectSector                                            \
        ROM_FlashCtl_protectSector
#else
#define MAP_FlashCtl_protectSector                                            \
        FlashCtl_protectSector
#endif
#ifdef ROM_FlashCtl_isSectorProtected
#define MAP_FlashCtl_isSectorProtected                                        \
        ROM_FlashCtl_isSectorProtected
#else
#define MAP_FlashCtl_isSectorProtected                                        \
        FlashCtl_isSectorProtected
#endif
#ifdef ROM_FlashCtl_verifyMemory
#define MAP_FlashCtl_verifyMemory                                             \
        ROM_FlashCtl_verifyMemory
#else
#define MAP_FlashCtl_verifyMemory                                             \
        FlashCtl_verifyMemory
#endif
#ifdef ROM_FlashCtl_performMassErase
#define MAP_FlashCtl_performMassErase                                         \
        ROM_FlashCtl_performMassErase
#else
#define MAP_FlashCtl_performMassErase                                         \
        FlashCtl_performMassErase
#endif
#ifdef ROM_FlashCtl_eraseSector
#define MAP_FlashCtl_eraseSector                                              \
        ROM_FlashCtl_eraseSector
#else
#define MAP_FlashCtl_eraseSector                                              \
        FlashCtl_eraseSector
#endif
#ifdef ROM_FlashCtl_programMemory
#define MAP_FlashCtl_programMemory                                            \
        ROM_FlashCtl_programMemory
#else
#define MAP_FlashCtl_programMemory                                            \
        FlashCtl_programMemory
#endif
#ifdef ROM_FlashCtl_setProgramVerification
#define MAP_FlashCtl_setProgramVerification                                   \
        ROM_FlashCtl_setProgramVerification
#else
#define MAP_FlashCtl_setProgramVerification                                   \
        FlashCtl_setProgramVerification
#endif
#ifdef ROM_FlashCtl_clearProgramVerification
#define MAP_FlashCtl_clearProgramVerification                                 \
        ROM_FlashCtl_clearProgramVerification
#else
#define MAP_FlashCtl_clearProgramVerification                                 \
        FlashCtl_clearProgramVerification
#endif
#ifdef ROM_FlashCtl_enableWordProgramming
#define MAP_FlashCtl_enableWordProgramming                                    \
        ROM_FlashCtl_enableWordProgramming
#else
#define MAP_FlashCtl_enableWordProgramming                                    \
        FlashCtl_enableWordProgramming
#endif
#ifdef ROM_FlashCtl_disableWordProgramming
#define MAP_FlashCtl_disableWordProgramming                                   \
        ROM_FlashCtl_disableWordProgramming
#else
#define MAP_FlashCtl_disableWordProgramming                                   \
        FlashCtl_disableWordProgramming
#endif
#ifdef ROM_FlashCtl_isWordProgrammingEnabled
#define MAP_FlashCtl_isWordProgrammingEnabled                                 \
        ROM_FlashCtl_isWordProgrammingEnabled
#else
#define MAP_FlashCtl_isWordProgrammingEnabled                                 \
        FlashCtl_isWordProgrammingEnabled
#endif
#ifdef ROM_FlashCtl_enableInterrupt
#define MAP_FlashCtl_enableInterrupt                                          \
        ROM_FlashCtl_enableInterrupt
#else
#define MAP_FlashCtl_enableInterrupt                                          \
        FlashCtl_enableInterrupt
#endif
#ifdef ROM_FlashCtl_disableInterrupt
#define MAP_FlashCtl_disableInterrupt                                         \
        ROM_FlashCtl_disableInterrupt
#else
#define MAP_FlashCtl_disableInterrupt                                         \
        FlashCtl_disableInterrupt
#endif
#ifdef ROM_FlashCtl_getEnabledInterruptStatus
#define MAP_FlashCtl_getEnabledInterruptStatus                                \
        ROM_FlashCtl_getEnabledInterruptStatus
#else
#define MAP_FlashCtl_getEnabledInterruptStatus                                \
        FlashCtl_getEnabledInterruptStatus
#endif
#ifdef ROM_FlashCtl_getInterruptStatus
#define MAP_FlashCtl_getInterruptStatus                                       \
        ROM_FlashCtl_getInterruptStatus
#else
#define MAP_FlashCtl_getInterruptStatus                                       \
        FlashCtl_getInterruptStatus
#endif
#ifdef ROM_FlashCtl_clearInterruptFlag
#define MAP_FlashCtl_clearInterruptFlag                                       \
        ROM_FlashCtl_clearInterruptFlag
#else
#define MAP_FlashCtl_clearInterruptFlag                                       \
        FlashCtl_clearInterruptFlag
#endif
#ifdef ROM_FlashCtl_setWaitState
#define MAP_FlashCtl_setWaitState                                             \
        ROM_FlashCtl_setWaitState
#else
#define MAP_FlashCtl_setWaitState                                             \
        FlashCtl_setWaitState
#endif
#ifdef ROM_FlashCtl_getWaitState
#define MAP_FlashCtl_getWaitState                                             \
        ROM_FlashCtl_getWaitState
#else
#define MAP_FlashCtl_getWaitState                                             \
        FlashCtl_getWaitState
#endif
#ifdef ROM_FlashCtl_setReadMode
#define MAP_FlashCtl_setReadMode                                              \
        ROM_FlashCtl_setReadMode
#else
#define MAP_FlashCtl_setReadMode                                              \
        FlashCtl_setReadMode
#endif
#ifdef ROM_FlashCtl_getReadMode
#define MAP_FlashCtl_getReadMode                                              \
        ROM_FlashCtl_getReadMode
#else
#define MAP_FlashCtl_getReadMode                                              \
        FlashCtl_getReadMode
#endif
#ifdef ROM_FlashCtl_registerInterrupt
#define MAP_FlashCtl_registerInterrupt                                        \
        ROM_FlashCtl_registerInterrupt
#else
#define MAP_FlashCtl_registerInterrupt                                        \
        FlashCtl_registerInterrupt
#endif
#ifdef ROM_FlashCtl_unregisterInterrupt
#define MAP_FlashCtl_unregisterInterrupt                                      \
        ROM_FlashCtl_unregisterInterrupt
#else
#define MAP_FlashCtl_unregisterInterrupt                                      \
        FlashCtl_unregisterInterrupt
#endif
#ifdef ROM___FlashCtl_remaskData8Post
#define MAP___FlashCtl_remaskData8Post                                        \
        ROM___FlashCtl_remaskData8Post
#else
#define MAP___FlashCtl_remaskData8Post                                        \
        __FlashCtl_remaskData8Post
#endif
#ifdef ROM___FlashCtl_remaskData8Pre
#define MAP___FlashCtl_remaskData8Pre                                         \
        ROM___FlashCtl_remaskData8Pre
#else
#define MAP___FlashCtl_remaskData8Pre                                         \
        __FlashCtl_remaskData8Pre
#endif
#ifdef ROM___FlashCtl_remaskData32Pre
#define MAP___FlashCtl_remaskData32Pre                                        \
        ROM___FlashCtl_remaskData32Pre
#else
#define MAP___FlashCtl_remaskData32Pre                                        \
        __FlashCtl_remaskData32Pre
#endif
#ifdef ROM___FlashCtl_remaskData32Post
#define MAP___FlashCtl_remaskData32Post                                       \
        ROM___FlashCtl_remaskData32Post
#else
#define MAP___FlashCtl_remaskData32Post                                       \
        __FlashCtl_remaskData32Post
#endif
#ifdef ROM___FlashCtl_remaskBurstDataPre
#define MAP___FlashCtl_remaskBurstDataPre                                     \
        ROM___FlashCtl_remaskBurstDataPre
#else
#define MAP___FlashCtl_remaskBurstDataPre                                     \
        __FlashCtl_remaskBurstDataPre
#endif
#ifdef ROM___FlashCtl_remaskBurstDataPost
#define MAP___FlashCtl_remaskBurstDataPost                                    \
        ROM___FlashCtl_remaskBurstDataPost
#else
#define MAP___FlashCtl_remaskBurstDataPost                                    \
        __FlashCtl_remaskBurstDataPost
#endif
#ifdef ROM_FlashCtl_initiateSectorErase
#define MAP_FlashCtl_initiateSectorErase                                      \
        ROM_FlashCtl_initiateSectorErase
#else
#define MAP_FlashCtl_initiateSectorErase                                      \
        FlashCtl_initiateSectorErase
#endif
#ifdef ROM_FlashCtl_initiateMassErase
#define MAP_FlashCtl_initiateMassErase                                        \
        ROM_FlashCtl_initiateMassErase
#else
#define MAP_FlashCtl_initiateMassErase                                        \
        FlashCtl_initiateMassErase
#endif
#ifdef ROM_FlashCtl_getMemoryInfo
#define MAP_FlashCtl_getMemoryInfo                                            \
        ROM_FlashCtl_getMemoryInfo
#else
#define MAP_FlashCtl_getMemoryInfo                                            \
        FlashCtl_getMemoryInfo
#endif

//*****************************************************************************
//
// Macros for the FPU API.
//
//*****************************************************************************
#ifdef ROM_FPU_enableModule
#define MAP_FPU_enableModule                                                  \
        ROM_FPU_enableModule
#else
#define MAP_FPU_enableModule                                                  \
        FPU_enableModule
#endif
#ifdef ROM_FPU_disableModule
#define MAP_FPU_disableModule                                                 \
        ROM_FPU_disableModule
#else
#define MAP_FPU_disableModule                                                 \
        FPU_disableModule
#endif
#ifdef ROM_FPU_enableStacking
#define MAP_FPU_enableStacking                                                \
        ROM_FPU_enableStacking
#else
#define MAP_FPU_enableStacking                                                \
        FPU_enableStacking
#endif
#ifdef ROM_FPU_enableLazyStacking
#define MAP_FPU_enableLazyStacking                                            \
        ROM_FPU_enableLazyStacking
#else
#define MAP_FPU_enableLazyStacking                                            \
        FPU_enableLazyStacking
#endif
#ifdef ROM_FPU_disableStacking
#define MAP_FPU_disableStacking                                               \
        ROM_FPU_disableStacking
#else
#define MAP_FPU_disableStacking                                               \
        FPU_disableStacking
#endif
#ifdef ROM_FPU_setHalfPrecisionMode
#define MAP_FPU_setHalfPrecisionMode                                          \
        ROM_FPU_setHalfPrecisionMode
#else
#define MAP_FPU_setHalfPrecisionMode                                          \
        FPU_setHalfPrecisionMode
#endif
#ifdef ROM_FPU_setNaNMode
#define MAP_FPU_setNaNMode                                                    \
        ROM_FPU_setNaNMode
#else
#define MAP_FPU_setNaNMode                                                    \
        FPU_setNaNMode
#endif
#ifdef ROM_FPU_setFlushToZeroMode
#define MAP_FPU_setFlushToZeroMode                                            \
        ROM_FPU_setFlushToZeroMode
#else
#define MAP_FPU_setFlushToZeroMode                                            \
        FPU_setFlushToZeroMode
#endif
#ifdef ROM_FPU_setRoundingMode
#define MAP_FPU_setRoundingMode                                               \
        ROM_FPU_setRoundingMode
#else
#define MAP_FPU_setRoundingMode                                               \
        FPU_setRoundingMode
#endif

//*****************************************************************************
//
// Macros for the GPIO API.
//
//*****************************************************************************
#ifdef ROM_GPIO_setAsOutputPin
#define MAP_GPIO_setAsOutputPin                                               \
        ROM_GPIO_setAsOutputPin
#else
#define MAP_GPIO_setAsOutputPin                                               \
        GPIO_setAsOutputPin
#endif
#ifdef ROM_GPIO_setOutputHighOnPin
#define MAP_GPIO_setOutputHighOnPin                                           \
        ROM_GPIO_setOutputHighOnPin
#else
#define MAP_GPIO_setOutputHighOnPin                                           \
        GPIO_setOutputHighOnPin
#endif
#ifdef ROM_GPIO_setOutputLowOnPin
#define MAP_GPIO_setOutputLowOnPin                                            \
        ROM_GPIO_setOutputLowOnPin
#else
#define MAP_GPIO_setOutputLowOnPin                                            \
        GPIO_setOutputLowOnPin
#endif
#ifdef ROM_GPIO_toggleOutputOnPin
#define MAP_GPIO_toggleOutputOnPin                                            \
        ROM_GPIO_toggleOutputOnPin
#else
#define MAP_GPIO_toggleOutputOnPin                                            \
        GPIO_toggleOutputOnPin
#endif
#ifdef ROM_GPIO_setAsInputPinWithPullDownResistor
#define MAP_GPIO_setAsInputPinWithPullDownResistor                            \
        ROM_GPIO_setAsInputPinWithPullDownResistor
#else
#define MAP_GPIO_setAsInputPinWithPullDownResistor                            \
        GPIO_setAsInputPinWithPullDownResistor
#endif
#ifdef ROM_GPIO_setAsInputPinWithPullUpResistor
#define MAP_GPIO_setAsInputPinWithPullUpResistor                              \
        ROM_GPIO_setAsInputPinWithPullUpResistor
#else
#define MAP_GPIO_setAsInputPinWithPullUpResistor                              \
        GPIO_setAsInputPinWithPullUpResistor
#endif
#ifdef ROM_GPIO_setAsPeripheralModuleFunctionOutputPin
#define MAP_GPIO_setAsPeripheralModuleFunctionOutputPin                       \
        ROM_GPIO_setAsPeripheralModuleFunctionOutputPin
#else
#define MAP_GPIO_setAsPeripheralModuleFunctionOutputPin                       \
        GPIO_setAsPeripheralModuleFunctionOutputPin
#endif
#ifdef ROM_GPIO_setAsPeripheralModuleFunctionInputPin
#define MAP_GPIO_setAsPeripheralModuleFunctionInputPin                        \
        ROM_GPIO_setAsPeripheralModuleFunctionInputPin
#else
#define MAP_GPIO_setAsPeripheralModuleFunctionInputPin                        \
        GPIO_setAsPeripheralModuleFunctionInputPin
#endif
#ifdef ROM_GPIO_getInputPinValue
#define MAP_GPIO_getInputPinValue                                             \
        ROM_GPIO_getInputPinValue
#else
#define MAP_GPIO_getInputPinValue                                             \
        GPIO_getInputPinValue
#endif
#ifdef ROM_GPIO_interruptEdgeSelect
#define MAP_GPIO_interruptEdgeSelect                                          \
        ROM_GPIO_interruptEdgeSelect
#else
#define MAP_GPIO_interruptEdgeSelect                                          \
        GPIO_interruptEdgeSelect
#endif
#ifdef ROM_GPIO_enableInterrupt
#define MAP_GPIO_enableInterrupt                                              \
        ROM_GPIO_enableInterrupt
#else
#define MAP_GPIO_enableInterrupt                                              \
        GPIO_enableInterrupt
#endif
#ifdef ROM_GPIO_disableInterrupt
#define MAP_GPIO_disableInterrupt                                             \
        ROM_GPIO_disableInterrupt
#else
#define MAP_GPIO_disableInterrupt                                             \
        GPIO_disableInterrupt
#endif
#ifdef ROM_GPIO_getInterruptStatus
#define MAP_GPIO_getInterruptStatus                                           \
        ROM_GPIO_getInterruptStatus
#else
#define MAP_GPIO_getInterruptStatus                                           \
        GPIO_getInterruptStatus
#endif
#ifdef ROM_GPIO_clearInterruptFlag
#define MAP_GPIO_clearInterruptFlag                                           \
        ROM_GPIO_clearInterruptFlag
#else
#define MAP_GPIO_clearInterruptFlag                                           \
        GPIO_clearInterruptFlag
#endif
#ifdef ROM_GPIO_setAsInputPin
#define MAP_GPIO_setAsInputPin                                                \
        ROM_GPIO_setAsInputPin
#else
#define MAP_GPIO_setAsInputPin                                                \
        GPIO_setAsInputPin
#endif
#ifdef ROM_GPIO_getEnabledInterruptStatus
#define MAP_GPIO_getEnabledInterruptStatus                                    \
        ROM_GPIO_getEnabledInterruptStatus
#else
#define MAP_GPIO_getEnabledInterruptStatus                                    \
        GPIO_getEnabledInterruptStatus
#endif
#ifdef ROM_GPIO_setDriveStrengthHigh
#define MAP_GPIO_setDriveStrengthHigh                                         \
        ROM_GPIO_setDriveStrengthHigh
#else
#define MAP_GPIO_setDriveStrengthHigh                                         \
        GPIO_setDriveStrengthHigh
#endif
#ifdef ROM_GPIO_setDriveStrengthLow
#define MAP_GPIO_setDriveStrengthLow                                          \
        ROM_GPIO_setDriveStrengthLow
#else
#define MAP_GPIO_setDriveStrengthLow                                          \
        GPIO_setDriveStrengthLow
#endif
#ifdef ROM_GPIO_registerInterrupt
#define MAP_GPIO_registerInterrupt                                            \
        ROM_GPIO_registerInterrupt
#else
#define MAP_GPIO_registerInterrupt                                            \
        GPIO_registerInterrupt
#endif
#ifdef ROM_GPIO_unregisterInterrupt
#define MAP_GPIO_unregisterInterrupt                                          \
        ROM_GPIO_unregisterInterrupt
#else
#define MAP_GPIO_unregisterInterrupt                                          \
        GPIO_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the I2C API.
//
//*****************************************************************************
#ifdef ROM_I2C_initMaster
#define MAP_I2C_initMaster                                                    \
        ROM_I2C_initMaster
#else
#define MAP_I2C_initMaster                                                    \
        I2C_initMaster
#endif
#ifdef ROM_I2C_initSlave
#define MAP_I2C_initSlave                                                     \
        ROM_I2C_initSlave
#else
#define MAP_I2C_initSlave                                                     \
        I2C_initSlave
#endif
#ifdef ROM_I2C_enableModule
#define MAP_I2C_enableModule                                                  \
        ROM_I2C_enableModule
#else
#define MAP_I2C_enableModule                                                  \
        I2C_enableModule
#endif
#ifdef ROM_I2C_disableModule
#define MAP_I2C_disableModule                                                 \
        ROM_I2C_disableModule
#else
#define MAP_I2C_disableModule                                                 \
        I2C_disableModule
#endif
#ifdef ROM_I2C_setSlaveAddress
#define MAP_I2C_setSlaveAddress                                               \
        ROM_I2C_setSlaveAddress
#else
#define MAP_I2C_setSlaveAddress                                               \
        I2C_setSlaveAddress
#endif
#ifdef ROM_I2C_setMode
#define MAP_I2C_setMode                                                       \
        ROM_I2C_setMode
#else
#define MAP_I2C_setMode                                                       \
        I2C_setMode
#endif
#ifdef ROM_I2C_slavePutData
#define MAP_I2C_slavePutData                                                  \
        ROM_I2C_slavePutData
#else
#define MAP_I2C_slavePutData                                                  \
        I2C_slavePutData
#endif
#ifdef ROM_I2C_slaveGetData
#define MAP_I2C_slaveGetData                                                  \
        ROM_I2C_slaveGetData
#else
#define MAP_I2C_slaveGetData                                                  \
        I2C_slaveGetData
#endif
#ifdef ROM_I2C_isBusBusy
#define MAP_I2C_isBusBusy                                                     \
        ROM_I2C_isBusBusy
#else
#define MAP_I2C_isBusBusy                                                     \
        I2C_isBusBusy
#endif
#ifdef ROM_I2C_masterSendSingleByte
#define MAP_I2C_masterSendSingleByte                                          \
        ROM_I2C_masterSendSingleByte
#else
#define MAP_I2C_masterSendSingleByte                                          \
        I2C_masterSendSingleByte
#endif
#ifdef ROM_I2C_masterSendSingleByteWithTimeout
#define MAP_I2C_masterSendSingleByteWithTimeout                               \
        ROM_I2C_masterSendSingleByteWithTimeout
#else
#define MAP_I2C_masterSendSingleByteWithTimeout                               \
        I2C_masterSendSingleByteWithTimeout
#endif
#ifdef ROM_I2C_masterSendMultiByteStart
#define MAP_I2C_masterSendMultiByteStart                                      \
        ROM_I2C_masterSendMultiByteStart
#else
#define MAP_I2C_masterSendMultiByteStart                                      \
        I2C_masterSendMultiByteStart
#endif
#ifdef ROM_I2C_masterSendMultiByteStartWithTimeout
#define MAP_I2C_masterSendMultiByteStartWithTimeout                           \
        ROM_I2C_masterSendMultiByteStartWithTimeout
#else
#define MAP_I2C_masterSendMultiByteStartWithTimeout                           \
        I2C_masterSendMultiByteStartWithTimeout
#endif
#ifdef ROM_I2C_masterSendMultiByteNext
#define MAP_I2C_masterSendMultiByteNext                                       \
        ROM_I2C_masterSendMultiByteNext
#else
#define MAP_I2C_masterSendMultiByteNext                                       \
        I2C_masterSendMultiByteNext
#endif
#ifdef ROM_I2C_masterSendMultiByteNextWithTimeout
#define MAP_I2C_masterSendMultiByteNextWithTimeout                            \
        ROM_I2C_masterSendMultiByteNextWithTimeout
#else
#define MAP_I2C_masterSendMultiByteNextWithTimeout                            \
        I2C_masterSendMultiByteNextWithTimeout
#endif
#ifdef ROM_I2C_masterSendMultiByteFinish
#define MAP_I2C_masterSendMultiByteFinish                                     \
        ROM_I2C_masterSendMultiByteFinish
#else
#define MAP_I2C_masterSendMultiByteFinish                                     \
        I2C_masterSendMultiByteFinish
#endif
#ifdef ROM_I2C_masterSendMultiByteFinishWithTimeout
#define MAP_I2C_masterSendMultiByteFinishWithTimeout                          \
        ROM_I2C_masterSendMultiByteFinishWithTimeout
#else
#define MAP_I2C_masterSendMultiByteFinishWithTimeout                          \
        I2C_masterSendMultiByteFinishWithTimeout
#endif
#ifdef ROM_I2C_masterSendMultiByteStop
#define MAP_I2C_masterSendMultiByteStop                                       \
        ROM_I2C_masterSendMultiByteStop
#else
#define MAP_I2C_masterSendMultiByteStop                                       \
        I2C_masterSendMultiByteStop
#endif
#ifdef ROM_I2C_masterSendMultiByteStopWithTimeout
#define MAP_I2C_masterSendMultiByteStopWithTimeout                            \
        ROM_I2C_masterSendMultiByteStopWithTimeout
#else
#define MAP_I2C_masterSendMultiByteStopWithTimeout                            \
        I2C_masterSendMultiByteStopWithTimeout
#endif
#ifdef ROM_I2C_masterReceiveStart
#define MAP_I2C_masterReceiveStart                                            \
        ROM_I2C_masterReceiveStart
#else
#define MAP_I2C_masterReceiveStart                                            \
        I2C_masterReceiveStart
#endif
#ifdef ROM_I2C_masterReceiveMultiByteNext
#define MAP_I2C_masterReceiveMultiByteNext                                    \
        ROM_I2C_masterReceiveMultiByteNext
#else
#define MAP_I2C_masterReceiveMultiByteNext                                    \
        I2C_masterReceiveMultiByteNext
#endif
#ifdef ROM_I2C_masterReceiveMultiByteFinish
#define MAP_I2C_masterReceiveMultiByteFinish                                  \
        ROM_I2C_masterReceiveMultiByteFinish
#else
#define MAP_I2C_masterReceiveMultiByteFinish                                  \
        I2C_masterReceiveMultiByteFinish
#endif
#ifdef ROM_I2C_masterReceiveMultiByteFinishWithTimeout
#define MAP_I2C_masterReceiveMultiByteFinishWithTimeout                       \
        ROM_I2C_masterReceiveMultiByteFinishWithTimeout
#else
#define MAP_I2C_masterReceiveMultiByteFinishWithTimeout                       \
        I2C_masterReceiveMultiByteFinishWithTimeout
#endif
#ifdef ROM_I2C_masterReceiveMultiByteStop
#define MAP_I2C_masterReceiveMultiByteStop                                    \
        ROM_I2C_masterReceiveMultiByteStop
#else
#define MAP_I2C_masterReceiveMultiByteStop                                    \
        I2C_masterReceiveMultiByteStop
#endif
#ifdef ROM_I2C_masterReceiveSingleByte
#define MAP_I2C_masterReceiveSingleByte                                       \
        ROM_I2C_masterReceiveSingleByte
#else
#define MAP_I2C_masterReceiveSingleByte                                       \
        I2C_masterReceiveSingleByte
#endif
#ifdef ROM_I2C_masterReceiveSingle
#define MAP_I2C_masterReceiveSingle                                           \
        ROM_I2C_masterReceiveSingle
#else
#define MAP_I2C_masterReceiveSingle                                           \
        I2C_masterReceiveSingle
#endif
#ifdef ROM_I2C_getReceiveBufferAddressForDMA
#define MAP_I2C_getReceiveBufferAddressForDMA                                 \
        ROM_I2C_getReceiveBufferAddressForDMA
#else
#define MAP_I2C_getReceiveBufferAddressForDMA                                 \
        I2C_getReceiveBufferAddressForDMA
#endif
#ifdef ROM_I2C_getTransmitBufferAddressForDMA
#define MAP_I2C_getTransmitBufferAddressForDMA                                \
        ROM_I2C_getTransmitBufferAddressForDMA
#else
#define MAP_I2C_getTransmitBufferAddressForDMA                                \
        I2C_getTransmitBufferAddressForDMA
#endif
#ifdef ROM_I2C_masterIsStopSent
#define MAP_I2C_masterIsStopSent                                              \
        ROM_I2C_masterIsStopSent
#else
#define MAP_I2C_masterIsStopSent                                              \
        I2C_masterIsStopSent
#endif
#ifdef ROM_I2C_masterIsStartSent
#define MAP_I2C_masterIsStartSent                                             \
        ROM_I2C_masterIsStartSent
#else
#define MAP_I2C_masterIsStartSent                                             \
        I2C_masterIsStartSent
#endif
#ifdef ROM_I2C_masterSendStart
#define MAP_I2C_masterSendStart                                               \
        ROM_I2C_masterSendStart
#else
#define MAP_I2C_masterSendStart                                               \
        I2C_masterSendStart
#endif
#ifdef ROM_I2C_enableMultiMasterMode
#define MAP_I2C_enableMultiMasterMode                                         \
        ROM_I2C_enableMultiMasterMode
#else
#define MAP_I2C_enableMultiMasterMode                                         \
        I2C_enableMultiMasterMode
#endif
#ifdef ROM_I2C_disableMultiMasterMode
#define MAP_I2C_disableMultiMasterMode                                        \
        ROM_I2C_disableMultiMasterMode
#else
#define MAP_I2C_disableMultiMasterMode                                        \
        I2C_disableMultiMasterMode
#endif
#ifdef ROM_I2C_enableInterrupt
#define MAP_I2C_enableInterrupt                                               \
        ROM_I2C_enableInterrupt
#else
#define MAP_I2C_enableInterrupt                                               \
        I2C_enableInterrupt
#endif
#ifdef ROM_I2C_disableInterrupt
#define MAP_I2C_disableInterrupt                                              \
        ROM_I2C_disableInterrupt
#else
#define MAP_I2C_disableInterrupt                                              \
        I2C_disableInterrupt
#endif
#ifdef ROM_I2C_clearInterruptFlag
#define MAP_I2C_clearInterruptFlag                                            \
        ROM_I2C_clearInterruptFlag
#else
#define MAP_I2C_clearInterruptFlag                                            \
        I2C_clearInterruptFlag
#endif
#ifdef ROM_I2C_getInterruptStatus
#define MAP_I2C_getInterruptStatus                                            \
        ROM_I2C_getInterruptStatus
#else
#define MAP_I2C_getInterruptStatus                                            \
        I2C_getInterruptStatus
#endif
#ifdef ROM_I2C_getEnabledInterruptStatus
#define MAP_I2C_getEnabledInterruptStatus                                     \
        ROM_I2C_getEnabledInterruptStatus
#else
#define MAP_I2C_getEnabledInterruptStatus                                     \
        I2C_getEnabledInterruptStatus
#endif
#ifdef ROM_I2C_getMode
#define MAP_I2C_getMode                                                       \
        ROM_I2C_getMode
#else
#define MAP_I2C_getMode                                                       \
        I2C_getMode
#endif
#ifdef ROM_I2C_registerInterrupt
#define MAP_I2C_registerInterrupt                                             \
        ROM_I2C_registerInterrupt
#else
#define MAP_I2C_registerInterrupt                                             \
        I2C_registerInterrupt
#endif
#ifdef ROM_I2C_unregisterInterrupt
#define MAP_I2C_unregisterInterrupt                                           \
        ROM_I2C_unregisterInterrupt
#else
#define MAP_I2C_unregisterInterrupt                                           \
        I2C_unregisterInterrupt
#endif
#ifdef ROM_I2C_slaveSendNAK
#define MAP_I2C_slaveSendNAK                                                  \
        ROM_I2C_slaveSendNAK
#else
#define MAP_I2C_slaveSendNAK                                                  \
        I2C_slaveSendNAK
#endif

//*****************************************************************************
//
// Macros for the Interrupt API.
//
//*****************************************************************************
#ifdef ROM_Interrupt_enableMaster
#define MAP_Interrupt_enableMaster                                            \
        ROM_Interrupt_enableMaster
#else
#define MAP_Interrupt_enableMaster                                            \
        Interrupt_enableMaster
#endif
#ifdef ROM_Interrupt_disableMaster
#define MAP_Interrupt_disableMaster                                           \
        ROM_Interrupt_disableMaster
#else
#define MAP_Interrupt_disableMaster                                           \
        Interrupt_disableMaster
#endif
#ifdef ROM_Interrupt_setPriorityGrouping
#define MAP_Interrupt_setPriorityGrouping                                     \
        ROM_Interrupt_setPriorityGrouping
#else
#define MAP_Interrupt_setPriorityGrouping                                     \
        Interrupt_setPriorityGrouping
#endif
#ifdef ROM_Interrupt_getPriorityGrouping
#define MAP_Interrupt_getPriorityGrouping                                     \
        ROM_Interrupt_getPriorityGrouping
#else
#define MAP_Interrupt_getPriorityGrouping                                     \
        Interrupt_getPriorityGrouping
#endif
#ifdef ROM_Interrupt_setPriority
#define MAP_Interrupt_setPriority                                             \
        ROM_Interrupt_setPriority
#else
#define MAP_Interrupt_setPriority                                             \
        Interrupt_setPriority
#endif
#ifdef ROM_Interrupt_getPriority
#define MAP_Interrupt_getPriority                                             \
        ROM_Interrupt_getPriority
#else
#define MAP_Interrupt_getPriority                                             \
        Interrupt_getPriority
#endif
#ifdef ROM_Interrupt_enableInterrupt
#define MAP_Interrupt_enableInterrupt                                         \
        ROM_Interrupt_enableInterrupt
#else
#define MAP_Interrupt_enableInterrupt                                         \
        Interrupt_enableInterrupt
#endif
#ifdef ROM_Interrupt_disableInterrupt
#define MAP_Interrupt_disableInterrupt                                        \
        ROM_Interrupt_disableInterrupt
#else
#define MAP_Interrupt_disableInterrupt                                        \
        Interrupt_disableInterrupt
#endif
#ifdef ROM_Interrupt_isEnabled
#define MAP_Interrupt_isEnabled                                               \
        ROM_Interrupt_isEnabled
#else
#define MAP_Interrupt_isEnabled                                               \
        Interrupt_isEnabled
#endif
#ifdef ROM_Interrupt_pendInterrupt
#define MAP_Interrupt_pendInterrupt                                           \
        ROM_Interrupt_pendInterrupt
#else
#define MAP_Interrupt_pendInterrupt                                           \
        Interrupt_pendInterrupt
#endif
#ifdef ROM_Interrupt_setPriorityMask
#define MAP_Interrupt_setPriorityMask                                         \
        ROM_Interrupt_setPriorityMask
#else
#define MAP_Interrupt_setPriorityMask                                         \
        Interrupt_setPriorityMask
#endif
#ifdef ROM_Interrupt_getPriorityMask
#define MAP_Interrupt_getPriorityMask                                         \
        ROM_Interrupt_getPriorityMask
#else
#define MAP_Interrupt_getPriorityMask                                         \
        Interrupt_getPriorityMask
#endif
#ifdef ROM_Interrupt_setVectorTableAddress
#define MAP_Interrupt_setVectorTableAddress                                   \
        ROM_Interrupt_setVectorTableAddress
#else
#define MAP_Interrupt_setVectorTableAddress                                   \
        Interrupt_setVectorTableAddress
#endif
#ifdef ROM_Interrupt_getVectorTableAddress
#define MAP_Interrupt_getVectorTableAddress                                   \
        ROM_Interrupt_getVectorTableAddress
#else
#define MAP_Interrupt_getVectorTableAddress                                   \
        Interrupt_getVectorTableAddress
#endif
#ifdef ROM_Interrupt_enableSleepOnIsrExit
#define MAP_Interrupt_enableSleepOnIsrExit                                    \
        ROM_Interrupt_enableSleepOnIsrExit
#else
#define MAP_Interrupt_enableSleepOnIsrExit                                    \
        Interrupt_enableSleepOnIsrExit
#endif
#ifdef ROM_Interrupt_disableSleepOnIsrExit
#define MAP_Interrupt_disableSleepOnIsrExit                                   \
        ROM_Interrupt_disableSleepOnIsrExit
#else
#define MAP_Interrupt_disableSleepOnIsrExit                                   \
        Interrupt_disableSleepOnIsrExit
#endif
#ifdef ROM_Interrupt_registerInterrupt
#define MAP_Interrupt_registerInterrupt                                       \
        ROM_Interrupt_registerInterrupt
#else
#define MAP_Interrupt_registerInterrupt                                       \
        Interrupt_registerInterrupt
#endif
#ifdef ROM_Interrupt_unregisterInterrupt
#define MAP_Interrupt_unregisterInterrupt                                     \
        ROM_Interrupt_unregisterInterrupt
#else
#define MAP_Interrupt_unregisterInterrupt                                     \
        Interrupt_unregisterInterrupt
#endif
#ifdef ROM_Interrupt_unpendInterrupt
#define MAP_Interrupt_unpendInterrupt                                         \
        ROM_Interrupt_unpendInterrupt
#else
#define MAP_Interrupt_unpendInterrupt                                         \
        Interrupt_unpendInterrupt
#endif

//*****************************************************************************
//
// Macros for the MPU API.
//
//*****************************************************************************
#ifdef ROM_MPU_enableModule
#define MAP_MPU_enableModule                                                  \
        ROM_MPU_enableModule
#else
#define MAP_MPU_enableModule                                                  \
        MPU_enableModule
#endif
#ifdef ROM_MPU_disableModule
#define MAP_MPU_disableModule                                                 \
        ROM_MPU_disableModule
#else
#define MAP_MPU_disableModule                                                 \
        MPU_disableModule
#endif
#ifdef ROM_MPU_getRegionCount
#define MAP_MPU_getRegionCount                                                \
        ROM_MPU_getRegionCount
#else
#define MAP_MPU_getRegionCount                                                \
        MPU_getRegionCount
#endif
#ifdef ROM_MPU_enableRegion
#define MAP_MPU_enableRegion                                                  \
        ROM_MPU_enableRegion
#else
#define MAP_MPU_enableRegion                                                  \
        MPU_enableRegion
#endif
#ifdef ROM_MPU_disableRegion
#define MAP_MPU_disableRegion                                                 \
        ROM_MPU_disableRegion
#else
#define MAP_MPU_disableRegion                                                 \
        MPU_disableRegion
#endif
#ifdef ROM_MPU_setRegion
#define MAP_MPU_setRegion                                                     \
        ROM_MPU_setRegion
#else
#define MAP_MPU_setRegion                                                     \
        MPU_setRegion
#endif
#ifdef ROM_MPU_getRegion
#define MAP_MPU_getRegion                                                     \
        ROM_MPU_getRegion
#else
#define MAP_MPU_getRegion                                                     \
        MPU_getRegion
#endif
#ifdef ROM_MPU_enableInterrupt
#define MAP_MPU_enableInterrupt                                               \
        ROM_MPU_enableInterrupt
#else
#define MAP_MPU_enableInterrupt                                               \
        MPU_enableInterrupt
#endif
#ifdef ROM_MPU_disableInterrupt
#define MAP_MPU_disableInterrupt                                              \
        ROM_MPU_disableInterrupt
#else
#define MAP_MPU_disableInterrupt                                              \
        MPU_disableInterrupt
#endif
#ifdef ROM_MPU_registerInterrupt
#define MAP_MPU_registerInterrupt                                             \
        ROM_MPU_registerInterrupt
#else
#define MAP_MPU_registerInterrupt                                             \
        MPU_registerInterrupt
#endif
#ifdef ROM_MPU_unregisterInterrupt
#define MAP_MPU_unregisterInterrupt                                           \
        ROM_MPU_unregisterInterrupt
#else
#define MAP_MPU_unregisterInterrupt                                           \
        MPU_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the PCM API.
//
//*****************************************************************************
#ifdef ROM_PCM_setCoreVoltageLevel
#define MAP_PCM_setCoreVoltageLevel                                           \
        ROM_PCM_setCoreVoltageLevel
#else
#define MAP_PCM_setCoreVoltageLevel                                           \
        PCM_setCoreVoltageLevel
#endif
#ifdef ROM_PCM_getCoreVoltageLevel
#define MAP_PCM_getCoreVoltageLevel                                           \
        ROM_PCM_getCoreVoltageLevel
#else
#define MAP_PCM_getCoreVoltageLevel                                           \
        PCM_getCoreVoltageLevel
#endif
#ifdef ROM_PCM_setCoreVoltageLevelWithTimeout
#define MAP_PCM_setCoreVoltageLevelWithTimeout                                \
        ROM_PCM_setCoreVoltageLevelWithTimeout
#else
#define MAP_PCM_setCoreVoltageLevelWithTimeout                                \
        PCM_setCoreVoltageLevelWithTimeout
#endif
#ifdef ROM_PCM_setPowerMode
#define MAP_PCM_setPowerMode                                                  \
        ROM_PCM_setPowerMode
#else
#define MAP_PCM_setPowerMode                                                  \
        PCM_setPowerMode
#endif
#ifdef ROM_PCM_setPowerModeWithTimeout
#define MAP_PCM_setPowerModeWithTimeout                                       \
        ROM_PCM_setPowerModeWithTimeout
#else
#define MAP_PCM_setPowerModeWithTimeout                                       \
        PCM_setPowerModeWithTimeout
#endif
#ifdef ROM_PCM_getPowerMode
#define MAP_PCM_getPowerMode                                                  \
        ROM_PCM_getPowerMode
#else
#define MAP_PCM_getPowerMode                                                  \
        PCM_getPowerMode
#endif
#ifdef ROM_PCM_setPowerState
#define MAP_PCM_setPowerState                                                 \
        ROM_PCM_setPowerState
#else
#define MAP_PCM_setPowerState                                                 \
        PCM_setPowerState
#endif
#ifdef ROM_PCM_setPowerStateWithTimeout
#define MAP_PCM_setPowerStateWithTimeout                                      \
        ROM_PCM_setPowerStateWithTimeout
#else
#define MAP_PCM_setPowerStateWithTimeout                                      \
        PCM_setPowerStateWithTimeout
#endif
#ifdef ROM_PCM_getPowerState
#define MAP_PCM_getPowerState                                                 \
        ROM_PCM_getPowerState
#else
#define MAP_PCM_getPowerState                                                 \
        PCM_getPowerState
#endif
#ifdef ROM_PCM_shutdownDevice
#define MAP_PCM_shutdownDevice                                                \
        ROM_PCM_shutdownDevice
#else
#define MAP_PCM_shutdownDevice                                                \
        PCM_shutdownDevice
#endif
#ifdef ROM_PCM_gotoLPM0
#define MAP_PCM_gotoLPM0                                                      \
        ROM_PCM_gotoLPM0
#else
#define MAP_PCM_gotoLPM0                                                      \
        PCM_gotoLPM0
#endif
#ifdef ROM_PCM_gotoLPM3
#define MAP_PCM_gotoLPM3                                                      \
        ROM_PCM_gotoLPM3
#else
#define MAP_PCM_gotoLPM3                                                      \
        PCM_gotoLPM3
#endif
#ifdef ROM_PCM_enableInterrupt
#define MAP_PCM_enableInterrupt                                               \
        ROM_PCM_enableInterrupt
#else
#define MAP_PCM_enableInterrupt                                               \
        PCM_enableInterrupt
#endif
#ifdef ROM_PCM_disableInterrupt
#define MAP_PCM_disableInterrupt                                              \
        ROM_PCM_disableInterrupt
#else
#define MAP_PCM_disableInterrupt                                              \
        PCM_disableInterrupt
#endif
#ifdef ROM_PCM_getInterruptStatus
#define MAP_PCM_getInterruptStatus                                            \
        ROM_PCM_getInterruptStatus
#else
#define MAP_PCM_getInterruptStatus                                            \
        PCM_getInterruptStatus
#endif
#ifdef ROM_PCM_getEnabledInterruptStatus
#define MAP_PCM_getEnabledInterruptStatus                                     \
        ROM_PCM_getEnabledInterruptStatus
#else
#define MAP_PCM_getEnabledInterruptStatus                                     \
        PCM_getEnabledInterruptStatus
#endif
#ifdef ROM_PCM_clearInterruptFlag
#define MAP_PCM_clearInterruptFlag                                            \
        ROM_PCM_clearInterruptFlag
#else
#define MAP_PCM_clearInterruptFlag                                            \
        PCM_clearInterruptFlag
#endif
#ifdef ROM_PCM_enableRudeMode
#define MAP_PCM_enableRudeMode                                                \
        ROM_PCM_enableRudeMode
#else
#define MAP_PCM_enableRudeMode                                                \
        PCM_enableRudeMode
#endif
#ifdef ROM_PCM_disableRudeMode
#define MAP_PCM_disableRudeMode                                               \
        ROM_PCM_disableRudeMode
#else
#define MAP_PCM_disableRudeMode                                               \
        PCM_disableRudeMode
#endif
#ifdef ROM_PCM_gotoLPM0InterruptSafe
#define MAP_PCM_gotoLPM0InterruptSafe                                         \
        ROM_PCM_gotoLPM0InterruptSafe
#else
#define MAP_PCM_gotoLPM0InterruptSafe                                         \
        PCM_gotoLPM0InterruptSafe
#endif
#ifdef ROM_PCM_gotoLPM3InterruptSafe
#define MAP_PCM_gotoLPM3InterruptSafe                                         \
        ROM_PCM_gotoLPM3InterruptSafe
#else
#define MAP_PCM_gotoLPM3InterruptSafe                                         \
        PCM_gotoLPM3InterruptSafe
#endif
#ifdef ROM_PCM_registerInterrupt
#define MAP_PCM_registerInterrupt                                             \
        ROM_PCM_registerInterrupt
#else
#define MAP_PCM_registerInterrupt                                             \
        PCM_registerInterrupt
#endif
#ifdef ROM_PCM_unregisterInterrupt
#define MAP_PCM_unregisterInterrupt                                           \
        ROM_PCM_unregisterInterrupt
#else
#define MAP_PCM_unregisterInterrupt                                           \
        PCM_unregisterInterrupt
#endif
#ifdef ROM_PCM_setCoreVoltageLevelNonBlocking
#define MAP_PCM_setCoreVoltageLevelNonBlocking                                \
        ROM_PCM_setCoreVoltageLevelNonBlocking
#else
#define MAP_PCM_setCoreVoltageLevelNonBlocking                                \
        PCM_setCoreVoltageLevelNonBlocking
#endif
#ifdef ROM_PCM_setPowerModeNonBlocking
#define MAP_PCM_setPowerModeNonBlocking                                       \
        ROM_PCM_setPowerModeNonBlocking
#else
#define MAP_PCM_setPowerModeNonBlocking                                       \
        PCM_setPowerModeNonBlocking
#endif
#ifdef ROM_PCM_setPowerStateNonBlocking
#define MAP_PCM_setPowerStateNonBlocking                                      \
        ROM_PCM_setPowerStateNonBlocking
#else
#define MAP_PCM_setPowerStateNonBlocking                                      \
        PCM_setPowerStateNonBlocking
#endif
#ifdef ROM_PCM_gotoLPM4
#define MAP_PCM_gotoLPM4                                                      \
        ROM_PCM_gotoLPM4
#else
#define MAP_PCM_gotoLPM4                                                      \
        PCM_gotoLPM4
#endif
#ifdef ROM_PCM_gotoLPM4InterruptSafe
#define MAP_PCM_gotoLPM4InterruptSafe                                         \
        ROM_PCM_gotoLPM4InterruptSafe
#else
#define MAP_PCM_gotoLPM4InterruptSafe                                         \
        PCM_gotoLPM4InterruptSafe
#endif

//*****************************************************************************
//
// Macros for the PMAP API.
//
//*****************************************************************************
#ifdef ROM_PMAP_configurePorts
#define MAP_PMAP_configurePorts                                               \
        ROM_PMAP_configurePorts
#else
#define MAP_PMAP_configurePorts                                               \
        PMAP_configurePorts
#endif

//*****************************************************************************
//
// Macros for the PSS API.
//
//*****************************************************************************
#ifdef ROM_PSS_enableHighSidePinToggle
#define MAP_PSS_enableHighSidePinToggle                                       \
        ROM_PSS_enableHighSidePinToggle
#else
#define MAP_PSS_enableHighSidePinToggle                                       \
        PSS_enableHighSidePinToggle
#endif
#ifdef ROM_PSS_disableHighSidePinToggle
#define MAP_PSS_disableHighSidePinToggle                                      \
        ROM_PSS_disableHighSidePinToggle
#else
#define MAP_PSS_disableHighSidePinToggle                                      \
        PSS_disableHighSidePinToggle
#endif
#ifdef ROM_PSS_enableHighSide
#define MAP_PSS_enableHighSide                                                \
        ROM_PSS_enableHighSide
#else
#define MAP_PSS_enableHighSide                                                \
        PSS_enableHighSide
#endif
#ifdef ROM_PSS_disableHighSide
#define MAP_PSS_disableHighSide                                               \
        ROM_PSS_disableHighSide
#else
#define MAP_PSS_disableHighSide                                               \
        PSS_disableHighSide
#endif
#ifdef ROM_PSS_enableLowSide
#define MAP_PSS_enableLowSide                                                 \
        ROM_PSS_enableLowSide
#else
#define MAP_PSS_enableLowSide                                                 \
        PSS_enableLowSide
#endif
#ifdef ROM_PSS_disableLowSide
#define MAP_PSS_disableLowSide                                                \
        ROM_PSS_disableLowSide
#else
#define MAP_PSS_disableLowSide                                                \
        PSS_disableLowSide
#endif
#ifdef ROM_PSS_setHighSidePerformanceMode
#define MAP_PSS_setHighSidePerformanceMode                                    \
        ROM_PSS_setHighSidePerformanceMode
#else
#define MAP_PSS_setHighSidePerformanceMode                                    \
        PSS_setHighSidePerformanceMode
#endif
#ifdef ROM_PSS_getHighSidePerformanceMode
#define MAP_PSS_getHighSidePerformanceMode                                    \
        ROM_PSS_getHighSidePerformanceMode
#else
#define MAP_PSS_getHighSidePerformanceMode                                    \
        PSS_getHighSidePerformanceMode
#endif
#ifdef ROM_PSS_setLowSidePerformanceMode
#define MAP_PSS_setLowSidePerformanceMode                                     \
        ROM_PSS_setLowSidePerformanceMode
#else
#define MAP_PSS_setLowSidePerformanceMode                                     \
        PSS_setLowSidePerformanceMode
#endif
#ifdef ROM_PSS_getLowSidePerformanceMode
#define MAP_PSS_getLowSidePerformanceMode                                     \
        ROM_PSS_getLowSidePerformanceMode
#else
#define MAP_PSS_getLowSidePerformanceMode                                     \
        PSS_getLowSidePerformanceMode
#endif
#ifdef ROM_PSS_enableHighSideMonitor
#define MAP_PSS_enableHighSideMonitor                                         \
        ROM_PSS_enableHighSideMonitor
#else
#define MAP_PSS_enableHighSideMonitor                                         \
        PSS_enableHighSideMonitor
#endif
#ifdef ROM_PSS_disableHighSideMonitor
#define MAP_PSS_disableHighSideMonitor                                        \
        ROM_PSS_disableHighSideMonitor
#else
#define MAP_PSS_disableHighSideMonitor                                        \
        PSS_disableHighSideMonitor
#endif
#ifdef ROM_PSS_setHighSideVoltageTrigger
#define MAP_PSS_setHighSideVoltageTrigger                                     \
        ROM_PSS_setHighSideVoltageTrigger
#else
#define MAP_PSS_setHighSideVoltageTrigger                                     \
        PSS_setHighSideVoltageTrigger
#endif
#ifdef ROM_PSS_getHighSideVoltageTrigger
#define MAP_PSS_getHighSideVoltageTrigger                                     \
        ROM_PSS_getHighSideVoltageTrigger
#else
#define MAP_PSS_getHighSideVoltageTrigger                                     \
        PSS_getHighSideVoltageTrigger
#endif
#ifdef ROM_PSS_enableInterrupt
#define MAP_PSS_enableInterrupt                                               \
        ROM_PSS_enableInterrupt
#else
#define MAP_PSS_enableInterrupt                                               \
        PSS_enableInterrupt
#endif
#ifdef ROM_PSS_disableInterrupt
#define MAP_PSS_disableInterrupt                                              \
        ROM_PSS_disableInterrupt
#else
#define MAP_PSS_disableInterrupt                                              \
        PSS_disableInterrupt
#endif
#ifdef ROM_PSS_getInterruptStatus
#define MAP_PSS_getInterruptStatus                                            \
        ROM_PSS_getInterruptStatus
#else
#define MAP_PSS_getInterruptStatus                                            \
        PSS_getInterruptStatus
#endif
#ifdef ROM_PSS_clearInterruptFlag
#define MAP_PSS_clearInterruptFlag                                            \
        ROM_PSS_clearInterruptFlag
#else
#define MAP_PSS_clearInterruptFlag                                            \
        PSS_clearInterruptFlag
#endif
#ifdef ROM_PSS_registerInterrupt
#define MAP_PSS_registerInterrupt                                             \
        ROM_PSS_registerInterrupt
#else
#define MAP_PSS_registerInterrupt                                             \
        PSS_registerInterrupt
#endif
#ifdef ROM_PSS_unregisterInterrupt
#define MAP_PSS_unregisterInterrupt                                           \
        ROM_PSS_unregisterInterrupt
#else
#define MAP_PSS_unregisterInterrupt                                           \
        PSS_unregisterInterrupt
#endif
#ifdef ROM_PSS_enableForcedDCDCOperation
#define MAP_PSS_enableForcedDCDCOperation                                     \
        ROM_PSS_enableForcedDCDCOperation
#else
#define MAP_PSS_enableForcedDCDCOperation                                     \
        PSS_enableForcedDCDCOperation
#endif
#ifdef ROM_PSS_disableForcedDCDCOperation
#define MAP_PSS_disableForcedDCDCOperation                                    \
        ROM_PSS_disableForcedDCDCOperation
#else
#define MAP_PSS_disableForcedDCDCOperation                                    \
        PSS_disableForcedDCDCOperation
#endif

//*****************************************************************************
//
// Macros for the Ref API.
//
//*****************************************************************************
#ifdef ROM_REF_A_setReferenceVoltage
#define MAP_REF_A_setReferenceVoltage                                         \
        ROM_REF_A_setReferenceVoltage
#else
#define MAP_REF_A_setReferenceVoltage                                         \
        REF_A_setReferenceVoltage
#endif
#ifdef ROM_REF_A_disableTempSensor
#define MAP_REF_A_disableTempSensor                                           \
        ROM_REF_A_disableTempSensor
#else
#define MAP_REF_A_disableTempSensor                                           \
        REF_A_disableTempSensor
#endif
#ifdef ROM_REF_A_enableTempSensor
#define MAP_REF_A_enableTempSensor                                            \
        ROM_REF_A_enableTempSensor
#else
#define MAP_REF_A_enableTempSensor                                            \
        REF_A_enableTempSensor
#endif
#ifdef ROM_REF_A_enableReferenceVoltageOutput
#define MAP_REF_A_enableReferenceVoltageOutput                                \
        ROM_REF_A_enableReferenceVoltageOutput
#else
#define MAP_REF_A_enableReferenceVoltageOutput                                \
        REF_A_enableReferenceVoltageOutput
#endif
#ifdef ROM_REF_A_disableReferenceVoltageOutput
#define MAP_REF_A_disableReferenceVoltageOutput                               \
        ROM_REF_A_disableReferenceVoltageOutput
#else
#define MAP_REF_A_disableReferenceVoltageOutput                               \
        REF_A_disableReferenceVoltageOutput
#endif
#ifdef ROM_REF_A_enableReferenceVoltage
#define MAP_REF_A_enableReferenceVoltage                                      \
        ROM_REF_A_enableReferenceVoltage
#else
#define MAP_REF_A_enableReferenceVoltage                                      \
        REF_A_enableReferenceVoltage
#endif
#ifdef ROM_REF_A_disableReferenceVoltage
#define MAP_REF_A_disableReferenceVoltage                                     \
        ROM_REF_A_disableReferenceVoltage
#else
#define MAP_REF_A_disableReferenceVoltage                                     \
        REF_A_disableReferenceVoltage
#endif
#ifdef ROM_REF_A_getBandgapMode
#define MAP_REF_A_getBandgapMode                                              \
        ROM_REF_A_getBandgapMode
#else
#define MAP_REF_A_getBandgapMode                                              \
        REF_A_getBandgapMode
#endif
#ifdef ROM_REF_A_isBandgapActive
#define MAP_REF_A_isBandgapActive                                             \
        ROM_REF_A_isBandgapActive
#else
#define MAP_REF_A_isBandgapActive                                             \
        REF_A_isBandgapActive
#endif
#ifdef ROM_REF_A_isRefGenBusy
#define MAP_REF_A_isRefGenBusy                                                \
        ROM_REF_A_isRefGenBusy
#else
#define MAP_REF_A_isRefGenBusy                                                \
        REF_A_isRefGenBusy
#endif
#ifdef ROM_REF_A_isRefGenActive
#define MAP_REF_A_isRefGenActive                                              \
        ROM_REF_A_isRefGenActive
#else
#define MAP_REF_A_isRefGenActive                                              \
        REF_A_isRefGenActive
#endif
#ifdef ROM_REF_A_getBufferedBandgapVoltageStatus
#define MAP_REF_A_getBufferedBandgapVoltageStatus                             \
        ROM_REF_A_getBufferedBandgapVoltageStatus
#else
#define MAP_REF_A_getBufferedBandgapVoltageStatus                             \
        REF_A_getBufferedBandgapVoltageStatus
#endif
#ifdef ROM_REF_A_getVariableReferenceVoltageStatus
#define MAP_REF_A_getVariableReferenceVoltageStatus                           \
        ROM_REF_A_getVariableReferenceVoltageStatus
#else
#define MAP_REF_A_getVariableReferenceVoltageStatus                           \
        REF_A_getVariableReferenceVoltageStatus
#endif
#ifdef ROM_REF_A_setReferenceVoltageOneTimeTrigger
#define MAP_REF_A_setReferenceVoltageOneTimeTrigger                           \
        ROM_REF_A_setReferenceVoltageOneTimeTrigger
#else
#define MAP_REF_A_setReferenceVoltageOneTimeTrigger                           \
        REF_A_setReferenceVoltageOneTimeTrigger
#endif
#ifdef ROM_REF_A_setBufferedBandgapVoltageOneTimeTrigger
#define MAP_REF_A_setBufferedBandgapVoltageOneTimeTrigger                     \
        ROM_REF_A_setBufferedBandgapVoltageOneTimeTrigger
#else
#define MAP_REF_A_setBufferedBandgapVoltageOneTimeTrigger                     \
        REF_A_setBufferedBandgapVoltageOneTimeTrigger
#endif

//*****************************************************************************
//
// Macros for the ResetCtl API.
//
//*****************************************************************************
#ifdef ROM_ResetCtl_initiateSoftReset
#define MAP_ResetCtl_initiateSoftReset                                        \
        ROM_ResetCtl_initiateSoftReset
#else
#define MAP_ResetCtl_initiateSoftReset                                        \
        ResetCtl_initiateSoftReset
#endif
#ifdef ROM_ResetCtl_initiateSoftResetWithSource
#define MAP_ResetCtl_initiateSoftResetWithSource                              \
        ROM_ResetCtl_initiateSoftResetWithSource
#else
#define MAP_ResetCtl_initiateSoftResetWithSource                              \
        ResetCtl_initiateSoftResetWithSource
#endif
#ifdef ROM_ResetCtl_getSoftResetSource
#define MAP_ResetCtl_getSoftResetSource                                       \
        ROM_ResetCtl_getSoftResetSource
#else
#define MAP_ResetCtl_getSoftResetSource                                       \
        ResetCtl_getSoftResetSource
#endif
#ifdef ROM_ResetCtl_clearSoftResetSource
#define MAP_ResetCtl_clearSoftResetSource                                     \
        ROM_ResetCtl_clearSoftResetSource
#else
#define MAP_ResetCtl_clearSoftResetSource                                     \
        ResetCtl_clearSoftResetSource
#endif
#ifdef ROM_ResetCtl_initiateHardReset
#define MAP_ResetCtl_initiateHardReset                                        \
        ROM_ResetCtl_initiateHardReset
#else
#define MAP_ResetCtl_initiateHardReset                                        \
        ResetCtl_initiateHardReset
#endif
#ifdef ROM_ResetCtl_initiateHardResetWithSource
#define MAP_ResetCtl_initiateHardResetWithSource                              \
        ROM_ResetCtl_initiateHardResetWithSource
#else
#define MAP_ResetCtl_initiateHardResetWithSource                              \
        ResetCtl_initiateHardResetWithSource
#endif
#ifdef ROM_ResetCtl_getHardResetSource
#define MAP_ResetCtl_getHardResetSource                                       \
        ROM_ResetCtl_getHardResetSource
#else
#define MAP_ResetCtl_getHardResetSource                                       \
        ResetCtl_getHardResetSource
#endif
#ifdef ROM_ResetCtl_clearHardResetSource
#define MAP_ResetCtl_clearHardResetSource                                     \
        ROM_ResetCtl_clearHardResetSource
#else
#define MAP_ResetCtl_clearHardResetSource                                     \
        ResetCtl_clearHardResetSource
#endif
#ifdef ROM_ResetCtl_getPSSSource
#define MAP_ResetCtl_getPSSSource                                             \
        ROM_ResetCtl_getPSSSource
#else
#define MAP_ResetCtl_getPSSSource                                             \
        ResetCtl_getPSSSource
#endif
#ifdef ROM_ResetCtl_clearPSSFlags
#define MAP_ResetCtl_clearPSSFlags                                            \
        ROM_ResetCtl_clearPSSFlags
#else
#define MAP_ResetCtl_clearPSSFlags                                            \
        ResetCtl_clearPSSFlags
#endif
#ifdef ROM_ResetCtl_getPCMSource
#define MAP_ResetCtl_getPCMSource                                             \
        ROM_ResetCtl_getPCMSource
#else
#define MAP_ResetCtl_getPCMSource                                             \
        ResetCtl_getPCMSource
#endif
#ifdef ROM_ResetCtl_clearPCMFlags
#define MAP_ResetCtl_clearPCMFlags                                            \
        ROM_ResetCtl_clearPCMFlags
#else
#define MAP_ResetCtl_clearPCMFlags                                            \
        ResetCtl_clearPCMFlags
#endif

//*****************************************************************************
//
// Macros for the RTC API.
//
//*****************************************************************************
#ifdef ROM_RTC_C_startClock
#define MAP_RTC_C_startClock                                                  \
        ROM_RTC_C_startClock
#else
#define MAP_RTC_C_startClock                                                  \
        RTC_C_startClock
#endif
#ifdef ROM_RTC_C_holdClock
#define MAP_RTC_C_holdClock                                                   \
        ROM_RTC_C_holdClock
#else
#define MAP_RTC_C_holdClock                                                   \
        RTC_C_holdClock
#endif
#ifdef ROM_RTC_C_setCalibrationFrequency
#define MAP_RTC_C_setCalibrationFrequency                                     \
        ROM_RTC_C_setCalibrationFrequency
#else
#define MAP_RTC_C_setCalibrationFrequency                                     \
        RTC_C_setCalibrationFrequency
#endif
#ifdef ROM_RTC_C_setCalibrationData
#define MAP_RTC_C_setCalibrationData                                          \
        ROM_RTC_C_setCalibrationData
#else
#define MAP_RTC_C_setCalibrationData                                          \
        RTC_C_setCalibrationData
#endif
#ifdef ROM_RTC_C_setTemperatureCompensation
#define MAP_RTC_C_setTemperatureCompensation                                  \
        ROM_RTC_C_setTemperatureCompensation
#else
#define MAP_RTC_C_setTemperatureCompensation                                  \
        RTC_C_setTemperatureCompensation
#endif
#ifdef ROM_RTC_C_initCalendar
#define MAP_RTC_C_initCalendar                                                \
        ROM_RTC_C_initCalendar
#else
#define MAP_RTC_C_initCalendar                                                \
        RTC_C_initCalendar
#endif
#ifdef ROM_RTC_C_getCalendarTime
#define MAP_RTC_C_getCalendarTime                                             \
        ROM_RTC_C_getCalendarTime
#else
#define MAP_RTC_C_getCalendarTime                                             \
        RTC_C_getCalendarTime
#endif
#ifdef ROM_RTC_C_setCalendarAlarm
#define MAP_RTC_C_setCalendarAlarm                                            \
        ROM_RTC_C_setCalendarAlarm
#else
#define MAP_RTC_C_setCalendarAlarm                                            \
        RTC_C_setCalendarAlarm
#endif
#ifdef ROM_RTC_C_setCalendarEvent
#define MAP_RTC_C_setCalendarEvent                                            \
        ROM_RTC_C_setCalendarEvent
#else
#define MAP_RTC_C_setCalendarEvent                                            \
        RTC_C_setCalendarEvent
#endif
#ifdef ROM_RTC_C_definePrescaleEvent
#define MAP_RTC_C_definePrescaleEvent                                         \
        ROM_RTC_C_definePrescaleEvent
#else
#define MAP_RTC_C_definePrescaleEvent                                         \
        RTC_C_definePrescaleEvent
#endif
#ifdef ROM_RTC_C_getPrescaleValue
#define MAP_RTC_C_getPrescaleValue                                            \
        ROM_RTC_C_getPrescaleValue
#else
#define MAP_RTC_C_getPrescaleValue                                            \
        RTC_C_getPrescaleValue
#endif
#ifdef ROM_RTC_C_setPrescaleValue
#define MAP_RTC_C_setPrescaleValue                                            \
        ROM_RTC_C_setPrescaleValue
#else
#define MAP_RTC_C_setPrescaleValue                                            \
        RTC_C_setPrescaleValue
#endif
#ifdef ROM_RTC_C_convertBCDToBinary
#define MAP_RTC_C_convertBCDToBinary                                          \
        ROM_RTC_C_convertBCDToBinary
#else
#define MAP_RTC_C_convertBCDToBinary                                          \
        RTC_C_convertBCDToBinary
#endif
#ifdef ROM_RTC_C_convertBinaryToBCD
#define MAP_RTC_C_convertBinaryToBCD                                          \
        ROM_RTC_C_convertBinaryToBCD
#else
#define MAP_RTC_C_convertBinaryToBCD                                          \
        RTC_C_convertBinaryToBCD
#endif
#ifdef ROM_RTC_C_enableInterrupt
#define MAP_RTC_C_enableInterrupt                                             \
        ROM_RTC_C_enableInterrupt
#else
#define MAP_RTC_C_enableInterrupt                                             \
        RTC_C_enableInterrupt
#endif
#ifdef ROM_RTC_C_disableInterrupt
#define MAP_RTC_C_disableInterrupt                                            \
        ROM_RTC_C_disableInterrupt
#else
#define MAP_RTC_C_disableInterrupt                                            \
        RTC_C_disableInterrupt
#endif
#ifdef ROM_RTC_C_getInterruptStatus
#define MAP_RTC_C_getInterruptStatus                                          \
        ROM_RTC_C_getInterruptStatus
#else
#define MAP_RTC_C_getInterruptStatus                                          \
        RTC_C_getInterruptStatus
#endif
#ifdef ROM_RTC_C_getEnabledInterruptStatus
#define MAP_RTC_C_getEnabledInterruptStatus                                   \
        ROM_RTC_C_getEnabledInterruptStatus
#else
#define MAP_RTC_C_getEnabledInterruptStatus                                   \
        RTC_C_getEnabledInterruptStatus
#endif
#ifdef ROM_RTC_C_clearInterruptFlag
#define MAP_RTC_C_clearInterruptFlag                                          \
        ROM_RTC_C_clearInterruptFlag
#else
#define MAP_RTC_C_clearInterruptFlag                                          \
        RTC_C_clearInterruptFlag
#endif
#ifdef ROM_RTC_C_registerInterrupt
#define MAP_RTC_C_registerInterrupt                                           \
        ROM_RTC_C_registerInterrupt
#else
#define MAP_RTC_C_registerInterrupt                                           \
        RTC_C_registerInterrupt
#endif
#ifdef ROM_RTC_C_unregisterInterrupt
#define MAP_RTC_C_unregisterInterrupt                                         \
        ROM_RTC_C_unregisterInterrupt
#else
#define MAP_RTC_C_unregisterInterrupt                                         \
        RTC_C_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the SPI API.
//
//*****************************************************************************
#ifdef ROM_SPI_initMaster
#define MAP_SPI_initMaster                                                    \
        ROM_SPI_initMaster
#else
#define MAP_SPI_initMaster                                                    \
        SPI_initMaster
#endif
#ifdef ROM_SPI_selectFourPinFunctionality
#define MAP_SPI_selectFourPinFunctionality                                    \
        ROM_SPI_selectFourPinFunctionality
#else
#define MAP_SPI_selectFourPinFunctionality                                    \
        SPI_selectFourPinFunctionality
#endif
#ifdef ROM_SPI_changeMasterClock
#define MAP_SPI_changeMasterClock                                             \
        ROM_SPI_changeMasterClock
#else
#define MAP_SPI_changeMasterClock                                             \
        SPI_changeMasterClock
#endif
#ifdef ROM_SPI_initSlave
#define MAP_SPI_initSlave                                                     \
        ROM_SPI_initSlave
#else
#define MAP_SPI_initSlave                                                     \
        SPI_initSlave
#endif
#ifdef ROM_SPI_changeClockPhasePolarity
#define MAP_SPI_changeClockPhasePolarity                                      \
        ROM_SPI_changeClockPhasePolarity
#else
#define MAP_SPI_changeClockPhasePolarity                                      \
        SPI_changeClockPhasePolarity
#endif
#ifdef ROM_SPI_transmitData
#define MAP_SPI_transmitData                                                  \
        ROM_SPI_transmitData
#else
#define MAP_SPI_transmitData                                                  \
        SPI_transmitData
#endif
#ifdef ROM_SPI_receiveData
#define MAP_SPI_receiveData                                                   \
        ROM_SPI_receiveData
#else
#define MAP_SPI_receiveData                                                   \
        SPI_receiveData
#endif
#ifdef ROM_SPI_enableModule
#define MAP_SPI_enableModule                                                  \
        ROM_SPI_enableModule
#else
#define MAP_SPI_enableModule                                                  \
        SPI_enableModule
#endif
#ifdef ROM_SPI_disableModule
#define MAP_SPI_disableModule                                                 \
        ROM_SPI_disableModule
#else
#define MAP_SPI_disableModule                                                 \
        SPI_disableModule
#endif
#ifdef ROM_SPI_getReceiveBufferAddressForDMA
#define MAP_SPI_getReceiveBufferAddressForDMA                                 \
        ROM_SPI_getReceiveBufferAddressForDMA
#else
#define MAP_SPI_getReceiveBufferAddressForDMA                                 \
        SPI_getReceiveBufferAddressForDMA
#endif
#ifdef ROM_SPI_getTransmitBufferAddressForDMA
#define MAP_SPI_getTransmitBufferAddressForDMA                                \
        ROM_SPI_getTransmitBufferAddressForDMA
#else
#define MAP_SPI_getTransmitBufferAddressForDMA                                \
        SPI_getTransmitBufferAddressForDMA
#endif
#ifdef ROM_SPI_isBusy
#define MAP_SPI_isBusy                                                        \
        ROM_SPI_isBusy
#else
#define MAP_SPI_isBusy                                                        \
        SPI_isBusy
#endif
#ifdef ROM_SPI_enableInterrupt
#define MAP_SPI_enableInterrupt                                               \
        ROM_SPI_enableInterrupt
#else
#define MAP_SPI_enableInterrupt                                               \
        SPI_enableInterrupt
#endif
#ifdef ROM_SPI_disableInterrupt
#define MAP_SPI_disableInterrupt                                              \
        ROM_SPI_disableInterrupt
#else
#define MAP_SPI_disableInterrupt                                              \
        SPI_disableInterrupt
#endif
#ifdef ROM_SPI_getInterruptStatus
#define MAP_SPI_getInterruptStatus                                            \
        ROM_SPI_getInterruptStatus
#else
#define MAP_SPI_getInterruptStatus                                            \
        SPI_getInterruptStatus
#endif
#ifdef ROM_SPI_getEnabledInterruptStatus
#define MAP_SPI_getEnabledInterruptStatus                                     \
        ROM_SPI_getEnabledInterruptStatus
#else
#define MAP_SPI_getEnabledInterruptStatus                                     \
        SPI_getEnabledInterruptStatus
#endif
#ifdef ROM_SPI_clearInterruptFlag
#define MAP_SPI_clearInterruptFlag                                            \
        ROM_SPI_clearInterruptFlag
#else
#define MAP_SPI_clearInterruptFlag                                            \
        SPI_clearInterruptFlag
#endif
#ifdef ROM_SPI_registerInterrupt
#define MAP_SPI_registerInterrupt                                             \
        ROM_SPI_registerInterrupt
#else
#define MAP_SPI_registerInterrupt                                             \
        SPI_registerInterrupt
#endif
#ifdef ROM_SPI_unregisterInterrupt
#define MAP_SPI_unregisterInterrupt                                           \
        ROM_SPI_unregisterInterrupt
#else
#define MAP_SPI_unregisterInterrupt                                           \
        SPI_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the SysCtl API.
//
//*****************************************************************************
#ifdef ROM_SysCtl_getSRAMSize
#define MAP_SysCtl_getSRAMSize                                                \
        ROM_SysCtl_getSRAMSize
#else
#define MAP_SysCtl_getSRAMSize                                                \
        SysCtl_getSRAMSize
#endif
#ifdef ROM_SysCtl_getFlashSize
#define MAP_SysCtl_getFlashSize                                               \
        ROM_SysCtl_getFlashSize
#else
#define MAP_SysCtl_getFlashSize                                               \
        SysCtl_getFlashSize
#endif
#ifdef ROM_SysCtl_rebootDevice
#define MAP_SysCtl_rebootDevice                                               \
        ROM_SysCtl_rebootDevice
#else
#define MAP_SysCtl_rebootDevice                                               \
        SysCtl_rebootDevice
#endif
#ifdef ROM_SysCtl_enableSRAMBank
#define MAP_SysCtl_enableSRAMBank                                             \
        ROM_SysCtl_enableSRAMBank
#else
#define MAP_SysCtl_enableSRAMBank                                             \
        SysCtl_enableSRAMBank
#endif
#ifdef ROM_SysCtl_disableSRAMBank
#define MAP_SysCtl_disableSRAMBank                                            \
        ROM_SysCtl_disableSRAMBank
#else
#define MAP_SysCtl_disableSRAMBank                                            \
        SysCtl_disableSRAMBank
#endif
#ifdef ROM_SysCtl_enableSRAMBankRetention
#define MAP_SysCtl_enableSRAMBankRetention                                    \
        ROM_SysCtl_enableSRAMBankRetention
#else
#define MAP_SysCtl_enableSRAMBankRetention                                    \
        SysCtl_enableSRAMBankRetention
#endif
#ifdef ROM_SysCtl_disableSRAMBankRetention
#define MAP_SysCtl_disableSRAMBankRetention                                   \
        ROM_SysCtl_disableSRAMBankRetention
#else
#define MAP_SysCtl_disableSRAMBankRetention                                   \
        SysCtl_disableSRAMBankRetention
#endif
#ifdef ROM_SysCtl_enablePeripheralAtCPUHalt
#define MAP_SysCtl_enablePeripheralAtCPUHalt                                  \
        ROM_SysCtl_enablePeripheralAtCPUHalt
#else
#define MAP_SysCtl_enablePeripheralAtCPUHalt                                  \
        SysCtl_enablePeripheralAtCPUHalt
#endif
#ifdef ROM_SysCtl_disablePeripheralAtCPUHalt
#define MAP_SysCtl_disablePeripheralAtCPUHalt                                 \
        ROM_SysCtl_disablePeripheralAtCPUHalt
#else
#define MAP_SysCtl_disablePeripheralAtCPUHalt                                 \
        SysCtl_disablePeripheralAtCPUHalt
#endif
#ifdef ROM_SysCtl_setWDTTimeoutResetType
#define MAP_SysCtl_setWDTTimeoutResetType                                     \
        ROM_SysCtl_setWDTTimeoutResetType
#else
#define MAP_SysCtl_setWDTTimeoutResetType                                     \
        SysCtl_setWDTTimeoutResetType
#endif
#ifdef ROM_SysCtl_setWDTPasswordViolationResetType
#define MAP_SysCtl_setWDTPasswordViolationResetType                           \
        ROM_SysCtl_setWDTPasswordViolationResetType
#else
#define MAP_SysCtl_setWDTPasswordViolationResetType                           \
        SysCtl_setWDTPasswordViolationResetType
#endif
#ifdef ROM_SysCtl_disableNMISource
#define MAP_SysCtl_disableNMISource                                           \
        ROM_SysCtl_disableNMISource
#else
#define MAP_SysCtl_disableNMISource                                           \
        SysCtl_disableNMISource
#endif
#ifdef ROM_SysCtl_enableNMISource
#define MAP_SysCtl_enableNMISource                                            \
        ROM_SysCtl_enableNMISource
#else
#define MAP_SysCtl_enableNMISource                                            \
        SysCtl_enableNMISource
#endif
#ifdef ROM_SysCtl_getNMISourceStatus
#define MAP_SysCtl_getNMISourceStatus                                         \
        ROM_SysCtl_getNMISourceStatus
#else
#define MAP_SysCtl_getNMISourceStatus                                         \
        SysCtl_getNMISourceStatus
#endif
#ifdef ROM_SysCtl_getTempCalibrationConstant
#define MAP_SysCtl_getTempCalibrationConstant                                 \
        ROM_SysCtl_getTempCalibrationConstant
#else
#define MAP_SysCtl_getTempCalibrationConstant                                 \
        SysCtl_getTempCalibrationConstant
#endif
#ifdef ROM_SysCtl_enableGlitchFilter
#define MAP_SysCtl_enableGlitchFilter                                         \
        ROM_SysCtl_enableGlitchFilter
#else
#define MAP_SysCtl_enableGlitchFilter                                         \
        SysCtl_enableGlitchFilter
#endif
#ifdef ROM_SysCtl_disableGlitchFilter
#define MAP_SysCtl_disableGlitchFilter                                        \
        ROM_SysCtl_disableGlitchFilter
#else
#define MAP_SysCtl_disableGlitchFilter                                        \
        SysCtl_disableGlitchFilter
#endif
#ifdef ROM_SysCtl_getTLVInfo
#define MAP_SysCtl_getTLVInfo                                                 \
        ROM_SysCtl_getTLVInfo
#else
#define MAP_SysCtl_getTLVInfo                                                 \
        SysCtl_getTLVInfo
#endif

//*****************************************************************************
//
// Macros for the SysTick API.
//
//*****************************************************************************
#ifdef ROM_SysTick_enableModule
#define MAP_SysTick_enableModule                                              \
        ROM_SysTick_enableModule
#else
#define MAP_SysTick_enableModule                                              \
        SysTick_enableModule
#endif
#ifdef ROM_SysTick_disableModule
#define MAP_SysTick_disableModule                                             \
        ROM_SysTick_disableModule
#else
#define MAP_SysTick_disableModule                                             \
        SysTick_disableModule
#endif
#ifdef ROM_SysTick_enableInterrupt
#define MAP_SysTick_enableInterrupt                                           \
        ROM_SysTick_enableInterrupt
#else
#define MAP_SysTick_enableInterrupt                                           \
        SysTick_enableInterrupt
#endif
#ifdef ROM_SysTick_disableInterrupt
#define MAP_SysTick_disableInterrupt                                          \
        ROM_SysTick_disableInterrupt
#else
#define MAP_SysTick_disableInterrupt                                          \
        SysTick_disableInterrupt
#endif
#ifdef ROM_SysTick_setPeriod
#define MAP_SysTick_setPeriod                                                 \
        ROM_SysTick_setPeriod
#else
#define MAP_SysTick_setPeriod                                                 \
        SysTick_setPeriod
#endif
#ifdef ROM_SysTick_getPeriod
#define MAP_SysTick_getPeriod                                                 \
        ROM_SysTick_getPeriod
#else
#define MAP_SysTick_getPeriod                                                 \
        SysTick_getPeriod
#endif
#ifdef ROM_SysTick_getValue
#define MAP_SysTick_getValue                                                  \
        ROM_SysTick_getValue
#else
#define MAP_SysTick_getValue                                                  \
        SysTick_getValue
#endif
#ifdef ROM_SysTick_registerInterrupt
#define MAP_SysTick_registerInterrupt                                         \
        ROM_SysTick_registerInterrupt
#else
#define MAP_SysTick_registerInterrupt                                         \
        SysTick_registerInterrupt
#endif
#ifdef ROM_SysTick_unregisterInterrupt
#define MAP_SysTick_unregisterInterrupt                                       \
        ROM_SysTick_unregisterInterrupt
#else
#define MAP_SysTick_unregisterInterrupt                                       \
        SysTick_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the Timer_A API.
//
//*****************************************************************************
#ifdef ROM_Timer_A_startCounter
#define MAP_Timer_A_startCounter                                              \
        ROM_Timer_A_startCounter
#else
#define MAP_Timer_A_startCounter                                              \
        Timer_A_startCounter
#endif
#ifdef ROM_Timer_A_configureContinuousMode
#define MAP_Timer_A_configureContinuousMode                                   \
        ROM_Timer_A_configureContinuousMode
#else
#define MAP_Timer_A_configureContinuousMode                                   \
        Timer_A_configureContinuousMode
#endif
#ifdef ROM_Timer_A_configureUpMode
#define MAP_Timer_A_configureUpMode                                           \
        ROM_Timer_A_configureUpMode
#else
#define MAP_Timer_A_configureUpMode                                           \
        Timer_A_configureUpMode
#endif
#ifdef ROM_Timer_A_configureUpDownMode
#define MAP_Timer_A_configureUpDownMode                                       \
        ROM_Timer_A_configureUpDownMode
#else
#define MAP_Timer_A_configureUpDownMode                                       \
        Timer_A_configureUpDownMode
#endif
#ifdef ROM_Timer_A_initCapture
#define MAP_Timer_A_initCapture                                               \
        ROM_Timer_A_initCapture
#else
#define MAP_Timer_A_initCapture                                               \
        Timer_A_initCapture
#endif
#ifdef ROM_Timer_A_initCompare
#define MAP_Timer_A_initCompare                                               \
        ROM_Timer_A_initCompare
#else
#define MAP_Timer_A_initCompare                                               \
        Timer_A_initCompare
#endif
#ifdef ROM_Timer_A_clearTimer
#define MAP_Timer_A_clearTimer                                                \
        ROM_Timer_A_clearTimer
#else
#define MAP_Timer_A_clearTimer                                                \
        Timer_A_clearTimer
#endif
#ifdef ROM_Timer_A_getSynchronizedCaptureCompareInput
#define MAP_Timer_A_getSynchronizedCaptureCompareInput                        \
        ROM_Timer_A_getSynchronizedCaptureCompareInput
#else
#define MAP_Timer_A_getSynchronizedCaptureCompareInput                        \
        Timer_A_getSynchronizedCaptureCompareInput
#endif
#ifdef ROM_Timer_A_getOutputForOutputModeOutBitValue
#define MAP_Timer_A_getOutputForOutputModeOutBitValue                         \
        ROM_Timer_A_getOutputForOutputModeOutBitValue
#else
#define MAP_Timer_A_getOutputForOutputModeOutBitValue                         \
        Timer_A_getOutputForOutputModeOutBitValue
#endif
#ifdef ROM_Timer_A_getCaptureCompareCount
#define MAP_Timer_A_getCaptureCompareCount                                    \
        ROM_Timer_A_getCaptureCompareCount
#else
#define MAP_Timer_A_getCaptureCompareCount                                    \
        Timer_A_getCaptureCompareCount
#endif
#ifdef ROM_Timer_A_setOutputForOutputModeOutBitValue
#define MAP_Timer_A_setOutputForOutputModeOutBitValue                         \
        ROM_Timer_A_setOutputForOutputModeOutBitValue
#else
#define MAP_Timer_A_setOutputForOutputModeOutBitValue                         \
        Timer_A_setOutputForOutputModeOutBitValue
#endif
#ifdef ROM_Timer_A_generatePWM
#define MAP_Timer_A_generatePWM                                               \
        ROM_Timer_A_generatePWM
#else
#define MAP_Timer_A_generatePWM                                               \
        Timer_A_generatePWM
#endif
#ifdef ROM_Timer_A_stopTimer
#define MAP_Timer_A_stopTimer                                                 \
        ROM_Timer_A_stopTimer
#else
#define MAP_Timer_A_stopTimer                                                 \
        Timer_A_stopTimer
#endif
#ifdef ROM_Timer_A_setCompareValue
#define MAP_Timer_A_setCompareValue                                           \
        ROM_Timer_A_setCompareValue
#else
#define MAP_Timer_A_setCompareValue                                           \
        Timer_A_setCompareValue
#endif
#ifdef ROM_Timer_A_clearInterruptFlag
#define MAP_Timer_A_clearInterruptFlag                                        \
        ROM_Timer_A_clearInterruptFlag
#else
#define MAP_Timer_A_clearInterruptFlag                                        \
        Timer_A_clearInterruptFlag
#endif
#ifdef ROM_Timer_A_clearCaptureCompareInterrupt
#define MAP_Timer_A_clearCaptureCompareInterrupt                              \
        ROM_Timer_A_clearCaptureCompareInterrupt
#else
#define MAP_Timer_A_clearCaptureCompareInterrupt                              \
        Timer_A_clearCaptureCompareInterrupt
#endif
#ifdef ROM_Timer_A_enableInterrupt
#define MAP_Timer_A_enableInterrupt                                           \
        ROM_Timer_A_enableInterrupt
#else
#define MAP_Timer_A_enableInterrupt                                           \
        Timer_A_enableInterrupt
#endif
#ifdef ROM_Timer_A_disableInterrupt
#define MAP_Timer_A_disableInterrupt                                          \
        ROM_Timer_A_disableInterrupt
#else
#define MAP_Timer_A_disableInterrupt                                          \
        Timer_A_disableInterrupt
#endif
#ifdef ROM_Timer_A_getInterruptStatus
#define MAP_Timer_A_getInterruptStatus                                        \
        ROM_Timer_A_getInterruptStatus
#else
#define MAP_Timer_A_getInterruptStatus                                        \
        Timer_A_getInterruptStatus
#endif
#ifdef ROM_Timer_A_getEnabledInterruptStatus
#define MAP_Timer_A_getEnabledInterruptStatus                                 \
        ROM_Timer_A_getEnabledInterruptStatus
#else
#define MAP_Timer_A_getEnabledInterruptStatus                                 \
        Timer_A_getEnabledInterruptStatus
#endif
#ifdef ROM_Timer_A_enableCaptureCompareInterrupt
#define MAP_Timer_A_enableCaptureCompareInterrupt                             \
        ROM_Timer_A_enableCaptureCompareInterrupt
#else
#define MAP_Timer_A_enableCaptureCompareInterrupt                             \
        Timer_A_enableCaptureCompareInterrupt
#endif
#ifdef ROM_Timer_A_disableCaptureCompareInterrupt
#define MAP_Timer_A_disableCaptureCompareInterrupt                            \
        ROM_Timer_A_disableCaptureCompareInterrupt
#else
#define MAP_Timer_A_disableCaptureCompareInterrupt                            \
        Timer_A_disableCaptureCompareInterrupt
#endif
#ifdef ROM_Timer_A_getCaptureCompareInterruptStatus
#define MAP_Timer_A_getCaptureCompareInterruptStatus                          \
        ROM_Timer_A_getCaptureCompareInterruptStatus
#else
#define MAP_Timer_A_getCaptureCompareInterruptStatus                          \
        Timer_A_getCaptureCompareInterruptStatus
#endif
#ifdef ROM_Timer_A_getCaptureCompareEnabledInterruptStatus
#define MAP_Timer_A_getCaptureCompareEnabledInterruptStatus                   \
        ROM_Timer_A_getCaptureCompareEnabledInterruptStatus
#else
#define MAP_Timer_A_getCaptureCompareEnabledInterruptStatus                   \
        Timer_A_getCaptureCompareEnabledInterruptStatus
#endif
#ifdef ROM_Timer_A_registerInterrupt
#define MAP_Timer_A_registerInterrupt                                         \
        ROM_Timer_A_registerInterrupt
#else
#define MAP_Timer_A_registerInterrupt                                         \
        Timer_A_registerInterrupt
#endif
#ifdef ROM_Timer_A_unregisterInterrupt
#define MAP_Timer_A_unregisterInterrupt                                       \
        ROM_Timer_A_unregisterInterrupt
#else
#define MAP_Timer_A_unregisterInterrupt                                       \
        Timer_A_unregisterInterrupt
#endif
#ifdef ROM_Timer_A_getCounterValue
#define MAP_Timer_A_getCounterValue                                           \
        ROM_Timer_A_getCounterValue
#else
#define MAP_Timer_A_getCounterValue                                           \
        Timer_A_getCounterValue
#endif

//*****************************************************************************
//
// Macros for the Timer32 API.
//
//*****************************************************************************
#ifdef ROM_Timer32_initModule
#define MAP_Timer32_initModule                                                \
        ROM_Timer32_initModule
#else
#define MAP_Timer32_initModule                                                \
        Timer32_initModule
#endif
#ifdef ROM_Timer32_setCount
#define MAP_Timer32_setCount                                                  \
        ROM_Timer32_setCount
#else
#define MAP_Timer32_setCount                                                  \
        Timer32_setCount
#endif
#ifdef ROM_Timer32_setCountInBackground
#define MAP_Timer32_setCountInBackground                                      \
        ROM_Timer32_setCountInBackground
#else
#define MAP_Timer32_setCountInBackground                                      \
        Timer32_setCountInBackground
#endif
#ifdef ROM_Timer32_getValue
#define MAP_Timer32_getValue                                                  \
        ROM_Timer32_getValue
#else
#define MAP_Timer32_getValue                                                  \
        Timer32_getValue
#endif
#ifdef ROM_Timer32_startTimer
#define MAP_Timer32_startTimer                                                \
        ROM_Timer32_startTimer
#else
#define MAP_Timer32_startTimer                                                \
        Timer32_startTimer
#endif
#ifdef ROM_Timer32_haltTimer
#define MAP_Timer32_haltTimer                                                 \
        ROM_Timer32_haltTimer
#else
#define MAP_Timer32_haltTimer                                                 \
        Timer32_haltTimer
#endif
#ifdef ROM_Timer32_enableInterrupt
#define MAP_Timer32_enableInterrupt                                           \
        ROM_Timer32_enableInterrupt
#else
#define MAP_Timer32_enableInterrupt                                           \
        Timer32_enableInterrupt
#endif
#ifdef ROM_Timer32_disableInterrupt
#define MAP_Timer32_disableInterrupt                                          \
        ROM_Timer32_disableInterrupt
#else
#define MAP_Timer32_disableInterrupt                                          \
        Timer32_disableInterrupt
#endif
#ifdef ROM_Timer32_clearInterruptFlag
#define MAP_Timer32_clearInterruptFlag                                        \
        ROM_Timer32_clearInterruptFlag
#else
#define MAP_Timer32_clearInterruptFlag                                        \
        Timer32_clearInterruptFlag
#endif
#ifdef ROM_Timer32_getInterruptStatus
#define MAP_Timer32_getInterruptStatus                                        \
        ROM_Timer32_getInterruptStatus
#else
#define MAP_Timer32_getInterruptStatus                                        \
        Timer32_getInterruptStatus
#endif
#ifdef ROM_Timer32_registerInterrupt
#define MAP_Timer32_registerInterrupt                                         \
        ROM_Timer32_registerInterrupt
#else
#define MAP_Timer32_registerInterrupt                                         \
        Timer32_registerInterrupt
#endif
#ifdef ROM_Timer32_unregisterInterrupt
#define MAP_Timer32_unregisterInterrupt                                       \
        ROM_Timer32_unregisterInterrupt
#else
#define MAP_Timer32_unregisterInterrupt                                       \
        Timer32_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the UART API.
//
//*****************************************************************************
#ifdef ROM_UART_initModule
#define MAP_UART_initModule                                                   \
        ROM_UART_initModule
#else
#define MAP_UART_initModule                                                   \
        UART_initModule
#endif
#ifdef ROM_UART_transmitData
#define MAP_UART_transmitData                                                 \
        ROM_UART_transmitData
#else
#define MAP_UART_transmitData                                                 \
        UART_transmitData
#endif
#ifdef ROM_UART_enableModule
#define MAP_UART_enableModule                                                 \
        ROM_UART_enableModule
#else
#define MAP_UART_enableModule                                                 \
        UART_enableModule
#endif
#ifdef ROM_UART_disableModule
#define MAP_UART_disableModule                                                \
        ROM_UART_disableModule
#else
#define MAP_UART_disableModule                                                \
        UART_disableModule
#endif
#ifdef ROM_UART_queryStatusFlags
#define MAP_UART_queryStatusFlags                                             \
        ROM_UART_queryStatusFlags
#else
#define MAP_UART_queryStatusFlags                                             \
        UART_queryStatusFlags
#endif
#ifdef ROM_UART_setDormant
#define MAP_UART_setDormant                                                   \
        ROM_UART_setDormant
#else
#define MAP_UART_setDormant                                                   \
        UART_setDormant
#endif
#ifdef ROM_UART_resetDormant
#define MAP_UART_resetDormant                                                 \
        ROM_UART_resetDormant
#else
#define MAP_UART_resetDormant                                                 \
        UART_resetDormant
#endif
#ifdef ROM_UART_transmitAddress
#define MAP_UART_transmitAddress                                              \
        ROM_UART_transmitAddress
#else
#define MAP_UART_transmitAddress                                              \
        UART_transmitAddress
#endif
#ifdef ROM_UART_transmitBreak
#define MAP_UART_transmitBreak                                                \
        ROM_UART_transmitBreak
#else
#define MAP_UART_transmitBreak                                                \
        UART_transmitBreak
#endif
#ifdef ROM_UART_getReceiveBufferAddressForDMA
#define MAP_UART_getReceiveBufferAddressForDMA                                \
        ROM_UART_getReceiveBufferAddressForDMA
#else
#define MAP_UART_getReceiveBufferAddressForDMA                                \
        UART_getReceiveBufferAddressForDMA
#endif
#ifdef ROM_UART_getTransmitBufferAddressForDMA
#define MAP_UART_getTransmitBufferAddressForDMA                               \
        ROM_UART_getTransmitBufferAddressForDMA
#else
#define MAP_UART_getTransmitBufferAddressForDMA                               \
        UART_getTransmitBufferAddressForDMA
#endif
#ifdef ROM_UART_selectDeglitchTime
#define MAP_UART_selectDeglitchTime                                           \
        ROM_UART_selectDeglitchTime
#else
#define MAP_UART_selectDeglitchTime                                           \
        UART_selectDeglitchTime
#endif
#ifdef ROM_UART_enableInterrupt
#define MAP_UART_enableInterrupt                                              \
        ROM_UART_enableInterrupt
#else
#define MAP_UART_enableInterrupt                                              \
        UART_enableInterrupt
#endif
#ifdef ROM_UART_disableInterrupt
#define MAP_UART_disableInterrupt                                             \
        ROM_UART_disableInterrupt
#else
#define MAP_UART_disableInterrupt                                             \
        UART_disableInterrupt
#endif
#ifdef ROM_UART_getInterruptStatus
#define MAP_UART_getInterruptStatus                                           \
        ROM_UART_getInterruptStatus
#else
#define MAP_UART_getInterruptStatus                                           \
        UART_getInterruptStatus
#endif
#ifdef ROM_UART_clearInterruptFlag
#define MAP_UART_clearInterruptFlag                                           \
        ROM_UART_clearInterruptFlag
#else
#define MAP_UART_clearInterruptFlag                                           \
        UART_clearInterruptFlag
#endif
#ifdef ROM_UART_receiveData
#define MAP_UART_receiveData                                                  \
        ROM_UART_receiveData
#else
#define MAP_UART_receiveData                                                  \
        UART_receiveData
#endif
#ifdef ROM_UART_getEnabledInterruptStatus
#define MAP_UART_getEnabledInterruptStatus                                    \
        ROM_UART_getEnabledInterruptStatus
#else
#define MAP_UART_getEnabledInterruptStatus                                    \
        UART_getEnabledInterruptStatus
#endif
#ifdef ROM_UART_registerInterrupt
#define MAP_UART_registerInterrupt                                            \
        ROM_UART_registerInterrupt
#else
#define MAP_UART_registerInterrupt                                            \
        UART_registerInterrupt
#endif
#ifdef ROM_UART_unregisterInterrupt
#define MAP_UART_unregisterInterrupt                                          \
        ROM_UART_unregisterInterrupt
#else
#define MAP_UART_unregisterInterrupt                                          \
        UART_unregisterInterrupt
#endif

//*****************************************************************************
//
// Macros for the WDT API.
//
//*****************************************************************************
#ifdef ROM_WDT_A_holdTimer
#define MAP_WDT_A_holdTimer                                                   \
        ROM_WDT_A_holdTimer
#else
#define MAP_WDT_A_holdTimer                                                   \
        WDT_A_holdTimer
#endif
#ifdef ROM_WDT_A_startTimer
#define MAP_WDT_A_startTimer                                                  \
        ROM_WDT_A_startTimer
#else
#define MAP_WDT_A_startTimer                                                  \
        WDT_A_startTimer
#endif
#ifdef ROM_WDT_A_clearTimer
#define MAP_WDT_A_clearTimer                                                  \
        ROM_WDT_A_clearTimer
#else
#define MAP_WDT_A_clearTimer                                                  \
        WDT_A_clearTimer
#endif
#ifdef ROM_WDT_A_initWatchdogTimer
#define MAP_WDT_A_initWatchdogTimer                                           \
        ROM_WDT_A_initWatchdogTimer
#else
#define MAP_WDT_A_initWatchdogTimer                                           \
        WDT_A_initWatchdogTimer
#endif
#ifdef ROM_WDT_A_initIntervalTimer
#define MAP_WDT_A_initIntervalTimer                                           \
        ROM_WDT_A_initIntervalTimer
#else
#define MAP_WDT_A_initIntervalTimer                                           \
        WDT_A_initIntervalTimer
#endif
#ifdef ROM_WDT_A_registerInterrupt
#define MAP_WDT_A_registerInterrupt                                           \
        ROM_WDT_A_registerInterrupt
#else
#define MAP_WDT_A_registerInterrupt                                           \
        WDT_A_registerInterrupt
#endif
#ifdef ROM_WDT_A_unregisterInterrupt
#define MAP_WDT_A_unregisterInterrupt                                         \
        ROM_WDT_A_unregisterInterrupt
#else
#define MAP_WDT_A_unregisterInterrupt                                         \
        WDT_A_unregisterInterrupt
#endif
#ifdef ROM_WDT_A_setPasswordViolationReset
#define MAP_WDT_A_setPasswordViolationReset                                   \
        ROM_WDT_A_setPasswordViolationReset
#else
#define MAP_WDT_A_setPasswordViolationReset                                   \
        WDT_A_setPasswordViolationReset
#endif
#ifdef ROM_WDT_A_setTimeoutReset
#define MAP_WDT_A_setTimeoutReset                                             \
        ROM_WDT_A_setTimeoutReset
#else
#define MAP_WDT_A_setTimeoutReset                                             \
        WDT_A_setTimeoutReset
#endif

//*****************************************************************************
//
// Macros for the SysCtl_A API.
//
//*****************************************************************************
#ifdef ROM_SysCtl_A_getSRAMSize
#define MAP_SysCtl_A_getSRAMSize                                              \
        ROM_SysCtl_A_getSRAMSize
#else
#define MAP_SysCtl_A_getSRAMSize                                              \
        SysCtl_A_getSRAMSize
#endif
#ifdef ROM_SysCtl_A_getFlashSize
#define MAP_SysCtl_A_getFlashSize                                             \
        ROM_SysCtl_A_getFlashSize
#else
#define MAP_SysCtl_A_getFlashSize                                             \
        SysCtl_A_getFlashSize
#endif
#ifdef ROM_SysCtl_A_rebootDevice
#define MAP_SysCtl_A_rebootDevice                                             \
        ROM_SysCtl_A_rebootDevice
#else
#define MAP_SysCtl_A_rebootDevice                                             \
        SysCtl_A_rebootDevice
#endif
#ifdef ROM_SysCtl_A_enableSRAM
#define MAP_SysCtl_A_enableSRAM                                               \
        ROM_SysCtl_A_enableSRAM
#else
#define MAP_SysCtl_A_enableSRAM                                               \
        SysCtl_A_enableSRAM
#endif
#ifdef ROM_SysCtl_A_disableSRAM
#define MAP_SysCtl_A_disableSRAM                                              \
        ROM_SysCtl_A_disableSRAM
#else
#define MAP_SysCtl_A_disableSRAM                                              \
        SysCtl_A_disableSRAM
#endif
#ifdef ROM_SysCtl_A_enableSRAMRetention
#define MAP_SysCtl_A_enableSRAMRetention                                      \
        ROM_SysCtl_A_enableSRAMRetention
#else
#define MAP_SysCtl_A_enableSRAMRetention                                      \
        SysCtl_A_enableSRAMRetention
#endif
#ifdef ROM_SysCtl_A_disableSRAMRetention
#define MAP_SysCtl_A_disableSRAMRetention                                     \
        ROM_SysCtl_A_disableSRAMRetention
#else
#define MAP_SysCtl_A_disableSRAMRetention                                     \
        SysCtl_A_disableSRAMRetention
#endif
#ifdef ROM_SysCtl_A_enablePeripheralAtCPUHalt
#define MAP_SysCtl_A_enablePeripheralAtCPUHalt                                \
        ROM_SysCtl_A_enablePeripheralAtCPUHalt
#else
#define MAP_SysCtl_A_enablePeripheralAtCPUHalt                                \
        SysCtl_A_enablePeripheralAtCPUHalt
#endif
#ifdef ROM_SysCtl_A_disablePeripheralAtCPUHalt
#define MAP_SysCtl_A_disablePeripheralAtCPUHalt                               \
        ROM_SysCtl_A_disablePeripheralAtCPUHalt
#else
#define MAP_SysCtl_A_disablePeripheralAtCPUHalt                               \
        SysCtl_A_disablePeripheralAtCPUHalt
#endif
#ifdef ROM_SysCtl_A_setWDTTimeoutResetType
#define MAP_SysCtl_A_setWDTTimeoutResetType                                   \
        ROM_SysCtl_A_setWDTTimeoutResetType
#else
#define MAP_SysCtl_A_setWDTTimeoutResetType                                   \
        SysCtl_A_setWDTTimeoutResetType
#endif
#ifdef ROM_SysCtl_A_setWDTPasswordViolationResetType
#define MAP_SysCtl_A_setWDTPasswordViolationResetType                         \
        ROM_SysCtl_A_setWDTPasswordViolationResetType
#else
#define MAP_SysCtl_A_setWDTPasswordViolationResetType                         \
        SysCtl_A_setWDTPasswordViolationResetType
#endif
#ifdef ROM_SysCtl_A_disableNMISource
#define MAP_SysCtl_A_disableNMISource                                         \
        ROM_SysCtl_A_disableNMISource
#else
#define MAP_SysCtl_A_disableNMISource                                         \
        SysCtl_A_disableNMISource
#endif
#ifdef ROM_SysCtl_A_enableNMISource
#define MAP_SysCtl_A_enableNMISource                                          \
        ROM_SysCtl_A_enableNMISource
#else
#define MAP_SysCtl_A_enableNMISource                                          \
        SysCtl_A_enableNMISource
#endif
#ifdef ROM_SysCtl_A_getNMISourceStatus
#define MAP_SysCtl_A_getNMISourceStatus                                       \
        ROM_SysCtl_A_getNMISourceStatus
#else
#define MAP_SysCtl_A_getNMISourceStatus                                       \
        SysCtl_A_getNMISourceStatus
#endif
#ifdef ROM_SysCtl_A_getTempCalibrationConstant
#define MAP_SysCtl_A_getTempCalibrationConstant                               \
        ROM_SysCtl_A_getTempCalibrationConstant
#else
#define MAP_SysCtl_A_getTempCalibrationConstant                               \
        SysCtl_A_getTempCalibrationConstant
#endif
#ifdef ROM_SysCtl_A_enableGlitchFilter
#define MAP_SysCtl_A_enableGlitchFilter                                       \
        ROM_SysCtl_A_enableGlitchFilter
#else
#define MAP_SysCtl_A_enableGlitchFilter                                       \
        SysCtl_A_enableGlitchFilter
#endif
#ifdef ROM_SysCtl_A_disableGlitchFilter
#define MAP_SysCtl_A_disableGlitchFilter                                      \
        ROM_SysCtl_A_disableGlitchFilter
#else
#define MAP_SysCtl_A_disableGlitchFilter                                      \
        SysCtl_A_disableGlitchFilter
#endif
#ifdef ROM_SysCtl_A_getTLVInfo
#define MAP_SysCtl_A_getTLVInfo                                               \
        ROM_SysCtl_A_getTLVInfo
#else
#define MAP_SysCtl_A_getTLVInfo                                               \
        SysCtl_A_getTLVInfo
#endif
#ifdef ROM_SysCtl_A_getInfoFlashSize
#define MAP_SysCtl_A_getInfoFlashSize                                         \
        ROM_SysCtl_A_getInfoFlashSize
#else
#define MAP_SysCtl_A_getInfoFlashSize                                         \
        SysCtl_A_getInfoFlashSize
#endif

//*****************************************************************************
//
// Macros for the Flash_A API.
//
//*****************************************************************************
#ifdef ROM_FlashCtl_A_enableReadParityCheck
#define MAP_FlashCtl_A_enableReadParityCheck                                  \
        ROM_FlashCtl_A_enableReadParityCheck
#else
#define MAP_FlashCtl_A_enableReadParityCheck                                  \
        FlashCtl_A_enableReadParityCheck
#endif
#ifdef ROM_FlashCtl_A_disableReadParityCheck
#define MAP_FlashCtl_A_disableReadParityCheck                                 \
        ROM_FlashCtl_A_disableReadParityCheck
#else
#define MAP_FlashCtl_A_disableReadParityCheck                                 \
        FlashCtl_A_disableReadParityCheck
#endif
#ifdef ROM_FlashCtl_A_enableReadBuffering
#define MAP_FlashCtl_A_enableReadBuffering                                    \
        ROM_FlashCtl_A_enableReadBuffering
#else
#define MAP_FlashCtl_A_enableReadBuffering                                    \
        FlashCtl_A_enableReadBuffering
#endif
#ifdef ROM_FlashCtl_A_disableReadBuffering
#define MAP_FlashCtl_A_disableReadBuffering                                   \
        ROM_FlashCtl_A_disableReadBuffering
#else
#define MAP_FlashCtl_A_disableReadBuffering                                   \
        FlashCtl_A_disableReadBuffering
#endif
#ifdef ROM_FlashCtl_A_unprotectMemory
#define MAP_FlashCtl_A_unprotectMemory                                        \
        ROM_FlashCtl_A_unprotectMemory
#else
#define MAP_FlashCtl_A_unprotectMemory                                        \
        FlashCtl_A_unprotectMemory
#endif
#ifdef ROM_FlashCtl_A_protectMemory
#define MAP_FlashCtl_A_protectMemory                                          \
        ROM_FlashCtl_A_protectMemory
#else
#define MAP_FlashCtl_A_protectMemory                                          \
        FlashCtl_A_protectMemory
#endif
#ifdef ROM_FlashCtl_A_isMemoryRangeProtected
#define MAP_FlashCtl_A_isMemoryRangeProtected                                 \
        ROM_FlashCtl_A_isMemoryRangeProtected
#else
#define MAP_FlashCtl_A_isMemoryRangeProtected                                 \
        FlashCtl_A_isMemoryRangeProtected
#endif
#ifdef ROM_FlashCtl_A_verifyMemory
#define MAP_FlashCtl_A_verifyMemory                                           \
        ROM_FlashCtl_A_verifyMemory
#else
#define MAP_FlashCtl_A_verifyMemory                                           \
        FlashCtl_A_verifyMemory
#endif
#ifdef ROM_FlashCtl_A_performMassErase
#define MAP_FlashCtl_A_performMassErase                                       \
        ROM_FlashCtl_A_performMassErase
#else
#define MAP_FlashCtl_A_performMassErase                                       \
        FlashCtl_A_performMassErase
#endif
#ifdef ROM_FlashCtl_A_eraseSector
#define MAP_FlashCtl_A_eraseSector                                            \
        ROM_FlashCtl_A_eraseSector
#else
#define MAP_FlashCtl_A_eraseSector                                            \
        FlashCtl_A_eraseSector
#endif
#ifdef ROM_FlashCtl_A_programMemory
#define MAP_FlashCtl_A_programMemory                                          \
        ROM_FlashCtl_A_programMemory
#else
#define MAP_FlashCtl_A_programMemory                                          \
        FlashCtl_A_programMemory
#endif
#ifdef ROM_FlashCtl_A_setProgramVerification
#define MAP_FlashCtl_A_setProgramVerification                                 \
        ROM_FlashCtl_A_setProgramVerification
#else
#define MAP_FlashCtl_A_setProgramVerification                                 \
        FlashCtl_A_setProgramVerification
#endif
#ifdef ROM_FlashCtl_A_clearProgramVerification
#define MAP_FlashCtl_A_clearProgramVerification                               \
        ROM_FlashCtl_A_clearProgramVerification
#else
#define MAP_FlashCtl_A_clearProgramVerification                               \
        FlashCtl_A_clearProgramVerification
#endif
#ifdef ROM_FlashCtl_A_enableWordProgramming
#define MAP_FlashCtl_A_enableWordProgramming                                  \
        ROM_FlashCtl_A_enableWordProgramming
#else
#define MAP_FlashCtl_A_enableWordProgramming                                  \
        FlashCtl_A_enableWordProgramming
#endif
#ifdef ROM_FlashCtl_A_disableWordProgramming
#define MAP_FlashCtl_A_disableWordProgramming                                 \
        ROM_FlashCtl_A_disableWordProgramming
#else
#define MAP_FlashCtl_A_disableWordProgramming                                 \
        FlashCtl_A_disableWordProgramming
#endif
#ifdef ROM_FlashCtl_A_isWordProgrammingEnabled
#define MAP_FlashCtl_A_isWordProgrammingEnabled                               \
        ROM_FlashCtl_A_isWordProgrammingEnabled
#else
#define MAP_FlashCtl_A_isWordProgrammingEnabled                               \
        FlashCtl_A_isWordProgrammingEnabled
#endif
#ifdef ROM_FlashCtl_A_enableInterrupt
#define MAP_FlashCtl_A_enableInterrupt                                        \
        ROM_FlashCtl_A_enableInterrupt
#else
#define MAP_FlashCtl_A_enableInterrupt                                        \
        FlashCtl_A_enableInterrupt
#endif
#ifdef ROM_FlashCtl_A_disableInterrupt
#define MAP_FlashCtl_A_disableInterrupt                                       \
        ROM_FlashCtl_A_disableInterrupt
#else
#define MAP_FlashCtl_A_disableInterrupt                                       \
        FlashCtl_A_disableInterrupt
#endif
#ifdef ROM_FlashCtl_A_getEnabledInterruptStatus
#define MAP_FlashCtl_A_getEnabledInterruptStatus                              \
        ROM_FlashCtl_A_getEnabledInterruptStatus
#else
#define MAP_FlashCtl_A_getEnabledInterruptStatus                              \
        FlashCtl_A_getEnabledInterruptStatus
#endif
#ifdef ROM_FlashCtl_A_getInterruptStatus
#define MAP_FlashCtl_A_getInterruptStatus                                     \
        ROM_FlashCtl_A_getInterruptStatus
#else
#define MAP_FlashCtl_A_getInterruptStatus                                     \
        FlashCtl_A_getInterruptStatus
#endif
#ifdef ROM_FlashCtl_A_clearInterruptFlag
#define MAP_FlashCtl_A_clearInterruptFlag                                     \
        ROM_FlashCtl_A_clearInterruptFlag
#else
#define MAP_FlashCtl_A_clearInterruptFlag                                     \
        FlashCtl_A_clearInterruptFlag
#endif
#ifdef ROM_FlashCtl_A_setWaitState
#define MAP_FlashCtl_A_setWaitState                                           \
        ROM_FlashCtl_A_setWaitState
#else
#define MAP_FlashCtl_A_setWaitState                                           \
        FlashCtl_A_setWaitState
#endif
#ifdef ROM_FlashCtl_A_getWaitState
#define MAP_FlashCtl_A_getWaitState                                           \
        ROM_FlashCtl_A_getWaitState
#else
#define MAP_FlashCtl_A_getWaitState                                           \
        FlashCtl_A_getWaitState
#endif
#ifdef ROM_FlashCtl_A_setReadMode
#define MAP_FlashCtl_A_setReadMode                                            \
        ROM_FlashCtl_A_setReadMode
#else
#define MAP_FlashCtl_A_setReadMode                                            \
        FlashCtl_A_setReadMode
#endif
#ifdef ROM_FlashCtl_A_getReadMode
#define MAP_FlashCtl_A_getReadMode                                            \
        ROM_FlashCtl_A_getReadMode
#else
#define MAP_FlashCtl_A_getReadMode                                            \
        FlashCtl_A_getReadMode
#endif
#ifdef ROM_FlashCtl_A_registerInterrupt
#define MAP_FlashCtl_A_registerInterrupt                                      \
        ROM_FlashCtl_A_registerInterrupt
#else
#define MAP_FlashCtl_A_registerInterrupt                                      \
        FlashCtl_A_registerInterrupt
#endif
#ifdef ROM_FlashCtl_A_unregisterInterrupt
#define MAP_FlashCtl_A_unregisterInterrupt                                    \
        ROM_FlashCtl_A_unregisterInterrupt
#else
#define MAP_FlashCtl_A_unregisterInterrupt                                    \
        FlashCtl_A_unregisterInterrupt
#endif
#ifdef ROM___FlashCtl_A_remaskData8Post
#define MAP___FlashCtl_A_remaskData8Post                                      \
        ROM___FlashCtl_A_remaskData8Post
#else
#define MAP___FlashCtl_A_remaskData8Post                                      \
        __FlashCtl_A_remaskData8Post
#endif
#ifdef ROM___FlashCtl_A_remaskData8Pre
#define MAP___FlashCtl_A_remaskData8Pre                                       \
        ROM___FlashCtl_A_remaskData8Pre
#else
#define MAP___FlashCtl_A_remaskData8Pre                                       \
        __FlashCtl_A_remaskData8Pre
#endif
#ifdef ROM___FlashCtl_A_remaskData32Pre
#define MAP___FlashCtl_A_remaskData32Pre                                      \
        ROM___FlashCtl_A_remaskData32Pre
#else
#define MAP___FlashCtl_A_remaskData32Pre                                      \
        __FlashCtl_A_remaskData32Pre
#endif
#ifdef ROM___FlashCtl_A_remaskData32Post
#define MAP___FlashCtl_A_remaskData32Post                                     \
        ROM___FlashCtl_A_remaskData32Post
#else
#define MAP___FlashCtl_A_remaskData32Post                                     \
        __FlashCtl_A_remaskData32Post
#endif
#ifdef ROM___FlashCtl_A_remaskBurstDataPre
#define MAP___FlashCtl_A_remaskBurstDataPre                                   \
        ROM___FlashCtl_A_remaskBurstDataPre
#else
#define MAP___FlashCtl_A_remaskBurstDataPre                                   \
        __FlashCtl_A_remaskBurstDataPre
#endif
#ifdef ROM___FlashCtl_A_remaskBurstDataPost
#define MAP___FlashCtl_A_remaskBurstDataPost                                  \
        ROM___FlashCtl_A_remaskBurstDataPost
#else
#define MAP___FlashCtl_A_remaskBurstDataPost                                  \
        __FlashCtl_A_remaskBurstDataPost
#endif
#ifdef ROM_FlashCtl_A_initiateSectorErase
#define MAP_FlashCtl_A_initiateSectorErase                                    \
        ROM_FlashCtl_A_initiateSectorErase
#else
#define MAP_FlashCtl_A_initiateSectorErase                                    \
        FlashCtl_A_initiateSectorErase
#endif
#ifdef ROM_FlashCtl_A_initiateMassErase
#define MAP_FlashCtl_A_initiateMassErase                                      \
        ROM_FlashCtl_A_initiateMassErase
#else
#define MAP_FlashCtl_A_initiateMassErase                                      \
        FlashCtl_A_initiateMassErase
#endif
#ifdef ROM_FlashCtl_A_isMemoryProtected
#define MAP_FlashCtl_A_isMemoryProtected                                      \
        ROM_FlashCtl_A_isMemoryProtected
#else
#define MAP_FlashCtl_A_isMemoryProtected                                      \
        FlashCtl_A_isMemoryProtected
#endif
#ifdef ROM_FlashCtl_A_getMemoryInfo
#define MAP_FlashCtl_A_getMemoryInfo                                          \
        ROM_FlashCtl_A_getMemoryInfo
#else
#define MAP_FlashCtl_A_getMemoryInfo                                          \
        FlashCtl_A_getMemoryInfo
#endif

//*****************************************************************************
//
// Macros for the LCD_F API.
//
//*****************************************************************************
#ifdef ROM_LCD_F_initModule
#define MAP_LCD_F_initModule                                                  \
        ROM_LCD_F_initModule
#else
#define MAP_LCD_F_initModule                                                  \
        LCD_F_initModule
#endif
#ifdef ROM_LCD_F_turnOn
#define MAP_LCD_F_turnOn                                                      \
        ROM_LCD_F_turnOn
#else
#define MAP_LCD_F_turnOn                                                      \
        LCD_F_turnOn
#endif
#ifdef ROM_LCD_F_turnOff
#define MAP_LCD_F_turnOff                                                     \
        ROM_LCD_F_turnOff
#else
#define MAP_LCD_F_turnOff                                                     \
        LCD_F_turnOff
#endif
#ifdef ROM_LCD_F_clearAllMemory
#define MAP_LCD_F_clearAllMemory                                              \
        ROM_LCD_F_clearAllMemory
#else
#define MAP_LCD_F_clearAllMemory                                              \
        LCD_F_clearAllMemory
#endif
#ifdef ROM_LCD_F_clearAllBlinkingMemory
#define MAP_LCD_F_clearAllBlinkingMemory                                      \
        ROM_LCD_F_clearAllBlinkingMemory
#else
#define MAP_LCD_F_clearAllBlinkingMemory                                      \
        LCD_F_clearAllBlinkingMemory
#endif
#ifdef ROM_LCD_F_selectDisplayMemory
#define MAP_LCD_F_selectDisplayMemory                                         \
        ROM_LCD_F_selectDisplayMemory
#else
#define MAP_LCD_F_selectDisplayMemory                                         \
        LCD_F_selectDisplayMemory
#endif
#ifdef ROM_LCD_F_setBlinkingControl
#define MAP_LCD_F_setBlinkingControl                                          \
        ROM_LCD_F_setBlinkingControl
#else
#define MAP_LCD_F_setBlinkingControl                                          \
        LCD_F_setBlinkingControl
#endif
#ifdef ROM_LCD_F_setAnimationControl
#define MAP_LCD_F_setAnimationControl                                         \
        ROM_LCD_F_setAnimationControl
#else
#define MAP_LCD_F_setAnimationControl                                         \
        LCD_F_setAnimationControl
#endif
#ifdef ROM_LCD_F_clearAllAnimationMemory
#define MAP_LCD_F_clearAllAnimationMemory                                     \
        ROM_LCD_F_clearAllAnimationMemory
#else
#define MAP_LCD_F_clearAllAnimationMemory                                     \
        LCD_F_clearAllAnimationMemory
#endif
#ifdef ROM_LCD_F_setPinAsLCDFunction
#define MAP_LCD_F_setPinAsLCDFunction                                         \
        ROM_LCD_F_setPinAsLCDFunction
#else
#define MAP_LCD_F_setPinAsLCDFunction                                         \
        LCD_F_setPinAsLCDFunction
#endif
#ifdef ROM_LCD_F_setPinAsPortFunction
#define MAP_LCD_F_setPinAsPortFunction                                        \
        ROM_LCD_F_setPinAsPortFunction
#else
#define MAP_LCD_F_setPinAsPortFunction                                        \
        LCD_F_setPinAsPortFunction
#endif
#ifdef ROM_LCD_F_setPinsAsLCDFunction
#define MAP_LCD_F_setPinsAsLCDFunction                                        \
        ROM_LCD_F_setPinsAsLCDFunction
#else
#define MAP_LCD_F_setPinsAsLCDFunction                                        \
        LCD_F_setPinsAsLCDFunction
#endif
#ifdef ROM_LCD_F_setPinAsCOM
#define MAP_LCD_F_setPinAsCOM                                                 \
        ROM_LCD_F_setPinAsCOM
#else
#define MAP_LCD_F_setPinAsCOM                                                 \
        LCD_F_setPinAsCOM
#endif
#ifdef ROM_LCD_F_setPinAsSEG
#define MAP_LCD_F_setPinAsSEG                                                 \
        ROM_LCD_F_setPinAsSEG
#else
#define MAP_LCD_F_setPinAsSEG                                                 \
        LCD_F_setPinAsSEG
#endif
#ifdef ROM_LCD_F_selectBias
#define MAP_LCD_F_selectBias                                                  \
        ROM_LCD_F_selectBias
#else
#define MAP_LCD_F_selectBias                                                  \
        LCD_F_selectBias
#endif
#ifdef ROM_LCD_F_setVLCDSource
#define MAP_LCD_F_setVLCDSource                                               \
        ROM_LCD_F_setVLCDSource
#else
#define MAP_LCD_F_setVLCDSource                                               \
        LCD_F_setVLCDSource
#endif
#ifdef ROM_LCD_F_clearInterrupt
#define MAP_LCD_F_clearInterrupt                                              \
        ROM_LCD_F_clearInterrupt
#else
#define MAP_LCD_F_clearInterrupt                                              \
        LCD_F_clearInterrupt
#endif
#ifdef ROM_LCD_F_getInterruptStatus
#define MAP_LCD_F_getInterruptStatus                                          \
        ROM_LCD_F_getInterruptStatus
#else
#define MAP_LCD_F_getInterruptStatus                                          \
        LCD_F_getInterruptStatus
#endif
#ifdef ROM_LCD_F_getEnabledInterruptStatus
#define MAP_LCD_F_getEnabledInterruptStatus                                   \
        ROM_LCD_F_getEnabledInterruptStatus
#else
#define MAP_LCD_F_getEnabledInterruptStatus                                   \
        LCD_F_getEnabledInterruptStatus
#endif
#ifdef ROM_LCD_F_enableInterrupt
#define MAP_LCD_F_enableInterrupt                                             \
        ROM_LCD_F_enableInterrupt
#else
#define MAP_LCD_F_enableInterrupt                                             \
        LCD_F_enableInterrupt
#endif
#ifdef ROM_LCD_F_disableInterrupt
#define MAP_LCD_F_disableInterrupt                                            \
        ROM_LCD_F_disableInterrupt
#else
#define MAP_LCD_F_disableInterrupt                                            \
        LCD_F_disableInterrupt
#endif
#ifdef ROM_LCD_F_registerInterrupt
#define MAP_LCD_F_registerInterrupt                                           \
        ROM_LCD_F_registerInterrupt
#else
#define MAP_LCD_F_registerInterrupt                                           \
        LCD_F_registerInterrupt
#endif
#ifdef ROM_LCD_F_unregisterInterrupt
#define MAP_LCD_F_unregisterInterrupt                                         \
        ROM_LCD_F_unregisterInterrupt
#else
#define MAP_LCD_F_unregisterInterrupt                                         \
        LCD_F_unregisterInterrupt
#endif

#endif // __ROM_MAP_H__
