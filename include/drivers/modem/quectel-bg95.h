#ifndef __MDM_QUECTEL_BG95_H__
#define __MDM_QUECTEL_BG95_H__

#include <kernel.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr.h>
#include <device.h>

#include <net/net_if.h>
#include <net/net_offload.h>
#include <net/socket_offload.h>
#include <net/http_parser.h>

/* uncomment for File operations API */
#define CONFIG_QUECTEL_BG95_FILE_OPS
#define CONFIG_QUECTEL_BG95_DFOTA

#define MDM_MANUFACTURER_LENGTH 10
#define MDM_MODEL_LENGTH 16
#define MDM_REVISION_LENGTH 64
#define MDM_IMEI_LENGTH 16
#define MDM_TIME_LENGTH 32
#define MAX_CI_BUF_SIZE   (64U * 6U)

struct usr_http_cfg {
	char *url;
	char *content_type;
	char *content_body;
	char *recv_buf;
	size_t recv_buf_len;
    char *resp_filename; 
	uint8_t method;
	uint16_t recv_read_len;
	uint16_t timeout;
};

struct mdm_ctx {
	/* modem data */
	char data_manufacturer[MDM_MANUFACTURER_LENGTH];
	char data_model[MDM_MODEL_LENGTH];
	char data_revision[MDM_REVISION_LENGTH];
	char data_imei[MDM_IMEI_LENGTH];
	char data_timeval[MDM_TIME_LENGTH];
    char data_cellinfo[MAX_CI_BUF_SIZE];
    uint32_t data_sys_timeval;
	int32_t   data_rssi;
};

struct usr_gps_cfg {
	char *lat;
	char *lon;
    char* gps_data;
    char* agps_filename;
    char* utc_time;
    int agps_status;
};

#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
typedef int (*m_fopen)(struct device *dev,
                        const char *file);
typedef int (*m_fread)(struct device *dev,
                        int fd, uint8_t* buf, size_t len);
typedef int (*m_fwrite)(struct device *dev,
                        int fd, uint8_t* buf, size_t buf_len);
typedef int (*m_fseek)(struct device *dev,
                        int fd, size_t off);
typedef int (*m_fclose)(struct device *dev,
                        int fd);
typedef int (*m_fstat)(struct device *dev,
                        const char* fname, size_t* f_sz);
typedef int (*m_fdel)(struct device *dev,
                        const char* fname);
#endif

#ifdef CONFIG_QUECTEL_BG95_DFOTA
typedef int (*m_dfota)(struct device *dev,
                        const char* url);
#endif

typedef int (*mdm_get_clock)(struct device *dev,
                        char *timeval);
typedef int (*mdm_get_ntp_time)(struct device *dev);

typedef int (*mdm_http_init)(struct device *dev,
                        struct usr_http_cfg *cfg);
typedef int (*mdm_http_execute)(struct device *dev,
                        struct usr_http_cfg *cfg);
typedef int (*mdm_http_term)(struct device *dev,
                        struct usr_http_cfg *cfg);

typedef int (*mdm_gps_init)(struct device *dev,
                        struct usr_gps_cfg *cfg);
typedef int (*mdm_gps_agps)(struct device *dev,
                        struct usr_gps_cfg *cfg);
typedef int (*mdm_gps_read)(struct device *dev,
                        struct usr_gps_cfg *cfg);
typedef int (*mdm_gps_close)(struct device *dev);

typedef int (*mdm_get_ctx)(struct device *dev,
                        struct mdm_ctx **ctx);

typedef int (*mdm_get_cell_info)(struct device *dev,
                        char **cell_info);

typedef void (*mdm_reset)(void);

struct modem_quectel_bg95_net_api {
    /* Need to be at the top */
	struct net_if_api net_api;

	/* Driver specific extension api */
    mdm_get_clock  get_clock;
    mdm_get_ntp_time get_ntp_time;

	mdm_http_init http_init;
	mdm_http_execute http_execute;
	mdm_http_term http_term;

	mdm_gps_init gps_init;
	mdm_gps_agps gps_agps;
	mdm_gps_read gps_read;
	mdm_gps_close gps_close;

#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
    m_fopen fopen;
    m_fread fread;
    m_fwrite fwrite;
    m_fseek fseek;
    m_fclose fclose;
    m_fstat fstat;
    m_fdel fdel;
#endif

#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
    m_dfota dfota;
#endif

	mdm_get_ctx get_ctx;

    mdm_get_cell_info get_cell_info;

	mdm_reset reset;
};

/*
enum http_method {
	HTTP_GET = 0,
	HTTP_POST,
};
*/

#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
__syscall int mdm_fopen(struct device *dev,
                const char *file);
static inline int mdm_fopen(struct device *dev,
        const char *file)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->fopen(dev, file);
}

