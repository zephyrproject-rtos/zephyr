# USB Host MSD — AI Development Harness

> 这是 AI 辅助开发 USB Host MSD 驱动的"缰绳"文档。
> 当使用 AI 大模型（如 Vero）进行该模块的代码生成、调试、优化时，
> 必须以本文档为输入上下文，确保 AI 不偏离已验证的设计决策。

---

## 1. 项目背景（Project Context）

**目标**：在 Zephyr RTOS 的 USB Host Stack 上实现 USB Mass Storage Class (MSC)
Bulk-Only Transport (BOT) 驱动，并通过 `disk_access` 子系统暴露给 FatFS。

**硬件**：NXP RD-RW612-BGA（`rd_rw612_bga`），EHCI USB 主机控制器。

**已验证状态**：
- 设备枚举 ✅
- SCSI INQUIRY / TEST UNIT READY / READ CAPACITY ✅
- 原始扇区读写（BOT CBW/CSW 协议）✅
- FatFS 挂载（exFAT，SanDisk 58GB）✅
- 文件写入 + 内容验证 ✅

---

## 2. 文件清单（File Map）

```
subsys/usb/host/class/
├── usbh_msd.c           ← 主驱动（922行，唯一实现）
├── Kconfig.uac2
├── Kconfig.uvc
└── Kconfig              ← 包含 rsource "Kconfig.msd"（需手动添加）

include/zephyr/usb/class/
└── usbh_msd.h           ← 公开 API 头文件

samples/subsys/usb/host_msd/
├── src/main.c           ← 样例应用
├── prj.conf             ← 配置
├── CMakeLists.txt
└── AI_HARNESS.md        ← 本文件

tests/subsys/usb/usbh_msd/
├── src/main.c           ← Ztest 测试套件
├── prj.conf
├── CMakeLists.txt
└── testcase.yaml
```

---

## 3. 核心 API 约定（API Contract）

AI 生成或修改代码时，必须遵守以下 API 约定：

```c
/* 设备句柄：probe 时将 c_data 指针 cast 为 const struct device * */
priv->dev = (const struct device *)c_data;   /* ← 必须这样赋值 */

/* 所有公开函数先通过 priv_from_dev(dev) 查找实例 */
struct usbh_msd_priv *priv = priv_from_dev(dev);
if (!priv || !buf) return -EINVAL;           /* ← NULL 检查必须在最前 */
if (!(priv->flags & MSD_DEVICE_FLAG_READY)) return -ENODEV;

/* sector_count 上限检查（防止 net_buf OOM）*/
if (sector_count > CONFIG_USBH_MSD_MAX_SECTORS_PER_XFER) return -EINVAL;
```

---

## 4. 已知 Bug 及修复方案（Bug Registry）

AI 在生成代码时必须**规避**以下历史 Bug：

| ID | 问题描述 | 错误写法 | 正确写法 |
|----|----------|----------|----------|
| B01 | `disk_name` 编号使用错误变量 | `snprintf(..., "USBDISK%d", i)` 其中 i 是重试循环变量 | 使用 `slot_idx`（实例槽位索引）|
| B02 | `priv->dev` 从未赋值 | 忘记赋值 → 公开 API 全部返回 -EINVAL | `priv->dev = (const struct device *)c_data` |
| B03 | BOT Reset 用 SET_INTERFACE 实现 | `usbh_req_set_alt(udev, iface, 0)` | `usbh_req_setup(..., 0x21, 0xFF, 0, iface, 0, NULL)` |
| B04 | 传输超时后内存泄漏 | 直接 `return -ETIMEDOUT` | 先 dequeue，等待 cancel 回调后再释放 nbuf |
| B05 | Data phase 失败后直接读 CSW | 跳过 STALL clear | 先 `msd_clear_stall(priv, ep)` 再读 CSW |
| B06 | CBW tag 全局变量 | `static uint32_t s_cbw_tag` | `priv->cbw_tag`（per-instance）|
| B07 | msd_class_completion_cb 重复信号 | 与 msd_bulk_cb 同时 k_sem_give | completion_cb 改为 no-op |
| B08 | GET MAX LUN 未实现 | 直接跳过 | probe 中发送 class request 0xFE |
| B09 | IN 传输预填充 buf | `net_buf_add(nbuf, len)` | 不调用，HCD 通过 tailroom 自动确定长度 |
| B10 | 挂载点拼写错误 | `/USB` | `/USBDISK0:` (translate_path 会去掉前导 /) |

