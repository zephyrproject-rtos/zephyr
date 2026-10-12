/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File system plugin.
 * @ingroup mpipe_fs
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FS_H_
#define ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FS_H_

/**
 * @defgroup mpipe_fs File System
 * @ingroup mpipe_plugins
 * @brief Elements that read from and write to a mounted file system.
 *
 * The file system plugin puts a file at either end of a graph: a source that
 * reads one in fixed-size blocks, and a sink that writes whatever reaches it.
 *
 * Neither knows what it is carrying, which is the point. A file is a stream of
 * bytes with no format attached, so a file source has nothing to announce and
 * negotiates no capability of its own; whatever follows it - typically a parser
 * - is what works out the format. That makes these two elements the usual way
 * to test a graph without hardware.
 */

#endif /* ZEPHYR_INCLUDE_MPIPE_FS_MPIPE_FS_H_ */
