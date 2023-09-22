/**************************************************************************//**
 * @file     espivw.h
 * @brief    Define ESPIVW's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ESPIVW_H
#define ESPIVW_H

/*****************************************************************************/
//  VW GPIO Data Structure:
//    8bit                    1bit   1bit   2bit         4bit
//  ----------------------------------------------------------------------
//  | Index                  | RSV  |In/Out| Int.Type   | Bit Offset     |
//  ----------------------------------------------------------------------
//  Ex: USR_ESPIVW_DIR_INDEX02   = 0
//      USR_ESPIVW_INT_INDEX02_8 = 0(No Interrupt Event)
//      ESPIVW_INDEX02_8 = 0x0208
//
//      USR_ESPIVW_DIR_INDEX04   = 1
//      USR_ESPIVW_INT_INDEX04_8 = 3(Valid High Interrupt Event)
//      ESPIVW_INDEX04_8 = 0x0478
/*****************************************************************************/
/********* VW Configuration **************************************************/
/*  Virtual Wire Index  : 0,1       IRQ[0-NIRQMAX]      For SKL-PCH, NIRQMAX is 119
    Virtual Wire Group  : System Event
    Reset               : eSPI Reset#
    Direction           : Slave to Master */
#define USR_ESPIVW_DIR_INDEX00                  ESPIVW_DIR_MISO         // IRQ [0~127]      Don't Change
#define USR_ESPIVW_DIR_INDEX01                  ESPIVW_DIR_MISO         // IRQ [128~255]    Don't Change
/*  Virtual Wire Index  : 2
    Virtual Wire Group  : System Event
    Reset               : eSPI Reset#
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX02                  ESPIVW_DIR_MOSI         // Don't Change
#define USR_ESPIVW_INT_INDEX02_0                ESPIVW_INT_NONE         // SLP_S3   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX02_1                ESPIVW_INT_NONE         // SLP_S4   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX02_2                ESPIVW_INT_NONE         // SLP_S5   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX02_3                ESPIVW_INT_NONE         // RSV      ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 3
    Virtual Wire Group  : System Event
    Reset               : eSPI Reset#
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX03                  ESPIVW_DIR_MOSI         // Don't Change
#define USR_ESPIVW_INT_INDEX03_0                ESPIVW_INT_NONE         // SUS_STAT#    ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX03_1                ESPIVW_INT_VALID        // PLTRST#      Don't Change
#define USR_ESPIVW_INT_INDEX03_2                ESPIVW_INT_VALID        // OOB_RST_WARN Don't Change
#define USR_ESPIVW_INT_INDEX03_3                ESPIVW_INT_NONE         // RSV          ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 4
    Virtual Wire Group  : System Event
    Reset               : eSPI Reset#
    Direction           : Slave to Master   */
#define USR_ESPIVW_DIR_INDEX04                  ESPIVW_DIR_MISO         // Don't Change
#define USR_ESPIVW_INT_INDEX04_0                ESPIVW_INT_NONE         // OOB_RST_ACK  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX04_1                ESPIVW_INT_NONE         // RSV          ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX04_2                ESPIVW_INT_NONE         // WAKE#        ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX04_3                ESPIVW_INT_NONE         // PME#         ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 5
    Virtual Wire Group  : System Event
    Reset               : eSPI Reset#
    Direction           : Slave to Master   */
#define USR_ESPIVW_DIR_INDEX05                  ESPIVW_DIR_MISO         // Don't Change
#define USR_ESPIVW_INT_INDEX05_0                ESPIVW_INT_NONE         // SLAVE_BOOT_LOAD_DONE     ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX05_1                ESPIVW_INT_NONE         // ERROR_FATAL              ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX05_2                ESPIVW_INT_NONE         // ERROR_NONFATAL           ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX05_3                ESPIVW_INT_NONE         // SLAVE_BOOT_LOAD_STATUS   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 6
    Virtual Wire Group  : System Event
    Reset               : PLTRST#
    Direction           : Slave to Master   */
#define USR_ESPIVW_DIR_INDEX06                  ESPIVW_DIR_MISO         // Don't Change
#define USR_ESPIVW_INT_INDEX06_0                ESPIVW_INT_NONE         // SCI#             ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX06_1                ESPIVW_INT_NONE         // SMI#             ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX06_2                ESPIVW_INT_NONE         // RCIN#            ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX06_3                ESPIVW_INT_NONE         // HOST_RST_ACK     ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 7
    Virtual Wire Group  : System Event
    Reset               : PLTRST#
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX07                  ESPIVW_DIR_MOSI         // Don't Change
#define USR_ESPIVW_INT_INDEX07_0                ESPIVW_INT_VALID        // HOST_RST_WARN    Don't Change
#define USR_ESPIVW_INT_INDEX07_1                ESPIVW_INT_NONE         // SMIOUT#          ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX07_2                ESPIVW_INT_NONE         // NMIOUT#          ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX07_3                ESPIVW_INT_NONE         // RSV              ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

/*  Virtual Wire Index  : 40h
    Reset               : eSPI Reset#
    Direction           : Slave to Master   */
#define USR_ESPIVW_DIR_INDEX40                  ESPIVW_DIR_MISO         // ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX40_0                ESPIVW_INT_NONE         // SUSACK#          ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX40_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX40_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX40_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 41h
    Reset               : eSPI Reset#
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX41                  ESPIVW_DIR_MOSI         // ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX41_0                ESPIVW_INT_VALID        // SUS_WARN         ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX41_1                ESPIVW_INT_NONE         // SUS_PWRDN_ACK    ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX41_2                ESPIVW_INT_NONE         // SLP_S0           ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX41_3                ESPIVW_INT_NONE         // SLP_A            ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 42h
    Reset               : DSW_PWR_OK
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX42                  ESPIVW_DIR_MOSI         // SLP_LAN          ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX42_0                ESPIVW_INT_NONE         // SLP_WLAN         ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX42_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX42_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX42_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 43h
    Reset               : eSPI Reset#
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX43                  ESPIVW_DIR_MOSI         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX43_0                ESPIVW_INT_NONE         // PCH_TO_EC_GEN0   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX43_1                ESPIVW_INT_NONE         // PCH_TO_EC_GEN1   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX43_2                ESPIVW_INT_NONE         // PCH_TO_EC_GEN2   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX43_3                ESPIVW_INT_NONE         // PCH_TO_EC_GEN3   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 44h
    Reset               : eSPI Reset#
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX44                  ESPIVW_DIR_MOSI         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX44_0                ESPIVW_INT_NONE         // PCH_TO_EC_GEN4   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX44_1                ESPIVW_INT_NONE         // PCH_TO_EC_GEN5   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX44_2                ESPIVW_INT_NONE         // PCH_TO_EC_GEN6   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX44_3                ESPIVW_INT_NONE         // PCH_TO_EC_GEN7   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 45h
    Reset               : eSPI Reset#
    Direction           : Slave to Master   */
#define USR_ESPIVW_DIR_INDEX45                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX45_0                ESPIVW_INT_NONE         // EC_TO_PCH_GEN0   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX45_1                ESPIVW_INT_NONE         // EC_TO_PCH_GEN1   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX45_2                ESPIVW_INT_NONE         // EC_TO_PCH_GEN2   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX45_3                ESPIVW_INT_NONE         // EC_TO_PCH_GEN3   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 46h
    Reset               : eSPI Reset#
    Direction           : Slave to Master   */
#define USR_ESPIVW_DIR_INDEX46                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX46_0                ESPIVW_INT_NONE         // EC_TO_PCH_GEN4   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX46_1                ESPIVW_INT_NONE         // EC_TO_PCH_GEN5   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX46_2                ESPIVW_INT_NONE         // EC_TO_PCH_GEN6   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX46_3                ESPIVW_INT_NONE         // EC_TO_PCH_GEN7   ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
/*  Virtual Wire Index  : 47h
    Reset               : PLTRST#
    Direction           : Master to Slave   */
#define USR_ESPIVW_DIR_INDEX47                  ESPIVW_DIR_MOSI         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX47_0                ESPIVW_INT_NONE         // HOST_C10         ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX47_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX47_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX47_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX48                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX48_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX48_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX48_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX48_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX49                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX49_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX49_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX49_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX49_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX4A                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX4A_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4A_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4A_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4A_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX4B                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX4B_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4B_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4B_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4B_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX4C                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX4C_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4C_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4C_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4C_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX4D                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX4D_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4D_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4D_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4D_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX4E                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX4E_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4E_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4E_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4E_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX4F                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX4F_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4F_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4F_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX4F_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX80                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX80_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX80_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX80_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX80_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX81                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX81_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX81_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX81_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX81_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX82                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX82_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX82_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX82_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX82_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX83                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX83_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX83_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX83_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX83_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX84                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX84_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX84_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX84_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX84_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX85                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX85_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX85_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX85_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX85_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX86                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX86_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX86_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX86_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX86_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX87                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX87_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX87_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX87_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX87_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX88                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX88_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX88_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX88_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX88_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX89                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX89_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX89_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX89_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX89_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX8A                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX8A_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8A_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8A_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8A_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX8B                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX8B_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8B_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8B_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8B_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX8C                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX8C_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8C_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8C_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8C_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX8D                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX8D_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8D_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8D_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8D_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX8E                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX8E_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8E_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8E_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8E_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

#define USR_ESPIVW_DIR_INDEX8F                  ESPIVW_DIR_MISO         //                  ESPIVW_DIR_MISO / ESPIVW_DIR_MOSI
#define USR_ESPIVW_INT_INDEX8F_0                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8F_1                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8F_2                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH
#define USR_ESPIVW_INT_INDEX8F_3                ESPIVW_INT_NONE         //                  ESPIVW_INT_NONE / ESPIVW_INT_VALID / ESPIVW_INT_LOW / ESPIVW_INT_HIGH

//************************************************************
//ESPIVW Initial Value Define
//************************************************************
#define USR_ESPIVW_INIT_INDEX02_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX02_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX02_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX02_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX03_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX03_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX03_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX03_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX04_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX04_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX04_2               ESPIVW_INIT_HIGH        //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX04_3               ESPIVW_INIT_HIGH        //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX05_0               ESPIVW_INIT_LOW         // Don't Change
#define USR_ESPIVW_INIT_INDEX05_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX05_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX05_3               ESPIVW_INIT_LOW         // Don't Change