__syscall int mdm_fread(struct device *dev,
                int fd, uint8_t* buf, size_t len);
static inline int mdm_fread(struct device *dev,
        int fd, uint8_t* buf, size_t len)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->fread(dev, fd, buf, len);
}

__syscall int mdm_fwrite(struct device *dev,
                int fd, uint8_t* buf, size_t buf_len);
static inline int mdm_fwrite(struct device *dev,
        int fd, uint8_t* buf, size_t buf_len)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->fwrite(dev, fd, buf, buf_len);
}

__syscall int mdm_fseek(struct device *dev,
                int fd, size_t off);
static inline int mdm_fseek(struct device *dev,
        int fd, size_t off)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->fseek(dev, fd, off);
}

__syscall int mdm_fclose(struct device *dev,
                int fd);
static inline int mdm_fclose(struct device *dev,
        int fd)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->fclose(dev, fd);
}

__syscall int mdm_fstat(struct device *dev,
                const char *fname, size_t* f_sz);
static inline int mdm_fstat(struct device *dev,
        const char *fname, size_t* f_sz)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->fstat(dev, fname, f_sz);
}

__syscall int mdm_fdel(struct device *dev,
                const char *fname);
static inline int mdm_fdel(struct device *dev,
        const char *fname)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->fdel(dev, fname);
}

#endif

#ifdef CONFIG_QUECTEL_BG95_FILE_OPS
__syscall int mdm_quectel_bg95_dfota(struct device *dev,
				const char* url);

static inline int mdm_quectel_bg95_dfota(struct device *dev,
					   const char* url)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->dfota(dev, url);
}
#endif

__syscall int mdm_quectel_bg95_get_ntp_time(struct device *dev);

static inline int mdm_quectel_bg95_get_ntp_time(struct device *dev)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->get_ntp_time(dev);
}

__syscall int mdm_quectel_bg95_get_clock(struct device *dev,
				char *timeval);

static inline int mdm_quectel_bg95_get_clock(struct device *dev,
					   char *timeval)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->get_clock(dev, timeval);
}

__syscall int mdm_quectel_bg95_http_init(struct device *dev,
				struct usr_http_cfg *cfg);

static inline int mdm_quectel_bg95_http_init(struct device *dev,
					   struct usr_http_cfg *cfg)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->http_init(dev, cfg);
}

__syscall int mdm_quectel_bg95_http_execute(struct device *dev,
				   struct usr_http_cfg *cfg);

static inline int mdm_quectel_bg95_http_execute(struct device *dev,
					      struct usr_http_cfg *cfg)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->http_execute(dev, cfg);
}

__syscall int mdm_quectel_bg95_http_term(struct device *dev,
				struct usr_http_cfg *cfg);

static inline int mdm_quectel_bg95_http_term(struct device *dev,
					   struct usr_http_cfg *cfg)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->http_term(dev, cfg);
}

__syscall int mdm_quectel_bg95_gps_init(struct device *dev,
				struct usr_gps_cfg *cfg);

static inline int mdm_quectel_bg95_gps_init(struct device *dev,
					   struct usr_gps_cfg *cfg)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->gps_init(dev, cfg);
}

__syscall int mdm_quectel_bg95_gps_agps(struct device *dev,
				struct usr_gps_cfg *cfg);

static inline int mdm_quectel_bg95_gps_agps(struct device *dev,
					   struct usr_gps_cfg *cfg)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->gps_agps(dev, cfg);
}

__syscall int mdm_quectel_bg95_gps_read(struct device *dev,
				struct usr_gps_cfg *cfg);

static inline int mdm_quectel_bg95_gps_read(struct device *dev,
					   struct usr_gps_cfg *cfg)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->gps_read(dev, cfg);
}

__syscall int mdm_quectel_bg95_gps_close(struct device *dev);

static inline int mdm_quectel_bg95_gps_close(struct device *dev)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->gps_close(dev);
}

__syscall int mdm_quectel_bg95_get_ctx(struct device *dev,
				struct mdm_ctx **ctx);

static inline int mdm_quectel_bg95_get_ctx(struct device *dev,
					   struct mdm_ctx **ctx)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->get_ctx(dev, ctx);
}

__syscall int mdm_quectel_bg95_get_cell_info(struct device *dev,
				char **cell_info);

static inline int mdm_quectel_bg95_get_cell_info(struct device *dev,
					   char **cell_info)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->get_cell_info(dev, cell_info);
}
__syscall void mdm_quectel_bg95_reset(struct device *dev);

static inline void mdm_quectel_bg95_reset(struct device *dev)
{
	const struct modem_quectel_bg95_net_api *api =
        (const struct modem_quectel_bg95_net_api *) dev->api;
	return api->reset();
}

#endif
