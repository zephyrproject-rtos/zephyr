# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_RPMSG_SERVICE_BACKEND_ZEPHYR rpmsg_backend.c)
zephyr_sources_ifdef(CONFIG_RPMSG_SERVICE_BACKEND_LINUX rpmsg_backend_linux.c)
zephyr_sources(rpmsg_service.c)
