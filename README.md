# Renogy-BT2-Reader
Arduino library to interrogate Renogy devices using BT2 dongle over bluetooth

Renogy make a number of solar MPPT controllers, DC:DC converters and other devices.   Their most recent products use a bluetooth dongle known as a [BT2](https://www.renogy.com/bt-2-bluetooth-module/), but Renogy never released any documentation on how to interrogate these devices.  By using Wireshark, I was able to decrypt the protocol used, and I built an Arduino library compatible with Adafruit's nrf52 series devices.  This library has been tested successfully with a [Renogy DCC30S device](https://www.renogy.com/dcc30s-12v-30a-dual-input-dc-dc-on-board-battery-charger-with-mppt/), and may work with other Renogy devices if the registers are known and decribed in the library header file.

This library is strictly use at your own risk.  I am not releasing any code on how to write to registers.  Use the Renogy apps (and any validation they provide) for that please.


## The BT2 Protocol
It took a lot of digging to find how to communicate with a BT2

First, a BLE Central device scans for service 0xFFD0 and a manufacturer ID 0x7DE0 and attempts a connection.   There are two services and two characteristics that need to be set up
```
txService        0000ffd0-0000-1000-8000-00805f9b34fb     // Tx service
txCharacteristic 0000ffd1-0000-1000-8000-00805f9b34fb     // Tx characteristic. Sends data to the BT2

rxService        0000fff0-0000-1000-8000-00805f9b34fb     // Rx service
rxCharacteristic 0000fff1-0000-1000-8000-00805f9b34fb     // Rx characteristic. Receives notifications from the BT2
```
The data format is relatively simple once you read enough data.  All 2 byte numbers are big endian (MSB, then LSB).  A full list of registers and their functions is available [here](/resources).  An example data handshake looks like this:
```
ff 03 01 00  00 07 10 2a                                     Command sent to BT2 on characteristic FFD1
[]                                                           All commands seem to start with 0xFF
   []                                                        0x03 appears to be the command for "read"
      [___]                                                  Starting register to read (0x0100)
             [___]                                           How many registers to read (7)
                   [___]                                     Modbus 16 checksum, MSB first

ff 03 0e 00 64 00 85 00 00 10 10 00 7a 00 00 00 00 31 68     Data received through notification on characteristic FFF1
[]                                                           Replies also start with 0xFF
   []                                                        Acknowledges "read", I believe
      []                                                     Length of data response in bytes (usually 2x registers requested, so 14 here)
         [___]                                               Contents of register 0x0100 (Aux batt SOC, which is 100%)
               [___]                                         Contents of register 0x0101 (Aux batt voltage * 10, so 13.3v)
                     [___] [___] [___] [___] [___]           Contents of registers 0x0102 through 0x0106
                                                   [___]     Modbus 16 checksum


6d 61 69 6e 20 72 65 63 76 20 64 61 74 61 5b 66 66 5d 20 5b  What appears to be a 20 byte ACK sent to BT2 on characteristic FFD1
                                                             it reads "main recv data[ff] [" and the ff corresponds to the first byte
                                                             of every 20 byte notification received; so a 50 byte datagram (3 notifications)
                                                             might trigger an ack sequence like "main recv data[ff] [", "main recv data[77] [",
                                                             and "main recv data[1e] [" depending on data received.  
                                                             The BT2 seems to respond OK without this ACK, but I do it also, as that's what the
                                                             Renogy BT app does
```