#define USR_ESPIVW_INIT_INDEX06_0               ESPIVW_INIT_HIGH        //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX06_1               ESPIVW_INIT_HIGH        //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX06_2               ESPIVW_INIT_HIGH        //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX06_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX07_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX07_1               ESPIVW_INIT_HIGH         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX07_2               ESPIVW_INIT_HIGH         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX07_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX40_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX40_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX40_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX40_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX41_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX41_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX41_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX41_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX42_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX42_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX42_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX42_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX43_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX43_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX43_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX43_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX44_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX44_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX44_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX44_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX45_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX45_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX45_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX45_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX46_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX46_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX46_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX46_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX47_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX47_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX47_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX47_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX48_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX48_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX48_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX48_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX49_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX49_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX49_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX49_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX4A_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4A_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4A_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4A_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX4B_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4B_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4B_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4B_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX4C_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4C_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4C_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4C_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX4D_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4D_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4D_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4D_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX4E_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4E_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4E_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4E_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX4F_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4F_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4F_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX4F_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX80_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX80_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX80_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX80_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX81_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX81_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX81_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX81_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX82_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX82_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX82_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX82_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX83_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX83_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX83_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX83_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX84_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX84_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX84_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX84_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX85_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX85_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX85_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX85_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX86_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX86_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX86_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX86_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX87_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX87_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX87_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX87_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX88_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX88_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX88_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX88_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX89_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX89_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX89_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX89_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX8A_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8A_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8A_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8A_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX8B_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8B_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8B_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8B_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX8C_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8C_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8C_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8C_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX8D_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8D_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8D_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8D_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX8E_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8E_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8E_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8E_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

#define USR_ESPIVW_INIT_INDEX8F_0               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8F_1               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8F_2               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH
#define USR_ESPIVW_INIT_INDEX8F_3               ESPIVW_INIT_LOW         //ESPIVW_INIT_LOW / ESPIVW_INIT_HIGH

//************************************************************
// OEM ESPIVW GPIO Naming
//************************************************************
//Please reference ESPIVWDEF.H
//Ex:
#define OEM_ESPIVW_SLP_S3                       ESPIVW_INDEX02_0
#define OEM_ESPIVW_SLP_S4                       ESPIVW_INDEX02_1
#define OEM_ESPIVW_SLP_S5                       ESPIVW_INDEX02_2
#define OEM_ESPIVW_SUS_STAT                     ESPIVW_INDEX03_0
#define OEM_ESPIVW_PLTRST                       ESPIVW_INDEX03_1
#define OEM_ESPIVW_OOB_RST_WARN                 ESPIVW_INDEX03_2
#define OEM_ESPIVW_OOB_RST_ACK                  ESPIVW_INDEX04_0
#define OEM_ESPIVW_INDEX04_2                    ESPIVW_INDEX04_1
#define OEM_ESPIVW_WAKE                         ESPIVW_INDEX04_2
#define OEM_ESPIVW_PME                          ESPIVW_INDEX04_3
#define OEM_ESPIVW_SLAVE_BOOT_LOAD_DONE         ESPIVW_INDEX05_0
#define OEM_ESPIVW_ERROR_FATAL                  ESPIVW_INDEX05_1
#define OEM_ESPIVW_ERROR_NONFATAL               ESPIVW_INDEX05_2
#define OEM_ESPIVW_SLAVE_BOOT_LOAD_STATUS       ESPIVW_INDEX05_3
#define OEM_ESPIVW_SCI                          ESPIVW_INDEX06_0
#define OEM_ESPIVW_SMI                          ESPIVW_INDEX06_1
#define OEM_ESPIVW_RCIN                         ESPIVW_INDEX06_2
#define OEM_ESPIVW_HOST_RST_ACK                 ESPIVW_INDEX06_3
#define OEM_ESPIVW_HOST_RST_WARN                ESPIVW_INDEX07_0
#define OEM_ESPIVW_SMIOUT                       ESPIVW_INDEX07_1
#define OEM_ESPIVW_NMIOUT                       ESPIVW_INDEX07_2

/*  Skylake virtual wires start */
#define OEM_ESPIVW_SUSACK                       ESPIVW_INDEX40_0

#define OEM_ESPIVW_SUS_WARN                     ESPIVW_INDEX41_0
#define OEM_ESPIVW_SUS_PWRDN_ACK                ESPIVW_INDEX41_1
#define OEM_ESPIVW_SLP_S0                       ESPIVW_INDEX41_2
#define OEM_ESPIVW_SLP_A                        ESPIVW_INDEX41_3

#define OEM_ESPIVW_SLP_LAN                      ESPIVW_INDEX42_0
#define OEM_ESPIVW_SLP_WLAN                     ESPIVW_INDEX42_1

#define OEM_ESPIVW_PCH_TO_EC_GEN0               ESPIVW_INDEX43_0
#define OEM_ESPIVW_PCH_TO_EC_GEN1               ESPIVW_INDEX43_1
#define OEM_ESPIVW_PCH_TO_EC_GEN2               ESPIVW_INDEX43_2
#define OEM_ESPIVW_PCH_TO_EC_GEN3               ESPIVW_INDEX43_3

#define OEM_ESPIVW_PCH_TO_EC_GEN4               ESPIVW_INDEX44_0
#define OEM_ESPIVW_PCH_TO_EC_GEN5               ESPIVW_INDEX44_1
#define OEM_ESPIVW_PCH_TO_EC_GEN6               ESPIVW_INDEX44_2
#define OEM_ESPIVW_PCH_TO_EC_GEN7               ESPIVW_INDEX44_3

#define OEM_ESPIVW_EC_TO_PCH_GEN0               ESPIVW_INDEX45_0
#define OEM_ESPIVW_EC_TO_PCH_GEN1               ESPIVW_INDEX45_1
#define OEM_ESPIVW_EC_TO_PCH_GEN2               ESPIVW_INDEX45_2
#define OEM_ESPIVW_EC_TO_PCH_GEN3               ESPIVW_INDEX45_3

#define OEM_ESPIVW_EC_TO_PCH_GEN4               ESPIVW_INDEX46_0
#define OEM_ESPIVW_EC_TO_PCH_GEN5               ESPIVW_INDEX46_1
#define OEM_ESPIVW_EC_TO_PCH_GEN6               ESPIVW_INDEX46_2
#define OEM_ESPIVW_EC_TO_PCH_GEN7               ESPIVW_INDEX46_3

#define OEM_ESPIVW_HOST_C10                     ESPIVW_INDEX47_0



/**
  \brief  Structure type to access ESPI Virtual Wire (ESPIVW).
 */
typedef volatile struct
{
  uint8_t  ESPIVWIE;                                  /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  ESPIVWPF;                                  /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  ESPIVWEF;                                  /*Error Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint16_t ESPIVWIRQ;                                 /*IRQ Configuration Register */
  uint16_t Reserved3;                                 /*Reserved */  
  uint32_t ESPISVWD;                                  /*System Event Direction Register */        
  uint32_t ESPIVWB10;                                 /*Block1/0 Register */                     
  uint32_t ESPIVWB32;                                 /*Block3/2 Register */     
  uint32_t Reserved4;                                 /*Reserved */
  uint16_t ESPIVWTX;                                  /*Tx Register */
  uint16_t Reserved5;                                 /*Reserved */           
  uint8_t  ESPIVWRXI;                                 /*Rx Index Register */
  uint8_t  ESPIVWRXV;                                 /*Rx Valid Flag Register */        
  uint16_t Reserved6;                                 /*Reserved */   
  uint32_t ESPIVWCNT;                                 /*Counter Value Register */ 
  uint32_t Reserved7[5];                              /*Reserved */  
  uint8_t  ESPIVWTF[32];                              /*Tx Flag Register */          
  uint8_t  ESPIVWRF[32];                              /*Rx Flag Register */
  uint32_t Reserved8[32];                             /*Reserved */ 
  uint8_t  ESPIVWIDX[256];                            /*Index Table Register */                                 
}  ESPIVW_T;

#define espivw                ((ESPIVW_T *) ESPIVW_BASE)             /* ESPIVW Struct       */
//#define ESPIVW                ((volatile unsigned long  *) ESPIVW_BASE)     /* ESPIVW  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define ESPIVW_V_OFFSET                 0x20001164UL
#define SUPPORT_ESPIVW                  1
    #define SUPPORT_ESPIVW_ISR_HANDLE   1
    #define ESPIVW_IRQ_UART_PS          1
    #define ESPIVW_IRQ_USR_PS           1
    #define ESPIVW_IRQ_KBC_PS           1
    #define ESPIVW_IRQ_IDX1_PS          1
    #define ESPIVW_IRQ_IDX0_PS          1
    #define ESPIVW_IRQ_UART_EL          0
    #define ESPIVW_IRQ_USR_EL           0
    #define ESPIVW_IRQ_KBC_EL           0
    #define ESPIVW_IRQ_IDX1_EL          0
    #define ESPIVW_IRQ_IDX0_EL          0
    #define SUPPORT_ESPIVW_B0           1
        #define ESPIVW_B0_RANGE         0x40
        #define ESPIVW_B0_DIRECT        ((USR_ESPIVW_DIR_INDEX47<<7)|(USR_ESPIVW_DIR_INDEX46<<6)|(USR_ESPIVW_DIR_INDEX45<<5)|(USR_ESPIVW_DIR_INDEX44<<4)|(USR_ESPIVW_DIR_INDEX43<<3)|(USR_ESPIVW_DIR_INDEX42<<2)|(USR_ESPIVW_DIR_INDEX41<<1)|USR_ESPIVW_DIR_INDEX40)
    #define SUPPORT_ESPIVW_B1           1
        #define ESPIVW_B1_RANGE         0x48
        #define ESPIVW_B1_DIRECT        ((USR_ESPIVW_DIR_INDEX4F<<7)|(USR_ESPIVW_DIR_INDEX4E<<6)|(USR_ESPIVW_DIR_INDEX4D<<5)|(USR_ESPIVW_DIR_INDEX4C<<4)|(USR_ESPIVW_DIR_INDEX4B<<3)|(USR_ESPIVW_DIR_INDEX4A<<2)|(USR_ESPIVW_DIR_INDEX49<<1)|USR_ESPIVW_DIR_INDEX48)
    #define SUPPORT_ESPIVW_B2           0
        #define ESPIVW_B2_RANGE         0x80
        #define ESPIVW_B2_DIRECT        ((USR_ESPIVW_DIR_INDEX87<<7)|(USR_ESPIVW_DIR_INDEX86<<6)|(USR_ESPIVW_DIR_INDEX85<<5)|(USR_ESPIVW_DIR_INDEX84<<4)|(USR_ESPIVW_DIR_INDEX83<<3)|(USR_ESPIVW_DIR_INDEX82<<2)|(USR_ESPIVW_DIR_INDEX81<<1)|USR_ESPIVW_DIR_INDEX80)
    #define SUPPORT_ESPIVW_B3           0
        #define ESPIVW_B3_RANGE         0x88
        #define ESPIVW_B3_DIRECT        ((USR_ESPIVW_DIR_INDEX8F<<7)|(USR_ESPIVW_DIR_INDEX8E<<6)|(USR_ESPIVW_DIR_INDEX8D<<5)|(USR_ESPIVW_DIR_INDEX8C<<4)|(USR_ESPIVW_DIR_INDEX8B<<3)|(USR_ESPIVW_DIR_INDEX8A<<2)|(USR_ESPIVW_DIR_INDEX89<<1)|USR_ESPIVW_DIR_INDEX88)
