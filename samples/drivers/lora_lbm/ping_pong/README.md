# LR11xx Ping Pong example

## Description

The application will automatically set the device in Ping-Pong mode.

The sample code will be used to perform test for both LoRa and FSK modem tests. Define macro `PACKET_TYPE` to `LR1110_RADIO_PKT_TYPE_LORA` or `LR1110_RADIO_PKT_TYPE_GFSK` (in file [`../common/apps_configuration.h`](../common/apps_configuration.h)) to enable each modem in the test.

The application can make device work as master or slave in Ping-Pong test. When starting up, it assumes to be a master and will send 'PING' packets and receive 'PONG' packets alternatively. When a 'PING' packet received, the device switches itself to slave mode, then send 'PONG' packets and expect to receive 'PING' packets. In slave mode, if the device receive a 'non-PING' packet, it will reset to master mode and start the previous process again.

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
&lora_semtech_lr1120mb1xxs {
    tcxo-wakeup-time = <7>;
};
```
