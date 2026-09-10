/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

/**
 * @file
 * @ingroup espi_interface
 * @brief Special operations for the Realtek RTS5912 ESPI driver
 */

#ifdef CONFIG_ESPI_RTS5912_PRESERVED_DATA_HOOKS
/**
 * @brief Get internal driver data to be preserved during a soft reset.
 *
 * The RTS5912 ESPI driver must track certain driver state in RAM. During a
 * soft reboot, the driver (but not the hardware) will re-initialize and lose
 * this current state data. This can cause unintended operation due to the
 * cached state not matching the hardware, which some applications intentionally
 * do not reset to avoid breaking the connection to the host while transitioning
 * the running firmware image.
 *
 * There is no way to recover this state from RTS5912 hardware registers post-
 * reboot. Instead, this API allows the application to capture certain driver
 * state to be preserved ahead of a soft reboot that can be stashed in a caller-
 * provided preserved memory area (e.g. BBRAM, NVRAM, protected RAM) and then
 * restored after the reboot (see espi_rts5912_set_preserved_data()).
 *
 * @param buf_max_size The maximum number of bytes that may be written out
 *                     to \p buf_out
 * @param buf_out A caller-provided output buffer where the driver will write
 *                out internal state data. This data is opaque and should be
 *                passed back to the driver exactly.
 *
 * @return The number of bytes written to \p buf_out
 * @return -ERANGE if \p buf_max_size is too small
 * @return -EINVAL if \p buf_out is NULL
 */
int espi_rts5912_get_preserved_data(size_t buf_max_size, uint8_t *buf_out);

/**
 * @brief Restore internal driver data following a soft reset.
 *
 * Allows preserved internal driver state captured through
 * espi_rts5912_get_preserved_data() to be reloaded into the driver following a
 * soft reboot.
 *
 * It is invalid to call this function after a cold/hard reboot or power cycle
 * event where the ESPI hardware itself was reset.
 *
 * This function MUST be called before the RTS5912 ESPI driver is initialized.
 *
 * @param size The number of bytes to read from \p buf_in.
 * @param buf_in Buffer holding the preserved internal driver state data.
 *
 * @return 0 on success
 * @return -EBUSY if the driver has already initialized.
 * @return -ERANGE if \p size is smaller than the driver expected. Caller must
 *         supply all of the bytes from espi_rts5912_get_preserved_data()
 * @return -EINVAL if \p buf_in is NULL or \p size is zero
 * @return -EBADMSG if the provided preserved data is invalid
 */
int espi_rts5912_set_preserved_data(size_t size, const uint8_t *buf_in);
#endif /* CONFIG_ESPI_RTS5912_PRESERVED_DATA_HOOKS */
