/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_PINMUX_H__
#define __QM_PINMUX_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Pin muxing configuration.
 *
 * @defgroup groupPinMux Pin Muxing setup
 * @{
 */

/**
 * Pin function type.
 */
typedef enum {
	QM_PMUX_FN_0, /**< Gpio function 0. */
	QM_PMUX_FN_1, /**< Gpio function 0. */
	QM_PMUX_FN_2, /**< Gpio function 0. */
	QM_PMUX_FN_3, /**< Gpio function 0. */
} qm_pmux_fn_t;

/**
 * Pin slew rate setting.
 */
typedef enum {
#if (QUARK_SE)
	QM_PMUX_SLEW_2MA, /**< Set gpio slew rate to 2MA. */
	QM_PMUX_SLEW_4MA, /**< Set gpio slew rate to 4MA. */
#else
	QM_PMUX_SLEW_12MA, /**< Set gpio slew rate to 12MA. */
	QM_PMUX_SLEW_16MA, /**< Set gpio slew rate to 16MA. */
#endif
	QM_PMUX_SLEW_NUM /**< Max number of slew rate options. */
} qm_pmux_slew_t;

/**
 * External Pad pin identifiers
 **/
typedef enum {
	QM_PIN_ID_0,  /**< Pin id 0. */
	QM_PIN_ID_1,  /**< Pin id 1. */
	QM_PIN_ID_2,  /**< Pin id 2. */
	QM_PIN_ID_3,  /**< Pin id 3. */
	QM_PIN_ID_4,  /**< Pin id 4. */
	QM_PIN_ID_5,  /**< Pin id 5. */
	QM_PIN_ID_6,  /**< Pin id 6. */
	QM_PIN_ID_7,  /**< Pin id 7. */
	QM_PIN_ID_8,  /**< Pin id 8. */
	QM_PIN_ID_9,  /**< Pin id 9. */
	QM_PIN_ID_10, /**< Pin id 10. */
	QM_PIN_ID_11, /**< Pin id 11. */
	QM_PIN_ID_12, /**< Pin id 12. */
	QM_PIN_ID_13, /**< Pin id 13. */
	QM_PIN_ID_14, /**< Pin id 14. */
	QM_PIN_ID_15, /**< Pin id 15. */
	QM_PIN_ID_16, /**< Pin id 16. */
	QM_PIN_ID_17, /**< Pin id 17. */
	QM_PIN_ID_18, /**< Pin id 18. */
	QM_PIN_ID_19, /**< Pin id 19. */
	QM_PIN_ID_20, /**< Pin id 20. */
	QM_PIN_ID_21, /**< Pin id 21. */
	QM_PIN_ID_22, /**< Pin id 22. */
	QM_PIN_ID_23, /**< Pin id 23. */
	QM_PIN_ID_24, /**< Pin id 24. */
#if (QUARK_SE)
	QM_PIN_ID_25, /**< Pin id 25. */
	QM_PIN_ID_26, /**< Pin id 26. */
	QM_PIN_ID_27, /**< Pin id 27. */
	QM_PIN_ID_28, /**< Pin id 28. */
	QM_PIN_ID_29, /**< Pin id 29. */
	QM_PIN_ID_30, /**< Pin id 30. */
	QM_PIN_ID_31, /**< Pin id 31. */
	QM_PIN_ID_32, /**< Pin id 32. */
	QM_PIN_ID_33, /**< Pin id 33. */
	QM_PIN_ID_34, /**< Pin id 34. */
	QM_PIN_ID_35, /**< Pin id 35. */
	QM_PIN_ID_36, /**< Pin id 36. */
	QM_PIN_ID_37, /**< Pin id 37. */
	QM_PIN_ID_38, /**< Pin id 38. */
	QM_PIN_ID_39, /**< Pin id 39. */
	QM_PIN_ID_40, /**< Pin id 40. */
	QM_PIN_ID_41, /**< Pin id 41. */
	QM_PIN_ID_42, /**< Pin id 42. */
	QM_PIN_ID_43, /**< Pin id 43. */
	QM_PIN_ID_44, /**< Pin id 44. */
	QM_PIN_ID_45, /**< Pin id 45. */
	QM_PIN_ID_46, /**< Pin id 46. */
	QM_PIN_ID_47, /**< Pin id 47. */
	QM_PIN_ID_48, /**< Pin id 48. */
	QM_PIN_ID_49, /**< Pin id 49. */
	QM_PIN_ID_50, /**< Pin id 50. */
	QM_PIN_ID_51, /**< Pin id 51. */
	QM_PIN_ID_52, /**< Pin id 52. */
	QM_PIN_ID_53, /**< Pin id 53. */
	QM_PIN_ID_54, /**< Pin id 54. */
	QM_PIN_ID_55, /**< Pin id 55. */
	QM_PIN_ID_56, /**< Pin id 56. */
	QM_PIN_ID_57, /**< Pin id 57. */
	QM_PIN_ID_58, /**< Pin id 58. */
	QM_PIN_ID_59, /**< Pin id 59. */
	QM_PIN_ID_60, /**< Pin id 60. */
	QM_PIN_ID_61, /**< Pin id 61. */
	QM_PIN_ID_62, /**< Pin id 62. */
	QM_PIN_ID_63, /**< Pin id 63. */
	QM_PIN_ID_64, /**< Pin id 64. */
	QM_PIN_ID_65, /**< Pin id 65. */
	QM_PIN_ID_66, /**< Pin id 66. */
	QM_PIN_ID_67, /**< Pin id 67. */
	QM_PIN_ID_68, /**< Pin id 68. */
#endif
	QM_PIN_ID_NUM
} qm_pin_id_t;

/**
 * Set up pin muxing for a SoC pin.  Select one of the pin functions.
 *
 * @param[in] pin which pin to configure.
 * @param[in] fn the function to assign to the pin.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 * */
int qm_pmux_select(const qm_pin_id_t pin, const qm_pmux_fn_t fn);

/**
 * Set up pin's slew rate in the pin mux controller.
 *
 * @param[in] pin which pin to configure.
 * @param[in] slew the slew rate to assign to the pin.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pmux_set_slew(const qm_pin_id_t pin, const qm_pmux_slew_t slew);

/**
 * Enable input for a pin in the pin mux controller.
 *
 * @param[in] pin which pin to configure.
 * @param[in] enable set to true to enable input.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pmux_input_en(const qm_pin_id_t pin, const bool enable);

/**
 * Enable pullup for a pin in the pin mux controller.
 *
 * @param[in] pin which pin to configure.
 * @param[in] enable set to true to enable pullup.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pmux_pullup_en(const qm_pin_id_t pin, const bool enable);

/**
 * @}
 */

#endif /* __QM_PINMUX_H__ */
