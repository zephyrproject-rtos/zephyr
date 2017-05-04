/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_device_registers.h"
#include "fsl_xcvr.h"
#include "ifr_radio.h"
#include "fsl_os_abstraction.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IFR_RAM             (0)

#if RADIO_IS_GEN_3P0
#define RDINDX              (0x41U) 
#define K3_BASE_INDEX       (0x11U) /* Based for read index */
#else
#define RDRSRC              (0x03U) 
#define KW4x_512_BASE       (0x20000U)
#define KW4x_256_BASE       (0x10000U)
#endif /* RADIO_IS_GEN_3P0 */

#if RADIO_IS_GEN_2P1
#define FTFA    (FTFE)
#endif /* RADIO_IS_GEN_2P1 */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
uint32_t read_another_ifr_word(void);
uint32_t read_first_ifr_word(uint32_t read_addr);

#if RADIO_IS_GEN_3P0
uint64_t read_index_ifr(uint32_t read_addr);
#else
/*! *********************************************************************************
 * @brief  Reads a location in block 1 IFR for use by the radio.
 *
 * This function handles reading IFR data from flash memory for trim loading.
 *
 * @param read_addr the address in the IFR to be read.
 *
 * @details This function wraps both the Gen2 read_resource command and the Gen2.1 and Gen3 read_index
***********************************************************************************/
#if RADIO_IS_GEN_2P1
uint64_t read_resource_ifr(uint32_t read_addr);
#else
uint32_t read_resource_ifr(uint32_t read_addr);
#endif /* RADIO_IS_GEN_2P1 */
#endif /* RADIO_IS_GEN_3P0 */

void store_sw_trim(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries, uint32_t addr, uint32_t data);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint32_t ifr_read_addr;

#if RADIO_IS_GEN_3P0
static uint64_t packed_data_long; /* Storage for 2 32 bit values to be read by read_index */
static uint8_t num_words_avail; /* Number of 32 bit words available in packed_data_long storage */
const uint32_t BLOCK_1_IFR[]=
{
    /* Revised fallback table which should work with untrimmed parts */
    0xABCDFFFEU, /* Version #FFFE indicates default trim values */

    /* Trim table is empty for Gen3 by default */

    /* No TRIM_STATUS in SW fallback array. */
    0xFEED0E0FU /* End of File */
};
#else
#if RADIO_IS_GEN_2P0
const uint32_t BLOCK_1_IFR[]=
{
    /* Revised fallback table which should work with untrimmed parts */
    0xABCDFFFEU, /* Version #FFFE indicates default trim values */

    0x4005912CU, /* RSIM_ANA_TRIM address */
    0x784B0000U, /* RSIM_ANA_TRIM default value */

    /* No TRIM_STATUS in SW fallback array. */
    0xFEED0E0FU /* End of File */
};
#else
static uint64_t packed_data_long; /* Storage for 2 32 bit values to be read by read_index */
static uint8_t num_words_avail; /* Number of 32 bit words available in packed_data_long storage */
const uint32_t BLOCK_1_IFR[]=
{
    /* Revised fallback table which should work with untrimmed parts */
    0xABCDFFFEU, /* Version #FFFE indicates default trim values */

    0x4005912CU, /* RSIM_ANA_TRIM address */
    0x784B0000U, /* RSIM_ANA_TRIM default value */

    /* No TRIM_STATUS in SW fallback array. */
    0xFEED0E0FU /* End of File */
};
#endif /* RADIO_IS_GEN_2P0 */
#endif /* RADIO_IS_GEN_3P0 */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*! *********************************************************************************
 * \brief  Read command for reading the first 32bit word from IFR, encapsulates different 
 *  flash IFR read mechanisms for multiple generations of SOC
 * 
 * \param read_addr flash address
 * 
 * \return 8 bytes of packed data containing radio trims only
 *
***********************************************************************************/
uint32_t read_first_ifr_word(uint32_t read_addr)
{
    ifr_read_addr = read_addr;
    return read_another_ifr_word();
}