---

## 5. 架构约束（Architecture Rules）

AI 必须遵守以下架构规则，**不得违反**：

### 5.1 传输流水线
```
调用层: msd_bulk_xfer()
  └── usbh_xfer_alloc(udev, ep, msd_bulk_cb, priv)  ← 回调必须是 msd_bulk_cb
  └── usbh_xfer_buf_alloc(udev, len)
  └── OUT: net_buf_add_mem(nbuf, buf, len)
      IN:  留空，HCD 填充（勿调用 net_buf_add/reserve）
  └── usbh_xfer_buf_add / usbh_xfer_enqueue
  └── k_sem_take(&priv->xfer_sem, K_MSEC(5000))
  └── 完成后: xfer->priv → priv，k_sem_give
```

### 5.2 BOT 三阶段协议
```
Phase 1: OUT  CBW (31字节, ep_out)
Phase 2: 数据  data_in ? ep_in : ep_out  (可选)
         失败时先 msd_clear_stall(ep) 再继续 Phase 3
Phase 3: IN   CSW (13字节, ep_in)
         验证 signature, tag, status
```

### 5.3 FatFS 挂载路径规则
```
disk 名称:  "USBDISK0"
挂载点:     "/USBDISK0:"
文件路径:   "/USBDISK0:/ZEPHYR.TXT"
原因: translate_path("/USBDISK0:") → "USBDISK0:" (去掉前导 /)
      FatFS 用 "USBDISK0" 匹配 VolumeStr[0]
```

### 5.4 端点地址解析
```c
/* 必须从配置描述符解析，禁止硬编码 0x81/0x02 */
struct usb_cfg_descriptor *cfg = (struct usb_cfg_descriptor *)udev->cfg_desc;
struct usb_desc_header *dhp = udev->ifaces[iface].dhp;
/* 遍历找 USB_EP_TYPE_BULK 端点 */
```

### 5.5 Kconfig FatFS 挂载点配置
```kconfig
# prj.conf 必须包含以下配置才能正确挂载 USB disk:
CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT=1
CONFIG_FS_FATFS_CUSTOM_MOUNT_POINTS="USBDISK0"
CONFIG_FS_FATFS_EXFAT=y      # SanDisk 大容量 U 盘通常是 exFAT
CONFIG_FS_FATFS_LFN=y
CONFIG_FS_FATFS_LBA64=y      # exFAT 需要 64-bit LBA
CONFIG_HEAP_MEM_POOL_SIZE=32768  # exFAT 比 FAT32 需要更多堆
```

---

## 6. 验证清单（Verification Checklist）

AI 生成代码后，必须按以下清单自检：

### 代码生成
- [ ] `priv->dev` 在 probe 中赋值？
- [ ] `disk_name` 使用 `slot_idx` 而非循环变量？
- [ ] `msd_bulk_xfer` IN 传输未调用 `net_buf_add/reserve`？
- [ ] `msd_bulk_cb` 作为回调传给 `usbh_xfer_alloc`？
- [ ] BOT Reset 使用正确 class request（0xFF）？
- [ ] `cbw_tag` 是 per-instance 字段？
- [ ] sector_count 上限检查在 NULL 检查之后？
- [ ] msd_class_completion_cb 是 no-op？

### 配置生成
- [ ] `CONFIG_USB_HOST_STACK=y` (不是 CONFIG_USB_HOST)？
- [ ] `CONFIG_FS_FATFS_CUSTOM_MOUNT_POINTS="USBDISK0"`？
- [ ] 挂载点宏定义为 `/USBDISK0:` 而非 `/USB`？
- [ ] `CONFIG_HEAP_MEM_POOL_SIZE` 足够大（≥32768）？

