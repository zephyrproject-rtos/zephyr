/*
 * Copyright (c) 2024 Abhinav Srivastava
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_STROPTS_H_
#define ZEPHYR_INCLUDE_POSIX_STROPTS_H_
#define RS_HIPRI BIT(0)

#ifdef __cplusplus
extern "C" {
#endif

struct strbuf {
	int maxlen;
	int len;
	char *buf;
};

int putmsg(int fildes, const struct strbuf *ctlptr, const struct strbuf *dataptr, int flags);
int fdetach(const char *path);
int fattach(int fildes, const char *path);
int getmsg(int fildes, struct strbuf *ctlptr, struct strbuf *dataptr, int *flagsp);
int getpmsg(int fildes, struct strbuf *ctlptr, struct strbuf *dataptr, int *bandp, int *flagsp);
int isastream(int fildes);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_STROPTS_H_ */
