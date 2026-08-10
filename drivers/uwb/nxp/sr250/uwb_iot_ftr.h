/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UWB_IOT_FTR_H_
#define UWB_IOT_FTR_H_

/* ************************************************************************** */
/* Defines                                                                    */
/* ************************************************************************** */

/* clang-format off */


/* # CMake Features : Start */


/** UWBIOT_UWBD : The UWB Device
 *
 * The device to which we are connecting
 */

/** SR150 with or without SE051W
 *
 * If you want to select SE051W, use Applet=SE051_UWB
 * If you want to skip SE051W, use Applet=None */
#define UWBIOT_UWBD_SR150 0

/** SR040 */
#define UWBIOT_UWBD_SR040 0

/** Helios SR100T */
#define UWBIOT_UWBD_SR100T 0

/** Helios SR100S */
#define UWBIOT_UWBD_SR100S 0

/** NXP Internal */
#define UWBIOT_UWBD_SR200T 0

/** NXP Internal */
#define UWBIOT_UWBD_SR250 1

/** Helios SR200S */
#define UWBIOT_UWBD_SR200S 0

#if (( 0                             \
    + UWBIOT_UWBD_SR150              \
    + UWBIOT_UWBD_SR040              \
    + UWBIOT_UWBD_SR100T             \
    + UWBIOT_UWBD_SR100S             \
    + UWBIOT_UWBD_SR200T             \
    + UWBIOT_UWBD_SR250              \
    + UWBIOT_UWBD_SR200S             \
    ) > 1)
#        error "Enable only one of 'UWBIOT_UWBD'"
#endif


#if (( 0                             \
    + UWBIOT_UWBD_SR150              \
    + UWBIOT_UWBD_SR040              \
    + UWBIOT_UWBD_SR100T             \
    + UWBIOT_UWBD_SR100S             \
    + UWBIOT_UWBD_SR200T             \
    + UWBIOT_UWBD_SR250              \
    + UWBIOT_UWBD_SR200S             \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_UWBD'"
#endif



/** UWBIOT_TML : Interface used for connection
 */

/** Plug And Play mode on Rhodes */
#define UWBIOT_TML_PNP 0

/** UART Mode of communication with S32K */
#define UWBIOT_TML_S32UART 0

/** Native SPI Communication */
#define UWBIOT_TML_SPI 1

/** Native I2C Communication */
#define UWBIOT_TML_I2C 0

/** Using network Sockets
 *
 * NXP Internal, for testing purpose.
 * Ongoing development. Not yet ready
 * NXP Internal
 */
#define UWBIOT_TML_SOCKET 0

/** Linux Kernel Interface for UWB Device
 *
 * **Only for Linux.** */
#define UWBIOT_TML_LIBUWBD 0

/** Stubbed TMLFor future internal testing.
 * NXP Internal
 */
#define UWBIOT_TML_STUB 0

#if (( 0                             \
    + UWBIOT_TML_PNP                 \
    + UWBIOT_TML_S32UART             \
    + UWBIOT_TML_SPI                 \
    + UWBIOT_TML_I2C                 \
    + UWBIOT_TML_SOCKET              \
    + UWBIOT_TML_LIBUWBD             \
    + UWBIOT_TML_STUB                \
    ) > 1)
#        error "Enable only one of 'UWBIOT_TML'"
#endif


#if (( 0                             \
    + UWBIOT_TML_PNP                 \
    + UWBIOT_TML_S32UART             \
    + UWBIOT_TML_SPI                 \
    + UWBIOT_TML_I2C                 \
    + UWBIOT_TML_SOCKET              \
    + UWBIOT_TML_LIBUWBD             \
    + UWBIOT_TML_STUB                \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_TML'"
#endif



/** UWBIOT_SR1XX_FW : SR100T, SR150 FW Variant
 */

/** Development variant for Rest of the World */
#define UWBIOT_SR1XX_FW_ROW_DEV 0

/** Production variant for rest of the World */
#define UWBIOT_SR1XX_FW_ROW_PROD 1

#if (( 0                             \
    + UWBIOT_SR1XX_FW_ROW_DEV        \
    + UWBIOT_SR1XX_FW_ROW_PROD       \
    ) > 1)