### 调试分析
当出现以下错误时，对应处理：

| 错误现象 | 优先排查 |
|----------|----------|
| CBW timeout (-116) | 1. msd_bulk_cb 是否正确注册；2. ep_out 是否从描述符解析 |
| Data phase -5 (EIO) | 1. IN 传输是否预填充了 buf；2. ep_in 地址是否正确 |
| fs_mount -5 | 1. 挂载点是否为 `/USBDISK0:`；2. Custom mount point 配置 |
| fs_opendir -2 (ENOENT) | 同 fs_mount -5 |
| 公开 API 返回 -EINVAL | priv->dev 是否赋值 |
| disk_name 错误 | slot_idx vs 循环变量 |

---

## 7. 测试框架（Test Framework）

### Ztest 套件位置
```
tests/subsys/usb/usbh_msd/src/main.c
```

### 测试分组
**静态测试（无硬件）**：
- t01: NULL 参数检查
- t02: 未注册设备返回错误
- t03: CBW=31B, CSW=13B 尺寸验证
- t04: sector_count > MAX 返回 -EINVAL

**硬件测试（需 USB 闪存）**：
- t05: disk_access_status() = DISK_STATUS_OK
- t06~t10: IOCTL 验证（sector_count/size/erase/sync/init）
- t11: 单扇区读写往返
- t12: 多扇区读写往返
- t13: 最后一个扇区边界测试
- t14: 数据完整性（每扇区唯一 pattern）

### 构建命令
```bash
# Sample 应用
west build -b rd_rw612_bga samples/subsys/usb/host_msd -d aa_rdrw612_host_mass
west flash -d aa_rdrw612_host_mass

# Test harness
west build -b rd_rw612_bga tests/subsys/usb/usbh_msd -d aa_rdrw612_host_msd_harness
west flash -d aa_rdrw612_host_msd_harness
```

---

## 8. AI 工作流程（AI Workflow）

### 任务：新增功能
```
1. 查阅本文档第 3、4、5 节约束
2. 先生成最小实现，不要一次写完
3. 检查第 6 节清单
4. 请求用户构建: west build ...
5. 根据错误日志迭代
```

### 任务：调试分析
```
1. 提供完整错误日志
2. 对照第 6 节错误现象表
3. 查阅第 4 节 Bug Registry
4. 定位最小修复点，勿大范围重写
```

### 任务：代码审查
```
1. 逐行对照第 5 节架构约束
2. 检查第 4 节每个 Bug ID 是否有对应风险
3. 特别关注: priv->dev 赋值、disk_name 编号、挂载点格式
```

---

## 9. 禁止项（DO NOT）

AI **严禁**在此项目中做以下事情：

1. ❌ 用 `SET_INTERFACE` 实现 BOT Reset
2. ❌ 硬编码端点地址 `0x81` / `0x02`
3. ❌ 对 IN 传输的 `net_buf` 调用 `net_buf_add()`
4. ❌ 将 CBW tag 定义为全局变量
5. ❌ 在 `msd_class_completion_cb` 中调用 `k_sem_give`
6. ❌ 挂载点使用 `/USB` 而非 `/USBDISK0:`
7. ❌ `prj.conf` 中使用 `CONFIG_USB_HOST=y`（正确是 `CONFIG_USB_HOST_STACK=y`）
8. ❌ 超时后不等待 cancel 回调直接 return（内存泄漏）
9. ❌ 用 for 循环变量 `i` 给 disk_name 编号

---

## 10. 版本记录（Change Log）

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.0 | 2026-07-29 | 初始驱动实现，完成硬件验证 |
| v1.1 | 2026-07-29 | 修复 9 个 Bug（B01-B09），优化 BOT Reset、内存管理 |
| v1.2 | 2026-07-29 | 修复 FatFS 挂载路径（B10），完整 FS 测试通过 |
| v1.3 | 2026-08-04 | 生成本 AI Harness 文档 |
