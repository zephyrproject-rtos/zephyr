/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
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
#include "ifr_radio.h"
#include "fsl_os_abstraction.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IFR_RAM                 (0)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
const uint32_t BLOCK_1_IFR[]=
{
  /* Revised fallback table which should work with untrimmed parts */
  0xABCDFFFEU, /* Version #FFFE indicates default trim values */
  
  0x4005912CU, /* RSIM_ANA_TRIM address */
  0x784B0000U, /* RSIM_ANA_TRIM default value */
  
  /* No TRIM_STATUS in SW fallback array. */
  0xFEED0E0FU /* End of File */
};

/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
void store_sw_trim(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries, uint32_t addr, uint32_t data);

/*==================================================================================================
                                       GLOBAL FUNCTIONS
==================================================================================================*/
/* /////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief  Read Resource IFR
///
///  Read command for reading from IFR
///
/// \param read_addr flash address
///
/// \return packed data containing radio trims only
///
/// \remarks PRE-CONDITIONS:
///    None.
///
///////////////////////////////////////////////////////////////////////////////////////////////// */
uint32_t read_resource_ifr(uint32_t read_addr)
{ 
  
  uint32_t packed_data;
  uint8_t flash_addr23_16,flash_addr15_8,flash_addr7_0;
  uint32_t read_data31_24,read_data23_16,read_data15_8,read_data7_0;
  
  flash_addr23_16 = (uint8_t)((read_addr & 0xFF0000)>>16);
  flash_addr15_8 = (uint8_t)((read_addr & 0x00FF00)>>8);
  flash_addr7_0 = (uint8_t)(read_addr & 0xFF);

  while((FTFA_FSTAT_CCIF_MASK & FTFA->FSTAT)==0); /* Wait till CCIF=1 */

  if ((FTFA->FSTAT & FTFA_FSTAT_ACCERR_MASK)== FTFA_FSTAT_ACCERR_MASK ) 
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
  while((FTFA_FSTAT_CCIF_MASK & FTFA->FSTAT)==0) /* Wait till CCIF=1 */
  {

  }
  
  OSA_InterruptEnable();
  
  /* Start reading */
  read_data31_24 = FTFA->FCCOB4; /* FTFA->FCCOB[4] */
  read_data23_16 = FTFA->FCCOB5; /* FTFA->FCCOB[5] */
  read_data15_8  = FTFA->FCCOB6; /* FTFA->FCCOB[6] */
  read_data7_0   = FTFA->FCCOB7; /* FTFA->FCCOB[7] */
  
  packed_data = (read_data31_24<<24)|(read_data23_16<<16)|(read_data15_8<<8)|(read_data7_0<<0);
  
  return packed_data;
}

uint32_t read_resource(uint16_t resource_id)
{
  uint32_t ifr_addr;

  /* Return the test arrays of packed bits */    
  switch (resource_id)
  {
  case 0x84:
  #if IFR_RAM
      return 0x4370; /* TZA_CAP_TUNE=0b0100 BBF_CAP_TUNE=4’b0011 BBF_RES_TUNE2=4’b0111 */
  #else
      ifr_addr = read_resource_ifr(0x84);
      return ifr_addr;
  #endif 
    break;
  case 0x98:
  #if IFR_RAM
      return 0x40000000;/* IQMC_GAIN)ADJ=0b10000000000 */
  #else
      ifr_addr = read_resource_ifr(0x98);
      return ifr_addr;
  #endif 
    break;
  case 0x9C:
  #if IFR_RAM
      return 0x37000000;/* BGAP_V Trim = 0b0011 & BGAP_I Trim=0b0111 */
  #else
      ifr_addr = read_resource_ifr(0x9C);
      return ifr_addr;
  #endif
    break;
  case 0x90:
      ifr_addr = read_resource_ifr(0x90);
      return ifr_addr;
      break;
  case 0x80:
      ifr_addr = read_resource_ifr(0x80);
      return ifr_addr;   
      break;
  case 0x88:
      ifr_addr = read_resource_ifr(0x88);
      return ifr_addr;   
      
    break;
  default:
    return 0x12345678;
    break;
  } 
}

