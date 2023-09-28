/**************************************************************************//**
 * @file     ISPI.h
 * @brief    Define ISPI's register and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ISPI_H
#define ISPI_H

/**
  \brief  Structure type to access X-Bus Interface (XBI).
 */
typedef volatile struct
{
  uint16_t ISPICFG;                               /*ISPI Configuration Register */
  uint16_t Reserved0;                             /*Reserved */
  uint8_t  ISPISTS;                               /*Status Register */
  uint8_t  Reserved1[3];                          /*Reserved */
  uint32_t ISPIADDR;                              /*Address Register */
  uint32_t ISPICMD;                               /*Command Register */
  uint32_t Reserved2[28];                         /*Reserved */
  uint32_t ISPIIOCTRL;
  uint32_t ISPIIOCTRL1;
  uint32_t Reserved3[2];
  uint32_t ISPIFCTR;
  uint32_t ISPIWDAT;
  uint32_t ISPIRDAT;
  uint32_t Reserved4[25];
  uint8_t  ISPIDAT[256];                          /*ISPI 256-Byte Data Buffer Register */
}  ISPI_T;

//-- Constant Define -------------------------------------------
#define ISPI_MAPPING_BASE_ADDRESS          0x60000000UL

#define OK_PATTERN_B                    0x00
#define OK_PATTERN                      0x0000
#define UPDATE_ERASE_PATTERN_B          0xAA
#define UPDATE_ERASE_PATTERN            0xAAAA
#define UPDATE_NOERASE_PATTERN_B        0xFF
#define UPDATE_NOERASE_PATTERN          0xFFFF

#define ISPI_FLASH_SIZE                     0x80000   // 512KB
#define ISPI_PAGE_SIZE                      0x100
#define ISPI_PAGE_SIZE_LONG             (ISPI_PAGE_SIZE/4)
#define ISPI_PAGE_MASK                  0xFF

//Erase
#define CMD_Sector4K_Erase_Single            0x00000020   //C+A
#define CMD_Block32K_Erase_Single            0x00000052   //C+A
#define CMD_Block64K_Erase_Single            0x000000D8   //C+A
#define CMD_Sector4K_Erase_Increase          0x00000120   //C+A
#define CMD_Block32K_Erase_Increase          0x00000152   //C+A
#define CMD_Block64K_Erase_Increase          0x000001D8   //C+A
#define CMD_ChipErase                        0x00000060   //C

//Program
#define CMD_Page256B_Program_Single          0x01000002   //C+A+W
#define CMD_Page256B_Program_Increase        0x01000102   //C+A+W

//Protect
#define CMD_WriteEnable                      0x00000006   //C
#define CMD_WriteDisable                     0x00000004   //C
#define CMD_WriteVolatileEnable              0x00000050   //C
#define CMD_ReadStatus                       0x00010005   //C+R
#define CMD_ReadStatus2                      0x00010009   //C+R
#define CMD_ReadStatus3                      0x00010035   //
#define CMD_ReadStatus4                      0x00010085   //
#define CMD_ReadConfigure                    0x00010015   //C+R
#define CMD_WriteStatus                      0x00010001   //C+W
#define CMD_WriteStatus2                     0x00010031   //C+W
#define CMD_WriteStatus3                     0x000100C1   //C+W

//Other command
#define CMD_DeepPowerDown                    0x000000B9   //C
#define CMD_ReleasePowerDown                 0x000000AB   //C
#define CMD_ReadIdentification               0x0003009F   //C+R
#define CMD_ReadDeviceID                     0x000100AB   //C+R
#define CMD_ReadUniqueID                     0x0010004B   //C+A+R
#define CMD_ReadSFDPMode                     0x0001005A   //C+A+R
#define CMD_ReadManufactureID                0x00020090   //C+A+

#define HIFRP                   0x80000000
#define I2CD32RP                0x40000000
#define EDI32RP                 0x20000000
#define DMARP                   0x10000000
#define MCURP                   0x08000000

//-- Macro Define -----------------------------------------------
#define mEnable_ISPICMD                 ispi->ISPICFG |= 0x00000001UL
#define mDisable_ISPICMD                ispi->ISPICFG &= ~0x00000001UL
#define mEnable_ISPI_MAPPING            ispi->ISPICFG |= 0x00000002UL
#define mDisable_ISPI_MAPPING           ispi->ISPICFG &= ~0x00000002UL
#define mISPI_Set_DummyCycle(cyc)       ispi->ISPICFG = (ispi->ISPICFG&(~0x000000C0))|(cyc<<6)
#define mISPI_Set_FlashSize(size)       ispi->ISPICFG = (ispi->ISPICFG&(~0x00000300))|(size<<8)

#define mISPI_Check_Busy                (ispi->ISPISTS&0x01)
#define mISPI_Set_Address(addr)         ispi->ISPIADDR = addr
#define mGet_ISPI_Data(num)             (ispi->ISPIDAT[num])
#define mISPI_Write_Data(num,dat)       ispi->ISPIDAT[num] = dat
#define mISPI_CMD_Issue(len,cmd)        ispi->ISPICMD = (len<<16)|cmd


#define FW_1Byte                 0x00
#define FW_2Byte                 0x04
#define FW_4Byte                 0x08
#define mISPI_FW_En                     ispi->ISPIFCTR |= 0x01
#define mISPI_FW_Dis                    ispi->ISPIFCTR &= ~0x01
#define mISPI_FW_CS_Low                 ispi->ISPIFCTR |= 0x02
#define mISPI_FW_CS_High                ispi->ISPIFCTR &= ~0x02
#define mISPI_FW_Byte_Set(ByteNum)      ispi->ISPIFCTR  = (((ispi->ISPIFCTR)&0xF3)|ByteNum)
#define mISPI_FW_Write(dat)             do {ispi->ISPIWDAT=dat; while(mISPI_Check_Busy);} while(0)
#define mISPI_FW_Read                   ispi->ISPIRDAT

#endif //ISPI_H