/*! *********************************************************************************
 * \brief  Read command for reading additional 32bit words from IFR. Encapsulates multiple IFR read mechanisms.
 * 
 * \param read_addr flash address
 * 
 * \return 8 bytes of packed data containing radio trims only
 *
 * \remarks PRE-CONDITIONS:
 *  The function read_first_ifr_word() must have been called so that the ifr_read_addr variable is setup prior to use.
 *
***********************************************************************************/
uint32_t read_another_ifr_word(void)
{
    uint32_t packed_data;

#if (RADIO_IS_GEN_3P0 || RADIO_IS_GEN_2P1)
    /* Using some static storage and alternating reads to read_index_ifr to replace read_resource_ifr */
    if (num_words_avail == 0)
    {
#if RADIO_IS_GEN_3P0
        packed_data_long = read_index_ifr(ifr_read_addr);
#else /* Use 64 bit return version of read_resource */
        packed_data_long = read_resource_ifr(ifr_read_addr);
#endif /* RADIO_IS_GEN_3P0 */

        num_words_avail = 2;
        ifr_read_addr++; /* Read index addresses increment by 1 */
    }

    packed_data = (uint32_t)(packed_data_long & 0xFFFFFFFF);
    packed_data_long = packed_data_long >> 32;
    num_words_avail--;
#else
    packed_data = read_resource_ifr(ifr_read_addr);
    ifr_read_addr += 4; /* Read resource addresses increment by 4 */
#endif /* (RADIO_IS_GEN_3P0 || RADIO_IS_GEN_2P1) */

    return packed_data;
}

#if RADIO_IS_GEN_3P0
/*! *********************************************************************************
 * \brief  Read command for reading from IFR using RDINDEX command
 * 
 * \param read_addr flash address
 * 
 * \return 8 bytes of packed data containing radio trims only
 *
***********************************************************************************/
uint64_t read_index_ifr(uint32_t read_addr)
{
    uint8_t rdindex = read_addr;
    uint64_t read_data;
    uint8_t i;

    while ((FTFE_FSTAT_CCIF_MASK & FTFE->FSTAT) == 0); /* Wait till CCIF=1 to make sure not interrupting a prior operation */

    if ((FTFE->FSTAT & FTFE_FSTAT_ACCERR_MASK) == FTFE_FSTAT_ACCERR_MASK ) 
    {
        FTFE->FSTAT = (1 << FTFE_FSTAT_ACCERR_SHIFT); /* Write 1 to ACCEER to clear errors */
    }

    FTFE->FCCOB[0] = RDINDX;
    FTFE->FCCOB[1] = rdindex;

    OSA_InterrupDisable();
    FTFE->FSTAT = FTFE_FSTAT_CCIF_MASK;
    while((FTFE_FSTAT_CCIF_MASK & FTFE->FSTAT) == 0); /* Wait till CCIF=1 */
    OSA_InterruptEnable();

    /* Pack read data back into 64 bit type */
    read_data = FTFE->FCCOB[11]; /* MSB goes in first, will be shifted left sequentially */
    for (i = 10; i > 3; i--)
    {
        read_data = read_data << 8;
        read_data |= FTFE->FCCOB[i];
    }

    return read_data;
}
#else

