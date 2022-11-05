//#include <main.h>
//#include "bluefruit.h"
//#include <array>


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

/*
#define MAX_RECEIVED_DATA_CAPACITY		100

BLEClientService txService = BLEClientService("0000ffD0-0000-1000-8000-00805f9b34fb");				// Renogy service
BLEClientCharacteristic txCharacteristic = BLEClientCharacteristic("0000ffD1-0000-1000-8000-00805f9b34fb");		// Renogy Tx and Rx service

BLEClientService rxService = BLEClientService("0000ffF0-0000-1000-8000-00805f9b34fb");				// Renogy service
BLEClientCharacteristic rxCharacteristic = BLEClientCharacteristic("0000ffF1-0000-1000-8000-00805f9b34fb");		// Renogy Tx and Rx service

const char RENOGY_DEVICE_NAME[] = "BT-TH-EA75B18F";
const char RENOGY_HEX[] = "0123456789abcdef";
const char RENOGY_HEX_UPPER_CASE[] = "0123456789ABCDEF";

const uint16_t RENOGY_BT2_TX_SERVICE = 0xFFD0;
const uint16_t RENOGY_BT2_MANUFACTURER_ID = 0x7DE0;

char targetBT2DeviceName[20] = {0};
uint8_t targetBT2DeviceAddress[6] = {0};
boolean targetBT2DeviceAddressSet = false;
char bt2Response[] = "main recv data[XX] [";

const uint8_t uuidDashes[16] = {0,0,0,1,0,1,0,1,0,1,0,0,0,0,0,0};				//inserts dashes at different positions when printing uuids
int renogyConnectionHandle = BLE_CONN_HANDLE_INVALID;							//connection handle reference
int ticker = 0;																	//increments every 5 seconds
int renogyDataLengthReceived = 0;												//sum of bytes received per message.  
																				//Sometimes messages are >20 bytes and get split
int renogyDataLengthExpected = 0;												//Data length expected (usually in byte 3 of received message)
boolean renogyDataError = false;												//if checksum error, buffer overrun, message error, discards message
uint8_t renogyDataReceived[MAX_RECEIVED_DATA_CAPACITY];							//where we put received data
int renogyRegisterExpected = 0;


int renogyPacketsReceived = 0;
uint8_t renogyFirstByteOfPacketReceived[20] = {0};


void connectCallback(uint16_t connectionHandle);
void disconnectCallback(uint16_t connectionHandle, uint8_t reason);
void scanCallback(ble_gap_evt_adv_report_t* report);
void renogyNotifyCallback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
boolean appendRenogyPacket(uint8_t * data, int dataLen);
uint16_t getProvidedModbusChecksum(uint8_t * data);
uint16_t getCalculatedModbusChecksum(uint8_t * data, int start, int end);
void printRenogyDataReceived(uint8_t * data);
int printRegister(uint8_t * data, int base, int index);
void printHex(uint8_t * data, int datalen);
void printHex(uint8_t * data, int datalen, boolean reverse);
void printUuid(uint8_t * data, int datalen);


void setup() {
	Serial.begin(115200);
	delay(100);
	Serial.println("Adafruit nrf52 connecting to Renogy BT-2 code example");
	Serial.println("------------------------------------------------\n");
	Bluefruit.begin(0, 1); 										// only using one central connection here, no peripheral connections
	Bluefruit.setName("BT2 reader");							// the Renogy DCC entity is the server in this context

	// essential that services and characteristics are set up in this order.  refer to Adafruit documentation
	txService.begin();
	txCharacteristic.begin();

	rxService.begin();
	rxCharacteristic.setNotifyCallback(renogyNotifyCallback);
	rxCharacteristic.begin();

	


	Bluefruit.setConnLedInterval(250);
	Bluefruit.Central.setConnectCallback(connectCallback);
	Bluefruit.Central.setDisconnectCallback(disconnectCallback);

	Bluefruit.Scanner.setRxCallback(scanCallback);
	Bluefruit.Scanner.restartOnDisconnect(true);
	Bluefruit.Scanner.setInterval(160, 80); 					// in unit of 0.625 ms
	Bluefruit.Scanner.useActiveScan(true);
	Bluefruit.Scanner.start(0);                   				// 0 = Don't stop scanning after n seconds
}

*/