//Ex:   ESPIVW_B0_RANGE:  0x70, VW Index 0x70~0x77 is Block0
//      ESPIVW_B0_DIRECT: 0x5C, [0]: 1, VW Index 0x70 is host only  Input(Rx, Host <== Slave)
//                              [1]: 0, VW Index 0x71 is host only Output(Tx, Host ==> Slave)
//                              [2]: 1, VW Index 0x72 is host only  Input(Rx, Host <== Slave)
//                              [3]: 0, VW Index 0x73 is host only Output(Tx, Host ==> Slave)
//                              [4]: 1, VW Index 0x74 is host only  Input(Rx, Host <== Slave)
//                              [5]: 1, VW Index 0x75 is host only  Input(Tx, Host <== Slave)
//                              [6]: 0, VW Index 0x76 is host only Output(Rx, Host ==> Slave)
//                              [7]: 0, VW Index 0x77 is host only Output(Tx, Host ==> Slave)

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define ESPIVW_RESET_HANDLE_TX          1       //0: MOSI no reset, MISO reset and Tx when reset event de-assertion
#define ESPIVW_RESET_HANDLE_NO_TX       0       //1: MOSI reset, MISO reset but do not Tx when reset event assertion
#define ESPIVW_RESET_HANDLE_MODE        ESPIVW_RESET_HANDLE_TX
#define ESPIVW_EMPTY_BLOCK              0xFF
#define ESPIVW_INDEX_WIRE_NUM           4

#define ESPIVW_SYSTEM_INDEX_BEGIN       0x02
#define ESPIVW_SYSTEM_INDEX_END         0x07
#define ESPIVW_BLOCK0_INDEX_BEGIN       0x40
#define ESPIVW_BLOCK0_INDEX_END         0x47
#define ESPIVW_BLOCK1_INDEX_BEGIN       0x48
#define ESPIVW_BLOCK1_INDEX_END         0x4F
#define ESPIVW_BLOCK2_INDEX_BEGIN       0x80
#define ESPIVW_BLOCK2_INDEX_END         0x87
#define ESPIVW_BLOCK3_INDEX_BEGIN       0x88
#define ESPIVW_BLOCK3_INDEX_END         0x8F

#define ESPIVW_IRQ_FALLING_EDGE         0
#define ESPIVW_IRQ_RISING_EDGE          1
#define ESPIVW_IRQ_LOW_LEVEL            2
#define ESPIVW_IRQ_HIGH_LEVEL           3
#define ESPIVW_IRQ_EDGE                 0
#define ESPIVW_IRQ_LEVEL                1
#define ESPIVW_IRQ_HIGH                 1
#define ESPIVW_IRQ_LOW                  0

#define ESPIVW_LEVEL_BIT                0x0F

#define ESPIVW_INT_BIT                  0x30
#define ESPIVW_INT_NONE                 0
#define ESPIVW_INT_VALID                1
#define ESPIVW_INT_LOW                  2
#define ESPIVW_INT_HIGH                 3

#define ESPIVW_SYS_DIRECT               ((ESPIVW_DIR_INDEX07<<7)|(ESPIVW_DIR_INDEX06<<6)|(ESPIVW_DIR_INDEX05<<5)|(ESPIVW_DIR_INDEX04<<4)|(ESPIVW_DIR_INDEX03<<3)|(ESPIVW_DIR_INDEX02<<2)|(ESPIVW_DIR_INDEX01<<1)|(ESPIVW_DIR_INDEX00<<0))
#define ESPIVW_DIR_BIT                  0x40
#define ESPIVW_DIR_MOSI                 0
#define ESPIVW_DIR_MISO                 1
#define ESPIVW_DIR_UNDEF                0xFF

#define ESPIVW_INIT_LOW                 0
#define ESPIVW_INIT_HIGH                1

#define ESPIVW_GPIO_UNDEF               0xFF

#define ESPIVW_SYSTEMEVNET_INDEX        0x00
#define ESPIVW_B0_INTERNAL_INDEX        0x40
#define ESPIVW_B1_INTERNAL_INDEX        0x48
#define ESPIVW_B2_INTERNAL_INDEX        0x80
#define ESPIVW_B3_INTERNAL_INDEX        0x88
// ESPIVW Initial Value
#define ESPIVW_Index0_InitValue         0x00
#define ESPIVW_Index1_InitValue         0x00
#define ESPIVW_Index2_InitValue         ((ESPIVW_INIT_INDEX02_0|(ESPIVW_INIT_INDEX02_1<<1)|(ESPIVW_INIT_INDEX02_2<<2)|(ESPIVW_INIT_INDEX02_3<<3))|0x70)
#define ESPIVW_Index3_InitValue         ((ESPIVW_INIT_INDEX03_0|(ESPIVW_INIT_INDEX03_1<<1)|(ESPIVW_INIT_INDEX03_2<<2)|(ESPIVW_INIT_INDEX03_3<<3))|0x70)
#define ESPIVW_Index4_InitValue         ((ESPIVW_INIT_INDEX04_0|(ESPIVW_INIT_INDEX04_1<<1)|(ESPIVW_INIT_INDEX04_2<<2)|(ESPIVW_INIT_INDEX04_3<<3))|0xD0)
#define ESPIVW_Index5_InitValue         ((ESPIVW_INIT_INDEX05_0|(ESPIVW_INIT_INDEX05_1<<1)|(ESPIVW_INIT_INDEX05_2<<2)|(ESPIVW_INIT_INDEX05_3<<3))|0xF0)
#define ESPIVW_Index6_InitValue         ((ESPIVW_INIT_INDEX06_0|(ESPIVW_INIT_INDEX06_1<<1)|(ESPIVW_INIT_INDEX06_2<<2)|(ESPIVW_INIT_INDEX06_3<<3))|0xF0)
#define ESPIVW_Index7_InitValue         ((ESPIVW_INIT_INDEX07_0|(ESPIVW_INIT_INDEX07_1<<1)|(ESPIVW_INIT_INDEX07_2<<2)|(ESPIVW_INIT_INDEX07_3<<3))|0xF0)

//BIT_8 VW_Index6
#define SCI_Request                     bit0
#define SMI_Request                     bit1
#define RCIN_Request                    bit2

typedef struct _ESPIVW  
{
    unsigned long ServFlag[5];
}_hwESPIVW;

//-- Macro Define -----------------------------------------------
#define mESPIVW_ServFlagClr(bBlkNum, bIndex, bDat)  hwESPIVW.ServFlag[bBlkNum] &= ~((unsigned long)0x0F<<((bIndex&0x07)<<2))
#define mESPIVW_TxBusyCheck(bIndex)                 ((espivw->ESPIVWTF[bIndex>>3]&(0x01<<((bIndex)&0x07)))?TRUE:FALSE)
#define mESPIVW_GetIndex(wVWGPIO)                   ((unsigned short)wVWGPIO>>8)
#define mESPIVW_GetLevel(wVWGPIO)                   (wVWGPIO&ESPIVW_LEVEL_BIT)
#define mESPIVW_GetDIR(wVWGPIO)                     ((wVWGPIO&ESPIVW_DIR_BIT)>>6)
#define mESPIVW_GetINT(wVWGPIO)                     ((wVWGPIO&ESPIVW_INT_BIT)>>4)
#define mESPIVW_IsRxValid                           ((espivw->ESPIVWRXV&0x01)==1)
#define mESPIVW_ClrRxValidFlag                      espivw->ESPIVWRXV = 0x01
#define mESPIVW_ClrRxPendingFlag                    espivw->ESPIVWPF = 0x01
#define mESPIVW_ClrTxPendingFlag                    espivw->ESPIVWPF = 0x02

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if SUPPORT_ESPIVW
extern _hwESPIVW hwESPIVW;
extern BIT_8 VW_Index6;
#endif  //SUPPORT_ESPIVW

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
#if SUPPORT_ESPIVW
void ESPIVW_PowerOn_Init(void);
//void Handle_ESPIVW_ISR(void);
#endif  //SUPPORT_ESPIVW
void EnESrvcESPIVW(void);
void ESPIVW_GPIO_TxValidClean(void);
void ESPIVW_Index06_Check_MainLoop(void);
void ESPIVW_GPIO_eSPI_RST_Internal_Init_ISR(void);
void ESPIVW_Index05_Init(void);

//-- For OEM Use -----------------------------------------------
#if SUPPORT_ESPIVW
void ESPIVW_GPIO_Init(unsigned char, unsigned char);
void ESPIVW_Table_Init(const unsigned char *pTab);
unsigned char ESPIVW_IndexRx(unsigned char bIndex);
unsigned char ESPIVW_IRQ_Transcation(unsigned char bIRQ_Num, unsigned char bType);
unsigned char ESPIVW_GPIO_GetBit(unsigned short wVWGPIO);
void ESPIVW_GPIO_SetIndex(unsigned char bIndex, unsigned char bDat);
void ESPIVW_GPIO_SetBit(unsigned short wVWGPIO, unsigned char bHighLow);
void ESPIVW_GPIO_DPWROK_Internal_Init(void);
#endif  //SUPPORT_ESPIVW