/*! *********************************************************************************
 * \brief  Read command for reading from IFR
 * 
 * \param read_addr flash address
 * 
 * \return packed data containing radio trims only
 *
***********************************************************************************/
#if RADIO_IS_GEN_2P0
uint32_t read_resource_ifr(uint32_t read_addr)
{ 

    uint32_t packed_data;
    uint8_t flash_addr23_16, flash_addr15_8, flash_addr7_0;
    uint32_t read_data31_24, read_data23_16, read_data15_8, read_data7_0;

    flash_addr23_16 = (uint8_t)((read_addr & 0xFF0000) >> 16);
    flash_addr15_8 = (uint8_t)((read_addr & 0x00FF00) >> 8);
    flash_addr7_0 = (uint8_t)(read_addr & 0xFF);

    while ((FTFA_FSTAT_CCIF_MASK & FTFA->FSTAT) == 0); /* Wait till CCIF=1 */

    if ((FTFA->FSTAT & FTFA_FSTAT_ACCERR_MASK) == FTFA_FSTAT_ACCERR_MASK )
    {
        FTFA->FSTAT = (1<<FTFA_FSTAT_ACCERR_SHIFT); /* Write 1 to ACCEER to clear errors */
    }

    FTFA->FCCOB0 = RDRSRC;
    FTFA->FCCOB1 = flash_addr23_16;
    FTFA->FCCOB2 = flash_addr15_8;
    FTFA->FCCOB3 = flash_addr7_0;
    FTFA->FCCOB8 = 0x00;

    OSA_InterruptDisable();
    FTFA->FSTAT = FTFA_FSTAT_CCIF_MASK;
    while ((FTFA_FSTAT_CCIF_MASK & FTFA->FSTAT) == 0); /* Wait till CCIF=1 */
    OSA_InterruptEnable();

    /* Start reading */
    read_data31_24 = FTFA->FCCOB4; /* FTFA->FCCOB[4] */
    read_data23_16 = FTFA->FCCOB5; /* FTFA->FCCOB[5] */
    read_data15_8  = FTFA->FCCOB6; /* FTFA->FCCOB[6] */
    read_data7_0   = FTFA->FCCOB7; /* FTFA->FCCOB[7] */

    packed_data = (read_data31_24 << 24) | (read_data23_16 << 16) | (read_data15_8 << 8) | (read_data7_0 << 0);

    return packed_data;
}
#else
uint64_t read_resource_ifr(uint32_t read_addr)
{ 

    uint64_t packed_data;
    uint8_t flash_addr23_16, flash_addr15_8, flash_addr7_0;
    uint8_t read_data[8];
    uint64_t temp_64;
    uint8_t i;

    flash_addr23_16 = (uint8_t)((read_addr & 0xFF0000) >> 16);
    flash_addr15_8 = (uint8_t)((read_addr & 0x00FF00) >> 8);
    flash_addr7_0 = (uint8_t)(read_addr & 0xFF);
    while((FTFE_FSTAT_CCIF_MASK & FTFE->FSTAT) == 0); /* Wait till CCIF=1 */

    if ((FTFE->FSTAT & FTFE_FSTAT_ACCERR_MASK) == FTFE_FSTAT_ACCERR_MASK )
    {
        FTFE->FSTAT = (1<<FTFE_FSTAT_ACCERR_SHIFT); /* Write 1 to ACCEER to clear errors */
    }

    FTFE->FCCOB0 = RDRSRC;
    FTFE->FCCOB1 = flash_addr23_16;
    FTFE->FCCOB2 = flash_addr15_8;
    FTFE->FCCOB3 = flash_addr7_0;
    FTFE->FCCOB4 = 0x00;

    OSA_InterruptDisable();
    FTFE->FSTAT = FTFE_FSTAT_CCIF_MASK;
    while ((FTFE_FSTAT_CCIF_MASK & FTFE->FSTAT) == 0); /* Wait till CCIF=1 */
    OSA_InterruptEnable();

    /* Start reading */
    read_data[7] = FTFE->FCCOB4;
    read_data[6] = FTFE->FCCOB5;
    read_data[5] = FTFE->FCCOB6;
    read_data[4] = FTFE->FCCOB7;
    read_data[3] = FTFE->FCCOB8;
    read_data[2] = FTFE->FCCOB9;
    read_data[1] = FTFE->FCCOBA;
    read_data[0] = FTFE->FCCOBB;

    packed_data = 0;
    for (i = 0; i < 8; i++)
    {
        temp_64 = read_data[i];
        packed_data |= temp_64 << (i * 8);
    }

    return packed_data;
}

#endif /* RADIO_IS_GEN_2P0 */
#endif /* RADIO_IS_GEN_3P0 */

/*! *********************************************************************************
 * \brief  Store a SW trim value in the table passed in from calling function.
 * 
 * \param sw_trim_tbl pointer to the software trim table to hold SW trim values
 * \param num_entries the number of entries in the SW trim table
 * \param addr the software trim ID
 * \param data the value of the software trim
 *
***********************************************************************************/
void store_sw_trim(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries, uint32_t addr, uint32_t data)
{
    uint16_t i;

    if (sw_trim_tbl != NULL)
    {
        for (i = 0; i < num_entries; i++)
        {
            if (addr == sw_trim_tbl[i].trim_id)
            {
                sw_trim_tbl[i].trim_value = data;
                sw_trim_tbl[i].valid = 1;
                break; /* Don't need to scan the array any further... */
            }
        }
    }
}

