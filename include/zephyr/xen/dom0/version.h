/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_DOM0_VERSION_H__
#define __XEN_DOM0_VERSION_H__
#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/version.h>
#include <zephyr/xen/public/xen.h>

int xen_version(void);
int xen_version_extraversion(char *extra, int len);

#endif /* __XEN_DOM0_VERSION_H__ */