#        error "Enable only one of 'UWBIOT_SR1XX_FW'"
#endif


#if (( 0                             \
    + UWBIOT_SR1XX_FW_ROW_DEV        \
    + UWBIOT_SR1XX_FW_ROW_PROD       \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_SR1XX_FW'"
#endif



/** UWBIOT_FW_VARIANT : SR150 FW Variant
 */

/** No variant */
#define UWBIOT_FW_VARIANT_NONE 1

/** Transit variant */
#define UWBIOT_FW_VARIANT_TRANSIT 0

/** SmartHome variant */
#define UWBIOT_FW_VARIANT_SMARTHOME 0

#if (( 0                             \
    + UWBIOT_FW_VARIANT_NONE         \
    + UWBIOT_FW_VARIANT_TRANSIT      \
    + UWBIOT_FW_VARIANT_SMARTHOME    \
    ) > 1)
#        error "Enable only one of 'UWBIOT_FW_VARIANT'"
#endif


#if (( 0                             \
    + UWBIOT_FW_VARIANT_NONE         \
    + UWBIOT_FW_VARIANT_TRANSIT      \
    + UWBIOT_FW_VARIANT_SMARTHOME    \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_FW_VARIANT'"
#endif



/** UWBIOT_SESN : SNXXX Selection
 */

/** No SNXXX support */
#define UWBIOT_SESN_NONE 1

/** Secure Element with SN110 */
#define UWBIOT_SESN_SN110 0

/** Secure Element with SN220 */
#define UWBIOT_SESN_SN220 0

/** Secure Element with P71 */
#define UWBIOT_SESN_P71 0

#if (( 0                             \
    + UWBIOT_SESN_NONE               \
    + UWBIOT_SESN_SN110              \
    + UWBIOT_SESN_SN220              \
    + UWBIOT_SESN_P71                \
    ) > 1)
#        error "Enable only one of 'UWBIOT_SESN'"
#endif


#if (( 0                             \
    + UWBIOT_SESN_NONE               \
    + UWBIOT_SESN_SN110              \
    + UWBIOT_SESN_SN220              \
    + UWBIOT_SESN_P71                \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_SESN'"
#endif



/** UWBIOT_Timing : Timing in RSTU or uSEC
 */

/** Timing in RSTU
 *
 * This is the default option. */
#define UWBIOT_TIMING_RSTU 1

/** Timing in Micro Seconds
 *
 * This option is only there to support older versions of the UWB FW. */
#define UWBIOT_TIMING_USEC 0

#if (( 0                             \
    + UWBIOT_TIMING_RSTU             \
    + UWBIOT_TIMING_USEC             \
    ) > 1)
#        error "Enable only one of 'UWBIOT_Timing'"
#endif


#if (( 0                             \
    + UWBIOT_TIMING_RSTU             \
    + UWBIOT_TIMING_USEC             \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_Timing'"
#endif



/** UWBIOT_OS : Operating system used
 */

/** running with FreeRTOS Implementation. Direct or Simulation. */
#define UWBIOT_OS_FREERTOS 0

/** running with Zephyr Implementation. Using Zephyr Kernel. */
#define UWBIOT_OS_ZEPHYR 1

/** Native Implementation. Using pthread */
#define UWBIOT_OS_NATIVE 0

#if (( 0                             \
    + UWBIOT_OS_FREERTOS             \
    + UWBIOT_OS_ZEPHYR               \
    + UWBIOT_OS_NATIVE               \
    ) > 1)
#        error "Enable only one of 'UWBIOT_OS'"
#endif


#if (( 0                             \
    + UWBIOT_OS_FREERTOS             \
    + UWBIOT_OS_ZEPHYR               \
    + UWBIOT_OS_NATIVE               \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_OS'"
#endif



/** UWBIOT_LOG : Logging Level
 */

/** Default Logging */
#define UWBIOT_LOG_DEFAULT 1

/** Very Verbose logging */
#define UWBIOT_LOG_VERBOSE 0

/** Totally silent logging */
#define UWBIOT_LOG_SILENT 0

#if (( 0                             \
    + UWBIOT_LOG_DEFAULT             \
    + UWBIOT_LOG_VERBOSE             \
    + UWBIOT_LOG_SILENT              \
    ) > 1)
