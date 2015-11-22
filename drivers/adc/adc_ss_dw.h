/* ==========================================================================
* Synopsys DesignWare Sensor and Control IP Subsystem IO Software Driver and
* documentation (hereinafter, "Software") is an Unsupported proprietary work
* of Synopsys, Inc. unless otherwise expressly agreed to in writing between
* Synopsys and you.
*
* The Software IS NOT an item of Licensed Software or Licensed Product under
* any End User Software License Agreement or Agreement for Licensed Product
* with Synopsys or any supplement thereto. You are permitted to use and
* redistribute this Software in source and binary forms, with or without
* modification, provided that redistributions of source code must retain this
* notice. You may not view, use, disclose, copy or distribute this file or
* any information contained herein except pursuant to this license grant from
* Synopsys. If you do not agree with this notice, including the disclaimer
* below, then you are not authorized to use the Software.
*
* THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
* ========================================================================== */

/**
 * @file
 *
 * @brief Designware ADC. Synopsys header file.
 */

#ifndef SS_DW_ADC_H_
#define SS_DW_ADC_H_

/**
 *
 * @brief
 *
 * Interruption service routine for data reception
 *
 * This routive service the hardware interruption
 * for raw data sampling and reception for the
 * Designware IP block for ADC.
 */
extern void adc_dw_rx_isr(void *arg);

/**
 *
 * @brief
 *
 * Interruption service routine for error reception
 *
 * This routine service the hardware interruption
 * for error signaled by the  Designware IP block
 * for ADC after a data read or sampling request.
 */
extern void adc_dw_err_isr(void *arg);

#endif /* SS_DW_ADC_H_ */
