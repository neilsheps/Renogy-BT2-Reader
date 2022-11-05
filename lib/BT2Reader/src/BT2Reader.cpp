#include "BT2Reader.h"

BT2Reader * BT2Reader::_pointerToBT2ReaderClass = NULL;



int BT2Reader::setDeviceTableSize(int i) {
	deviceTableSize = min(max(1, i), MAXIMUM_BT2_DEVICES);
	deviceTable = new DEVICE[deviceTableSize];
	for (int i = 0; i < deviceTableSize; i++) {
		memset(deviceTable[i].peerName, 0, 20);
		memset(deviceTable[i].peerAddress, 0, 6);
		deviceTable[i].slotNamed = false;
		deviceTable[i].handle = BLE_CONN_HANDLE_INVALID;
	}
	log("deviceTable is %d entries long\n", deviceTableSize);
	return deviceTableSize;
}


boolean BT2Reader::addTargetBT2Device(char * peerName) {
	if (deviceTableSize == 0) { setDeviceTableSize(1); }
	for (int i = 0; i < deviceTableSize; i++) {
		if (!deviceTable[i].slotNamed) {
			memcpy(deviceTable[i].peerName, peerName, strlen(peerName));
			deviceTable[i].slotNamed = true;
			log("added device %s to deviceTable\n", peerName);
			return true;
		}	
	}
	return false;
}

boolean BT2Reader::addTargetBT2Device(uint8_t * peerAddress) {
	if (deviceTableSize == 0) { setDeviceTableSize(1); }
	for (int i = 0; i < deviceTableSize; i++) {
		if (!deviceTable[i].slotNamed) {
			memcpy(deviceTable[i].peerAddress, peerAddress, 6);
			deviceTable[i].slotNamed = true;
			log("Added target peer Address ");
			for (int i = 0; i < 6; i++) { logprintf("%02X ",peerAddress[i]); }
			logprintf("to deviceTable\n");
			return true;
		}
	}
	return false;
}


void BT2Reader::begin() {
	if (deviceTableSize == 0) { setDeviceTableSize(1); }
	_pointerToBT2ReaderClass = this;
	registerDescriptionSize = sizeof(registerDescription) / sizeof(registerDescription[0]);
	registerValueSize = 0;
	for (int i = 0; i < registerDescriptionSize; i++) { registerValueSize += (registerDescription[i].bytesUsed / 2); }

	log("BT2Reader: registerDescription is %d entries, registerValue is %d entries\n", registerDescriptionSize, registerValueSize);

	for (int i = 0; i < deviceTableSize; i++) {
		DEVICE * device = &deviceTable[i];
		device->registerValues = new REGISTER_VALUE[registerValueSize];
		device->handle = BLE_CONN_HANDLE_INVALID;
		device->dataReceivedLength = 0;
		device->dataError = false;
		device->registerExpected = 0;
		device->newDataAvailable = false;
		
		device->txService.begin();
		device->txCharacteristic.begin();

		device->rxService.begin();
		device->rxCharacteristic.setNotifyCallback(notifyCallbackWrapper);
		device->rxCharacteristic.begin();

		int registerValueIndex = 0;
		for (int j = 0; j < registerValueSize; j++) {
			int registerLength = registerDescription[j].bytesUsed / 2;
			int registerAddress = registerDescription[j].address;
			for (int k = 0; k < registerLength; k++) {
				device->registerValues[registerValueIndex].lastUpdateMillis = 0;
				device->registerValues[registerValueIndex].value = 0;
				device->registerValues[registerValueIndex++].registerAddress = registerAddress++;
			}
		}
	}
	invalidRegister.lastUpdateMillis = 0;
	invalidRegister.value = 0;
}


