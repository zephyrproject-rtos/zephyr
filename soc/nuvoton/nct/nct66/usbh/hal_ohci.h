#ifndef __HAL_OHCI_H__
#define __HAL_OHCI_H__

/**
  * @brief This function set the value to register of HcControl.
  * @param[in] val is the value which will be set to register of HcControl.
  * @return none
  */
void halUSBH_Set_HcControl(uint32_t val);
/**
  * @brief This function returns the value in register of HcControl.
  * @return HcControl value.
  */
uint32_t halUSBH_Get_HcControl(void);

/**
  * @brief This function set the value to register of HcCommandStatus.
  * @param[in] val is the value which will be set to register of HcCommandStatus.
  * @return none
  */
void halUSBH_Set_HcCommandStatus(uint32_t val);
/**
  * @brief This function returns the value in register of HcCommandStatus.
  * @return HcCommandStatus value.
  */
uint32_t halUSBH_Get_HcCommandStatus(void);

/**
  * @brief This function write 1 clear the register of HcInterruptStatus.
  * @param[in] mask is the value which will be clear to register of HcInterruptStatus.
  * @return none
  */
void halUSBH_Set_HcInterruptStatus(uint32_t mask);
/**
  * @brief This function returns the value in register of HcInterruptStatus.
  * @return HcInterruptStatus value.
  */
uint32_t halUSBH_Get_HcInterruptStatus(void);

/**
  * @brief This function set the value to register of HcInterruptEnable.
  * @param[in] mask is the value which will be set to register of HcInterruptEnable.
  * @return none
  */
void halUSBH_Set_HcInterruptEnable(uint32_t mask);

/**
  * @brief This function set the value to register of HcInterruptDisable.
  * @param[in] mask is the value which will be set to register of HcInterruptDisable.
  * @return none
  */
void halUSBH_Set_HcInterruptDisable(uint32_t mask);

/**
  * @brief This function set the value to register of HcHCCA.
  * @param[in] hcca is the value which will be set to register of HcHCCA.
  * @return none
  */
void halUSBH_Set_HcHCCA(uint32_t hcca);

/**
  * @brief This function set the value to register of HcControlHeadED.
  * @param[in] ed is the value which will be set to register of HcControlHeadED.
  * @return none
  */
void halUSBH_Set_HcControlHeadED(uint32_t ed);

/**
  * @brief This function set the value to register of HcControlCurrentED.
  * @param[in] ed is the value which will be set to register of HcControlCurrentED.
  * @return none
  */
void halUSBH_Set_HcControlCurrentED(uint32_t ed);

/**
  * @brief This function set the value to register of HcBulkHeadED.
  * @param[in] ed is the value which will be set to register of HcBulkHeadED.
  * @return none
  */
void halUSBH_Set_HcBulkHeadED(uint32_t ed);

/**
  * @brief This function set the value to register of HcBulkCurrentED.
  * @param[in] ed is the value which will be set to register of HcBulkCurrentED.
  * @return none
  */
void halUSBH_Set_HcBulkCurrentED(uint32_t ed);

/**
  * @brief This function set the value to register of HcFmInterval.
  * @param[in] val is the value which will be set to register of HcFmInterval.
  * @return none
  */
void halUSBH_Set_HcFmInterval(uint32_t val);

/**
  * @brief This function returns the value in register of HcFmNumber.
  * @return HcFmNumber value.
  */
uint32_t halUSBH_Get_HcFmNumber(void);

/**
  * @brief This function set the value to register of HcPeriodicStart.
  * @param[in] val is the value which will be set to register of HcPeriodicStart.
  * @return none
  */
void halUSBH_Set_HcPeriodicStart(uint32_t val);

/**
  * @brief This function set the value to register of HcLSThreshold.
  * @param[in] val is the value which will be set to register of HcLSThreshold.
  * @return none
  */
void halUSBH_Set_HcLSThreshold(uint32_t val);

/**
  * @brief This function returns the value in register of HcRhDescriptorA.
  * @return HcRhDescriptorA value.
  */
uint32_t halUSBH_Get_HcRhDescriptorA(void);

/**
  * @brief This function returns the value in register of PowerOnToPowerGoodTime.
  * @return PowerOnToPowerGoodTime value.
  */
uint32_t halUSBH_Get_PowerOnToPowerGoodTime(void);

/**
  * @brief This function returns the value in register of HcRhDescriptorB.
  * @return HcRhDescriptorB value.
  */
uint32_t halUSBH_Get_HcRhDescriptorB(void);

/**
  * @brief This function set the value to register of HcRhStatus.
  * @param[in] val is the value which will be set to register of HcRhStatus.
  * @return none
  */
void halUSBH_Set_HcRhStatus(uint32_t val);
/**
  * @brief This function returns the value in register of HcRhStatus.
  * @return HcRhStatus value.
  */
uint32_t halUSBH_Get_HcRhStatus(void);

/**
  * @brief This function set the value to register of HcRhPortStatus.
  * @param[in] wIndex is the index of HcRhPortStatus.
  * @param[in] val is the value which will be set to register of HcRhPortStatus.
  * @return none
  */
void halUSBH_Set_HcRhPortStatus(uint16_t wIndex, uint32_t val);
/**
  * @brief This function returns the value in register of HcRhPortStatus.
  * @param[in] wIndex is the index of HcRhPortStatus.
  * @return HcRhPortStatus value.
  */
uint32_t halUSBH_Get_HcRhPortStatus(uint16_t wIndex);


/**
  * @brief This function open USB host.
  */
void halUSBH_Open(void);

/**
  * @brief This function close USB host.
  */
void halUSBH_Close(void);

/**
  * @brief This function suspend all root hub port.
  */
void halUSBH_SuspendAllRhPort(void);

/**
  * @brief This function suspend host control.
  */
void halUSBH_SuspendHostControl(void);

/**
  * @brief This function resume all root hub port.
  */
void halUSBH_ResumeAllRhPort(void);

/**
  * @brief This function resume host control.
  */
void halUSBH_ResumeHostControl(void);

/**
  * @brief This function enable remote wakup.
  */
void halUSBH_RemoteWkup_EN(void);

/**
  * @brief This function initial usb basic setting.
  */
void halUSBH_InitialSetting(void);

/**
  * @brief This function initial root hub basic setting.
  */
void halUSBH_RootHubInit(void);

#endif //__HAL_OHCI_H__

