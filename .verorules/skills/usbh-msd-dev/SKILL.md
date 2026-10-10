---
name: usbh-msd-dev
description: USB Host Mass Storage Class (MSD) development assistant for Zephyr RTOS. Activates when working on usbh_msd.c, usbh_msd.h, or the host_msd sample. Enforces verified design decisions, known bug registry, and architecture constraints derived from hardware validation on NXP RD-RW612-BGA. Use when generating, debugging, reviewing, or optimizing MSD driver code.
---

## USB Host MSD Development Skill

Always begin by acknowledging: "I am operating under the USB Host MSD harness rules."

---

### Context Loading

Before generating any code, read the harness document:

```
samples/subsys/usb/host_msd/AI_HARNESS.md
```

This document is the authoritative source for:
- API contracts (Section 3)
- Bug registry (Section 4)
- Architecture rules (Section 5)
- Verification checklist (Section 6)

---

### Mandatory Pre-generation Checks

Before writing ANY driver code, verify:

1. **Device handle assignment** - `priv->dev` must be assigned in probe:
   ```c
   priv->dev = (const struct device *)c_data;
   ```

2. **disk_name indexing** - Must use `slot_idx`, never the retry loop variable:
   ```c
   int slot_idx;
   for (slot_idx = 0; slot_idx < CONFIG_USBH_MSD_INSTANCES_COUNT; slot_idx++) { ... }
   snprintf(priv->disk_name, ..., "USBDISK%d", slot_idx);  /* correct */
   ```

3. **IN transfer buffer** - Leave empty, HCD fills via tailroom:
   ```c
   /* OUT: */ net_buf_add_mem(nbuf, buf, len);
   /* IN:  */ /* nothing - HCD uses net_buf_tailroom() */
   ```

4. **Transfer callback** - Must register `msd_bulk_cb`, never NULL:
   ```c
   xfer = usbh_xfer_alloc(priv->udev, ep, msd_bulk_cb, priv);
   ```

5. **CBW tag** - Per-instance field, never global:
   ```c
   cbw.tag = sys_cpu_to_le32(++priv->cbw_tag);  /* not s_cbw_tag */
   ```

6. **BOT Reset** - Correct class request, never SET_INTERFACE:
   ```c
   usbh_req_setup(udev, 0x21, 0xFF, 0, iface, 0, NULL);
   ```

---

### Mandatory prj.conf Rules

When generating or editing `prj.conf` for MSD:

```kconfig
CONFIG_USB_HOST_STACK=y          # NOT CONFIG_USB_HOST
CONFIG_USBH_MSD_CLASS=y
CONFIG_DISK_ACCESS=y
CONFIG_FILE_SYSTEM=y
CONFIG_FAT_FILESYSTEM_ELM=y
CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT=1
CONFIG_FS_FATFS_CUSTOM_MOUNT_POINTS="USBDISK0"
CONFIG_FS_FATFS_EXFAT=y
CONFIG_FS_FATFS_LFN=y
CONFIG_FS_FATFS_LBA64=y
CONFIG_HEAP_MEM_POOL_SIZE=32768
CONFIG_DCACHE=n                  # Required for RW612
```

---

### Mandatory Mount Point Rule

```c
/* CORRECT */
#define MSD_MOUNT_POINT  "/USBDISK0:"
/* WRONG */
#define MSD_MOUNT_POINT  "/USB"
```

`translate_path()` strips the leading `/`, leaving `USBDISK0:` which FatFS
matches against `VolumeStr[0]`.

---

### Endpoint Parsing Rule

Never hardcode endpoint addresses. Always parse from descriptor:

```c
struct usb_cfg_descriptor *cfg = (struct usb_cfg_descriptor *)udev->cfg_desc;
struct usb_desc_header *dhp = udev->ifaces[iface].dhp;
void *desc_end = (void *)((uint8_t *)cfg + cfg->wTotalLength);

while (dhp != NULL && (void *)dhp < desc_end) {
    if (dhp->bDescriptorType == USB_DESC_ENDPOINT) {
        struct usb_ep_descriptor *ep = (struct usb_ep_descriptor *)dhp;
        if ((ep->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
            if (USB_EP_DIR_IS_IN(ep->bEndpointAddress)) ep_in = ep->bEndpointAddress;
            else ep_out = ep->bEndpointAddress;
        }
    }
    if (dhp->bDescriptorType == USB_DESC_INTERFACE && dhp != udev->ifaces[iface].dhp) break;
    dhp = (void *)((uint8_t *)dhp + dhp->bLength);
}
```

---

### Timeout Handling Rule

After dequeue, wait for cancel callback before freeing:

```c
if (k_sem_take(&priv->xfer_sem, K_MSEC(MSD_XFER_TIMEOUT_MS))) {
    usbh_xfer_dequeue(priv->udev, xfer);
    k_sem_take(&priv->xfer_sem, K_MSEC(500));  /* wait for cancel */
    usbh_xfer_buf_free(priv->udev, nbuf);
    return -ETIMEDOUT;
}
```

---

### Error Diagnosis Table

When the user shows an error log, diagnose using:

| Error | Root Cause | Fix |
|-------|-----------|-----|
| `Bulk transfer timeout ep=0x02` | NULL callback in xfer_alloc | Add `msd_bulk_cb` |
| `Data phase failed: -5` | net_buf pre-filled for IN | Remove `net_buf_add()` |
| `CBW send failed: -5` | Wrong ep (hardcoded vs actual) | Parse from descriptor |
| `fs mount error (-5)` | Mount point `/USB` wrong | Use `/USBDISK0:` |
| `file open error (-2)` | Mount point or VolumeStr mismatch | Check CUSTOM_MOUNT_POINTS |
| `get_info returns -EINVAL` | priv->dev not assigned | Assign in probe |
| `disk_name = USBDISK5` | slot_idx vs retry loop var | Fix variable |
| `prj.conf: undefined CONFIG_USB_HOST` | Wrong Kconfig symbol | Use USB_HOST_STACK |

---

### Test Harness Location

```
tests/subsys/usb/usbh_msd/src/main.c
```

Build command:
```bash
west build -b rd_rw612_bga tests/subsys/usb/usbh_msd -d aa_rdrw612_host_msd_harness
```

---

### DO NOT Rules

1. Never use `SET_INTERFACE` for BOT Reset
2. Never hardcode `0x81` / `0x02` as endpoint addresses
3. Never call `net_buf_add()` for IN transfer buffers
4. Never use a global CBW tag counter
5. Never add `k_sem_give` in `msd_class_completion_cb`
6. Never use `/USB` as mount point
7. Never use `CONFIG_USB_HOST=y` (use `CONFIG_USB_HOST_STACK=y`)
8. Never return immediately on timeout without freeing resources
9. Never index `disk_name` with a for-loop retry variable