/** Scan callback method
 * Kept simple here.  Checks for the name of the device... and for the BT2's advertised service 
 * If so, it connects
 
void scanCallback(ble_gap_evt_adv_report_t* report) {

	uint8_t buffer[32];
	memset(buffer, 0, sizeof(buffer));


	if (targetBT2DeviceAddressSet && !report->type.scan_response) {

		for (int i = 0; i < 6; i++) {
			if (targetBT2DeviceAddress[i] != report->peer_addr.addr[i]) { break; }
			if (i == 5) {
				Serial.print("Regular adv packet found for peer address: ");
				for (int i = 0; i < 6; i++) {
					Serial.printf("%02X ", targetBT2DeviceAddress[i]);
				}
				Serial.print("\nData found = ");

				uint8_t len = report->data.len;
				for (int i = 0; i < len; i++) {
					Serial.printf("%02X ", report->data.p_data[i]);
				}
				Serial.println();

				if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE, buffer, sizeof(buffer))) {
					Serial.printf("found service UUID: %02X%02X\n", buffer[1], buffer[0]);
				}

				len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buffer, sizeof(buffer));
				if(len > 0) {
					Serial.printf("found manufacturer data, length %d: manufacturer: %02X%02X, data: ", len, buffer[1], buffer[0]);
					for (int i = 0; i < len; i++) { Serial.printf("%02X ", buffer[i]); }
					Serial.println();
				}

				if (Bluefruit.Scanner.checkReportForService(report, txService)) {
					Serial.printf("%14s\n", "Renogy service found: ");
					printUuid((uint8_t *)txService.uuid._uuid128,16);
					Bluefruit.Central.connect(report);
				}
			}
		}
	}



	if(!targetDeviceAddressSet && report->type.scan_response && Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, buffer, sizeof(buffer))) {

		Serial.printf("%14s %s\n", "Complete name:", buffer);
			
		if (strstr((char *)buffer, RENOGY_DEVICE_NAME) == (char *)buffer ) {
			Serial.printf("%14s %s\n", "Name matches target:", buffer);
			Serial.print("Peer address is: ");
			for (int i = 0; i < 6; i++) {
				targetBT2DeviceAddress[i] = report->peer_addr.addr[i];
				Serial.printf("%02X ", targetBT2DeviceAddress[i]);
			}
			targetBT2DeviceAddressSet = true;
			Serial.println();

			if (Bluefruit.Scanner.checkReportForService(report, txService)) {
				Serial.printf("%14s\n", "Renogy service found: ");
				printUuid((uint8_t *)txService.uuid._uuid128,16);
				Bluefruit.Central.connect(report);
			}


		}
	}
	Bluefruit.Scanner.resume();
}
*/

/** Connect callback method.  Here it's important to discover first the service and then
 * both the Tx and Rx characteristics, else you can't receive a callback when data is 
 * received, or transmit.
 * 
 * Frequently after connection, the BLE device cannot discover the services or characteristics
 * supposedly advertised; the success rate seems to be about 60% for me, so multiple
 * retries are sometimes needed 
 
void connectCallback(uint16_t connectionHandle) {

	if (txService.discover(connectionHandle) && txCharacteristic.discover()) {
		Serial.print("Renogy Tx service and characteristic discovered:\n");
		printUuid((uint8_t * )txService.uuid._uuid128, 16);
		printUuid((uint8_t * )txCharacteristic.uuid._uuid128, 16);
	} else {
		Serial.println("Renogy Tx service or characteristic not discovered, disconnecting");
		Bluefruit.disconnect(connectionHandle);
		return;
	}

	if (rxService.discover(connectionHandle) && rxCharacteristic.discover()) {
		Serial.print("Renogy Rx service and characteristic discovered:\n");
		printUuid((uint8_t * )rxService.uuid._uuid128, 16);
		printUuid((uint8_t * )rxCharacteristic.uuid._uuid128, 16);		
		rxCharacteristic.enableNotify();
		Serial.print("Renogy Rx characteristic notify enabled:\n");
		
	} else {
		Serial.println("Renogy Rx service or characteristic not discovered, disconnecting");
		Bluefruit.disconnect(connectionHandle);
		return;
	}
	renogyConnectionHandle = connectionHandle;
}

*/

