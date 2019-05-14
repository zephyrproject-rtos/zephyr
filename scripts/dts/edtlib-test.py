#!/usr/bin/env python3

import edtlib


edt = edtlib.EDT("test.dts", "../../dts/bindings")
for dev in edt.devices.values():
    print("registers in " + dev.name + ":")
    for i, reg in enumerate(dev.regs):
        print("register " + str(i) + ": ")
        if reg.name is not None:
            print("\tname: " + reg.name)
        print("\taddress: " + hex(reg.addr))
        print("\tsize: " + hex(reg.size))
    print()