//************************************************************
//ESPIVW Index Direction Configuration
//************************************************************
#define ESPIVW_DIR_INDEX00                  USR_ESPIVW_DIR_INDEX00
#define ESPIVW_DIR_INDEX01                  USR_ESPIVW_DIR_INDEX01
#define ESPIVW_DIR_INDEX02                  USR_ESPIVW_DIR_INDEX02
#define ESPIVW_DIR_INDEX03                  USR_ESPIVW_DIR_INDEX03
#define ESPIVW_DIR_INDEX04                  USR_ESPIVW_DIR_INDEX04
#define ESPIVW_DIR_INDEX05                  USR_ESPIVW_DIR_INDEX05
#define ESPIVW_DIR_INDEX06                  USR_ESPIVW_DIR_INDEX06
#define ESPIVW_DIR_INDEX07                  USR_ESPIVW_DIR_INDEX07

#define ESPIVW_DIR_INDEX40                  USR_ESPIVW_DIR_INDEX40
#define ESPIVW_DIR_INDEX41                  USR_ESPIVW_DIR_INDEX41
#define ESPIVW_DIR_INDEX42                  USR_ESPIVW_DIR_INDEX42
#define ESPIVW_DIR_INDEX43                  USR_ESPIVW_DIR_INDEX43
#define ESPIVW_DIR_INDEX44                  USR_ESPIVW_DIR_INDEX44
#define ESPIVW_DIR_INDEX45                  USR_ESPIVW_DIR_INDEX45
#define ESPIVW_DIR_INDEX46                  USR_ESPIVW_DIR_INDEX46
#define ESPIVW_DIR_INDEX47                  USR_ESPIVW_DIR_INDEX47

#define ESPIVW_DIR_INDEX48                  USR_ESPIVW_DIR_INDEX48
#define ESPIVW_DIR_INDEX49                  USR_ESPIVW_DIR_INDEX49
#define ESPIVW_DIR_INDEX4A                  USR_ESPIVW_DIR_INDEX4A
#define ESPIVW_DIR_INDEX4B                  USR_ESPIVW_DIR_INDEX4B
#define ESPIVW_DIR_INDEX4C                  USR_ESPIVW_DIR_INDEX4C
#define ESPIVW_DIR_INDEX4D                  USR_ESPIVW_DIR_INDEX4D
#define ESPIVW_DIR_INDEX4E                  USR_ESPIVW_DIR_INDEX4E
#define ESPIVW_DIR_INDEX4F                  USR_ESPIVW_DIR_INDEX4F

#define ESPIVW_DIR_INDEX80                  USR_ESPIVW_DIR_INDEX80
#define ESPIVW_DIR_INDEX81                  USR_ESPIVW_DIR_INDEX81
#define ESPIVW_DIR_INDEX82                  USR_ESPIVW_DIR_INDEX82
#define ESPIVW_DIR_INDEX83                  USR_ESPIVW_DIR_INDEX83
#define ESPIVW_DIR_INDEX84                  USR_ESPIVW_DIR_INDEX84
#define ESPIVW_DIR_INDEX85                  USR_ESPIVW_DIR_INDEX85
#define ESPIVW_DIR_INDEX86                  USR_ESPIVW_DIR_INDEX86
#define ESPIVW_DIR_INDEX87                  USR_ESPIVW_DIR_INDEX87

#define ESPIVW_DIR_INDEX88                  USR_ESPIVW_DIR_INDEX88
#define ESPIVW_DIR_INDEX89                  USR_ESPIVW_DIR_INDEX89
#define ESPIVW_DIR_INDEX8A                  USR_ESPIVW_DIR_INDEX8A
#define ESPIVW_DIR_INDEX8B                  USR_ESPIVW_DIR_INDEX8B
#define ESPIVW_DIR_INDEX8C                  USR_ESPIVW_DIR_INDEX8C
#define ESPIVW_DIR_INDEX8D                  USR_ESPIVW_DIR_INDEX8D
#define ESPIVW_DIR_INDEX8E                  USR_ESPIVW_DIR_INDEX8E
#define ESPIVW_DIR_INDEX8F                  USR_ESPIVW_DIR_INDEX8F

//************************************************************
//ESPIVW GPIO Interrupt Configuration
//************************************************************
#define ESPIVW_INT_INDEX02_0                USR_ESPIVW_INT_INDEX02_0
#define ESPIVW_INT_INDEX02_1                USR_ESPIVW_INT_INDEX02_1
#define ESPIVW_INT_INDEX02_2                USR_ESPIVW_INT_INDEX02_2
#define ESPIVW_INT_INDEX02_3                USR_ESPIVW_INT_INDEX02_3

#define ESPIVW_INT_INDEX03_0                USR_ESPIVW_INT_INDEX03_0
#define ESPIVW_INT_INDEX03_1                USR_ESPIVW_INT_INDEX03_1            //PLTRST, Kernel handle it in ISR
#define ESPIVW_INT_INDEX03_2                USR_ESPIVW_INT_INDEX03_2            //OOB_RST_WARN, Kernel handle it in ISR
#define ESPIVW_INT_INDEX03_3                USR_ESPIVW_INT_INDEX03_3

#define ESPIVW_INT_INDEX04_0                USR_ESPIVW_INT_INDEX04_0
#define ESPIVW_INT_INDEX04_1                USR_ESPIVW_INT_INDEX04_1
#define ESPIVW_INT_INDEX04_2                USR_ESPIVW_INT_INDEX04_2
#define ESPIVW_INT_INDEX04_3                USR_ESPIVW_INT_INDEX04_3

#define ESPIVW_INT_INDEX05_0                USR_ESPIVW_INT_INDEX05_0
#define ESPIVW_INT_INDEX05_1                USR_ESPIVW_INT_INDEX05_1
#define ESPIVW_INT_INDEX05_2                USR_ESPIVW_INT_INDEX05_2
#define ESPIVW_INT_INDEX05_3                USR_ESPIVW_INT_INDEX05_3

#define ESPIVW_INT_INDEX06_0                USR_ESPIVW_INT_INDEX06_0
#define ESPIVW_INT_INDEX06_1                USR_ESPIVW_INT_INDEX06_1
#define ESPIVW_INT_INDEX06_2                USR_ESPIVW_INT_INDEX06_2
#define ESPIVW_INT_INDEX06_3                USR_ESPIVW_INT_INDEX06_3

#define ESPIVW_INT_INDEX07_0                USR_ESPIVW_INT_INDEX07_0            //HOST_RST_WARN, Kernel handle it in ISR
#define ESPIVW_INT_INDEX07_1                USR_ESPIVW_INT_INDEX07_1
#define ESPIVW_INT_INDEX07_2                USR_ESPIVW_INT_INDEX07_2
#define ESPIVW_INT_INDEX07_3                USR_ESPIVW_INT_INDEX07_3

#define ESPIVW_INT_INDEX40_0                USR_ESPIVW_INT_INDEX40_0
#define ESPIVW_INT_INDEX40_1                USR_ESPIVW_INT_INDEX40_1
#define ESPIVW_INT_INDEX40_2                USR_ESPIVW_INT_INDEX40_2
#define ESPIVW_INT_INDEX40_3                USR_ESPIVW_INT_INDEX40_3

#define ESPIVW_INT_INDEX41_0                USR_ESPIVW_INT_INDEX41_0
#define ESPIVW_INT_INDEX41_1                USR_ESPIVW_INT_INDEX41_1
#define ESPIVW_INT_INDEX41_2                USR_ESPIVW_INT_INDEX41_2
#define ESPIVW_INT_INDEX41_3                USR_ESPIVW_INT_INDEX41_3

#define ESPIVW_INT_INDEX42_0                USR_ESPIVW_INT_INDEX42_0
#define ESPIVW_INT_INDEX42_1                USR_ESPIVW_INT_INDEX42_1
#define ESPIVW_INT_INDEX42_2                USR_ESPIVW_INT_INDEX42_2
#define ESPIVW_INT_INDEX42_3                USR_ESPIVW_INT_INDEX42_3

#define ESPIVW_INT_INDEX43_0                USR_ESPIVW_INT_INDEX43_0
#define ESPIVW_INT_INDEX43_1                USR_ESPIVW_INT_INDEX43_1
#define ESPIVW_INT_INDEX43_2                USR_ESPIVW_INT_INDEX43_2
#define ESPIVW_INT_INDEX43_3                USR_ESPIVW_INT_INDEX43_3

#define ESPIVW_INT_INDEX44_0                USR_ESPIVW_INT_INDEX44_0
#define ESPIVW_INT_INDEX44_1                USR_ESPIVW_INT_INDEX44_1
#define ESPIVW_INT_INDEX44_2                USR_ESPIVW_INT_INDEX44_2
#define ESPIVW_INT_INDEX44_3                USR_ESPIVW_INT_INDEX44_3

#define ESPIVW_INT_INDEX45_0                USR_ESPIVW_INT_INDEX45_0
#define ESPIVW_INT_INDEX45_1                USR_ESPIVW_INT_INDEX45_1
#define ESPIVW_INT_INDEX45_2                USR_ESPIVW_INT_INDEX45_2
#define ESPIVW_INT_INDEX45_3                USR_ESPIVW_INT_INDEX45_3

#define ESPIVW_INT_INDEX46_0                USR_ESPIVW_INT_INDEX46_0
#define ESPIVW_INT_INDEX46_1                USR_ESPIVW_INT_INDEX46_1
#define ESPIVW_INT_INDEX46_2                USR_ESPIVW_INT_INDEX46_2
#define ESPIVW_INT_INDEX46_3                USR_ESPIVW_INT_INDEX46_3

#define ESPIVW_INT_INDEX47_0                USR_ESPIVW_INT_INDEX47_0
#define ESPIVW_INT_INDEX47_1                USR_ESPIVW_INT_INDEX47_1
#define ESPIVW_INT_INDEX47_2                USR_ESPIVW_INT_INDEX47_2
#define ESPIVW_INT_INDEX47_3                USR_ESPIVW_INT_INDEX47_3

#define ESPIVW_INT_INDEX48_0                USR_ESPIVW_INT_INDEX48_0
#define ESPIVW_INT_INDEX48_1                USR_ESPIVW_INT_INDEX48_1
#define ESPIVW_INT_INDEX48_2                USR_ESPIVW_INT_INDEX48_2
#define ESPIVW_INT_INDEX48_3                USR_ESPIVW_INT_INDEX48_3