/** Disconnect callback.  Obvious functionality

void disconnectCallback(uint16_t connectionHandle, uint8_t reason) {
	(void) connectionHandle;
	(void) reason;

	renogyConnectionHandle = BLE_CONN_HANDLE_INVALID;
	Serial.printf("Disconnected, reason = 0x%02X\n", reason);
}
*/


/** When data is received, this callback is called.  Typically up to 20 bytes is received at a time,
 * so larger packets need to be reconstructed
 
void renogyNotifyCallback(BLEClientCharacteristic * chr, uint8_t* data, uint16_t len) {
	
	
	//Serial.println("Received notification!");
	//printHex(data, len);

	if (renogyDataError) { return; }							// don't append anything if there's already an error

	if (renogyDataLengthReceived == 0)  {
		if (data[0] == 0xFF) {									// start of packet always starts with 0xDD
			renogyDataLengthExpected = data[2] + 5;					// Data length is always in byte [2] (zero indexed)
			renogyDataError = !appendRenogyPacket(data, len);	// if no errors, append first packet
		}
	} else {
		renogyDataError = !appendRenogyPacket(data, len);		// append second or greater packet
	}

	if (!renogyDataError) {
		if (renogyDataLengthReceived == renogyDataLengthExpected) {// dataLengthExpected is always 5 less than total 
																// datagram if properly formed (additional bytes are for
																// 0xFF, read/write code (usually 0x03), length in bytes, <data> then 2 checksum end bytes)

			//Serial.println("Complete packet received, now must validate checksum");
			
			if (getCalculatedModbusChecksum(renogyDataReceived, 0, renogyDataLengthExpected - 2)
					== getProvidedModbusChecksum(renogyDataReceived)) {
						
						Serial.printf("Complete datagram of %d bytes, %d registers (%d packets) received:\n", 
										renogyDataLengthReceived, (renogyDataLengthReceived - 5)/2, renogyPacketsReceived);
						printHex(renogyDataReceived, renogyDataLengthReceived);
						printRenogyDataReceived(renogyDataReceived);

						for (int i = 0; i < renogyPacketsReceived; i++) {
							bt2Response[15] = RENOGY_HEX[(renogyFirstByteOfPacketReceived[i] / 16) & 0x0F];
							bt2Response[16] = RENOGY_HEX[(renogyFirstByteOfPacketReceived[i]) & 0x0F];
							//Serial.printf("Sending response #%d to BT2: %s\n", i, bt2Response);
							txCharacteristic.write(bt2Response, 20);
						}

			} else {
				Serial.printf("Checksum error: received is 0x%04X, calculated is 0x%04X\n", 
					getProvidedModbusChecksum(renogyDataReceived),
					getCalculatedModbusChecksum(renogyDataReceived, 0, renogyDataLengthExpected - 2)
					);
			}
		}
	} else {
		Serial.printf("Data error: data[2] contains 0x%02d, renogyDataLengthReceived is %d\n", renogyDataReceived[2], renogyDataLengthReceived);
	}
}
*/

