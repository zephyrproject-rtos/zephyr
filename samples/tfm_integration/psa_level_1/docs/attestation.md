# Initial Attestation

Initial attestation is used to provide a snapshot of key details about the
device, and can be used to verify device authenticity through shared keys.

The attestation process is based around the **PSA initial attestation token
(IAT)** ([IETF draft][IA1]), which is generated via a request to the secure
processing environment (SPE), and signed using a private attestation key only
accessible inside the SPE.

[IA1]:https://datatracker.ietf.org/doc/draft-tschofenig-rats-psa-token/

## Token Technical Details

This IAT response is encoded in **CBOR** ([RFC7049][TTD1]), wrapped and
signed using **CBOR Object Signing and Encryption (COSE)** ([RFC8152][TTD1]),
with a tagged `COSE_Sign1` structure (CBOR tag `18`).

The current TF-M implementation supports **ECDSA P256** with **SHA256** hashes.

[TTD1]:https://tools.ietf.org/html/rfc7049
[TTD2]:https://tools.ietf.org/html/rfc8152

## Key-Based Device Verification

A public/private key pair is used between the attesting TF-M device and the
remote server to establish the TF-M device's identity:

- The remote server holds a copy of the public attestation key
- The TF-M device holds the private attestation key, either in secure storage
  or via a HW security element or Crypto peripheral.

Every time an initial attestation token is requested by the remote server from
the TF-M device, the response packet will be signed using the private
attestation key, and the signature can be verified by the remote server using
the public key it has on record.

### Example Verification Workflow

The follow sequence diagram illustrates how IAT **might** be used for device
authentication:

![][fig1]

[fig1]:img/iat_auth.png

### Nonce/Challenge

A key requirement for a secure attestation process is the use of a
32/48/64-byte server-generated [nonce][NCP1] or challenge.

A unique value is assigned to the challenge field to ensure that previous IAT
responses can not be replayed to the remote server, potentially allowing an
attacker to bypass the device authentication process.

**Always assign a unique value to the nonce field for every IAT request!**

[NCP1]:https://en.wikipedia.org/wiki/Cryptographic_nonce

### Exporting the Public Key

In order to make effective use of IAT for device authentication, the remote
server must have the public attestation key that matches the private
attestation key held by the TF-M device.

Since the MPS2+/AN521 board does not have HW crypto support, it uses a
hard-coded public/private key pair located in the TF-M tree at:
`[tfmroot]/platform/ext/common/tfm_initial_attestation_key.pem`

> **NOTE**: The default key should **NEVER** be used in the real-world, and a
  custom key **MUST** but generated per device on any hardware being used in
  the real world!

The content of this key can be displayed via the following command, which
should match the values found in
`tfm_initial_attestation_key_material.c`:

```
$ openssl ec -inform pem -in \
[zephyr]/ext/tfm/tfm/platform/ext/common/tfm_initial_attestation_key.pem \
-pubout -text

read EC key
Private-Key: (256 bit)
priv:
    00:a9:b4:54:b2:6d:6f:90:a4:ea:31:19:35:64:cb:
    a9:1f:ec:6f:9a:00:2a:7d:c0:50:4b:92:a1:93:71:
    34:58:5f
pub:
    04:79:eb:a9:0e:8b:f4:50:a6:75:15:76:ad:45:99:
    b0:7a:df:93:8d:a3:bb:0b:d1:7d:00:36:ed:49:a2:
    d0:fc:3f:bf:cd:fa:89:56:b5:68:bf:db:86:73:e6:
    48:d8:b5:8d:92:99:55:b1:4a:26:c3:08:0f:34:11:
    7d:97:1d:68:64
ASN1 OID: prime256v1
NIST CURVE: P-256
writing EC key
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEeeupDov0UKZ1FXatRZmwet+TjaO7
C9F9ADbtSaLQ/D+/zfqJVrVov9uGc+ZI2LWNkplVsUomwwgPNBF9lx1oZA==
-----END PUBLIC KEY-----
```

For testing purposes, you can generate a .pem file of the public key using
the following command:

