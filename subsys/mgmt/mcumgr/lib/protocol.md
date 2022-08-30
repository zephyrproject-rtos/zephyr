# SMP (Simple Management Protocol) Protocol Notes

The `mcumgr` SMP server and `mcumgr-cli` SMP client tool ([source](https://github.com/apache/mynewt-mcumgr-cli))
use a custom protocol to send commands and responses between the server and client using a
variety of transports (currently TTY serial or BLE).

The protocol isn't fully documented but the following information has been inferred
from the source code available on Github and using the `-l DEBUG` flag when
executing commands.

## Source Code

**SMP** is based on the earlier **NMP**, which is part of [Apache Mynewt](https://mynewt.apache.org/).

The golang source for the original **newtmgr** is [available here](https://github.com/apache/mynewt-newtmgr),
and can be used to provide some insight into how data is exchanged between the
utility and the device under test.

This repository (`mynewt-mcumgr`) implements an SMP server in **C**,
and a new command-line SMP client called **mcumgr** was created at
[apache/mynewt-mcumgr-cli](https://github.com/apache/mynewt-mcumgr-cli).

## SMP Frame Format

### Endianness

Frames are normally serialized as **Big Endian** when dealing with values > 8 bits. This is
mandatory in NMP, but the SMP implementation does add support for **Little Endian** as an
option at the struct level, as shown below.

### Frame Header

Frames in SMP have the following header format:

```
struct mgmt_hdr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t  nh_op:3;           /* MGMT_OP_[...] */
    uint8_t  _res1:5;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint8_t  _res1:5;
    uint8_t  nh_op:3;           /* MGMT_OP_[...] */
#endif
    uint8_t  nh_flags;          /* Reserved for future flags */
    uint16_t nh_len;            /* Length of the payload */
    uint16_t nh_group;          /* MGMT_GROUP_ID_[...] */
    uint8_t  nh_seq;            /* Sequence number */
    uint8_t  nh_id;             /* Message ID within group */
};
```

The NMP/newtmgr [go source](https://github.com/apache/mynewt-newtmgr/blob/master/nmxact/nmp/nmp.go) is as follows,
without the option to select endianness:

```
type NmpHdr struct {
	Op    uint8 /* 3 bits of opcode */
	Flags uint8
	Len   uint16
	Group uint16
	Seq   uint8
	Id    uint8
}
```

`nh_op` (or `Op` in newtmgr) can be one of the following values:

```
/** Opcodes; encoded in first byte of header. */
#define MGMT_OP_READ            0
#define MGMT_OP_READ_RSP        1
#define MGMT_OP_WRITE           2
#define MGMT_OP_WRITE_RSP       3
```

- **`op`**: The operation code
- **`Flags`**: TBD
- **`Len`**:  The payload len when `Data` is present
- **`Group`**: Commands are organized into groups. Groups are defined
  [here](https://github.com/apache/mynewt-mcumgr/blob/master/mgmt/include/mgmt/mgmt.h).
- **`Seq`**: TBD
- **`Id`**: The command ID to send. Commands in the default `Group` are defined
  [here](https://github.com/apache/mynewt-mcumgr/blob/master/mgmt/include/mgmt/mgmt.h).

SMP header (Little Endian)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  OP |   Res.  |      Flags    |             Length            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            Group ID           |    Sequence   |   Command ID  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### `Data` Payload

If `nh_len` (`Len` in nmp) is non-zero, the `nh_len` byte payload (referred to as **`Data`** in this document) immediately follows the frame header.

### Example Packets

The following example commands show how the different fields work:

#### Simple Read Request: `taskstats`

The following example corresponds to the `taskstats` command ([source](https://github.com/apache/mynewt-mcumgr/blob/master/cmd/os_mgmt/include/os_mgmt/os_mgmt.h)), and
can be seen by running `mcumgr -l DEBUG -c serial taskstats`:

```
Op:    0  # NMGR_OP_READ
Flags: 0
Len:   0  # No payload present
Group: 0  # 0x00 = NMGR_GROUP_ID_DEFAULT
Seq:   0
Id:    2  # 0x02 in group 0x00 = OS_MGMT_ID_TASKSTAT
Data:  [] # No payload (len = 0 above)
```

When serialized this will be sent as `0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x02`.

If this was sent using the serial port, you would get the following request and response:

```
$ mcumgr -l DEBUG -c serial taskstats
2016/11/11 12:45:44 [DEBUG] Writing newtmgr request &{Op:0 Flags:0 Len:0 Group:0 Seq:0 Id:2 Data:[]}
2016/11/11 12:45:44 [DEBUG] Serializing request &{Op:0 Flags:0 Len:0 Group:0 Seq:0 Id:2 Data:[]} into buffer [0 0 0 0 0 0 0 2]
2016/11/11 12:45:44 [DEBUG] Tx packet dump:
00000000  00 00 00 00 00 00 00 02                           |........|

2016/11/11 12:45:44 [DEBUG] Writing [6 9] to data channel
2016/11/11 12:45:44 [DEBUG] Writing [65 65 111 65 65 65 65 65 65 65 65 65 65 105 66 67] to data channel
2016/11/11 12:45:44 [DEBUG] Writing [10] to data channel
2016/11/11 12:45:44 [DEBUG] Reading [6 9 65 65 111 65 65 65 65 65 65 65 65 65 65 105 66 67] from data channel
2016/11/11 12:45:44 [DEBUG] Rx packet dump:
00000000  00 00 00 00 00 00 00 02                           |........|

2016/11/11 12:45:44 [DEBUG] Deserialized response &{Op:0 Flags:0 Len:0 Group:0 Seq:0 Id:2 Data:[]}
2016/11/11 12:45:44 [DEBUG] Reading [13] from data channel
2016/11/11 12:45:44 [DEBUG] Reading [6 9 65 90 119 66 65 81 71 83 65 65 65 65 65 114 57 105 99 109 77 65 90 88 82 104 99 50 116 122 118 50 82 112 90 71 120 108 118 50 82 119 99 109 108 118 71 80 57 106 100 71 108 107 65 71 86 122 100 71 70 48 90 81 70 109 99 51 82 114 100 88 78 108 71 66 108 109 99 51 82 114 99 50 108 54 71 69 66 109 89 51 78 51 89 50 53 48 71 103 65 85 102 109 112 110 99 110 86 117 100 71 108 116 90 82 111 65 69 53 120 80 98 71 120 104 99 51 82 102] from data channel
2016/11/11 12:45:44 [DEBUG] Reading [4 20 89 50 104 108 89 50 116 112 98 103 66 115 98 109 86 52 100 70 57 106 97 71 86 106 97 50 108 117 65 80 57 109 89 109 120 108 88 50 120 115 118 50 82 119 99 109 108 118 65 71 78 48 97 87 81 66 90 88 78 48 89 88 82 108 65 109 90 122 100 71 116 49 99 50 85 89 79 109 90 122 100 71 116 122 97 88 111 89 85 71 90 106 99 51 100 106 98 110 81 90 54 112 120 110 99 110 86 117 100 71 108 116 90 82 107 74 82 87 120 115 89 88 78 48 88 50 78 111] from data channel
2016/11/11 12:45:44 [DEBUG] Reading [4 20 90 87 78 114 97 87 52 65 98 71 53 108 101 72 82 102 89 50 104 108 89 50 116 112 98 103 68 47 98 109 74 115 90 88 86 104 99 110 82 102 89 110 74 112 90 71 100 108 118 50 82 119 99 109 108 118 66 87 78 48 97 87 81 67 90 88 78 48 89 88 82 108 65 87 90 122 100 71 116 49 99 50 85 89 72 50 90 122 100 71 116 122 97 88 111 90 65 81 66 109 89 51 78 51 89 50 53 48 71 103 65 84 113 89 78 110 99 110 86 117 100 71 108 116 90 81 66 115] from data channel
2016/11/11 12:45:44 [DEBUG] Reading [4 20 98 71 70 122 100 70 57 106 97 71 86 106 97 50 108 117 65 71 120 117 90 88 104 48 88 50 78 111 90 87 78 114 97 87 52 65 47 50 100 105 98 71 86 119 99 110 66 111 118 50 82 119 99 109 108 118 65 87 78 48 97 87 81 68 90 88 78 48 89 88 82 108 65 87 90 122 100 71 116 49 99 50 85 89 48 50 90 122 100 71 116 122 97 88 111 90 65 86 66 109 89 51 78 51 89 50 53 48 71 81 113 68 90 51 74 49 98 110 82 112 98 87 85 69 98 71 120 104] from data channel
2016/11/11 12:45:44 [DEBUG] Reading [4 20 99 51 82 102 89 50 104 108 89 50 116 112 98 103 66 115 98 109 86 52 100 70 57 106 97 71 86 106 97 50 108 117 65 80 47 47 47 56 85 88] from data channel
2016/11/11 12:45:44 [DEBUG] Rx packet dump:
00000000  01 01 01 92 00 00 00 02  bf 62 72 63 00 65 74 61  |.........brc.eta|
00000010  73 6b 73 bf 64 69 64 6c  65 bf 64 70 72 69 6f 18  |sks.didle.dprio.|
00000020  ff 63 74 69 64 00 65 73  74 61 74 65 01 66 73 74  |.ctid.estate.fst|
00000030  6b 75 73 65 18 19 66 73  74 6b 73 69 7a 18 40 66  |kuse..fstksiz.@f|
00000040  63 73 77 63 6e 74 1a 00  14 7e 6a 67 72 75 6e 74  |cswcnt...~jgrunt|
00000050  69 6d 65 1a 00 13 9c 4f  6c 6c 61 73 74 5f 63 68  |ime....Ollast_ch|
00000060  65 63 6b 69 6e 00 6c 6e  65 78 74 5f 63 68 65 63  |eckin.lnext_chec|
00000070  6b 69 6e 00 ff 66 62 6c  65 5f 6c 6c bf 64 70 72  |kin..fble_ll.dpr|
00000080  69 6f 00 63 74 69 64 01  65 73 74 61 74 65 02 66  |io.ctid.estate.f|
00000090  73 74 6b 75 73 65 18 3a  66 73 74 6b 73 69 7a 18  |stkuse.:fstksiz.|
000000a0  50 66 63 73 77 63 6e 74  19 ea 9c 67 72 75 6e 74  |Pfcswcnt...grunt|
000000b0  69 6d 65 19 09 45 6c 6c  61 73 74 5f 63 68 65 63  |ime..Ellast_chec|
000000c0  6b 69 6e 00 6c 6e 65 78  74 5f 63 68 65 63 6b 69  |kin.lnext_checki|
000000d0  6e 00 ff 6e 62 6c 65 75  61 72 74 5f 62 72 69 64  |n..nbleuart_brid|
000000e0  67 65 bf 64 70 72 69 6f  05 63 74 69 64 02 65 73  |ge.dprio.ctid.es|
000000f0  74 61 74 65 01 66 73 74  6b 75 73 65 18 1f 66 73  |tate.fstkuse..fs|
00000100  74 6b 73 69 7a 19 01 00  66 63 73 77 63 6e 74 1a  |tksiz...fcswcnt.|
00000110  00 13 a9 83 67 72 75 6e  74 69 6d 65 00 6c 6c 61  |....gruntime.lla|
00000120  73 74 5f 63 68 65 63 6b  69 6e 00 6c 6e 65 78 74  |st_checkin.lnext|
00000130  5f 63 68 65 63 6b 69 6e  00 ff 67 62 6c 65 70 72  |_checkin..gblepr|
00000140  70 68 bf 64 70 72 69 6f  01 63 74 69 64 03 65 73  |ph.dprio.ctid.es|
00000150  74 61 74 65 01 66 73 74  6b 75 73 65 18 d3 66 73  |tate.fstkuse..fs|
00000160  74 6b 73 69 7a 19 01 50  66 63 73 77 63 6e 74 19  |tksiz..Pfcswcnt.|
00000170  0a 83 67 72 75 6e 74 69  6d 65 04 6c 6c 61 73 74  |..gruntime.llast|
00000180  5f 63 68 65 63 6b 69 6e  00 6c 6e 65 78 74 5f 63  |_checkin.lnext_c|
00000190  68 65 63 6b 69 6e 00 ff  ff ff                    |heckin....|

2016/11/11 12:45:44 [DEBUG] Deserialized response &{Op:1 Flags:1 Len:402 Group:0 Seq:0 Id:2 Data:[191 98 114 99 0 101 116 97 115 107 115 191 100 105 100 108 101 191 100 112 114 105 111 24 255 99 116 105 100 0 101 115 116 97 116 101 1 102 115 116 107 117 115 101 24 25 102 115 116 107 115 105 122 24 64 102 99 115 119 99 110 116 26 0 20 126 106 103 114 117 110 116 105 109 101 26 0 19 156 79 108 108 97 115 116 95 99 104 101 99 107 105 110 0 108 110 101 120 116 95 99 104 101 99 107 105 110 0 255 102 98 108 101 95 108 108 191 100 112 114 105 111 0 99 116 105 100 1 101 115 116 97 116 101 2 102 115 116 107 117 115 101 24 58 102 115 116 107 115 105 122 24 80 102 99 115 119 99 110 116 25 234 156 103 114 117 110 116 105 109 101 25 9 69 108 108 97 115 116 95 99 104 101 99 107 105 110 0 108 110 101 120 116 95 99 104 101 99 107 105 110 0 255 110 98 108 101 117 97 114 116 95 98 114 105 100 103 101 191 100 112 114 105 111 5 99 116 105 100 2 101 115 116 97 116 101 1 102 115 116 107 117 115 101 24 31 102 115 116 107 115 105 122 25 1 0 102 99 115 119 99 110 116 26 0 19 169 131 103 114 117 110 116 105 109 101 0 108 108 97 115 116 95 99 104 101 99 107 105 110 0 108 110 101 120 116 95 99 104 101 99 107 105 110 0 255 103 98 108 101 112 114 112 104 191 100 112 114 105 111 1 99 116 105 100 3 101 115 116 97 116 101 1 102 115 116 107 117 115 101 24 211 102 115 116 107 115 105 122 25 1 80 102 99 115 119 99 110 116 25 10 131 103 114 117 110 116 105 109 101 4 108 108 97 115 116 95 99 104 101 99 107 105 110 0 108 110 101 120 116 95 99 104 101 99 107 105 110 0 255 255 255]}
Return Code = 0
      task pri tid  runtime      csw    stksz   stkuse last_checkin next_checkin
  bleuart_bridge   5   2        0  1288579      256       31        0        0
   bleprph   1   3        4     2691      336      211        0        0
      idle 255   0  1285199  1343082       64       25        0        0
    ble_ll   0   1     2373    60060       80       58        0        0
```

#### Group Read Request: `image list`

The following command lists images on the device and uses commands from `Group`
0x01 (`MGMT_GROUP_ID_IMAGE`), and was generated with `$ mcumgr -l DEBUG -c serial image list`:

> See [img_mgmt](https://github.com/apache/mynewt-mcumgr/tree/master/cmd/img_mgmt)
for a full list of commands in the IMAGE `Group`.

```
$ mcumgr -l DEBUG -c serial image list
2016/11/11 12:25:51 [DEBUG] Writing newtmgr request &{Op:0 Flags:0 Len:0 Group:1 Seq:0 Id:0 Data:[]}
2016/11/11 12:25:51 [DEBUG] Serializing request &{Op:0 Flags:0 Len:0 Group:1 Seq:0 Id:0 Data:[]} into buffer [0 0 0 0 0 1 0 0]
2016/11/11 12:25:51 [DEBUG] Tx packet dump:
00000000  00 00 00 00 00 01 00 00                           |........|

2016/11/11 12:25:51 [DEBUG] Writing [6 9] to data channel
2016/11/11 12:25:51 [DEBUG] Writing [65 65 111 65 65 65 65 65 65 65 69 65 65 68 99 119] to data channel
2016/11/11 12:25:51 [DEBUG] Writing [10] to data channel
2016/11/11 12:25:51 [DEBUG] Reading [6 9 65 65 111 65 65 65 65 65 65 65 69 65 65 68 99 119] from data channel
2016/11/11 12:25:51 [DEBUG] Rx packet dump:
00000000  00 00 00 00 00 01 00 00                           |........|

2016/11/11 12:25:51 [DEBUG] Deserialized response &{Op:0 Flags:0 Len:0 Group:1 Seq:0 Id:0 Data:[]}
2016/11/11 12:25:51 [DEBUG] Reading [13] from data channel
2016/11/11 12:25:51 [DEBUG] Reading [6 9 65 73 85 66 65 81 66 55 65 65 69 65 65 76 57 109 97 87 49 104 90 50 86 122 110 55 57 107 99 50 120 118 100 65 66 110 100 109 86 121 99 50 108 118 98 109 85 119 76 106 77 117 77 71 82 111 89 88 78 111 87 67 68 83 84 76 77 70 69 49 81 88 75 55 85 81 110 53 121 48 114 110 104 104 50 87 49 113 47 102 120 71 50 48 103 115 54 121 48 48 113 75 101 79 48 71 104 105 98 50 57 48 89 87 74 115 90 102 86 110 99 71 86 117 90 71 108 117] from data channel
2016/11/11 12:25:51 [DEBUG] Reading [4 20 90 47 82 112 89 50 57 117 90 109 108 121 98 87 86 107 57 87 90 104 89 51 82 112 100 109 88 49 47 47 57 114 99 51 66 115 97 88 82 84 100 71 70 48 100 88 77 65 47 49 78 116] from data channel
2016/11/11 12:25:51 [DEBUG] Rx packet dump:
00000000  01 01 00 7b 00 01 00 00  bf 66 69 6d 61 67 65 73  |...{.....fimages|
00000010  9f bf 64 73 6c 6f 74 00  67 76 65 72 73 69 6f 6e  |..dslot.gversion|
00000020  65 30 2e 33 2e 30 64 68  61 73 68 58 20 d2 4c b3  |e0.3.0dhashX .L.|
00000030  05 13 54 17 2b b5 10 9f  9c b4 ae 78 61 d9 6d 6a  |..T.+......xa.mj|
00000040  fd fc 46 db 48 2c eb 2d  34 a8 a7 8e d0 68 62 6f  |..F.H,.-4....hbo|
00000050  6f 74 61 62 6c 65 f5 67  70 65 6e 64 69 6e 67 f4  |otable.gpending.|
00000060  69 63 6f 6e 66 69 72 6d  65 64 f5 66 61 63 74 69  |iconfirmed.facti|
00000070  76 65 f5 ff ff 6b 73 70  6c 69 74 53 74 61 74 75  |ve...ksplitStatu|
00000080  73 00 ff                                          |s..|

2016/11/11 12:25:51 [DEBUG] Deserialized response &{Op:1 Flags:1 Len:123 Group:1 Seq:0 Id:0 Data:[191 102 105 109 97 103 101 115 159 191 100 115 108 111 116 0 103 118 101 114 115 105 111 110 101 48 46 51 46 48 100 104 97 115 104 88 32 210 76 179 5 19 84 23 43 181 16 159 156 180 174 120 97 217 109 106 253 252 70 219 72 44 235 45 52 168 167 142 208 104 98 111 111 116 97 98 108 101 245 103 112 101 110 100 105 110 103 244 105 99 111 110 102 105 114 109 101 100 245 102 97 99 116 105 118 101 245 255 255 107 115 112 108 105 116 83 116 97 116 117 115 0 255]}
Images:
 slot=0
    version: 0.3.0
    bootable: true
    flags: active confirmed
    hash: d24cb3051354172bb5109f9cb4ae7861d96d6afdfc46db482ceb2d34a8a78ed0
Split status: N/A
```

When serialised this will be sent as `0x00 0x00 0x00 0x00 0x00 0x01 0x00 0x00`.

## Transports

### Mcumgr/Newtmgr SMP Client Over Serial

`mcumgr` or `newtmgr` can be used over TTY serial with the following parameters
to connect to a SMP server running on the target device:

- Baud Rate: 115200
- HW Flow Control: None

### Mcumgr/Newtmgr SMP Client Over BLE

`mcumgr` or `newtmgr` can be used over BLE with the following GATT service and
characteristic UUIDs to connect to a SMP server running on the target device:

- **Service UUID**: `8D53DC1D-1DB7-4CD3-868B-8A527460AA84`
- **Characteristic UUID**: `DA2E7828-FBCE-4E01-AE9E-261174997C48`

The  "SMP" GATT service consists of one **write no-rsp characteristic**
for SMP requests: a single-byte characteristic that
can only accepts write-without-response commands. The contents of
each write command contains an SMP request.

SMP responses are sent back in the form of unsolicited notifications
from the same characteristic.
