# Renogy-BT2-Reader
Arduino library to interrogate Renogy devices using BT2 dongle over bluetooth

Renogy make a number of solar MPPT controllers, DC:DC converters and other devices.   Their most recent products use a bluetooth dongle known as a [BT2](https://www.renogy.com/bt-2-bluetooth-module/), but Renogy never released any documentation on how to interrogate these devices.  By using Wireshark, I was able to decrypt the protocol used, and I built an Arduino library compatible with Adafruit's nrf52 series devices.  This library has been tested successfully with a [Renogy DCC30S device](https://www.renogy.com/dcc30s-12v-30a-dual-input-dc-dc-on-board-battery-charger-with-mppt/), and may work with other Renogy devices if the registers are known and decribed in the library header file.

This library is strictly use at your own risk.  I am not releasing any code on how to write to registers.  Use the Renogy apps (and any validation they provide) for that please.


## The BT2 Protocol
It took a lot of digging to find how to communicate with a BT2

First, a BLE Central device scans for service 0xFFD0 and a manufacturer ID 0x7DE0 and attempts a connection.   There are two services and two characteristics that need to be set up
```
tx Service       0000ffD0-0000-1000-8000-00805f9b34fb     // Tx service
txCharacteristic 0000ffD1-0000-1000-8000-00805f9b34fb    // Tx characteristic. Sends data to the BT2

rxService        0000ffF0-0000-1000-8000-00805f9b34fb     // Rx service
rxCharacteristic 0000ffF1-0000-1000-8000-00805f9b34fb     // Rx characteristic. Receives notifications from the BT2
```
The data format is relatively simple once you read enough data.  All 2 byte numbers are bid endian (MSB, then LSB).  An example data handshake looks like this:
```
ff 03 01 00  00 07 10 2a          // command sent to BT2 on tx characteristic FFD1
[]--------------------------------All commands seem to start with 0xFF
   []-----------------------------0x03 appears to be the command for "read"
      [___]-----------------------Starting register to read (0x0100)
             [___]----------------How many registers to read (7)
                   [___]----------Modbus 16 checksum, MSB first

ff 03 0e 00 64 00 85 00 00 10 10 00 7a 00 00 00 00 31 68
[]-------------------------------------------------------- Replies also start with 0xFF
   []----------------------------------------------------- Acknowledges "read", I believe
      []-------------------------------------------------- Length of data response in bytes (usually 2x registers requested)
                                                           Here it's 14, which is everything from response[3] to response[16] inclusive
         [_______________________________________]         14 bytes of data
         [___]-------------------------------------------- Contents of register 0x0100 (Aux batt SOC, which is 100%)







