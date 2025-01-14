# LR11xx Ping Shell example

## Description

This sample provides a simple shell command to send pings between LoRa devices.

When starting up, it will listen to ping requests and will reply to any received.

You can send a ping request by typing

```
lorademo ping
```

## Configuration

Several parameters can be updated in [`../common/apps_configuration.h`](../common/apps_configuration.h) header file, refer to [`../common/README.md`](../common/README.md) for more details.

Several parameters can be updated in [`main_ping_pong.h`](main_ping_pong.h) header file:

| Constant           | Comments                    |
| ------------------ | --------------------------- |
| `RX_TIMEOUT_VALUE` | timeout value for reception |

### Build and board configuration

You will need to build for a board with a LoRa transceiver or with such a shield.

For example:

```
west build -- -DSHIELD=semtech_lr1120mb1xxs
```

You might want to customize the LoRa transceiver configuration by adding a file in the `boards` subdirectory or via `app.overlay`.

For example, to set a TCXO instead of the default XTAL:

```
&lora_semtech_lr11xxmb1xxs {
    tcxo-wakeup-time = <7>;
};
```
