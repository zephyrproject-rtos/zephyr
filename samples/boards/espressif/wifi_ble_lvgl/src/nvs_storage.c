/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nvs_storage.h"
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/devicetree.h>
#include <string.h>

LOG_MODULE_REGISTER(nvs_storage, LOG_LEVEL_INF);

static struct nvs_fs fs;

int nvs_storage_init(void)
{
    int rc = 0;

    /* Use the storage partition defined in DTS */
    fs.flash_device = FIXED_PARTITION_DEVICE(storage_partition);
    if (!device_is_ready(fs.flash_device)) {
        LOG_ERR("Flash device not ready");
        return -ENODEV;
    }

    fs.offset = FIXED_PARTITION_OFFSET(storage_partition);
    fs.sector_size = 4096; /* Standard flash sector size */
    fs.sector_count = FIXED_PARTITION_SIZE(storage_partition) / fs.sector_size;

    LOG_INF("NVS: offset=0x%lx, size=%d, sector_size=%d, sector_count=%d",
            (unsigned long)fs.offset, FIXED_PARTITION_SIZE(storage_partition),
            fs.sector_size, fs.sector_count);

    rc = nvs_mount(&fs);
    if (rc) {
        LOG_ERR("Flash Init failed: %d", rc);
        return rc;
    }

    LOG_INF("NVS storage initialized successfully");
    return 0;
}

int nvs_save_wifi_credentials(const char *ssid, const char *password)
{
    int rc;

    if (!ssid || !password) {
        return -EINVAL;
    }

    if (strlen(ssid) >= MAX_SSID_LEN || strlen(password) >= MAX_PASSWORD_LEN) {
        LOG_ERR("SSID or password too long");
        return -EINVAL;
    }

    /* Save SSID */
    rc = nvs_write(&fs, WIFI_SSID_KEY, ssid, strlen(ssid) + 1);
    if (rc < 0) {
        LOG_ERR("Failed to write SSID: %d", rc);
        return rc;
    }

    /* Save password */
    rc = nvs_write(&fs, WIFI_PASSWORD_KEY, password, strlen(password) + 1);
    if (rc < 0) {
        LOG_ERR("Failed to write password: %d", rc);
        return rc;
    }

    /* Mark credentials as valid */
    uint8_t valid = 1;
    rc = nvs_write(&fs, WIFI_CREDENTIALS_VALID_KEY, &valid, sizeof(valid));
    if (rc < 0) {
        LOG_ERR("Failed to write credentials valid flag: %d", rc);
        return rc;
    }

    LOG_INF("WiFi credentials saved successfully - SSID: %s", ssid);
    return 0;
}

int nvs_read_wifi_credentials(char *ssid, char *password)
{
    int rc;

    if (!ssid || !password) {
        return -EINVAL;
    }

    /* Check if credentials are valid */
    if (!nvs_has_wifi_credentials()) {
        return -ENOENT;
    }

    /* Read SSID */
    rc = nvs_read(&fs, WIFI_SSID_KEY, ssid, MAX_SSID_LEN);
    if (rc < 0) {
        LOG_ERR("Failed to read SSID: %d", rc);
        return rc;
    }

    /* Read password */
    rc = nvs_read(&fs, WIFI_PASSWORD_KEY, password, MAX_PASSWORD_LEN);
    if (rc < 0) {
        LOG_ERR("Failed to read password: %d", rc);
        return rc;
    }

    LOG_INF("WiFi credentials read successfully - SSID: %s", ssid);
    return 0;
}

bool nvs_has_wifi_credentials(void)
{
    uint8_t valid = 0;
    int rc = nvs_read(&fs, WIFI_CREDENTIALS_VALID_KEY, &valid, sizeof(valid));

    if (rc < 0) {
        LOG_DBG("No valid credentials flag found: %d", rc);
        return false;
    }

    return (valid == 1);
}

int nvs_clear_wifi_credentials(void)
{
    int rc;

    /* Delete all WiFi credential keys */
    rc = nvs_delete(&fs, WIFI_SSID_KEY);
    if (rc < 0 && rc != -ENOENT) {
        LOG_ERR("Failed to delete SSID: %d", rc);
        return rc;
    }

    rc = nvs_delete(&fs, WIFI_PASSWORD_KEY);
    if (rc < 0 && rc != -ENOENT) {
        LOG_ERR("Failed to delete password: %d", rc);
        return rc;
    }

    rc = nvs_delete(&fs, WIFI_CREDENTIALS_VALID_KEY);
    if (rc < 0 && rc != -ENOENT) {
        LOG_ERR("Failed to delete credentials valid flag: %d", rc);
        return rc;
    }

    LOG_INF("WiFi credentials cleared successfully");
    return 0;
}