/** Appends received data.  Returns false if there's potential for buffer overrun, true otherwise

boolean appendRenogyPacket(uint8_t * data, int dataLen) {
	if (dataLen + renogyDataLengthReceived >= MAX_RECEIVED_DATA_CAPACITY -1) { return false; }
	if (renogyDataLengthReceived == 0) { renogyPacketsReceived = 0; }
	renogyFirstByteOfPacketReceived[renogyPacketsReceived++] = data[0];
	for (int i = 0; i < dataLen; i++) { renogyDataReceived[renogyDataLengthReceived++] = data[i]; }
	//Serial.printf("Appended packet %d\n", renogyPacketsReceived);
	return true;
}
*/
/*
uint16_t getProvidedModbusChecksum(uint8_t * data) {
	int checksumIndex = (int)((data[2]) + 3);
	return (data[checksumIndex] + data[checksumIndex + 1] * 256);
}


uint16_t getCalculatedModbusChecksum(uint8_t * data, int start, int end) {	
	uint8_t xxor = 0;
	uint16_t crc = 0xFFFF;
	for (int i = start; i < end; i++) {
		xxor = data[i] ^ crc;
		crc >>= 8;
		crc ^= MODBUS_TABLE_A001[xxor & 0xFF];
	}
	return crc;
}

void printHex(uint8_t * data, int datalen) { printHex(data, datalen, false); }
void printHex(uint8_t * data, int datalen, boolean reverse) {
	for (int i = 0; i < datalen; i++) { Serial.printf("%2d%s", (reverse ? datalen - i: i), (i < datalen-1 ? " " : "\n")); }
	for (int i = 0; i < datalen; i++) { Serial.printf("%02X%s", (uint8_t)data[(reverse ? datalen - i: i)], (i < datalen-1 ? " " : "\n")); }
}

void printUuid(uint8_t * data, int datalen) {
	for (int i = datalen - 1; i >= 0; i--) {
		Serial.printf("%02X%s", (uint8_t)data[i], (uuidDashes[datalen - i - 1] == 1 ? "-" : ""));
	}
	Serial.println();
}


void sendReadCommand(uint16_t startRegister, uint16_t numberOfRegisters) {
	uint8_t command[20];
	command[0] = 0xFF;
	command[1] = 0x03;
	command[2] = (startRegister >> 8) & 0xFF;
	command[3] = startRegister & 0xFF;
	command[4] = (numberOfRegisters >> 8) & 0xFF;
	command[5] = numberOfRegisters & 0xFF;
	uint16_t checksum = getCalculatedModbusChecksum(command, 0, 6);
	command[6] = checksum & 0xFF;
	command[7] = (checksum >> 8) & 0xFF;

	Serial.print("Sending command sequence: ");
	for (int i = 0; i < 8; i++) { Serial.printf("%02X ", command[i]); }
	Serial.println();
	txCharacteristic.write(command, 8);

	renogyRegisterExpected = startRegister;
	renogyDataLengthReceived = 0;
	renogyDataLengthExpected = 0;
	renogyDataError = false;
}


void loop() {

	Serial.printf("Tick %03d: \n",ticker);
	if (renogyConnectionHandle != BLE_CONN_HANDLE_INVALID) {
		int commandSequenceToSend = (ticker % 8);		
		sendReadCommand(renogyCommands[commandSequenceToSend].startRegister, 
						renogyCommands[commandSequenceToSend].numberOfRegisters);
	}
	
	ticker++;
	delay(5000);
}
*/