```
$ openssl ec \
-in [zephyr]/ext/tfm/tfm/platform/ext/common/tfm_initial_attestation_key.pem \
-pubout -out publickey.pem
```

This .pem file can be used with tools like `check_iat` to verify the signature
of IAT packets, discussed in the example section of this guide.

### IAT Verification via `check_iat`

TF-M conveniently provides an IAT verification tool in the
`tools/iat-verifier` folder. Follow the instructions there to install the
tool, and you can verify the above IAT response packet by:

1. Converting the hex dump to a binary CBOR file via xxd (removing everything
   except the hex values before saving the hex dump as a text file):

```
$ xxd -r -p docs/samples/iatresp.cbor.txt docs/samples/iatresp.cbor
```

2. Checking the signature of the CBOR file using the public key in .pem
   format, optionally display the decoded CBOR IAT data in JSON format (`-p`):

```
$ check_iat -k docs/samples/publickey.pem -p docs/samples/iatresp.bin
Signature OK
...
```

## Example IAT Response

If a server sends an IAT request with the following 64-byte nonce ...

```
u8_t nonce_buf[ATT_MAX_TOKEN_SIZE] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0
};
```

... we should get back the following IAT response, assuming the default
key pair was used:

```
          0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00000000 D2 84 43 A1 01 26 A1 04 58 20 07 8C 18 F1 10 F4 ..C..&..X ......
00000010 32 FF 78 0C D8 DA E5 80 69 A2 A0 D8 22 77 CB C6 2.x.....i..."w..
00000020 64 50 C8 58 1D D4 7D 96 A2 2E 59 01 80 AA 3A 00 dP.X..}...Y...:.
00000030 01 24 FF 58 40 00 11 22 33 44 55 66 77 88 99 AA .$.X@.."3DUfw...
00000040 BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA ......."3DUfw...
00000050 BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA ......."3DUfw...
00000060 BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA ......."3DUfw...
00000070 BB CC DD EE FF 3A 00 01 24 FB 58 20 A0 A1 A2 A3 .....:..$.X ....
00000080 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF B0 B1 B2 B3 ................
00000090 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF 3A 00 01 25 ............:..%
000000A0 01 77 77 77 77 2E 74 72 75 73 74 65 64 66 69 72 .wwww.trustedfir
000000B0 6D 77 61 72 65 2E 6F 72 67 3A 00 01 24 F7 71 50 mware.org:..$.qP
000000C0 53 41 5F 49 4F 54 5F 50 52 4F 46 49 4C 45 5F 31 SA_IOT_PROFILE_1
000000D0 3A 00 01 25 00 58 21 01 FA 58 75 5F 65 86 27 CE :..%.X!..Xu_e.'.
000000E0 54 60 F2 9B 75 29 67 13 24 8C AE 7A D9 E2 98 4B T`..u)g.$..z...K
000000F0 90 28 0E FC BC B5 02 48 3A 00 01 24 FC 72 30 36 .(.....H:..$.r06
00000100 30 34 35 36 35 32 37 32 38 32 39 31 30 30 31 30 0456527282910010
00000110 3A 00 01 24 FA 58 20 AA AA AA AA AA AA AA AA BB :..$.X .........
00000120 BB BB BB BB BB BB BB CC CC CC CC CC CC CC CC DD ................
00000130 DD DD DD DD DD DD DD 3A 00 01 24 F8 20 3A 00 01 .......:..$. :..
00000140 24 F9 19 30 00 3A 00 01 24 FD 81 A6 01 68 4E 53 $..0.:..$....hNS
00000150 50 45 5F 53 50 45 04 65 30 2E 30 2E 30 03 00 02 PE_SPE.e0.0.0...
00000160 58 20 87 1D AC 20 24 E2 1A 8D E9 0A A2 67 A4 35 X ... $......g.5
00000170 97 2C 70 D4 7F 50 2A E9 15 3B B3 20 78 6B FC DE .,p..P*..;. xk..
00000180 43 7E 06 66 53 48 41 32 35 36 05 58 20 BF E6 D8 C~.fSHA256.X ...
00000190 6F 88 26 F4 FF 97 FB 96 C4 E6 FB C4 99 3E 46 19 o.&..........>F.
000001A0 FC 56 5D A2 6A DF 34 C3 29 48 9A DC 38 58 40 D9 .V].j.4.)H..8X@.
000001B0 49 32 21 DB 84 16 89 A7 43 33 E4 9C DF EF 55 07 I2!.....C3....U.
000001C0 C2 81 85 C7 AE 54 77 D9 A1 66 6A B0 76 77 7A AE .....Tw..fj.vwz.
000001D0 C5 20 A1 42 C5 F8 8E 9A 70 6F 63 0D AA DD 63 F3 . .B....poc...c.
000001E0 C7 18 A6 94 D1 FE 34 46 1F E8 3A 6D 30 61 0E    ......4F..:m0a.
```

The contents of the IAT packet can be analyzed using `decompile_token`, which
is installed along with the `check_iat` command described earlier in this
guide:

```
$ decompile_token docs/samples/iatresp.cbor
boot_seed: !!binary |
  oKGio6SlpqeoqaqrrK2ur7CxsrO0tba3uLm6u7y9vr8=