#define ESPIVW_INT_INDEX49_0                USR_ESPIVW_INT_INDEX49_0
#define ESPIVW_INT_INDEX49_1                USR_ESPIVW_INT_INDEX49_1
#define ESPIVW_INT_INDEX49_2                USR_ESPIVW_INT_INDEX49_2
#define ESPIVW_INT_INDEX49_3                USR_ESPIVW_INT_INDEX49_3

#define ESPIVW_INT_INDEX4A_0                USR_ESPIVW_INT_INDEX4A_0
#define ESPIVW_INT_INDEX4A_1                USR_ESPIVW_INT_INDEX4A_1
#define ESPIVW_INT_INDEX4A_2                USR_ESPIVW_INT_INDEX4A_2
#define ESPIVW_INT_INDEX4A_3                USR_ESPIVW_INT_INDEX4A_3

#define ESPIVW_INT_INDEX4B_0                USR_ESPIVW_INT_INDEX4B_0
#define ESPIVW_INT_INDEX4B_1                USR_ESPIVW_INT_INDEX4B_1
#define ESPIVW_INT_INDEX4B_2                USR_ESPIVW_INT_INDEX4B_2
#define ESPIVW_INT_INDEX4B_3                USR_ESPIVW_INT_INDEX4B_3

#define ESPIVW_INT_INDEX4C_0                USR_ESPIVW_INT_INDEX4C_0
#define ESPIVW_INT_INDEX4C_1                USR_ESPIVW_INT_INDEX4C_1
#define ESPIVW_INT_INDEX4C_2                USR_ESPIVW_INT_INDEX4C_2
#define ESPIVW_INT_INDEX4C_3                USR_ESPIVW_INT_INDEX4C_3

#define ESPIVW_INT_INDEX4D_0                USR_ESPIVW_INT_INDEX4D_0
#define ESPIVW_INT_INDEX4D_1                USR_ESPIVW_INT_INDEX4D_1
#define ESPIVW_INT_INDEX4D_2                USR_ESPIVW_INT_INDEX4D_2
#define ESPIVW_INT_INDEX4D_3                USR_ESPIVW_INT_INDEX4D_3

#define ESPIVW_INT_INDEX4E_0                USR_ESPIVW_INT_INDEX4E_0
#define ESPIVW_INT_INDEX4E_1                USR_ESPIVW_INT_INDEX4E_1
#define ESPIVW_INT_INDEX4E_2                USR_ESPIVW_INT_INDEX4E_2
#define ESPIVW_INT_INDEX4E_3                USR_ESPIVW_INT_INDEX4E_3

#define ESPIVW_INT_INDEX4F_0                USR_ESPIVW_INT_INDEX4F_0
#define ESPIVW_INT_INDEX4F_1                USR_ESPIVW_INT_INDEX4F_1
#define ESPIVW_INT_INDEX4F_2                USR_ESPIVW_INT_INDEX4F_2
#define ESPIVW_INT_INDEX4F_3                USR_ESPIVW_INT_INDEX4F_3

#define ESPIVW_INT_INDEX80_0                USR_ESPIVW_INT_INDEX80_0
#define ESPIVW_INT_INDEX80_1                USR_ESPIVW_INT_INDEX80_1
#define ESPIVW_INT_INDEX80_2                USR_ESPIVW_INT_INDEX80_2
#define ESPIVW_INT_INDEX80_3                USR_ESPIVW_INT_INDEX80_3

#define ESPIVW_INT_INDEX81_0                USR_ESPIVW_INT_INDEX81_0
#define ESPIVW_INT_INDEX81_1                USR_ESPIVW_INT_INDEX81_1
#define ESPIVW_INT_INDEX81_2                USR_ESPIVW_INT_INDEX81_2
#define ESPIVW_INT_INDEX81_3                USR_ESPIVW_INT_INDEX81_3

#define ESPIVW_INT_INDEX82_0                USR_ESPIVW_INT_INDEX82_0
#define ESPIVW_INT_INDEX82_1                USR_ESPIVW_INT_INDEX82_1
#define ESPIVW_INT_INDEX82_2                USR_ESPIVW_INT_INDEX82_2
#define ESPIVW_INT_INDEX82_3                USR_ESPIVW_INT_INDEX82_3

#define ESPIVW_INT_INDEX83_0                USR_ESPIVW_INT_INDEX83_0
#define ESPIVW_INT_INDEX83_1                USR_ESPIVW_INT_INDEX83_1
#define ESPIVW_INT_INDEX83_2                USR_ESPIVW_INT_INDEX83_2
#define ESPIVW_INT_INDEX83_3                USR_ESPIVW_INT_INDEX83_3

#define ESPIVW_INT_INDEX84_0                USR_ESPIVW_INT_INDEX84_0
#define ESPIVW_INT_INDEX84_1                USR_ESPIVW_INT_INDEX84_1
#define ESPIVW_INT_INDEX84_2                USR_ESPIVW_INT_INDEX84_2
#define ESPIVW_INT_INDEX84_3                USR_ESPIVW_INT_INDEX84_3

#define ESPIVW_INT_INDEX85_0                USR_ESPIVW_INT_INDEX85_0
#define ESPIVW_INT_INDEX85_1                USR_ESPIVW_INT_INDEX85_1
#define ESPIVW_INT_INDEX85_2                USR_ESPIVW_INT_INDEX85_2
#define ESPIVW_INT_INDEX85_3                USR_ESPIVW_INT_INDEX85_3

#define ESPIVW_INT_INDEX86_0                USR_ESPIVW_INT_INDEX86_0
#define ESPIVW_INT_INDEX86_1                USR_ESPIVW_INT_INDEX86_1
#define ESPIVW_INT_INDEX86_2                USR_ESPIVW_INT_INDEX86_2
#define ESPIVW_INT_INDEX86_3                USR_ESPIVW_INT_INDEX86_3

#define ESPIVW_INT_INDEX87_0                USR_ESPIVW_INT_INDEX87_0
#define ESPIVW_INT_INDEX87_1                USR_ESPIVW_INT_INDEX87_1
#define ESPIVW_INT_INDEX87_2                USR_ESPIVW_INT_INDEX87_2
#define ESPIVW_INT_INDEX87_3                USR_ESPIVW_INT_INDEX87_3

#define ESPIVW_INT_INDEX88_0                USR_ESPIVW_INT_INDEX88_0
#define ESPIVW_INT_INDEX88_1                USR_ESPIVW_INT_INDEX88_1
#define ESPIVW_INT_INDEX88_2                USR_ESPIVW_INT_INDEX88_2
#define ESPIVW_INT_INDEX88_3                USR_ESPIVW_INT_INDEX88_3

#define ESPIVW_INT_INDEX89_0                USR_ESPIVW_INT_INDEX89_0
#define ESPIVW_INT_INDEX89_1                USR_ESPIVW_INT_INDEX89_1
#define ESPIVW_INT_INDEX89_2                USR_ESPIVW_INT_INDEX89_2
#define ESPIVW_INT_INDEX89_3                USR_ESPIVW_INT_INDEX89_3

#define ESPIVW_INT_INDEX8A_0                USR_ESPIVW_INT_INDEX8A_0
#define ESPIVW_INT_INDEX8A_1                USR_ESPIVW_INT_INDEX8A_1
#define ESPIVW_INT_INDEX8A_2                USR_ESPIVW_INT_INDEX8A_2
#define ESPIVW_INT_INDEX8A_3                USR_ESPIVW_INT_INDEX8A_3

#define ESPIVW_INT_INDEX8B_0                USR_ESPIVW_INT_INDEX8B_0
#define ESPIVW_INT_INDEX8B_1                USR_ESPIVW_INT_INDEX8B_1
#define ESPIVW_INT_INDEX8B_2                USR_ESPIVW_INT_INDEX8B_2
#define ESPIVW_INT_INDEX8B_3                USR_ESPIVW_INT_INDEX8B_3

#define ESPIVW_INT_INDEX8C_0                USR_ESPIVW_INT_INDEX8C_0
#define ESPIVW_INT_INDEX8C_1                USR_ESPIVW_INT_INDEX8C_1
#define ESPIVW_INT_INDEX8C_2                USR_ESPIVW_INT_INDEX8C_2
#define ESPIVW_INT_INDEX8C_3                USR_ESPIVW_INT_INDEX8C_3

#define ESPIVW_INT_INDEX8D_0                USR_ESPIVW_INT_INDEX8D_0
#define ESPIVW_INT_INDEX8D_1                USR_ESPIVW_INT_INDEX8D_1
#define ESPIVW_INT_INDEX8D_2                USR_ESPIVW_INT_INDEX8D_2
#define ESPIVW_INT_INDEX8D_3                USR_ESPIVW_INT_INDEX8D_3

#define ESPIVW_INT_INDEX8E_0                USR_ESPIVW_INT_INDEX8E_0
#define ESPIVW_INT_INDEX8E_1                USR_ESPIVW_INT_INDEX8E_1
#define ESPIVW_INT_INDEX8E_2                USR_ESPIVW_INT_INDEX8E_2
#define ESPIVW_INT_INDEX8E_3                USR_ESPIVW_INT_INDEX8E_3

#define ESPIVW_INT_INDEX8F_0                USR_ESPIVW_INT_INDEX8F_0
#define ESPIVW_INT_INDEX8F_1                USR_ESPIVW_INT_INDEX8F_1
#define ESPIVW_INT_INDEX8F_2                USR_ESPIVW_INT_INDEX8F_2
#define ESPIVW_INT_INDEX8F_3                USR_ESPIVW_INT_INDEX8F_3

