/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PEER_BOTTOM_H
#define PEER_BOTTOM_H

/* Return the file descriptor of a socket listening on the path, or -1 on failure. */
int peer_listen(const char *path);

/* Return a non-blocking file descriptor for a connection already made, or -1 on failure. */
int peer_accept(int listen_fd);

/* Close the listening socket and remove its path. */
void peer_stop_listening(int listen_fd, const char *path);

#endif /* PEER_BOTTOM_H */
