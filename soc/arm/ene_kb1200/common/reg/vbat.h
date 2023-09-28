/**************************************************************************//**
 * @file     vbat.h
 * @brief    Define VBAT module's function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef VBAT_H
#define VBAT_H

/**
  \brief  Structure type to access VCC0 Power Domain Module (VCC0).
 */
typedef struct
{
  uint8_t  BKUPSTS;                                   /*VBAT Battery-Backed Status Register */
  uint8_t  PASCR[127];                                /*VBAT Scratch Register */
  uint8_t  INTRCTR;                                   /*VBAT INTRUSION# Control register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t Reserved2[7];                              /*Reserved */
  uint8_t  MTCCFG;                                    /*Monotonic Counter Configure register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  MTCIE;                                     /*Monotonic Counter Interrupt register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  MTCPF;                                     /*Monotonic Counter Pending Flag register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint32_t MTCMAT;                                    /*Monotonic Counter Match register */
  uint32_t MTCCNT;                                    /*Monotonic Counter Value register */
} VBAT_T;


#define SUPPORT_MTC                         USR_SUPPORT_MTC

//-- Macro Define -----------------------------------------------
#define mGet_VCCFail_Flag                   (vbat->BKUPSTS&0x01)
#define mClear_VCCFail_Flag                 vbat->BKUPSTS = 0x01
#define mGet_VCC0Fail_Flag                  (vbat->BKUPSTS&0x02)
#define mClear_VCC0Fail_Flag                vbat->BKUPSTS = 0x02
#define mGet_IntruderAlarm_Flag             (vbat->BKUPSTS&0x40)
#define mClear_IntruderAlarm_Flag           vbat->BKUPSTS = 0x40
#define mGet_InvalidVBATRAM_Flag            (vbat->BKUPSTS&0x80)
#define mClear_InvalidVBATRAM_Flag          vbat->BKUPSTS = 0x80

#define mWrite_VBAT_Scratch_B(num,dat)      do { vbat->PASCR[num] = dat; } while (0)
#define mRead_VBAT_Scratch_B(num)           (vbat->PASCR[num])

#define mWrite_VBAT_Scratch(num,dat)        do { VBAT[num] = dat; } while (0)
#define mRead_VBAT_Scratch(num)             (VBAT[num])

#define mIntrusion_Detect_Enable()          do { vbat->INTRCTR |= 0x01; } while (0)
#define mIntrusion_Detect_Disable()         do { vbat->INTRCTR &=~0x01; } while (0)
#define mIntrusion_Function_Lock()          do { vbat->INTRCTR |= 0x02; } while (0)

#define mMTC_Enable                         vbat->MTCCFG |= 0x01
#define mMTC_Disable                        vbat->MTCCFG &= ~0x01
#define mMTC_Interrupt_Enable               vbat->MTCIE |= 0x01
#define mMTC_Interrupt_Disable              vbat->MTCIE &= ~0x01
#define mGet_MTC_PF                         (vbat->MTCPF&0x01)
#define mClear_MTC_PF                       vbat->MTCPF = 0x01
#define mMTC_MatchValue_Set(mv)             vbat->MTCMAT = mv
#define mMTC_CountValue_Get                 vbat->MTCCNT
#define mMTC_CountValue_Set(cv)             vbat->MTCCNT = cv
#define mMTC_SecToCnt(Sec)                  (Sec)

#define mMTC_CountValue_Reset               do { mMTC_Disable;mMTC_Enable; } while (0)

#endif //VBAT_H
