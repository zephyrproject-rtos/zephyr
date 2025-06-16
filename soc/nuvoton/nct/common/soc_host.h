/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NCT_SOC_HOST_H_
#define _NUVOTON_NCT_SOC_HOST_H_

#ifdef __cplusplus
extern "C" {
#endif

/** MSWC API **/
void host_mswc_WriteReg(uint8_t index, uint8_t val);
uint8_t host_mswc_ReadReg(uint8_t index);

/** BBRAM API **/
uint32_t host_bbram_GetBBramSpaceAdr(void);
bool host_bbram_BKUPSTS_IsSet(uint8_t mask);
void host_bbram_BKUPSTS_Clear(uint8_t mask);

/** SHM API **/
void host_shm_SetWinBaseAddr(uint8_t win, volatile uint8_t* addr);
uint32_t host_shm_GetWrProtect(uint8_t win);
void host_shm_SetWrProtect(uint8_t win, uint8_t val);
void host_shm_SetRdProtect(uint8_t win, uint8_t val);
void host_shm_SetOffset(uint8_t win, uint16_t offset);
bool host_shm_IsRdOffsetIE(uint8_t win);
bool host_shm_IsWrOffsetIE(uint8_t win);
void host_shm_ClrRdOffsetSts(uint8_t win);
void host_shm_ClrWrOffsetSts(uint8_t win);
void host_shm_EnableSemaphore(uint8_t flags);
void host_shm_SetHostSemaphore(uint8_t win, uint8_t val);
uint8_t host_shm_GetHostSemaphore(uint8_t win);
bool host_shm_IsHostSemIE(uint8_t win);
bool host_shm_IsHostSemEnable(uint8_t win);
void host_shm_ClrHostSemSts(uint8_t win);
void host_shm_SetWinSize(uint32_t win, uint8_t size);
void host_shm_EnableOffsetInterrupt(uint8_t win, uint8_t flags);
void host_shm_EnableSemaphoreIE(uint8_t win);
void host_shm_DisableSemaphoreIE(uint8_t win);
void host_shm_AddCBtoShmISR(void * CB);
// port80
void host_shm_SetP80Ctrl(uint8_t val);
bool host_shm_IsP80STS(uint8_t val);
uint32_t host_shm_GetP80Buf(uint8_t Buf);


/** C2H(SIB) API **/
void host_c2h_write_reg(uint8_t c2h_device, uint8_t reg_index, uint8_t reg_data);
uint8_t host_c2h_read_reg(uint8_t c2h_device, uint8_t reg_index);

//PMCH
bool host_pmch_is_obf(uint8_t pmch);
void host_pmch_ibf_int_enable(uint8_t pmch);
bool host_pmch_is_ibf(uint8_t pmch);
void host_pmch_write_data(uint8_t pmch, uint8_t data);
void host_pmch_write_data_with_smi(uint8_t pmch, uint8_t data);
uint8_t host_pmch_read_data(uint8_t pmch);
uint8_t host_pmch_shadow_read_data(uint8_t pmch);
bool host_pmch_is_rcv_cmd(uint8_t pmch);
void host_pmch_manual_hw_sci_enable(uint8_t pmch);
void host_pmch_auto_hw_sci_enable(uint8_t pmch);
void host_pmch_set_sci_mode(uint8_t pmch, uint8_t type);
void host_pmch_gen_sci_on_ibf_start(uint8_t pmch);
void host_pmch_gen_sci_manually(uint8_t pmch);
void host_pmch_write_data_with_sci(uint8_t pmch, uint8_t data);
uint8_t host_pmch_read_data_with_sci(uint8_t pmch);
uint8_t host_pmch_get_st(uint8_t pmch);
void host_pmch_set_st(uint8_t pmch, uint8_t msk);
void host_pmch_clr_st(uint8_t pmch, uint8_t msk);
void host_pmch_set_enhance_mode(uint8_t pmch);
void host_pmch_ibf_irp_enable(void);
void host_pmch_ibf_irp_disable(void);
void host_pmch_AddCBtopmchibfISR(void* CB);

/**
 * @brief Initializes all host sub-modules in Core domain.
 *
 * This routine initalizes all host sub-modules which HW blocks belong to
 * Core domain. And it also saves the pointer of eSPI callback list to report
 * any peripheral events application layer.
 *
 * @param dev Pointer to the device structure for the host bus driver instance.
 * @param callbacks A pointer to the list of espi callback functions.
 *
 * @retval 0 If successful.
 * @retval -EIO if cannot turn on host sub-module source clocks in core domain.
 */
int nct_host_init_subs_core_domain(const struct device *host_bus_dev,
							sys_slist_t *callbacks);

/**
 * @brief Initializes all host sub-modules in Host domain.
 *
 * This routine initalizes all host sub-modules which HW blocks belong to
 * Host domain. Please notcie it must be executed after receiving PLT_RST
 * de-asserted signal and eSPI peripheral channel is enabled and ready.
 */
void nct_host_init_subs_host_domain(void);

/**
 * @brief Reads data from a host sub-module which is updated via eSPI.
 *
 * This routine provides a generic interface to read a host sub-module which
 * information was updated by an eSPI transaction through peripheral channel.
 *
 * @param op Enum representing opcode for peripheral type and read request.
 * @param data Parameter to be read from to the host sub-module.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI peripheral is off or not supported.
 * @retval -EINVAL for unimplemented lpc opcode, but in range.
 */
int nct_host_periph_read_request(enum lpc_peripheral_opcode op,
								uint32_t *data);

/**
 * @brief Writes data to a host sub-module which generates an eSPI transaction.
 *
 * This routine provides a generic interface to write data to a host sub-module
 * which triggers an eSPI transaction through peripheral channel.
 *
 * @param op Enum representing an opcode for peripheral type and write request.
 * @param data Represents the parameter passed to the host sub-module.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI peripheral is off or not supported.
 * @retval -EINVAL for unimplemented lpc opcode, but in range.
 */
int nct_host_periph_write_request(enum lpc_peripheral_opcode op,
							const uint32_t *data);

/**
 * @brief Enable host access wake-up interrupt. Usually, it is used to wake up
 * ec during system is in Modern standby power mode.
 */
void nct_host_enable_access_interrupt(void);

/**
 * @brief Disable host access wake-up interrupt.
 */
void nct_host_disable_access_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NCT_SOC_HOST_H_ */