/** Prints bms data received.  Just follow the Serial.printf code to reconstruct any voltages, temperatures etc.
 *  https://www.dropbox.com/s/03vfqklw97hziqr/%E9%80%9A%E7%94%A8%E5%8D%8F%E8%AE%AE%20V2%20%28%E6%94%AF%E6%8C%8130%E4%B8%B2%29%28Engrish%29.xlsx?dl=0
 *	^^^ has details on the data formats
 
void printRenogyDataReceived(uint8_t * data) {
	int registerBase = renogyRegisterExpected;
	int registerOffset = 0;
	int registersProvided = data[2] / 2;
	
	while (registerOffset < registersProvided) {
		registerOffset += printRegister(data, registerBase, registerOffset);
	}
}

int printRegister(uint8_t * data, int base, int offset) {
	int registerDescriptionSize = sizeof(renogyregisterDescription) / sizeof(renogyregisterDescription[0]);
	int registerAddress= base + offset;
	uint8_t msb = data[offset * 2 + 3];
	uint8_t lsb = data[offset * 2 + 4];
	uint16_t registerValue = lsb + msb * 256;
	

	
	REGISTER_DESCRIPTION * rr;
	boolean registerFound  = false;
	for (int i = 0; i < registerDescriptionSize; i++) {
		if (registerAddress == renogyregisterDescription[i].address) {
			rr = &renogyregisterDescription[i];
			registerFound = true;
			break;
		}
	}

	if (!registerFound) {
		Serial.printf("%35s (%04X): %02X %02X\n", "Register unknown", registerAddress, msb, lsb);
		return 1;
	}
	
	Serial.printf("%35s (%04X): ", rr->name, rr->address);

	switch(rr->type) {
		case RENOGY_BYTES: 
			{
			for (int i = 0; i < rr->bytesUsed; i++) { Serial.printf("%02X ", data[offset * 2 + 3 +i]);}
			break;
			}

		case RENOGY_DECIMAL: Serial.printf("%5d", registerValue); break;
		case RENOGY_CHARS: {for (int i = 0; i < rr->bytesUsed; i++) { Serial.print((char) data[offset * 2 + i + 3]); }; break;}
		case RENOGY_VOLTS: Serial.printf("%5.1f Volts", (float)(registerValue) * rr->multiplier); break;
		case RENOGY_AMPS: Serial.printf("%5.2f Amps", (float)(registerValue) * rr->multiplier); break;
		case RENOGY_AMP_HOURS: Serial.printf("%3d AH", registerValue); break;
		case RENOGY_COEFFICIENT: Serial.printf("%5.3f mV/℃/2V", (float)(registerValue) * rr->multiplier); break;
		case RENOGY_TEMPERATURE:
			{
				Serial.print((lsb & 0x80) > 0 ? '-' : '+');
				Serial.printf("%d C, ", lsb & 0x7F);
				Serial.print((msb & 0x80) > 0 ? '-' : '+');
				Serial.printf("%d C, ", msb & 0x7F); 
				break;
			}
			
		case RENOGY_OPTIONS:
			{
				Serial.printf("Option %d (", registerValue);
				int renogyOptionsSize = sizeof(renogyOptions) / sizeof(renogyOptions[0]);
				for (int i = 0; i < renogyOptionsSize; i++) {
					if (renogyOptions[i].registerAddress == registerAddress && registerValue == renogyOptions[i].option) {
						Serial.printf("%s)",renogyOptions[i].optionName);
						break;
					}
				}
				break;
			}
		
		case RENOGY_BIT_FLAGS:
			{
				Serial.printf("\n%42s", " ");
				for (int i = 15; i >= 0; i--) { 
					Serial.print(RENOGY_HEX_UPPER_CASE[i]);
					if (i == 8) { Serial.print(' ');}
				}
				Serial.printf("\n%42s", " ");
				for (int i = 15; i >= 0; i--) { 
					Serial.printf("%01d", (registerValue >> i) %0x01);
					if (i == 8) { Serial.print(' ');}
				}
				
				int registerIndex = -1;
				int renogyBitFlagsSize = sizeof(renogyBitFlags) / sizeof(renogyBitFlags[0]);
				for (int i = 0; i < renogyBitFlagsSize; i++) {
					if (renogyBitFlags[i].registerAddress == registerAddress) {
						registerIndex = i;
						break;
					}
				}

				if (registerIndex < 0) { break; }
				do {
					Serial.printf("\n%42s", " ");
					int bit = renogyBitFlags[registerIndex].bit;
					for (int i = 15; i > bit; i--) {Serial.print('.'); if (i == 8) { Serial.print(' '); }}
					Serial.print(((registerValue >> bit) % 0x01 == 1 ? "1": "0"));
					for (int i = bit; i > 0; i--) { Serial.print('.'); if (i == 8) { Serial.print(' '); } }
					Serial.printf("  Bit %2d:", bit);
					Serial.print(renogyBitFlags[registerIndex].bitName);
					Serial.print(((registerValue >> bit) % 0x01 == 1 ? " TRUE": " FALSE"));
					registerIndex++;
				} while (registerIndex < renogyBitFlagsSize && (renogyBitFlags[registerIndex].registerAddress == registerAddress));
				break;
			}
	}
	Serial.println();
	return (rr->bytesUsed / 2);
}
*/