boolean BT2Reader::scanCallback(ble_gap_evt_adv_report_t* report) {
	if (numberOfConnections == deviceTableSize) { return false; }

	// has service, manufacturer data
	if (!report->type.scan_response) {

		if (Bluefruit.Scanner.checkReportForService(report, deviceTable[0].txService)) {
			
			uint8_t buffer[20];
			int len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buffer, 20);
			if (len == 0 || (buffer[1] * 256 + buffer[0] != BT2_MANUFACTURER_ID)) {
				return false;
			}

			for (int i = 0; i < deviceTableSize; i++) {

				if (memcmp(report->peer_addr.addr, deviceTable[i].peerAddress, 6) == 0) {
					if (deviceTable[i].slotNamed) {
						log("BT2Reader: Found targeted BT2 device, attempting connection\n");
						Bluefruit.Central.connect(report);
					} else {
						//log("BT2Reader: Found untargeted BT2 device, will connect once name determined\n");
					}
					return true;
				} else {
					if (!deviceTable[i].slotNamed
						&& memcmp(BLANK_MACID, deviceTable[i].peerAddress, 6) == 0) {
						memcpy(deviceTable[i].peerAddress, report->peer_addr.addr, 6);
						log("BT2Reader: Found untargeted BT2 device, adding it to deviceTable for future connection\n");
						return true;
					}
				}								
			}
		}

		return false;

	} else {
		uint8_t buffer[20];
		int len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, buffer, 20);
		
		if (len > 0) {
			log("found device named %s", (char *)buffer);

			for (int i = 0; i < deviceTableSize; i++) {
				if (strcmp((char *)buffer, deviceTable[i].peerName) == 0 
					|| memcmp(report->peer_addr.addr, deviceTable[i].peerAddress, 6) == 0) {
					memcpy(deviceTable[i].peerAddress, report->peer_addr.addr, 6);
					logprintf(", attempting connection\n");
					Bluefruit.Central.connect(report);
					return true;
				}				
			}
			logprintf("\n");
		}
	}
	return false;
}
	


boolean BT2Reader::connectCallback(uint16_t connectionHandle) {

	BLEConnection * connection = Bluefruit.Connection(connectionHandle);
	
	uint8_t peerAddress[6];
	memcpy(peerAddress, connection->getPeerAddr().addr, 6);
	
	for (int i = 0; i < deviceTableSize; i++) {
		if (memcmp(peerAddress, deviceTable[i].peerAddress, 6) != 0) { continue; }

		DEVICE * device = &deviceTable[i];
		if (device->txService.discover(connectionHandle) && device->txCharacteristic.discover()) {
			//Serial.print("Renogy Tx service and characteristic discovered:\n");
			//printUuid((uint8_t * )device->txService.uuid._uuid128, 16);
			//printUuid((uint8_t * )device->txCharacteristic.uuid._uuid128, 16);
		} else {
			logerror("Renogy Tx service or characteristic not discovered, disconnecting\n");
			Bluefruit.disconnect(connectionHandle);
			return true;
		}

		if (device->rxService.discover(connectionHandle) && device->rxCharacteristic.discover()) {
			//Serial.print("Renogy Rx service and characteristic discovered:\n");
			//printUuid((uint8_t * )device->rxService.uuid._uuid128, 16);
			//printUuid((uint8_t * )device->rxCharacteristic.uuid._uuid128, 16);		
			device->rxCharacteristic.enableNotify();
			//Serial.print("Renogy Rx characteristic notify enabled:\n");
			
		} else {
			logerror("Renogy Rx service or characteristic not discovered, disconnecting\n");
			Bluefruit.disconnect(connectionHandle);
			return true;
		}

		device->handle = connectionHandle;
		connection->getPeerName(device->peerName, 20);
		numberOfConnections++;
		log("Connected to device %s, active connections = %d\n", device->peerName, numberOfConnections);
		return true;
	}
	return false;
}

boolean BT2Reader::disconnectCallback(uint16_t connectionHandle, uint8_t reason) {
	
	for (int i = 0; i < deviceTableSize; i++) {
		if (deviceTable[i].handle == connectionHandle) {
			deviceTable[i].handle = BLE_CONN_HANDLE_INVALID;
			if (!deviceTable[i].slotNamed) {
				memset(deviceTable[i].peerAddress, 0, 6);
				memset(deviceTable[i].peerName, 0, 20);
			}
		}
		numberOfConnections--;
		log("Disconnected, reason = 0x%02X, active connections = %d\n", reason, numberOfConnections);
		return true;
	}
	return false;
}


void BT2Reader::notifyCallbackWrapper(BLEClientCharacteristic * rxCharacteristic, uint8_t* data, uint16_t len) {
	_pointerToBT2ReaderClass->notifyCallback(rxCharacteristic, data, len);
}