challenge: !!binary |
  ABEiM0RVZneImaq7zN3u/wARIjNEVWZ3iJmqu8zd7v8AESIzRFVmd4iZqrvM3e7/ABEiM0RVZneI
  maq7zN3u/w==
client_id: -1
hardware_id: 060456527282910010
implementation_id: !!binary |
  qqqqqqqqqqq7u7u7u7u7u8zMzMzMzMzM3d3d3d3d3d0=
instance_id: !!binary |
  AfpYdV9lhifOVGDym3UpZxMkjK562eKYS5AoDvy8tQJI
originator: www.trustedfirmware.org
profile_id: PSA_IOT_PROFILE_1
security_lifecycle: SL_SECURED
sw_components:
- sw_component_type: NSPE_SPE
  sw_component_version: 0.0.0
  3: 0
  measurement_value: !!binary |
    hx2sICTiGo3pCqJnpDWXLHDUf1Aq6RU7syB4a/zeQ34=
  measurement_description: SHA256
  signer_id: !!binary |
    v+bYb4gm9P+X+5bE5vvEmT5GGfxWXaJq3zTDKUia3Dg=
```

Binary values are encoded in **BASE64**. Decoding the `challenge` value from
BASE64 to hex, for example, yields the following, matching the nonce data
provided during the IAT request (newlines added for clarity):

```
00112233445566778899AABBCCDDEEFF
00112233445566778899AABBCCDDEEFF
00112233445566778899AABBCCDDEEFF
00112233445566778899AABBCCDDEEFF
```

## Sample Attestation Token Usage

### Unidirectional Data Attestation

In situations where data is being sent unidirectionally or asynchronously, it
may not be possible to assign a unique nonce/challenge value from a remote
server.

This would apply with an intermediary MQTT broker, for example, where a TF-M
based sensor node pushes data to the MQTT broker, and a remote data aggregation
server asynchronously connects to the MQTT broker to retrieve the data at a
later date, but where data still needs to validated for provenance, etc.

In this situation, to 'attest' to the authenticity of the data sent to the MQTT
broker, we could repurpose the 32/48/64-byte challenge field to contain the
sensor data, along with a unique timestamp or some other unique field, and the
entire attestation packet (including the sensor data) will then be signed with
the private key on the TF-M device, and the signature verified by the
aggregating server via the public key(s) it has on record.

The follow sequence diagram illustrates how this might work:

![][fig2]

[fig2]:img/iat_async_data.png

#### Tradeoffs

An important tradeoff of this approach is that instead of sending relatively
small packets of sensor data, an entire attestation token needs to be sent
per sample, which is close to 500 bytes in length. Sensor payloads are also
limited to 32/48/64 bytes in length.

If bandwidth is an issue, an alternative approach may be to use attestation for
a public key associated with the sensor data, and the remote server can retreive
the public key in one attested operation, and decrypt future sensor data samples
using the attested public key. This approach potentially allows for key updates,
as well as distinct keys per sensor type if relevant, and allows for lower
per-sample bandwidth requirements.