#        error "Enable only one of 'UWBIOT_LOG'"
#endif


#if (( 0                             \
    + UWBIOT_LOG_DEFAULT             \
    + UWBIOT_LOG_VERBOSE             \
    + UWBIOT_LOG_SILENT              \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_LOG'"
#endif



/** UWBIOT_TRANSIT_AUTH : Authentication mechanism to use in transit use-case
 */

/** NXP Proprietary authentication */
#define UWBIOT_TRANSIT_AUTH_NXP 1

/** Mifare authentication */
#define UWBIOT_TRANSIT_AUTH_MIFARE 0

#if (( 0                             \
    + UWBIOT_TRANSIT_AUTH_NXP        \
    + UWBIOT_TRANSIT_AUTH_MIFARE     \
    ) > 1)
#        error "Enable only one of 'UWBIOT_TRANSIT_AUTH'"
#endif


#if (( 0                             \
    + UWBIOT_TRANSIT_AUTH_NXP        \
    + UWBIOT_TRANSIT_AUTH_MIFARE     \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_TRANSIT_AUTH'"
#endif



/** UWBIOT_P71_TAG : P71 Tag board support configuration
 */

/** P71 Tag board support Disabled */
#define UWBIOT_P71_TAG_DISABLED 1

/** P71 Tag board support Enabled */
#define UWBIOT_P71_TAG_ENABLED 0

#if (( 0                             \
    + UWBIOT_P71_TAG_DISABLED        \
    + UWBIOT_P71_TAG_ENABLED         \
    ) > 1)
#        error "Enable only one of 'UWBIOT_P71_TAG'"
#endif


#if (( 0                             \
    + UWBIOT_P71_TAG_DISABLED        \
    + UWBIOT_P71_TAG_ENABLED         \
    ) == 0)
#        error "Enable at-least one of 'UWBIOT_P71_TAG'"
#endif



/* ====================================================================== *
 * == UWB Features ====================================================== *
 * ====================================================================== */
/*
 *
 * Select / Enable individual features
 */


/** @define UWBFTR_TWR
 *
 * Support Two Way Ranging
 *
 * Either DSTWR or SSTWR
 */
#define UWBFTR_TWR 1


/** @define UWBFTR_UWBS_DEBUG_Dump
 *
 * UWBS Debug dumps
 *
 * Enable processing of Debug notifications and Dumps
 */
#define UWBFTR_UWBS_DEBUG_Dump 1


/** @define UWBFTR_DL_TDoA_Anchor
 *
 * Downlink TDoA Anchor
 *
 * Use UWBS as an anchor for Downlink TDoA.
 * Only for SR1XX. Not for SR040
 */
#define UWBFTR_DL_TDoA_Anchor 1


/** @define UWBFTR_DL_TDoA_Tag
 *
 * Downlink TDoA Tag
 *
 * Use UWBS as a tag for Downlink TDoA.
 * Only for SR1XX. Not for SR040
 */
#define UWBFTR_DL_TDoA_Tag 0


/** @define UWBFTR_UL_TDoA_Anchor
 *
 * Uplink TDoA Anchor
 *
 * Use UWBS as an anchor for Uplink TDoA.
 * Only for SR1XX. Not for SR040
 */
#define UWBFTR_UL_TDoA_Anchor 0


/** @define UWBFTR_UL_TDoA_Tag
 *
 * Uplink TDoA Tag
 *
 * Use UWBS as a tag for Uplink TDoA.
 */
#define UWBFTR_UL_TDoA_Tag 0


/** @define UWBFTR_CCC
 *
 * Connected Car Configuration
 *
 * Only for SR150, SR100S and SR200S
 */
#define UWBFTR_CCC 1


/** @define UWBFTR_TransitProp
 *
 * Enabling Transit Proprietary
 *
 * Only for SR150, SR100S and SR250
 */
#define UWBFTR_TransitProp 0


/** @define UWBFTR_Radar
 *
 * Radar Feature
 *
 * Enable Handling of RADAR Feature
 * Only for SR200T or SR250
 * NXP Internal
 */
#define UWBFTR_Radar 1


/** @define UWBFTR_BlobParser
 *
 * Blob Parser
 *
 * Enable for Near by Interaction Feature
 */