//************************************************************                                                                              
//ESPIVW Index02 GPIO Initial Value                                                                          
//************************************************************                                                                              
#define ESPIVW_INIT_INDEX02_0               USR_ESPIVW_INIT_INDEX02_0
#define ESPIVW_INIT_INDEX02_1               USR_ESPIVW_INIT_INDEX02_1
#define ESPIVW_INIT_INDEX02_2               USR_ESPIVW_INIT_INDEX02_2
#define ESPIVW_INIT_INDEX02_3               USR_ESPIVW_INIT_INDEX02_3
//************************************************************                                        
//ESPIVW Index03 Initial Value                                                                         
//************************************************************                                        
#define ESPIVW_INIT_INDEX03_0               USR_ESPIVW_INIT_INDEX03_0
#define ESPIVW_INIT_INDEX03_1               USR_ESPIVW_INIT_INDEX03_1
#define ESPIVW_INIT_INDEX03_2               USR_ESPIVW_INIT_INDEX03_2
#define ESPIVW_INIT_INDEX03_3               USR_ESPIVW_INIT_INDEX03_3
//************************************************************                                        
//ESPIVW Index04 Initial Value                                                                       
//************************************************************                                        
#define ESPIVW_INIT_INDEX04_0               USR_ESPIVW_INIT_INDEX04_0
#define ESPIVW_INIT_INDEX04_1               USR_ESPIVW_INIT_INDEX04_1
#define ESPIVW_INIT_INDEX04_2               USR_ESPIVW_INIT_INDEX04_2
#define ESPIVW_INIT_INDEX04_3               USR_ESPIVW_INIT_INDEX04_3
//************************************************************                                        
//ESPIVW Index05 Initial Value                                                                
//************************************************************                                        
#define ESPIVW_INIT_INDEX05_0               USR_ESPIVW_INIT_INDEX05_0
#define ESPIVW_INIT_INDEX05_1               USR_ESPIVW_INIT_INDEX05_1
#define ESPIVW_INIT_INDEX05_2               USR_ESPIVW_INIT_INDEX05_2
#define ESPIVW_INIT_INDEX05_3               USR_ESPIVW_INIT_INDEX05_3
//************************************************************                                        
//ESPIVW Index06 Initial Value                                                                         
//************************************************************                                        
#define ESPIVW_INIT_INDEX06_0               USR_ESPIVW_INIT_INDEX06_0
#define ESPIVW_INIT_INDEX06_1               USR_ESPIVW_INIT_INDEX06_1
#define ESPIVW_INIT_INDEX06_2               USR_ESPIVW_INIT_INDEX06_2
#define ESPIVW_INIT_INDEX06_3               USR_ESPIVW_INIT_INDEX06_3
//************************************************************                                        
//ESPIVW Index07 Initial Value                                                                     
//************************************************************                                        
#define ESPIVW_INIT_INDEX07_0               USR_ESPIVW_INIT_INDEX07_0
#define ESPIVW_INIT_INDEX07_1               USR_ESPIVW_INIT_INDEX07_1
#define ESPIVW_INIT_INDEX07_2               USR_ESPIVW_INIT_INDEX07_2
#define ESPIVW_INIT_INDEX07_3               USR_ESPIVW_INIT_INDEX07_3

