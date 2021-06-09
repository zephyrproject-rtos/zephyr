## Using mcumgr with Zephyr

### Configuration

The `samples/smp_svr/zephyr/prj.conf` file provides a good starting point for
configuring an application to use *mcumgr*.  The major configuration settings
are described below:

| Setting       | Description   | Default |
| ------------- | ------------- | ------- |
| `CONFIG_MCUMGR` | Enable the mcumgr management library. | n |
| `CONFIG_MCUMGR_CMD_FS_MGMT` | Enable mcumgr handlers for file management | n |
| `CONFIG_MCUMGR_CMD_IMG_MGMT` | Enable mcumgr handlers for image management | n |
| `CONFIG_MCUMGR_CMD_LOG_MGMT` | Enable mcumgr handlers for log management | n |
| `CONFIG_MCUMGR_CMD_OS_MGMT` | Enable mcumgr handlers for OS management | n |
