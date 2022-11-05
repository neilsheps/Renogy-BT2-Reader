# Renogy-BT2-Reader
Arduino library to interrogate Renogy devices using BT2 dongle over bluetooth

Renogy make a number of solar MPPT controllers, DC:DC converters and other devices.   Their most recent products use a bluetooth dongle known as a [BT2](https://www.renogy.com/bt-2-bluetooth-module/), but Renogy has never released any documentation on how to interrogate these devices.  By using Wireshark on an Android Phone and Renogy's BT app, I was able to decrypt the protocol used.  I built an Arduino library compatible with Adafruit's nrf52 series devices.  This library has been tested successfully with a [Renogy DCC30S device](https://www.renogy.com/dcc30s-12v-30a-dual-input-dc-dc-on-board-battery-charger-with-mppt/), and may work with other Renogy devices if the registers are known and decribed in the library header file.

This library is strictly use at your own risk.  I am not releasing any code on how to write to registers.  Use the Renogy apps (and any validation they provide) for that please.


## The BT2 Protocol

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
                   [___]                                     Modbus 16 checksum, MSB first on bytes 0 - 5 inclusive

ff 03 0e 00 64 00 85 00 00 10 10 00 7a 00 00 00 00 31 68     Data response from BT2 through notification on characteristic FFF1
[]                                                           Replies also start with 0xFF
   []                                                        Acknowledges "read", I believe
      []                                                     Length of data response in bytes (usually 2x registers requested, so 14 here)
         [___]                                               Contents of register 0x0100 (Aux batt SOC, which is 100%)
               [___]                                         Contents of register 0x0101 (Aux batt voltage * 10, so 13.3v)
                     [___] [___] [___] [___] [___]           Contents of registers 0x0102 through 0x0106
                                                   [___]     Modbus 16 checksum on bytes 0 - 16 inclusive (in this case)


6d 61 69 6e 20 72 65 63 76 20 64 61 74 61 5b 66 66 5d 20 5b  20 byte ACK sent to BT2 on characteristic FFD1
 m  a  i  n     r  e  c  v     d  a  t  a  [  f  f  ]     [  it reads "main recv data[ff] [" and the ff corresponds to the first byte
                                                             of every 20 byte notification received; so a 50 byte datagram (3 notifications)
                                                             might trigger an ack sequence like "main recv data[ff] [", "main recv data[77] [",
                                                             and "main recv data[1e] [" depending on data received.  
                                                             The BT2 seems to respond OK without this ACK, but I do it also, as that's what the
                                                             Renogy BT app does
```

## Using the BT2Reader library
Setup is fairly straightforward.  You need to set up the Bluefruit.Scanner preparatory calls as usual - see Adafruit's [examples](https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Bluefruit52Lib/examples/Central/central_scan/central_scan.ino) for help here.  The setup functions are:
```
uint16_t myConnectionHandle = BLE_CONN_HANDLE_INVALID;       // a variable to retain connection handle number
BT2Reader bt2Reader;                                         // creating the class that handles interacting with the BT2
```
In begin() {} before starting to scan, you need to add these lines:
```
Bluefruit.begin(0, 2);                                       // sets bluefruit to 2 central connections here
bt2Reader.setDeviceTableSize(1);                             // creates space for 1 connection to a BT2 device (the library can handle more)
bt2Reader.addTargetBT2Device((char *)"BT-TH-XXXXXXX");       // sets the target BT2 device name to connect to
bt2Reader.setLoggingLevel(BT2READER_VERBOSE);		     // BT2READER_ERRORS_ONLY and BT2READER_QUIET also supports
bt2Reader.begin();                                           // initializes the class
```
Add these lines to scanCallback:
```
void scanCallback(ble_gap_evt_adv_report_t* report) {
	if (bt2Reader.scanCallback(report)) {                // returns true if scanCallback found a BT2 device, so you can skip other checks
                                                             // and save a few CPU cycles
		Bluefruit.Scanner.resume();
		return;
	}
	/* Your code to check for other devices while scanning*/
	Bluefruit.Scanner.resume();
}
```
Add these lines to connectCallback:
```
void connectCallback(uint16_t connectionHandle) {
	if (bt2Reader.connectCallback(connectionHandle)) {   // checks if device is a BT2, returns true on successful BT2 connection, false otherwise
		myConnectionHandle = connectionHandle;       // store the connection handle on a successful connection
		return;
	}
	// any other connection code goes here

}
```
Add these lines to disconnectCallback
```
void disconnectCallback(uint16_t connectionHandle, uint8_t reason) {
	if (bt2Reader.disconnectCallback(connectionHandle, reason)) {	// returns true if it's a BT2 device that's being disconnected
		myConnectionHandle = BLE_CONN_HANDLE_INVALID;
		return;
	}
	// other disconnection callback code here
}
```
At any time in the main loop you can initiate a request for fresh data and reading it using this code:
```
uint32_t sendReadCommandTime = millis();
bt2Reader.sendReadCommand(myConnectionHandle, startRegister, numberOfRegisters);
/* usually it takes ~70-120ms to get a response back from the BT2 depending on how many registers are requested */
while (!bt2Reader.getIsNewDataAvailable(myConnectionHandle) && (millis() - sendReadCommandTime < 5000)) {
	delay(2);
}

/* you can obtain the register values (16 bits) by calling getRegister */
for (int i = 0; i < numberOfRegisters; i++) {
	Serial.printf("Register 0x%04X contains %d\n", 
		startRegister + i,
		bt2Reader.getRegister(myConnectionHandle, startRegister + i)->value
	);
}

/* you can also print out each register formatted how it's intended with printRegister.
   it can handle float, decimal, char arrays and even bit flags */
for (int i = 0; i < numberOfRegisters; i++) {
	bt2Reader.printRegister(myConnectionHandle, startRegister + i);
}

```

....it's late.... have to finish this week

