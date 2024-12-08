/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Part of EEPROM simulator which interacts with the host OS / Linux
 *
 * When building for the native simulator, this file is built in the
 * native simulator runner/host context, and not in Zephyr/embedded context.
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <nsi_tracing.h>

/*
 * Initialize the EEPROM buffer.
 * And, if the content is to be kept on disk map it to the buffer to the file.
 *
 * Returns -1 on failure
 *	    0 on success
 */
int eeprom_mock_init_native(bool eeprom_in_ram, char **mock_eeprom, unsigned int size,
			    int *eeprom_fd, const char *eeprom_file_path, unsigned int erase_value,
			    bool eeprom_erase_at_start)
{
	struct stat f_stat;
	int rc;

	if (eeprom_in_ram == true) {
		*mock_eeprom = (char *)malloc(size);
		if (*mock_eeprom == NULL) {
			nsi_print_warning("Could not allocate EEPROM in the process heap %s\n",
					  strerror(errno));
			return -1;
		}
	} else {
		*eeprom_fd = open(eeprom_file_path, O_RDWR | O_CREAT, (mode_t)0600);
		if (*eeprom_fd == -1) {
			nsi_print_warning("Failed to open EEPROM device file %s: %s\n",
					  eeprom_file_path, strerror(errno));
			return -1;
		}

		rc = fstat(*eeprom_fd, &f_stat);
		if (rc) {
			nsi_print_warning("Failed to get status of EEPROM device file %s: %s\n",
					  eeprom_file_path, strerror(errno));
			return -1;
		}

		if (ftruncate(*eeprom_fd, size) == -1) {
			nsi_print_warning("Failed to resize EEPROM device file %s: %s\n",
					  eeprom_file_path, strerror(errno));
			return -1;
		}

		*mock_eeprom = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, *eeprom_fd, 0);
		if (*mock_eeprom == MAP_FAILED) {
			nsi_print_warning("Failed to mmap EEPROM device file %s: %s\n",
					  eeprom_file_path, strerror(errno));
			return -1;
		}
	}

	if ((eeprom_erase_at_start == true) || (eeprom_in_ram == true) || (f_stat.st_size == 0)) {
		/* Erase the EEPROM by setting all bits to the configured erase value */
		(void)memset(*mock_eeprom, erase_value, size);
	}

	return 0;
}

/*
 * If in RAM: Free the mock buffer
 * If in disk: unmap the eeprom file from RAM, close the file, and if configured to do so,
 * delete the file.
 */
void eeprom_mock_cleanup_native(bool eeprom_in_ram, int eeprom_fd, char *mock_eeprom,
				unsigned int size, const char *eeprom_file_path,
				bool eeprom_rm_at_exit)
{

	if (eeprom_in_ram == true) {
		if (mock_eeprom != NULL) {
			free(mock_eeprom);
		}
		return;
	}

	if ((mock_eeprom != MAP_FAILED) && (mock_eeprom != NULL)) {
		munmap(mock_eeprom, size);
	}

	if (eeprom_fd != -1) {
		close(eeprom_fd);
	}

	if ((eeprom_rm_at_exit == true) && (eeprom_file_path != NULL)) {
		/* We try to remove the file but do not error out if we can't */
		(void)remove(eeprom_file_path);
	}
}