/* /////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief  Store a SW trim value in the table passed in from calling function.
///
/// \param sw_trim_tbl pointer to the software trim table to hold SW trim values
/// \param num_entries the number of entries in the SW trim table
/// \param addr the software trim ID
/// \param data the value of the software trim
///
/// \remarks PRE-CONDITIONS:
///    None.
///
///////////////////////////////////////////////////////////////////////////////////////////////// */
void store_sw_trim(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries, uint32_t addr, uint32_t data)
{
  uint16_t i;
  if (sw_trim_tbl)
  {
    for (i=0;i<num_entries;i++)
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

/* /////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief  Process block 1 IFR data.
///
/// \param sw_trim_tbl pointer to the software trim table to hold SW trim values
/// \param num_entries the number of entries in the SW trim table
///
/// \remarks 
///    Uses a IFR v2 formatted default array if the IFR is blank or corrupted.
///    Stores SW trim values to an array passed into this function.
///
///////////////////////////////////////////////////////////////////////////////////////////////// */
void handle_ifr(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries)
{
#ifdef CPU_MKW41Z256VHT4
  uint32_t read_addr = 0x10000;
#else
  uint32_t read_addr = 0x20000;
#endif
  uint32_t dest_addr;
  uint32_t dest_data;
  uint32_t packed_data;
  uint32_t *ifr_ptr;
  
  /* Read first entry in IFR table */
  packed_data = read_resource_ifr(read_addr);
  read_addr+=4;
  if ((packed_data&~IFR_VERSION_MASK) == IFR_VERSION_HDR)
  {
    /* Valid header was found, process real IFR data */
    XCVR_MISC->OVERWRITE_VER = (packed_data & IFR_VERSION_MASK);
    store_sw_trim(sw_trim_tbl, num_entries, 0xABCD, (packed_data & IFR_VERSION_MASK)); /* Place IFR version # in SW trim array*/
    packed_data = read_resource_ifr(read_addr);
    while (packed_data !=IFR_EOF_SYMBOL)
    {
      if(IS_A_SW_ID(packed_data)) /* SW Trim case (non_reg writes) */
      {
        dest_addr = packed_data;
        read_addr+=4;
        packed_data = read_resource_ifr(read_addr);
        dest_data = packed_data;
        /* Place SW trim in array for driver SW to use */
        store_sw_trim(sw_trim_tbl, num_entries, dest_addr, dest_data);
      }
      else
      { /*SH: 4/21/2015 -> Corrected IFR transfer to Radio Regs.*/
        if (IS_VALID_REG_ADDR(packed_data)) /* Valid register write address */
        {
          dest_addr = packed_data;
          read_addr+=4;
          packed_data = read_resource_ifr(read_addr);
          dest_data = packed_data;        
          *(uint32_t *)(dest_addr) = dest_data;
        }
        else
        { /* Invalid address case */
          
        }
      } 
      read_addr+=4;
      packed_data=read_resource_ifr(read_addr);
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
    while(packed_data!=IFR_EOF_SYMBOL)
    {
      if(IS_A_SW_ID(packed_data)){/* SW Trim case (non_reg writes) */
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
       if (IS_VALID_REG_ADDR(dest_addr)) /* Valid register write address */
       {
         *(uint32_t *)(dest_addr) = dest_data;
       }
       else /* Invalid address case */
       {
         
       }
      }
      ifr_ptr++;
      packed_data= *ifr_ptr;
    }
  }
}



uint32_t handle_ifr_die_id(void)
{
  uint32_t id_x,id_y;
  uint32_t id;
    /* id = read_resource(0x90) */
    id = read_resource_ifr(0x90);
    id_x = id&0x00FF0000;
    id_y = id&0x000000FF;
    return (id_x|id_y);
}

uint32_t handle_ifr_die_kw_type(void)
{
  uint32_t zb,ble;
    zb = read_resource_ifr(0x80)&0x8000;
    ble= read_resource_ifr(0x88)&0x100000;

    return (zb|ble);
}

/* /////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief  Dumps block 1 IFR data to an array.
///
/// \param dump_tbl pointer to the table to hold the dumped IFR values
/// \param num_entries the number of entries to dump
///
/// \remarks 
///    Starts at the first address in IFR and dumps sequential entries.
///
///////////////////////////////////////////////////////////////////////////////////////////////// */
void dump_ifr(uint32_t * dump_tbl, uint8_t num_entries)
{
  uint32_t ifr_address = 0x20000;
  uint32_t * dump_ptr = dump_tbl;
  uint8_t i;
  
  for (i=0;i<num_entries;i++)
  {
    *dump_ptr = read_resource_ifr(ifr_address);
    dump_ptr++;
    ifr_address += 4;
  }
}
