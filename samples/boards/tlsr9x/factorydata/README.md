# Factory Data verifier

This Zephyr sample demonstrates the contents of factory data partition. The `factory_partition` is used to store essential configuration data required by the Matter application.

**NOTE**
The partition must be pre-written with binary data generated from the Matter tool before running this sample.
Use the address from DeviceTree `factory_partition`, for ex. at address `0xFF000` size `0x1000`:
```bash
    &flash {
        partitions {
            factory_partition: partition@ff000 {
                label = "factory-data";
                reg = <0xff000 0x1000>;
            };
        };
    };
```

This `factory_partition` is the same for Matter board overlay.


Example output:
```
*** Booting Zephyr OS build v3.1.0-rc1-14633-g45897bdea39d ***
Partition image: offset: 0xff000 size: 0x1000
Parsed Factory Data OK!
Factory Data:
Version: 1
Serial Number: 11223344556677889900
Date: 2022-01-01
Vendor ID: 65521
Product ID: 32773
Vendor Name: Telink Semiconductor
Product Name: not-specified
Part Number: Not available
Product URL: Not available
Hardware Version: 0
Hardware Version String: prerelease
RD UID (Hex): 25809665ABF28A51EB8CB2A99B13F6C8
DAC Certificate (Hex): 308201E83082018EA00302010202086FDCB6ED06F358F9300A06082A1
DAC Private Key (Hex): 7582A14ECC92470B405B9E16DED8BE417A610A13C3E02B2899B470AE1
Spake2 Salt (Hex): AFAB2270EC1C3C55F36C5AEC00FF16385544CE16F219DDF9C03C173AA5904
Spake2 Verifier (Hex): 15A44438310D2879AFC6C58F693840005D9D11A928D9CF01840A5047F
Spake2 Iterations: 1000
Enable Key (Hex): 30303131323233333434353536363737383839394141424243434444454546
User: Not available
Certificate Declaration (Hex): 308201B33082015AA003020102020845DAF39DE47AA08F30B
Discriminator: 3840
Passcode: 20202021
```

And in case of failure:
```
*** Booting Zephyr OS build v3.1.0-rc1-14633-g45897bdea39d ***
Partition image: offset: 0xff000 size: 0x1000
[E] Failed to parse factory data
```