//************************************************************
//ESPIVW Index02 GPIO Define
//************************************************************
#define ESPIVW_INDEX02_0                    (0x0201|(ESPIVW_DIR_INDEX02<<6)|(ESPIVW_INT_INDEX02_0<<4))
#define ESPIVW_INDEX02_1                    (0x0202|(ESPIVW_DIR_INDEX02<<6)|(ESPIVW_INT_INDEX02_1<<4))
#define ESPIVW_INDEX02_2                    (0x0204|(ESPIVW_DIR_INDEX02<<6)|(ESPIVW_INT_INDEX02_2<<4))
#define ESPIVW_INDEX02_3                    (0x0208|(ESPIVW_DIR_INDEX02<<6)|(ESPIVW_INT_INDEX02_3<<4))
//************************************************************                                       
//ESPIVW Index03 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX03_0                    (0x0301|(ESPIVW_DIR_INDEX03<<6)|(ESPIVW_INT_INDEX03_0<<4))
#define ESPIVW_INDEX03_1                    (0x0302|(ESPIVW_DIR_INDEX03<<6)|(ESPIVW_INT_INDEX03_1<<4))
#define ESPIVW_INDEX03_2                    (0x0304|(ESPIVW_DIR_INDEX03<<6)|(ESPIVW_INT_INDEX03_2<<4))
#define ESPIVW_INDEX03_3                    (0x0308|(ESPIVW_DIR_INDEX03<<6)|(ESPIVW_INT_INDEX03_3<<4))
//************************************************************                                       
//ESPIVW Index04 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX04_0                    (0x0401|(ESPIVW_DIR_INDEX04<<6)|(ESPIVW_INT_INDEX04_0<<4))
#define ESPIVW_INDEX04_1                    (0x0402|(ESPIVW_DIR_INDEX04<<6)|(ESPIVW_INT_INDEX04_1<<4))
#define ESPIVW_INDEX04_2                    (0x0404|(ESPIVW_DIR_INDEX04<<6)|(ESPIVW_INT_INDEX04_2<<4))
#define ESPIVW_INDEX04_3                    (0x0408|(ESPIVW_DIR_INDEX04<<6)|(ESPIVW_INT_INDEX04_3<<4))
//************************************************************                                       
//ESPIVW Index05 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX05_0                    (0x0501|(ESPIVW_DIR_INDEX05<<6)|(ESPIVW_INT_INDEX05_0<<4))
#define ESPIVW_INDEX05_1                    (0x0502|(ESPIVW_DIR_INDEX05<<6)|(ESPIVW_INT_INDEX05_1<<4))
#define ESPIVW_INDEX05_2                    (0x0504|(ESPIVW_DIR_INDEX05<<6)|(ESPIVW_INT_INDEX05_2<<4))
#define ESPIVW_INDEX05_3                    (0x0508|(ESPIVW_DIR_INDEX05<<6)|(ESPIVW_INT_INDEX05_3<<4))
//************************************************************                                       
//ESPIVW Index06 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX06_0                    (0x0601|(ESPIVW_DIR_INDEX06<<6)|(ESPIVW_INT_INDEX06_0<<4))
#define ESPIVW_INDEX06_1                    (0x0602|(ESPIVW_DIR_INDEX06<<6)|(ESPIVW_INT_INDEX06_1<<4))
#define ESPIVW_INDEX06_2                    (0x0604|(ESPIVW_DIR_INDEX06<<6)|(ESPIVW_INT_INDEX06_2<<4))
#define ESPIVW_INDEX06_3                    (0x0608|(ESPIVW_DIR_INDEX06<<6)|(ESPIVW_INT_INDEX06_3<<4))
//************************************************************                                       
//ESPIVW Index07 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX07_0                    (0x0701|(ESPIVW_DIR_INDEX07<<6)|(ESPIVW_INT_INDEX07_0<<4))
#define ESPIVW_INDEX07_1                    (0x0702|(ESPIVW_DIR_INDEX07<<6)|(ESPIVW_INT_INDEX07_1<<4))
#define ESPIVW_INDEX07_2                    (0x0704|(ESPIVW_DIR_INDEX07<<6)|(ESPIVW_INT_INDEX07_2<<4))
#define ESPIVW_INDEX07_3                    (0x0708|(ESPIVW_DIR_INDEX07<<6)|(ESPIVW_INT_INDEX07_3<<4))
//************************************************************                                       
//ESPIVW Index40 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX40_0                    (0x4001|(ESPIVW_DIR_INDEX40<<6)|(ESPIVW_INT_INDEX40_0<<4))
#define ESPIVW_INDEX40_1                    (0x4002|(ESPIVW_DIR_INDEX40<<6)|(ESPIVW_INT_INDEX40_1<<4))
#define ESPIVW_INDEX40_2                    (0x4004|(ESPIVW_DIR_INDEX40<<6)|(ESPIVW_INT_INDEX40_2<<4))
#define ESPIVW_INDEX40_3                    (0x4008|(ESPIVW_DIR_INDEX40<<6)|(ESPIVW_INT_INDEX40_3<<4))
//************************************************************                                       
//ESPIVW Index41 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX41_0                    (0x4101|(ESPIVW_DIR_INDEX41<<6)|(ESPIVW_INT_INDEX41_0<<4))
#define ESPIVW_INDEX41_1                    (0x4102|(ESPIVW_DIR_INDEX41<<6)|(ESPIVW_INT_INDEX41_1<<4))
#define ESPIVW_INDEX41_2                    (0x4104|(ESPIVW_DIR_INDEX41<<6)|(ESPIVW_INT_INDEX41_2<<4))
#define ESPIVW_INDEX41_3                    (0x4108|(ESPIVW_DIR_INDEX41<<6)|(ESPIVW_INT_INDEX41_3<<4))
//************************************************************                                       
//ESPIVW Index42 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX42_0                    (0x4201|(ESPIVW_DIR_INDEX42<<6)|(ESPIVW_INT_INDEX42_0<<4))
#define ESPIVW_INDEX42_1                    (0x4202|(ESPIVW_DIR_INDEX42<<6)|(ESPIVW_INT_INDEX42_1<<4))
#define ESPIVW_INDEX42_2                    (0x4204|(ESPIVW_DIR_INDEX42<<6)|(ESPIVW_INT_INDEX42_2<<4))
#define ESPIVW_INDEX42_3                    (0x4208|(ESPIVW_DIR_INDEX42<<6)|(ESPIVW_INT_INDEX42_3<<4))
//************************************************************                                       
//ESPIVW Index43 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX43_0                    (0x4301|(ESPIVW_DIR_INDEX43<<6)|(ESPIVW_INT_INDEX43_0<<4))
#define ESPIVW_INDEX43_1                    (0x4302|(ESPIVW_DIR_INDEX43<<6)|(ESPIVW_INT_INDEX43_1<<4))
#define ESPIVW_INDEX43_2                    (0x4304|(ESPIVW_DIR_INDEX43<<6)|(ESPIVW_INT_INDEX43_2<<4))
#define ESPIVW_INDEX43_3                    (0x4308|(ESPIVW_DIR_INDEX43<<6)|(ESPIVW_INT_INDEX43_3<<4))
//************************************************************                                       
//ESPIVW Index44 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX44_0                    (0x4401|(ESPIVW_DIR_INDEX44<<6)|(ESPIVW_INT_INDEX44_0<<4))
#define ESPIVW_INDEX44_1                    (0x4402|(ESPIVW_DIR_INDEX44<<6)|(ESPIVW_INT_INDEX44_1<<4))
#define ESPIVW_INDEX44_2                    (0x4404|(ESPIVW_DIR_INDEX44<<6)|(ESPIVW_INT_INDEX44_2<<4))
#define ESPIVW_INDEX44_3                    (0x4408|(ESPIVW_DIR_INDEX44<<6)|(ESPIVW_INT_INDEX44_3<<4))
//************************************************************                                       
//ESPIVW Index45 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX45_0                    (0x4501|(ESPIVW_DIR_INDEX45<<6)|(ESPIVW_INT_INDEX45_0<<4))
#define ESPIVW_INDEX45_1                    (0x4502|(ESPIVW_DIR_INDEX45<<6)|(ESPIVW_INT_INDEX45_1<<4))
#define ESPIVW_INDEX45_2                    (0x4504|(ESPIVW_DIR_INDEX45<<6)|(ESPIVW_INT_INDEX45_2<<4))
#define ESPIVW_INDEX45_3                    (0x4508|(ESPIVW_DIR_INDEX45<<6)|(ESPIVW_INT_INDEX45_3<<4))
//************************************************************                                       
//ESPIVW Index46 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX46_0                    (0x4601|(ESPIVW_DIR_INDEX46<<6)|(ESPIVW_INT_INDEX46_0<<4))
#define ESPIVW_INDEX46_1                    (0x4602|(ESPIVW_DIR_INDEX46<<6)|(ESPIVW_INT_INDEX46_1<<4))
#define ESPIVW_INDEX46_2                    (0x4604|(ESPIVW_DIR_INDEX46<<6)|(ESPIVW_INT_INDEX46_2<<4))
#define ESPIVW_INDEX46_3                    (0x4608|(ESPIVW_DIR_INDEX46<<6)|(ESPIVW_INT_INDEX46_3<<4))
//************************************************************                                       
//ESPIVW Index47 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX47_0                    (0x4701|(ESPIVW_DIR_INDEX47<<6)|(ESPIVW_INT_INDEX47_0<<4))
#define ESPIVW_INDEX47_1                    (0x4702|(ESPIVW_DIR_INDEX47<<6)|(ESPIVW_INT_INDEX47_1<<4))
#define ESPIVW_INDEX47_2                    (0x4704|(ESPIVW_DIR_INDEX47<<6)|(ESPIVW_INT_INDEX47_2<<4))
#define ESPIVW_INDEX47_3                    (0x4708|(ESPIVW_DIR_INDEX47<<6)|(ESPIVW_INT_INDEX47_3<<4))
//************************************************************                                       
//ESPIVW Index48 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX48_0                    (0x4801|(ESPIVW_DIR_INDEX48<<6)|(ESPIVW_INT_INDEX48_0<<4))
#define ESPIVW_INDEX48_1                    (0x4802|(ESPIVW_DIR_INDEX48<<6)|(ESPIVW_INT_INDEX48_1<<4))
#define ESPIVW_INDEX48_2                    (0x4804|(ESPIVW_DIR_INDEX48<<6)|(ESPIVW_INT_INDEX48_2<<4))
#define ESPIVW_INDEX48_3                    (0x4808|(ESPIVW_DIR_INDEX48<<6)|(ESPIVW_INT_INDEX48_3<<4))
//************************************************************                                       
//ESPIVW Index49 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX49_0                    (0x4901|(ESPIVW_DIR_INDEX49<<6)|(ESPIVW_INT_INDEX49_0<<4))
#define ESPIVW_INDEX49_1                    (0x4902|(ESPIVW_DIR_INDEX49<<6)|(ESPIVW_INT_INDEX49_1<<4))
#define ESPIVW_INDEX49_2                    (0x4904|(ESPIVW_DIR_INDEX49<<6)|(ESPIVW_INT_INDEX49_2<<4))
#define ESPIVW_INDEX49_3                    (0x4908|(ESPIVW_DIR_INDEX49<<6)|(ESPIVW_INT_INDEX49_3<<4))
//************************************************************                                       
//ESPIVW Index4A GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX4A_0                    (0x4A01|(ESPIVW_DIR_INDEX4A<<6)|(ESPIVW_INT_INDEX4A_0<<4))
#define ESPIVW_INDEX4A_1                    (0x4A02|(ESPIVW_DIR_INDEX4A<<6)|(ESPIVW_INT_INDEX4A_1<<4))
#define ESPIVW_INDEX4A_2                    (0x4A04|(ESPIVW_DIR_INDEX4A<<6)|(ESPIVW_INT_INDEX4A_2<<4))
#define ESPIVW_INDEX4A_3                    (0x4A08|(ESPIVW_DIR_INDEX4A<<6)|(ESPIVW_INT_INDEX4A_3<<4))
//************************************************************                                       
//ESPIVW Index4B GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX4B_0                    (0x4B01|(ESPIVW_DIR_INDEX4B<<6)|(ESPIVW_INT_INDEX4B_0<<4))
#define ESPIVW_INDEX4B_1                    (0x4B02|(ESPIVW_DIR_INDEX4B<<6)|(ESPIVW_INT_INDEX4B_1<<4))
#define ESPIVW_INDEX4B_2                    (0x4B04|(ESPIVW_DIR_INDEX4B<<6)|(ESPIVW_INT_INDEX4B_2<<4))
#define ESPIVW_INDEX4B_3                    (0x4B08|(ESPIVW_DIR_INDEX4B<<6)|(ESPIVW_INT_INDEX4B_3<<4))
//************************************************************                                       
//ESPIVW Index4C GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX4C_0                    (0x4C01|(ESPIVW_DIR_INDEX4C<<6)|(ESPIVW_INT_INDEX4C_0<<4))
#define ESPIVW_INDEX4C_1                    (0x4C02|(ESPIVW_DIR_INDEX4C<<6)|(ESPIVW_INT_INDEX4C_1<<4))
#define ESPIVW_INDEX4C_2                    (0x4C04|(ESPIVW_DIR_INDEX4C<<6)|(ESPIVW_INT_INDEX4C_2<<4))
#define ESPIVW_INDEX4C_3                    (0x4C08|(ESPIVW_DIR_INDEX4C<<6)|(ESPIVW_INT_INDEX4C_3<<4))
//************************************************************                                       
//ESPIVW Index4D GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX4D_0                    (0x4D01|(ESPIVW_DIR_INDEX4D<<6)|(ESPIVW_INT_INDEX4D_0<<4))
#define ESPIVW_INDEX4D_1                    (0x4D02|(ESPIVW_DIR_INDEX4D<<6)|(ESPIVW_INT_INDEX4D_1<<4))
#define ESPIVW_INDEX4D_2                    (0x4D04|(ESPIVW_DIR_INDEX4D<<6)|(ESPIVW_INT_INDEX4D_2<<4))
#define ESPIVW_INDEX4D_3                    (0x4D08|(ESPIVW_DIR_INDEX4D<<6)|(ESPIVW_INT_INDEX4D_3<<4))
//************************************************************                                       
//ESPIVW Index4E GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX4E_0                    (0x4E01|(ESPIVW_DIR_INDEX4E<<6)|(ESPIVW_INT_INDEX4E_0<<4))
#define ESPIVW_INDEX4E_1                    (0x4E02|(ESPIVW_DIR_INDEX4E<<6)|(ESPIVW_INT_INDEX4E_1<<4))
#define ESPIVW_INDEX4E_2                    (0x4E04|(ESPIVW_DIR_INDEX4E<<6)|(ESPIVW_INT_INDEX4E_2<<4))
#define ESPIVW_INDEX4E_3                    (0x4E08|(ESPIVW_DIR_INDEX4E<<6)|(ESPIVW_INT_INDEX4E_3<<4))
//************************************************************                                       
//ESPIVW Index4F GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX4F_0                    (0x4F01|(ESPIVW_DIR_INDEX4F<<6)|(ESPIVW_INT_INDEX4F_0<<4))
#define ESPIVW_INDEX4F_1                    (0x4F02|(ESPIVW_DIR_INDEX4F<<6)|(ESPIVW_INT_INDEX4F_1<<4))
#define ESPIVW_INDEX4F_2                    (0x4F04|(ESPIVW_DIR_INDEX4F<<6)|(ESPIVW_INT_INDEX4F_2<<4))
#define ESPIVW_INDEX4F_3                    (0x4F08|(ESPIVW_DIR_INDEX4F<<6)|(ESPIVW_INT_INDEX4F_3<<4))
//************************************************************                                       
//ESPIVW Index80 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX80_0                    (0x8001|(ESPIVW_DIR_INDEX80<<6)|(ESPIVW_INT_INDEX80_0<<4))
#define ESPIVW_INDEX80_1                    (0x8002|(ESPIVW_DIR_INDEX80<<6)|(ESPIVW_INT_INDEX80_1<<4))
#define ESPIVW_INDEX80_2                    (0x8004|(ESPIVW_DIR_INDEX80<<6)|(ESPIVW_INT_INDEX80_2<<4))
#define ESPIVW_INDEX80_3                    (0x8008|(ESPIVW_DIR_INDEX80<<6)|(ESPIVW_INT_INDEX80_3<<4))
//************************************************************                                       
//ESPIVW Index81 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX81_0                    (0x8101|(ESPIVW_DIR_INDEX81<<6)|(ESPIVW_INT_INDEX81_0<<4))
#define ESPIVW_INDEX81_1                    (0x8102|(ESPIVW_DIR_INDEX81<<6)|(ESPIVW_INT_INDEX81_1<<4))
#define ESPIVW_INDEX81_2                    (0x8104|(ESPIVW_DIR_INDEX81<<6)|(ESPIVW_INT_INDEX81_2<<4))
#define ESPIVW_INDEX81_3                    (0x8108|(ESPIVW_DIR_INDEX81<<6)|(ESPIVW_INT_INDEX81_3<<4))
//************************************************************                                       
//ESPIVW Index82 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX82_0                    (0x8201|(ESPIVW_DIR_INDEX82<<6)|(ESPIVW_INT_INDEX82_0<<4))
#define ESPIVW_INDEX82_1                    (0x8202|(ESPIVW_DIR_INDEX82<<6)|(ESPIVW_INT_INDEX82_1<<4))
#define ESPIVW_INDEX82_2                    (0x8204|(ESPIVW_DIR_INDEX82<<6)|(ESPIVW_INT_INDEX82_2<<4))
#define ESPIVW_INDEX82_3                    (0x8208|(ESPIVW_DIR_INDEX82<<6)|(ESPIVW_INT_INDEX82_3<<4))
//************************************************************                                      
//ESPIVW Index83 GPIO Define                                                                        
//************************************************************                                      
#define ESPIVW_INDEX83_0                    (0x8301|(ESPIVW_DIR_INDEX83<<6)|(ESPIVW_INT_INDEX83_0<<4))
#define ESPIVW_INDEX83_1                    (0x8302|(ESPIVW_DIR_INDEX83<<6)|(ESPIVW_INT_INDEX83_1<<4))
#define ESPIVW_INDEX83_2                    (0x8304|(ESPIVW_DIR_INDEX83<<6)|(ESPIVW_INT_INDEX83_2<<4))
#define ESPIVW_INDEX83_3                    (0x8308|(ESPIVW_DIR_INDEX83<<6)|(ESPIVW_INT_INDEX83_3<<4))
//************************************************************                                      
//ESPIVW Index84 GPIO Define                                                                        
//************************************************************                                      
#define ESPIVW_INDEX84_0                    (0x8401|(ESPIVW_DIR_INDEX84<<6)|(ESPIVW_INT_INDEX84_0<<4))
#define ESPIVW_INDEX84_1                    (0x8402|(ESPIVW_DIR_INDEX84<<6)|(ESPIVW_INT_INDEX84_1<<4))
#define ESPIVW_INDEX84_2                    (0x8404|(ESPIVW_DIR_INDEX84<<6)|(ESPIVW_INT_INDEX84_2<<4))
#define ESPIVW_INDEX84_3                    (0x8408|(ESPIVW_DIR_INDEX84<<6)|(ESPIVW_INT_INDEX84_3<<4))
//************************************************************                                       
//ESPIVW Index85 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX85_0                    (0x8501|(ESPIVW_DIR_INDEX85<<6)|(ESPIVW_INT_INDEX85_0<<4))
#define ESPIVW_INDEX85_1                    (0x8502|(ESPIVW_DIR_INDEX85<<6)|(ESPIVW_INT_INDEX85_1<<4))
#define ESPIVW_INDEX85_2                    (0x8504|(ESPIVW_DIR_INDEX85<<6)|(ESPIVW_INT_INDEX85_2<<4))
#define ESPIVW_INDEX85_3                    (0x8508|(ESPIVW_DIR_INDEX85<<6)|(ESPIVW_INT_INDEX85_3<<4))
//************************************************************                                       
//ESPIVW Index86 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX86_0                    (0x8601|(ESPIVW_DIR_INDEX86<<6)|(ESPIVW_INT_INDEX86_0<<4))
#define ESPIVW_INDEX86_1                    (0x8602|(ESPIVW_DIR_INDEX86<<6)|(ESPIVW_INT_INDEX86_1<<4))
#define ESPIVW_INDEX86_2                    (0x8604|(ESPIVW_DIR_INDEX86<<6)|(ESPIVW_INT_INDEX86_2<<4))
#define ESPIVW_INDEX86_3                    (0x8608|(ESPIVW_DIR_INDEX86<<6)|(ESPIVW_INT_INDEX86_3<<4))
//************************************************************                                       
//ESPIVW Index87 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX87_0                    (0x8701|(ESPIVW_DIR_INDEX87<<6)|(ESPIVW_INT_INDEX87_0<<4))
#define ESPIVW_INDEX87_1                    (0x8702|(ESPIVW_DIR_INDEX87<<6)|(ESPIVW_INT_INDEX87_1<<4))
#define ESPIVW_INDEX87_2                    (0x8704|(ESPIVW_DIR_INDEX87<<6)|(ESPIVW_INT_INDEX87_2<<4))
#define ESPIVW_INDEX87_3                    (0x8708|(ESPIVW_DIR_INDEX87<<6)|(ESPIVW_INT_INDEX87_3<<4))
//************************************************************                                       
//ESPIVW Index88 GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX88_0                    (0x8801|(ESPIVW_DIR_INDEX88<<6)|(ESPIVW_INT_INDEX88_0<<4))
#define ESPIVW_INDEX88_1                    (0x8802|(ESPIVW_DIR_INDEX88<<6)|(ESPIVW_INT_INDEX88_1<<4))
#define ESPIVW_INDEX88_2                    (0x8804|(ESPIVW_DIR_INDEX88<<6)|(ESPIVW_INT_INDEX88_2<<4))
#define ESPIVW_INDEX88_3                    (0x8808|(ESPIVW_DIR_INDEX88<<6)|(ESPIVW_INT_INDEX88_3<<4))
//************************************************************                                      
//ESPIVW Index89 GPIO Define                                                                        
//************************************************************                                      
#define ESPIVW_INDEX89_0                    (0x8901|(ESPIVW_DIR_INDEX89<<6)|(ESPIVW_INT_INDEX89_0<<4))
#define ESPIVW_INDEX89_1                    (0x8902|(ESPIVW_DIR_INDEX89<<6)|(ESPIVW_INT_INDEX89_1<<4))
#define ESPIVW_INDEX89_2                    (0x8904|(ESPIVW_DIR_INDEX89<<6)|(ESPIVW_INT_INDEX89_2<<4))
#define ESPIVW_INDEX89_3                    (0x8908|(ESPIVW_DIR_INDEX89<<6)|(ESPIVW_INT_INDEX89_3<<4))
//************************************************************                                       
//ESPIVW Index8A GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX8A_0                    (0x8A01|(ESPIVW_DIR_INDEX8A<<6)|(ESPIVW_INT_INDEX8A_0<<4))
#define ESPIVW_INDEX8A_1                    (0x8A02|(ESPIVW_DIR_INDEX8A<<6)|(ESPIVW_INT_INDEX8A_1<<4))
#define ESPIVW_INDEX8A_2                    (0x8A04|(ESPIVW_DIR_INDEX8A<<6)|(ESPIVW_INT_INDEX8A_2<<4))
#define ESPIVW_INDEX8A_3                    (0x8A08|(ESPIVW_DIR_INDEX8A<<6)|(ESPIVW_INT_INDEX8A_3<<4))
//************************************************************                                       
//ESPIVW Index8B GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX8B_0                    (0x8B01|(ESPIVW_DIR_INDEX8B<<6)|(ESPIVW_INT_INDEX8B_0<<4))
#define ESPIVW_INDEX8B_1                    (0x8B02|(ESPIVW_DIR_INDEX8B<<6)|(ESPIVW_INT_INDEX8B_1<<4))
#define ESPIVW_INDEX8B_2                    (0x8B04|(ESPIVW_DIR_INDEX8B<<6)|(ESPIVW_INT_INDEX8B_2<<4))
#define ESPIVW_INDEX8B_3                    (0x8B08|(ESPIVW_DIR_INDEX8B<<6)|(ESPIVW_INT_INDEX8B_3<<4))
//************************************************************                                       
//ESPIVW Index8C GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX8C_0                    (0x8C01|(ESPIVW_DIR_INDEX8C<<6)|(ESPIVW_INT_INDEX8C_0<<4))
#define ESPIVW_INDEX8C_1                    (0x8C02|(ESPIVW_DIR_INDEX8C<<6)|(ESPIVW_INT_INDEX8C_1<<4))
#define ESPIVW_INDEX8C_2                    (0x8C04|(ESPIVW_DIR_INDEX8C<<6)|(ESPIVW_INT_INDEX8C_2<<4))
#define ESPIVW_INDEX8C_3                    (0x8C08|(ESPIVW_DIR_INDEX8C<<6)|(ESPIVW_INT_INDEX8C_3<<4))
//************************************************************                                       
//ESPIVW Index8D GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX8D_0                    (0x8D01|(ESPIVW_DIR_INDEX8D<<6)|(ESPIVW_INT_INDEX8D_0<<4))
#define ESPIVW_INDEX8D_1                    (0x8D02|(ESPIVW_DIR_INDEX8D<<6)|(ESPIVW_INT_INDEX8D_1<<4))
#define ESPIVW_INDEX8D_2                    (0x8D04|(ESPIVW_DIR_INDEX8D<<6)|(ESPIVW_INT_INDEX8D_2<<4))
#define ESPIVW_INDEX8D_3                    (0x8D08|(ESPIVW_DIR_INDEX8D<<6)|(ESPIVW_INT_INDEX8D_3<<4))
//************************************************************                                       
//ESPIVW Index8E GPIO Define                                                                         
//************************************************************                                       
#define ESPIVW_INDEX8E_0                    (0x8E01|(ESPIVW_DIR_INDEX8E<<6)|(ESPIVW_INT_INDEX8E_0<<4))
#define ESPIVW_INDEX8E_1                    (0x8E02|(ESPIVW_DIR_INDEX8E<<6)|(ESPIVW_INT_INDEX8E_1<<4))
#define ESPIVW_INDEX8E_2                    (0x8E04|(ESPIVW_DIR_INDEX8E<<6)|(ESPIVW_INT_INDEX8E_2<<4))
#define ESPIVW_INDEX8E_3                    (0x8E08|(ESPIVW_DIR_INDEX8E<<6)|(ESPIVW_INT_INDEX8E_3<<4))
//************************************************************                                      
//ESPIVW Index8F GPIO Define                                                                        
//************************************************************                                      
#define ESPIVW_INDEX8F_0                    (0x8F01|(ESPIVW_DIR_INDEX8F<<6)|(ESPIVW_INT_INDEX8F_0<<4))
#define ESPIVW_INDEX8F_1                    (0x8F02|(ESPIVW_DIR_INDEX8F<<6)|(ESPIVW_INT_INDEX8F_1<<4))
#define ESPIVW_INDEX8F_2                    (0x8F04|(ESPIVW_DIR_INDEX8F<<6)|(ESPIVW_INT_INDEX8F_2<<4))
#define ESPIVW_INDEX8F_3                    (0x8F08|(ESPIVW_DIR_INDEX8F<<6)|(ESPIVW_INT_INDEX8F_3<<4))

//************************************************************ 
//Kernel ESPIVW GPIO Naming
//************************************************************ 
#define ESPIVW_PLTRST                       ESPIVW_INDEX03_1
#define ESPIVW_OOB_RST_WARN                 ESPIVW_INDEX03_2

#define ESPIVW_OOB_RST_ACK                  ESPIVW_INDEX04_0

#define ESPIVW_SLAVE_BOOT_LOAD_DONE         ESPIVW_INDEX05_0
#define ESPIVW_SLAVE_BOOT_LOAD_STATUS       ESPIVW_INDEX05_3

#define ESPIVW_SCI                          ESPIVW_INDEX06_0
#define ESPIVW_SMI                          ESPIVW_INDEX06_1
#define ESPIVW_RCIN                         ESPIVW_INDEX06_2
#define ESPIVW_HOST_RST_ACK                 ESPIVW_INDEX06_3

#define ESPIVW_HOST_RST_WARN                ESPIVW_INDEX07_0

#endif //ESPIVW_H
