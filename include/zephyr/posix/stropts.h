/*
 * Copyright (c) 2024 Abhinav Srivastava
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STREAMS interface.
 * @ingroup posix
 *
 * Provides the STREAMS I/O control interface (a legacy XSI extension). STREAMS is
 * rarely implemented in its entirety; Zephyr provides minimal support for compatibility.
 *
 * @posix_header{stropts.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_STROPTS_H_
#define ZEPHYR_INCLUDE_POSIX_STROPTS_H_

/**
 * @brief Flag: message carries high-priority data.
 */
#define RS_HIPRI BIT(0)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buffer descriptor for STREAMS getmsg() / putmsg() control and data parts.
 */
struct strbuf {
	/**
	 * @brief Maximum buffer length.
	 */
	int maxlen;
	/**
	 * @brief Length of data, or -1 to indicate no data/control part.
	 */
	int len;
	/**
	 * @brief Pointer to data buffer.
	 */
	char *buf;
};

/**
 * @brief Send a message on a STREAM.
 *
 * @param fildes  STREAMS file descriptor.
 * @param ctlptr  Control part of the message, or NULL.
 * @param dataptr Data part of the message, or NULL.
 * @param flags   0, or @c RS_HIPRI to send a high-priority message.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{putmsg}
 */
int putmsg(int fildes, const struct strbuf *ctlptr, const struct strbuf *dataptr, int flags);

/**
 * @brief Detach a name from a STREAMS-based file descriptor.
 *
 * @param path Pathname previously used with fattach().
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fdetach}
 */
int fdetach(const char *path);

/**
 * @brief Attach a STREAMS-based file descriptor to a file in the file system name space.
 *
 * @param fildes STREAMS file descriptor to attach.
 * @param path   Existing pathname to attach the STREAMS file descriptor to.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{fattach}
 */
int fattach(int fildes, const char *path);

/**
 * @brief Receive next message from a STREAMS file.
 *
 * @param fildes  STREAMS file descriptor.
 * @param ctlptr  Output: control part buffer, or NULL.
 * @param dataptr Output: data part buffer, or NULL.
 * @param flagsp  Input/output: 0, or @c RS_HIPRI to request high-priority messages only.
 *
 * @return 0 if a complete message was received, a positive value if more of the message
 *         remains, or -1 with errno set on failure.
 *
 * @posix_func{getmsg}
 */
int getmsg(int fildes, struct strbuf *ctlptr, struct strbuf *dataptr, int *flagsp);

/**
 * @brief Receive a priority-banded message from a STREAMS file.
 *
 * @param fildes  STREAMS file descriptor.
 * @param ctlptr  Output: control part buffer, or NULL.
 * @param dataptr Output: data part buffer, or NULL.
 * @param bandp   Input/output: priority band of the message.
 * @param flagsp  Input/output: message flags.
 *
 * @return 0 if a complete message was received, a positive value if more of the message
 *         remains, or -1 with errno set on failure.
 *
 * @posix_func{getpmsg}
 */
int getpmsg(int fildes, struct strbuf *ctlptr, struct strbuf *dataptr, int *bandp, int *flagsp);

/**
 * @brief Test whether a file descriptor refers to a STREAMS file.
 *
 * @param fildes File descriptor to test.
 *
 * @return 1 if @p fildes is a STREAMS file, 0 if not, or -1 with errno set on error.
 *
 * @posix_func{isastream}
 */
int isastream(int fildes);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_STROPTS_H_ */
