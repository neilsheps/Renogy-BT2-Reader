#include "main.h"

/**	Adafruit nrf52 code for communicating with Renogy DCC series MPPT solar controllers and DC:DC converters
 * These include:
 * Renogy DCC30s - https://www.renogy.com/dcc30s-12v-30a-dual-input-dc-dc-on-board-battery-charger-with-mppt/
 * Renogy DCC50s - https://www.renogy.com/dcc50s-12v-50a-dc-dc-on-board-battery-charger-with-mppt/ 
 * 
 * This library can read operating parameters.  While it is possible with some effort to modify this code to
 * write parameters, I have not exposed this here, as it is a security risk.  USE THIS AT YOUR OWN RISK!
 * 
 * Copyright Neil Shepherd 2022
 * Released under GPL license
 * 
 * Thanks go to Wireshark for allowing me to read the bluetooth packets used 
 */


const char RENOGY_DEVICE_NAME[] = "BT-TH-EA75B18F";
uint16_t myConnectionHandle = BLE_CONN_HANDLE_INVALID;
BT2Reader bt2Reader;
int ticker = 0;


void setup() {
	Serial.begin(115200);
	delay(100);
	Serial.println("Adafruit nrf52 connecting to Renogy BT-2 code example");
	Serial.println("-----------------------------------------------------\n");
	Bluefruit.begin(0, 2); 										// only using one central connection here, no peripheral connections
	Bluefruit.setName("BT2 reader");							// the Renogy DCC entity is the server in this context

	//bt2Reader.setDeviceTableSize(1);
	//bt2Reader.addTargetBT2Device((char *)(RENOGY_DEVICE_NAME));
	bt2Reader.setLoggingLevel(BT2READER_ERRORS_ONLY);
	bt2Reader.begin();
	

	Bluefruit.setConnLedInterval(250);
	Bluefruit.Central.setConnectCallback(connectCallback);
	Bluefruit.Central.setDisconnectCallback(disconnectCallback);

	Bluefruit.Scanner.setRxCallback(scanCallback);
	Bluefruit.Scanner.restartOnDisconnect(true);
	Bluefruit.Scanner.setInterval(160, 80); 					// in unit of 0.625 ms
	Bluefruit.Scanner.useActiveScan(true);
	delay(2000);
	Serial.println("About to start scanning");
	Bluefruit.Scanner.start(0);                   				// 0 = Don't stop scanning after n seconds
}


/** Scan callback method.  Be sure to run bt2Reader.scanCallback(report).  The BT2Reader
 * library will initiate a connection if it can.  
 * 
 * The exact order is up to the user.  Be sure not to consume > 1 or 2 millis total else
 * receiving a scan_response doesn't seem to work
 * 
 * bt2Reader.scanCallback(report) returns true if the report includes a BT2 device, and if so you can skip 
 * other scanCallback checks to save CPU cycles
 */
void scanCallback(ble_gap_evt_adv_report_t* report) {
	
	if (bt2Reader.scanCallback(report)) {
		Bluefruit.Scanner.resume();
		return;
	}

	// your code

	Bluefruit.Scanner.resume();
}


/** Connect callback method.  Exact order of operations is up to the user.  if bt2Reader attempted a connection
 * it returns true (whether or not it succeeds).  Usually you can then skip any other code in this connectCallback
 * method, because it's unlikely to be relevant for other possible connections, saving CPU cycles
 */ 
void connectCallback(uint16_t connectionHandle) {

	if (bt2Reader.connectCallback(connectionHandle)) {
		myConnectionHandle = connectionHandle;
		Serial.println("Connected to BT2 device");
		return;
	}
	// any other connection code goes here

}

void disconnectCallback(uint16_t connectionHandle, uint8_t reason) {
	if (bt2Reader.disconnectCallback(connectionHandle, reason)) {
		myConnectionHandle = BLE_CONN_HANDLE_INVALID;
		Serial.println("Disconnected from BT2 device");
		return;
	}

	// other disconnection callback code here
}

void loop() {
	
	Serial.printf("Tick %03d: \n",ticker);

	if (myConnectionHandle != BLE_CONN_HANDLE_INVALID) {
	
		int commandSequenceToSend = (ticker % 8);	
		uint16_t startRegister = bt2Commands[commandSequenceToSend].startRegister;
		uint16_t numberOfRegisters = bt2Commands[commandSequenceToSend].numberOfRegisters;
		uint32_t sendReadCommandTime = millis();
		bt2Reader.sendReadCommand(myConnectionHandle, startRegister, numberOfRegisters);

		while (!bt2Reader.getIsNewDataAvailable(myConnectionHandle) && (millis() - sendReadCommandTime < 5000)) {
			delay(2);
		}

		if (millis() - sendReadCommandTime >= 5000) {
			Serial.println("Timeout error; did not receive response from BT2 within 5 seconds");
		} else {
			Serial.printf("Received response for %d registers 0x%04X - 0x%04X in %dms: ", 
					numberOfRegisters,
					startRegister,
					startRegister + numberOfRegisters - 1,
					(millis() - sendReadCommandTime)
			);
			bt2Reader.printHex(bt2Reader.getDevice(myConnectionHandle)->dataReceived, bt2Reader.getDevice(myConnectionHandle)->dataReceivedLength, false);

			
			for (int i = 0; i < numberOfRegisters; i++) {
				Serial.printf("Register 0x%04X contains %d\n",
					startRegister + i,
					bt2Reader.getRegister(myConnectionHandle, startRegister + i)->value
				);
			}

			for (int i = 0; i < numberOfRegisters; i++) {
				bt2Reader.printRegister(myConnectionHandle, startRegister + i);
			}
		}
		ticker++;
	}
	
	delay(5000);
}