/*! *********************************************************************************
 * \brief  Process block 1 IFR data.
 * 
 * \param sw_trim_tbl pointer to the software trim table to hold SW trim values
 * \param num_entries the number of entries in the SW trim table
 *
 * \remarks 
 *  Uses a IFR v2 formatted default array if the IFR is blank or corrupted.
 *  Stores SW trim values to an array passed into this function.
 *
***********************************************************************************/
void handle_ifr(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries)
{
    uint32_t dest_addr;
    uint32_t read_addr;
    uint32_t dest_data;
    uint32_t packed_data;
    uint32_t *ifr_ptr;

#if RADIO_IS_GEN_3P0
    num_words_avail = 0; /* Prep for handling 64 bit words from flash */
#endif /* RADIO_IS_GEN_3P0 */

#if RADIO_IS_GEN_3P0
    read_addr = K3_BASE_INDEX;
#else
#ifdef CPU_MKW41Z256VHT4
    read_addr = KW4x_256_BASE;
#else
    read_addr = KW4x_512_BASE;
#endif /* CPU_MKW41Z256VHT4 */
#endif /* RADIO_IS_GEN_3P0 */

    /* Read first entry in IFR table */
    packed_data = read_first_ifr_word(read_addr);
    if ((packed_data&~IFR_VERSION_MASK) == IFR_VERSION_HDR)
    {
        /* Valid header was found, process real IFR data */
        XCVR_MISC->OVERWRITE_VER = (packed_data & IFR_VERSION_MASK);
        store_sw_trim(sw_trim_tbl, num_entries, 0xABCD, (packed_data & IFR_VERSION_MASK)); /* Place IFR version # in SW trim array*/
        packed_data = read_another_ifr_word();

        while (packed_data !=IFR_EOF_SYMBOL)
        {
            if (IS_A_SW_ID(packed_data)) /* SW Trim case (non_reg writes) */
            {
                dest_addr = packed_data;
                packed_data = read_another_ifr_word();
                dest_data = packed_data;
                /* Place SW trim in array for driver SW to use */
                store_sw_trim(sw_trim_tbl, num_entries, dest_addr, dest_data);
            }
            else
            {
                if (IS_VALID_REG_ADDR(packed_data)) /* Valid register write address */
                {
                    dest_addr = packed_data;
                    packed_data = read_another_ifr_word();
                    dest_data = packed_data;
                    *(uint32_t *)(dest_addr) = dest_data;
                }
                else
                { /* Invalid address case */

                }
            } 

        packed_data=read_another_ifr_word();
        }
    } 
    else 
    {
        /* Valid header is not present, use blind IFR trim table */
        ifr_ptr = (void *)BLOCK_1_IFR;
        packed_data = *ifr_ptr;
        XCVR_MISC->OVERWRITE_VER = (packed_data & IFR_VERSION_MASK);
        store_sw_trim(sw_trim_tbl, num_entries, 0xABCD, (packed_data & IFR_VERSION_MASK)); /* Place IFR version # in SW trim array */
        ifr_ptr++;
        packed_data= *ifr_ptr;

        while (packed_data != IFR_EOF_SYMBOL)
        {
            if (IS_A_SW_ID(packed_data))
            {
                /* SW Trim case (non_reg writes) */
                dest_addr = packed_data;
                ifr_ptr++;
                packed_data = *(ifr_ptr);
                dest_data = packed_data;
                /* Place SW trim in array for driver SW to use */
                store_sw_trim(sw_trim_tbl, num_entries, dest_addr, dest_data);
            }
            else 
            {
                dest_addr = packed_data;
                ifr_ptr++;
                packed_data = *ifr_ptr;
                dest_data = packed_data;

                /* Valid register write address */
                if (IS_VALID_REG_ADDR(dest_addr))
                {
                    *(uint32_t *)(dest_addr) = dest_data;
                }
                else
                {
                    /* Invalid address case */
                }
            }

            ifr_ptr++;
            packed_data= *ifr_ptr;
        }
    }
}

#if RADIO_IS_GEN_3P0

#else
uint32_t handle_ifr_die_id(void)
{
    uint32_t id_x, id_y;
    uint32_t id;

    id = read_resource_ifr(0x90);
    id_x = id & 0x00FF0000;
    id_y = id & 0x000000FF;

    return (id_x | id_y);
}

uint32_t handle_ifr_die_kw_type(void)
{
    uint32_t zb, ble;

    zb = read_resource_ifr(0x80) & 0x8000;
    ble= read_resource_ifr(0x88) & 0x100000;

    return (zb | ble);
}

#endif /* RADIO_IS_GEN_3P0 */

/*! *********************************************************************************
 * \brief  Dumps block 1 IFR data to an array.
 * 
 * \param dump_tbl pointer to the table to hold the dumped IFR values
 * \param num_entries the number of entries to dump
 *
 * \remarks 
 *  Starts at the first address in IFR and dumps sequential entries.
 *
***********************************************************************************/
void dump_ifr(uint32_t * dump_tbl, uint8_t num_entries)
{
#if RADIO_IS_GEN_3P0
    uint32_t ifr_address = 0x20000;
#else
    uint32_t ifr_address = 0x20000;
#endif /* RADIO_IS_GEN_3P0 */
    uint32_t * dump_ptr = dump_tbl;
    uint8_t i;

    *dump_ptr = read_first_ifr_word(ifr_address);
    dump_ptr++;

    for (i = 0; i < num_entries - 1; i++)
    {
        *dump_ptr = read_another_ifr_word();
        dump_ptr++;
    }
}