void BT2Reader::notifyCallback(BLEClientCharacteristic * rxCharacteristic, uint8_t* data, uint16_t len) {

	int index = getDeviceIndex(rxCharacteristic);
	if (index < 0) {
		logerror("renogyNotifyCallback; characteristic not found in device table\n");
		return;
	}

	DEVICE * device = &deviceTable[index];

	if (device->dataError) { return; }									// don't append anything if there's already an error

	if (device->dataReceivedLength > 0 || data[0] == 0xFF) {
		device->dataError = !appendRenogyPacket(device, data, len);		// append second or greater packet
	}

	if (!device->dataError && device->dataReceivedLength == getExpectedLength(device->dataReceived)) {

		if (getIsReceivedDataValid(device->dataReceived)) {
			//Serial.printf("Complete datagram of %d bytes, %d registers (%d packets) received:\n", 
			//	device->dataReceivedLength, device->dataReceived[2], device->dataReceivedLength % 20 + 1);
			//printHex(device->dataReceived, device->dataReceivedLength);
			processDataReceived(device);

			char bt2Response[21] = "main recv data[XX] [";
			for (int i = 0; i < device->dataError; i+= 20) {
				bt2Response[15] = HEX_LOWER_CASE[(device->dataReceived[i] / 16) & 0x0F];
				bt2Response[16] = HEX_LOWER_CASE[(device->dataReceived[i]) & 0x0F];
				//Serial.printf("Sending response #%d to BT2: %s\n", i, bt2Response);
				device->txCharacteristic.write(bt2Response, 20);
			}
		} else {
			//Serial.printf("Checksum error: received is 0x%04X, calculated is 0x%04X\n", 
			//	getProvidedModbusChecksum(device->dataReceived), getCalculatedModbusChecksum(device->dataReceived));
		}
	} 
}


/** Appends received data.  Returns false if there's potential for buffer overrun, true otherwise
 */
boolean BT2Reader::appendRenogyPacket(DEVICE * device, uint8_t * data, int dataLen) {
	if (dataLen + device->dataReceivedLength >= DEFAULT_DATA_BUFFER_LENGTH -1) {
		logerror("BT2Reader: Buffer overrun receiving data\n");
		return false;
		}
	memcpy(&device->dataReceived[device->dataReceivedLength], data, dataLen);
	//for (int i = 0; i < dataLen; i++) { device->dataReceived[device->dataReceivedLength++] = data[i]; }
	device->dataReceivedLength += dataLen;
	if (getExpectedLength(device->dataReceived) < device->dataReceivedLength) {
		logerror("BT2Reader: Buffer overrun receiving data\n");
		return false;
	}
	return true;
}

void BT2Reader::processDataReceived(DEVICE * device) {

	int registerOffset = 0;
	int registersProvided = device->dataReceived[2] / 2;
	
	while (registerOffset < registersProvided) {
		int registerIndex = getRegisterValueIndex(device, device->registerExpected + registerOffset);
		if (registerIndex >= 0) {
			uint8_t msb = device->dataReceived[registerOffset * 2 + 3];
			uint8_t lsb = device->dataReceived[registerOffset * 2 + 4];
			device->registerValues[registerIndex].value = msb * 256 + lsb;
			device->registerValues[registerIndex].lastUpdateMillis = millis();
		}
		registerOffset++;
	}	
	device->newDataAvailable = true;
}


void BT2Reader::sendReadCommand(char * name, uint16_t startRegister, uint16_t numberOfRegisters) { sendReadCommand(getDeviceIndex(name), startRegister, numberOfRegisters); }
void BT2Reader::sendReadCommand(uint8_t * address, uint16_t startRegister, uint16_t numberOfRegisters) { sendReadCommand(getDeviceIndex(address), startRegister, numberOfRegisters); }
void BT2Reader::sendReadCommand(uint16_t handle, uint16_t startRegister, uint16_t numberOfRegisters) { sendReadCommand(getDeviceIndex(handle), startRegister, numberOfRegisters); }
void BT2Reader::sendReadCommand(int index, uint16_t startRegister, uint16_t numberOfRegisters) {
	if (index == -1) {
		logerror("SendReadCommand: invalid name, mac address, or index provided\n");
		return;
	}
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

	log("Sending command sequence: ");
	for (int i = 0; i < 8; i++) { logprintf("%02X ", command[i]); }
	logprintf("\n");

	DEVICE * device = &deviceTable[index];
	device->txCharacteristic.write(command, 8);
	device->registerExpected = startRegister;
	device->dataReceivedLength = 0;
	device->dataError = false;
	device->newDataAvailable = false;
}


