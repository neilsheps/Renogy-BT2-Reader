# Renogy-BT2-Reader
Arduino library to interrogate Renogy devices using BT2 dongle over bluetooth

Renogy make a number of solar MPPT controllers, DC:DC converters and other devices.   Their most recent products use a bluetooth dongle known as a BT2 https://www.renogy.com/bt-2-bluetooth-module/, but Renogy never released any documentation on how to interrogate these devices.  By using Wireshark, I was able to decrypt the protocol used, and I built an Arduino library compatible with Adafruit's nrf52 series devices.  This library has been tested successfully with a Renogy DCC30S device https://www.renogy.com/dcc30s-12v-30a-dual-input-dc-dc-on-board-battery-charger-with-mppt/, and may work with other Renogy devices if the registers are known and decribed in the library header file.

This library is strictly use at your own risk.  I am not releasing any code on how to write to registers.  Use the Renogy apps (and any validation they provide) for that please.





The BT2 Protocol
It took a lot of digging to find how to communicate with a BT2

First, a BLE Central device scans for service 0xFFD0 and a manufacturer ID 0x7DE0 and attempts a connection.   There are two services and two characteristics that need to be set up

tx Service       0000ffD0-0000-1000-8000-00805f9b34fb     // Tx service
txCharacteristic 0000ffD1-0000-1000-8000-00805f9b34fb    	// Tx characteristic.  This sends data to the BT2

rxService        0000ffF0-0000-1000-8000-00805f9b34fb     // Rx service
rxCharacteristic 0000ffF1-0000-1000-8000-00805f9b34fb     // Rx characteristic.  This receives data from the BT2 by notifications

The data format is relatively simple once you read enough data.  First, the Central device (e.g. Adafruit nrf52840) sends a request over the Tx characteristic.   Usually 100-300 ms later you get back a binary file response through a rxCharacteristic notification split into 20 byte packets as necessary.  Thereafter, the Central device sends the BT2 a string that appears to be an acknowledgement of the data received for every 20 byte packet.  It's not clear if this is necessary, as the code seems to work without it, but the Renogy BT app on Android does this, so I kept it.