#define UWBFTR_BlobParser 1


/** @define UWBFTR_FactoryMode
 *
 * Support Factory Mode
 *
 * Support Factory Mode APIs
 */
#define UWBFTR_FactoryMode 0


/** @define UWBFTR_DataTransfer
 *
 * Support Data transfer
 *
 * Support Data Transfer APIs
 */
#define UWBFTR_DataTransfer 1


/** @define UWBFTR_ChainedUCI
 *
 * Chained UCI Packed
 *
 * Allow processing of UCI Packets chaining
 */
/* UWBFTR_ChainedUCI is derived below */


/** @define UWBFTR_CSA
 *
 * Connectivity Standards Alliance
 *
 * Only for SR150, SR250, SR100S and SR200S
 */
#define UWBFTR_CSA 1


/** @define UWBFTR_AoA_FoV
 *
 * Feature for 3D AoA, 2D FoV, 360 Degree AoA
 *
 */
#define UWBFTR_AoA_FoV 1



/* ====================================================================== *
 * == Feature selection/values ========================================== *
 * ====================================================================== */


/* ====================================================================== *
 * == Computed Options ================================================== *
 * ====================================================================== */



#define UWBIOT_UWBD_SR1XXT \
 (UWBIOT_UWBD_SR100T | UWBIOT_UWBD_SR150 | UWBIOT_UWBD_SR100S)

#define UWBIOT_UWBD_SR2XXT \
 (UWBIOT_UWBD_SR200T | UWBIOT_UWBD_SR250 | UWBIOT_UWBD_SR200S)

#define UWBIOT_UWBD_SR1XXT_SR2XXT \
 (UWBIOT_UWBD_SR1XXT | UWBIOT_UWBD_SR2XXT)

#define UWBIOT_SESN_SNXXX \
 (UWBIOT_SESN_SN110 | UWBIOT_SESN_SN220 | UWBIOT_SESN_P71)

#define UWBFTR_ChainedUCI \
 (UWBFTR_UL_TDoA_Anchor | UWBFTR_DL_TDoA_Tag | UWBFTR_UWBS_DEBUG_Dump | UWBFTR_Radar | UWBFTR_DataTransfer | UWBIOT_UWBD_SR040)

/**
* Enable CCC as all configurations of CCC are applicable for CSA
*/
#if (UWBFTR_CSA == 1)
// Remove the previous definition
#undef UWBFTR_CCC
// Redeclare with new value
#define UWBFTR_CCC   0
#endif // (UWBFTR_CSA == 1)

/**
* Features not supported by UWBIOT_FW_VARIANT_TRANSIT
*/
#if (UWBIOT_FW_VARIANT_TRANSIT == 1)
#if (( 0                             \
    + UWBFTR_UL_TDoA_Tag             \
    + UWBFTR_DL_TDoA_Tag             \
    + UWBFTR_AoA_FoV                 \
    + UWBFTR_BlobParser              \
    + UWBFTR_CSA                     \
    + UWBFTR_CCC                     \
    + UWBFTR_Radar                   \
    ) != 0)
#        error "The above list of features to be disabled for UWBIOT_FW_VARIANT_TRANSIT"
#endif //
#endif // (UWBIOT_FW_VARIANT_TRANSIT == 1)

/**
* Features not supported by UWBIOT_FW_VARIANT_SMARTHOME
*/
#if (UWBIOT_FW_VARIANT_SMARTHOME == 1)
#if (( 0                             \
    + UWBFTR_TransitProp             \
    + UWBFTR_UL_TDoA_Anchor          \
    + UWBFTR_UL_TDoA_Tag             \
    + UWBFTR_DL_TDoA_Anchor          \
    + UWBFTR_Radar                   \
    ) != 0)
#        error "The above list of features to be disabled for UWBIOT_FW_VARIANT_SMARTHOME"
#endif //
#endif // (UWBIOT_FW_VARIANT_SMARTHOME == 1)

/** Deprecated items. Used here for backwards compatibility. */


/* # CMake Features : END */


/* Compiling with SE051W for Secure Ranging use cases */

#define UWBFTR_SE_SE051W 0

/* clang-format on */

#endif /* UWB_IOT_FTR_H_ */
