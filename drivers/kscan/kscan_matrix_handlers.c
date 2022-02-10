/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/kscan.h>
#include <syscall_handler.h>

#include <syscalls/kscan_matrix_configure_mrsh.c>

static int z_vrfy_kscan_matrix_drive_column(const struct device *dev, int col)
{
	Z_OOPS(Z_SYSCALL_DRIVER_KSCAN(dev, matrix_drive_column));

	return z_impl_kscan_matrix_drive_column((const struct device *)dev, col);
}
#include <syscalls/kscan_matrix_drive_column_mrsh.c>

static int z_vrfy_kscan_matrix_read_row(const struct device *dev, int *row)
{
	Z_OOPS(Z_SYSCALL_DRIVER_KSCAN(dev, matrix_read_row));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(row, sizeof(*row)));

	return z_impl_kscan_matrix_read_row((const struct device *)dev, row);
}
#include <syscalls/kscan_matrix_read_row_mrsh.c>

static int z_vrfy_kscan_matrix_resume_detection(const struct device *dev, bool resume)
{
	Z_OOPS(Z_SYSCALL_DRIVER_KSCAN(dev, matrix_resume_detection));

	return z_impl_kscan_matrix_resume_detection((const struct device *)dev, resume);
}
#include <syscalls/kscan_matrix_resume_detection_mrsh.c>