int BT2Reader::getRegisterValueIndex(DEVICE * device, uint16_t registerAddress) {
	int left = 0;
	int right = registerValueSize - 1;
	while (left <= right) {
		int mid = (left + right) / 2;
		if (device->registerValues[mid].registerAddress == registerAddress) { return mid; }
		if (device->registerValues[mid].registerAddress < registerAddress) { 
			left = mid + 1;
		} else {
			right = mid -1;
		}
	}
	return -1;
}

int BT2Reader::getRegisterDescriptionIndex(uint16_t registerAddress) {
	int left = 0;
	int right = registerDescriptionSize - 1;
	while (left <= right) {
		int mid = (left + right) / 2;
		if (registerDescription[mid].address == registerAddress) { return mid; }
		if (registerDescription[mid].address < registerAddress) { 
			left = mid + 1;
		} else {
			right = mid -1;
		}
	}
	return -1;
}


int BT2Reader::getExpectedLength(uint8_t * data) { return data[2] + 5; }

int BT2Reader::getDeviceIndex(uint16_t connectionHandle) {
	for (int i = 0; i < deviceTableSize; i++) {
			if (connectionHandle == deviceTable[i].handle) { return i; }
		}
	return -1;
}

int BT2Reader::getDeviceIndex(char * name) {
	for (int i = 0; i < deviceTableSize; i++) {
			if (strcmp(name, deviceTable[i].peerName) == 0) { return i; }
		}
	return -1;
}

int BT2Reader::getDeviceIndex(uint8_t * address) {
	for (int i = 0; i < deviceTableSize; i++) {
		if (memcmp(address, deviceTable[i].peerAddress, 6) == 0) { return i; }
	}
	return -1;
}

int BT2Reader::getDeviceIndex(BLEClientCharacteristic * characteristic) {
	for (int i = 0; i < deviceTableSize; i++) {
		if (characteristic == &deviceTable[i].txCharacteristic) { return i; }
		if (characteristic == &deviceTable[i].rxCharacteristic) { return i; }
	}
	return -1;
}

REGISTER_VALUE * BT2Reader::getRegister(char * name, uint16_t registerAddress) { return (getRegister(getDeviceIndex(name), registerAddress)); }
REGISTER_VALUE * BT2Reader::getRegister(uint8_t * address, uint16_t registerAddress) { return (getRegister(getDeviceIndex(address), registerAddress)); }
REGISTER_VALUE * BT2Reader::getRegister(uint16_t connectionHandle, uint16_t registerAddress) { return (getRegister(getDeviceIndex(connectionHandle), registerAddress)); }
REGISTER_VALUE * BT2Reader::getRegister(int deviceIndex, uint16_t registerAddress) {
	if (deviceIndex < 0) { return &invalidRegister; }
	int registerValueIndex = getRegisterValueIndex(&deviceTable[deviceIndex], registerAddress);
	if (registerValueIndex < 0) { return &invalidRegister; }
	return (&deviceTable[deviceIndex].registerValues[registerValueIndex]);
}

boolean BT2Reader::getIsNewDataAvailable(char * name) { return (getIsNewDataAvailable(getDeviceIndex(name))); }
boolean BT2Reader::getIsNewDataAvailable(uint8_t * address) { return (getIsNewDataAvailable(getDeviceIndex(address))); }
boolean BT2Reader::getIsNewDataAvailable(uint16_t connectionHandle) { return (getIsNewDataAvailable(getDeviceIndex(connectionHandle))); }
boolean BT2Reader::getIsNewDataAvailable(int index) {
	if (index == -1) { return false; }
	boolean isNewDataAvailable = deviceTable[index].newDataAvailable;
	deviceTable[index].newDataAvailable = false;
	return (isNewDataAvailable);
}

DEVICE * BT2Reader::getDevice(char * name) { return (getDevice(getDeviceIndex(name))); }
DEVICE * BT2Reader::getDevice(uint8_t * address) { return (getDevice(getDeviceIndex(address))); }
DEVICE * BT2Reader::getDevice(uint16_t connectionHandle) { return (getDevice(getDeviceIndex(connectionHandle))); }
DEVICE * BT2Reader::getDevice(int index) { 
	if (index == -1) { return NULL; }
	return (&deviceTable[index]);
}
