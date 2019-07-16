#!/usr/bin/env python3
#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import dbus
try:
  from gi.repository import GObject
except ImportError:
  import gobject as GObject
import sys
import numpy as np

from dbus.mainloop.glib import DBusGMainLoop

bus = None
mainloop = None

BLUEZ_SERVICE_NAME = 'org.bluez'
DBUS_OM_IFACE =      'org.freedesktop.DBus.ObjectManager'
DBUS_PROP_IFACE =    'org.freedesktop.DBus.Properties'

GATT_SERVICE_IFACE = 'org.bluez.GattService1'
GATT_CHRC_IFACE =    'org.bluez.GattCharacteristic1'

ESS_SVC_UUID =       '0000181a-0000-1000-8000-00805f9b34fb'
ESS_TEMP_UUID =      '00002a6e-0000-1000-8000-00805f9b34fb'
ESS_HEAT_UUID =      '00002a7a-0000-1000-8000-00805f9b34fb'
ESS_HUM_UUID =       '00002a6f-0000-1000-8000-00805f9b34fb'

HR_MSRMT_UUID =      '00002a37-0000-1000-8000-00805f9b34fb'
BODY_SNSR_LOC_UUID = '00002a38-0000-1000-8000-00805f9b34fb'
HR_CTRL_PT_UUID =    '00002a39-0000-1000-8000-00805f9b34fb'


# The objects that we interact with.
hr_service = None
ess_service = None

hr_msrmt_chrc = None
body_snsr_loc_chrc = None
hr_ctrl_pt_chrc = None

ess_msrmt_temp = None
ess_msrmt_heat = None
ess_msrmt_hum  = None

temperature = None	# read val
heat_idx = None		# write val
humidity = None		# read val


def generic_error_cb(error):
    print('D-Bus call failed: ' + str(error))
    mainloop.quit()


def temp_val_cb(value):
    if len(value) != 2:
        print("Temperature Error: invalid data")
        return
    global temperature
    temperature = np.int16((value[1] << 8) + value[0])
    if (temperature > 1250) or (temperature < -500):
        print("Temperature Error: value: %d is out of range" % (temperature))
        return
    # print("Temperature = %d" % (temperature))


def hum_val_cb(value):
    if len(value) != 2:
        print("Humidity Error: invalid data")
        return
    global humidity
    humidity = np.int16((value[1] << 8) + value[0])
    if (humidity > 1000) or (humidity < 0):
        print("Humidity Error: value: %d is out of range" % (humidity))
        return
    # print("humidity = %d" % (humidity))


def write_handler_cb(val):
    return


def start_client():
    # Read the Temperature value and print it asynchronously.
    ess_msrmt_temp[0].ReadValue({}, reply_handler=temp_val_cb,
                                    error_handler=generic_error_cb,
                                    dbus_interface=GATT_CHRC_IFACE)

    # Read the Humidity value and print it asynchronously.
    ess_msrmt_hum[0].ReadValue({}, reply_handler=hum_val_cb,
                                   error_handler=generic_error_cb,
                                   dbus_interface=GATT_CHRC_IFACE)

    # Write the Heat index value
    global heat_idx
    msb = np.uint8(heat_idx >> 8)
    lsb = np.uint8(heat_idx & 0x0FF)
    ess_msrmt_heat[0].WriteValue([lsb,msb], {},
                                 dbus_interface=GATT_CHRC_IFACE)


def process_chrc(chrc_path):
    chrc = bus.get_object(BLUEZ_SERVICE_NAME, chrc_path)
    chrc_props = chrc.GetAll(GATT_CHRC_IFACE,
                             dbus_interface=DBUS_PROP_IFACE)

    uuid = chrc_props['UUID']

    if uuid == ESS_TEMP_UUID:
        global ess_msrmt_temp
        ess_msrmt_temp = (chrc, chrc_props)
    elif uuid == ESS_HEAT_UUID:
        global ess_msrmt_heat
        ess_msrmt_heat = (chrc, chrc_props)
    elif uuid == ESS_HUM_UUID:
        global ess_msrmt_hum
        ess_msrmt_hum = (chrc, chrc_props)
    else:
        print('Unrecognized characteristic: ' + uuid)

    return True


def process_ess_service(service_path, chrc_paths):
    service = bus.get_object(BLUEZ_SERVICE_NAME, service_path)
    service_props = service.GetAll(GATT_SERVICE_IFACE,
                                   dbus_interface=DBUS_PROP_IFACE)

    uuid = service_props['UUID']

    if uuid != ESS_SVC_UUID:
        return False

    # print('Environmental Sensing Service found: ' + service_path)

    # Process the characteristics.
    for chrc_path in chrc_paths:
        process_chrc(chrc_path)

    global ess_service
    ess_service = (service, service_props, service_path)

    return True


def interfaces_removed_cb(object_path, interfaces):
    if not hr_service:
        return

    if object_path == hr_service[2]:
        print('Service was removed')
        mainloop.quit()


def timeout():
    global mainloop
    mainloop.quit()


def bt_exchange(external_tmp):
    # Set up the main loop.
    DBusGMainLoop(set_as_default=True)
    global bus
    bus = dbus.SystemBus()
    global mainloop
    mainloop = GObject.MainLoop()
    GObject.timeout_add(1000, timeout)
    global heat_idx
    heat_idx = external_tmp

    om = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, '/'), DBUS_OM_IFACE)
    om.connect_to_signal('InterfacesRemoved', interfaces_removed_cb)

    # print('Getting objects...')
    objects = om.GetManagedObjects()
    chrcs = []

    # List characteristics found
    for path, interfaces in objects.items():
        if GATT_CHRC_IFACE not in interfaces.keys():
            continue
        chrcs.append(path)

    # List sevices found
    for path, interfaces in objects.items():
        if GATT_SERVICE_IFACE not in interfaces.keys():
            continue

        chrc_paths = [d for d in chrcs if d.startswith(path + "/")]

        if process_ess_service(path, chrc_paths):
            break

    if not ess_service:
        # print('No Environmental Sensing Service found')
        sys.exit(1)

    start_client()
    mainloop.run()

    global temperature
    global humidity
    ret_val = {'T_reel' : temperature, 'H_reel' : humidity}
    return ret_val


if __name__ == "__main__":
    bt_exchange(226)
