#ifndef BT2_READER_H
#define BT2_READER_H

#include "bluefruit.h"
#include <array>

/**	Adafruit nrf52 code for communicating with Renogy DCC series MPPT solar controllers and DC:DC converters
 * These include:
 * Renogy DCC30s - https://www.renogy.com/dcc30s-12v-30a-dual-input-dc-dc-on-board-battery-charger-with-mppt/
 * Renogy DCC50s - https://www.renogy.com/dcc50s-12v-50a-dc-dc-on-board-battery-charger-with-mppt/ 
 * 
 * This library can read operating parameters.  While it is possible with some effort to modify this code to
 * write parameters, I have not exposed this here, as it is a security risk.  USE THIS AT YOUR OWN RISK!
 * 
 * Other devices that use the BT-2 could likely use this library too, albeit with additional register lookups
 * Currently this library only supports connecting to one BT2 device, future versions will have support for 
 * multiple connected devices  
 * 
 * Copyright Neil Shepherd 2022
 * Released under GPL license
 * 
 * Thanks go to Wireshark for allowing me to read the bluetooth packets used 
 */
#include "Arduino.h"


static const uint16_t MODBUS_TABLE_A001[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};


#define BT2READER_QUIET					0
#define BT2READER_ERRORS_ONLY			1
#define BT2READER_VERBOSE				2

#define MAXIMUM_BT2_DEVICES				8
#define DEFAULT_DATA_BUFFER_LENGTH		100

#define RENOGY_BYTES					0
#define RENOGY_DECIMAL					1
#define RENOGY_CHARS					2
#define RENOGY_VOLTS					3
#define RENOGY_AMPS						4
#define RENOGY_AMP_HOURS				5
#define RENOGY_BIT_FLAGS				6
#define RENOGY_OPTIONS					7
#define RENOGY_COEFFICIENT				8
#define RENOGY_TEMPERATURE				9


#define INVALID_REGISTER				0x0000
#define RENOGY_PRODUCT_MODEL			0x000C
#define RENOGY_SOFTWARE_VERSION			0x0014
#define RENOGY_HARDWARE_VERSION			0x0016
#define RENOGY_SERIAL_NUMBER			0x0018
#define RENOGY_CONTROLLER_ADDRESS		0x001A

#define RENOGY_AUX_BATT_SOC				0x0100
#define RENOGY_AUX_BATT_VOLTAGE			0x0101
#define RENOGY_MAX_CHARGE_CURRENT		0x0102
#define RENOGY_AUX_BATT_TEMPERATURE		0x0103

#define RENOGY_ALTERNATOR_VOLTAGE		0x0104
#define RENOGY_ALTERNATOR_CURRENT		0x0105
#define RENOGY_ALTERNATOR_POWER			0x0106

#define RENOGY_SOLAR_VOLTAGE			0x0107
#define RENOGY_SOLAR_CURRENT			0x0108
#define RENOGY_SOLAR_POWER				0x0109

#define RENOGY_AUX_BATT_LOW_VOLTAGE		0x010B
#define RENOGY_AUX_BATT_HIGH_VOLTAGE	0x010C

#define RENOGY_TODAY_HIGHEST_CURRENT	0x010D
#define RENOGY_TODAY_HIGHEST_POWER		0x010F
#define RENOGY_TODAY_AMP_HOURS			0x0111
#define RENOGY_TODAY_POWER				0x0113
#define RENOGY_CHARGING_MODE			0x0120
#define RENOGY_ERROR_FLAGS_1			0x0121
#define RENOGY_ERROR_FLAGS_2			0x0122

#define RENOGY_AUX_BATT_CAPACITY		0xE002
#define RENOGY_AUX_BATT_TYPE			0xE004
#define REGISTER_DESCRIPTION_UNKNOWN1	0xFFF1
#define REGISTER_DESCRIPTION_UNKNOWN2	0xFFF2
#define REGISTER_DESCRIPTION_UNKNOWN3	0xFFF3
#define REGISTER_DESCRIPTION_UNKNOWN4	0xFFF4

	
struct RENOGY_COMMANDS {
	uint16_t startRegister;
	uint16_t numberOfRegisters;
};

const RENOGY_COMMANDS renogyCommands[8] = {
	{0x000C, 2},												// Startup; this is always the first command send on connection
	{0x000C, 8},												// Product model
	{0x0014, 4},												// Software, hardware version
	{0x0018, 3},												// Serial number, unit address
	{0x0100, 7},												// Aux batt, alternator
	{0x0107, 4},												// solar
	{0x0120, 3},												// flags for charging state, error condition
	{0xE001, 0x21}												// battery type and other settings
};

struct REGISTER_DESCRIPTION {
	uint16_t address;
	uint8_t bytesUsed;
	const char * name;
	uint8_t type;
	float multiplier;
};

/** This table describes bms data received.  
 *  https://www.dropbox.com/s/03vfqklw97hziqr/%E9%80%9A%E7%94%A8%E5%8D%8F%E8%AE%AE%20V2%20%28%E6%94%AF%E6%8C%8130%E4%B8%B2%29%28Engrish%29.xlsx?dl=0
 *	^^^ has details on the data formats
 */
const REGISTER_DESCRIPTION registerDescription[] = {
	{INVALID_REGISTER, 2, "Invalid register", RENOGY_CHARS, 1},
	{RENOGY_PRODUCT_MODEL, 16, "Product model", RENOGY_CHARS, 1},
	{RENOGY_SOFTWARE_VERSION, 4, "Software version", RENOGY_BYTES, 1},
	{RENOGY_HARDWARE_VERSION, 4, "Hardware version", RENOGY_BYTES, 1},
	{RENOGY_SERIAL_NUMBER, 4, "Serial number", RENOGY_BYTES, 1},
	{RENOGY_CONTROLLER_ADDRESS, 2, "Controller address", RENOGY_BYTES, 1},
	{RENOGY_AUX_BATT_SOC, 2, "Aux battery SOC\%", RENOGY_DECIMAL, 1},
	{RENOGY_AUX_BATT_VOLTAGE, 2, "Aux battery voltage", RENOGY_VOLTS, 0.1},
	{RENOGY_MAX_CHARGE_CURRENT, 2, "Max charge current", RENOGY_AMPS, 0.01},
	{RENOGY_AUX_BATT_TEMPERATURE, 2, "Aux battery, controller temperature", RENOGY_TEMPERATURE, 1},

	{RENOGY_ALTERNATOR_VOLTAGE, 2, "Alternator voltage", RENOGY_VOLTS, 0.1},
	{RENOGY_ALTERNATOR_CURRENT, 2, "Alternator current", RENOGY_AMPS, 0.01},
	{RENOGY_ALTERNATOR_POWER, 2, "Alternator power (Watts)", RENOGY_DECIMAL, 1},
	{RENOGY_SOLAR_VOLTAGE, 2, "Solar Voltage", RENOGY_VOLTS, 0.1},
	{RENOGY_SOLAR_CURRENT, 2, "Solar current", RENOGY_AMPS, 0.01},
	{RENOGY_SOLAR_POWER, 2, "Solar power (Watts)", RENOGY_DECIMAL, 1},

	{RENOGY_AUX_BATT_LOW_VOLTAGE, 2, "Aux battery low voltage", RENOGY_VOLTS, 0.1},
	{RENOGY_AUX_BATT_HIGH_VOLTAGE, 2, "Aux battery high voltage", RENOGY_VOLTS, 0.1},
	{RENOGY_TODAY_HIGHEST_CURRENT, 2, "Today highest current (Solar+Alt)", RENOGY_AMPS, 0.01},
	{RENOGY_TODAY_HIGHEST_POWER, 2, "Today highest power (Watts)", RENOGY_DECIMAL, 1},

	{RENOGY_TODAY_AMP_HOURS, 2, "Today Amp Hours", RENOGY_DECIMAL, 1},
	{RENOGY_TODAY_POWER, 2, "Today Watt Hours", RENOGY_DECIMAL, 1},
	{RENOGY_CHARGING_MODE, 2, "Renogy charging mode", RENOGY_OPTIONS, 1},
	{RENOGY_ERROR_FLAGS_1, 2, "Renogy error codes 1", RENOGY_BIT_FLAGS, 1},
	{RENOGY_ERROR_FLAGS_2, 2, "Renogy error codes 2", RENOGY_BIT_FLAGS, 1},
	
	{RENOGY_AUX_BATT_CAPACITY, 2, "Aux battery capacity (Amp Hours)", RENOGY_DECIMAL, 1},
	{RENOGY_AUX_BATT_TYPE, 2, "Aux battery chemistry", RENOGY_OPTIONS, 1},

//	{REGISTER_DESCRIPTION_UNKNOWN1, 2, "REGISTER_DESCRIPTION_UNKNOWN1", RENOGY_DECIMAL, 1},
//	{REGISTER_DESCRIPTION_UNKNOWN2, 2, "REGISTER_DESCRIPTION_UNKNOWN2", RENOGY_DECIMAL, 1},
//	{REGISTER_DESCRIPTION_UNKNOWN3, 2, "REGISTER_DESCRIPTION_UNKNOWN3", RENOGY_DECIMAL, 1},
//	{REGISTER_DESCRIPTION_UNKNOWN4, 2, "REGISTER_DESCRIPTION_UNKNOWN4", RENOGY_DECIMAL, 1}

};

struct RENOGY_BIT_FLAG_TABLE {
	int registerAddress;
	int bit;
	const char * bitName;
};

const RENOGY_BIT_FLAG_TABLE renogyBitFlags[] {
	{RENOGY_ERROR_FLAGS_1, 11, "Aux batt low temperature" },
	{RENOGY_ERROR_FLAGS_1, 10, "Aux batt overcharge protection" },
	{RENOGY_ERROR_FLAGS_1, 9, "Starter batt reverse polarity" },
	{RENOGY_ERROR_FLAGS_1, 8, "Alternator input overvoltage" },
	{RENOGY_ERROR_FLAGS_1, 5, "Alternator input overcurrent" },
	{RENOGY_ERROR_FLAGS_1, 4, "Controller overtemperature" },
	
	{RENOGY_ERROR_FLAGS_2, 12, "Solar reverse polarity" },
	{RENOGY_ERROR_FLAGS_2, 9, "Solar input overvoltage" },
	{RENOGY_ERROR_FLAGS_2, 7, "Solar input overcurrent" },
	{RENOGY_ERROR_FLAGS_2, 5, "Controller overtemperature" },
	{RENOGY_ERROR_FLAGS_2, 2, "Aux batt undervoltage" },
	{RENOGY_ERROR_FLAGS_2, 1, "Aux batt overvoltage" },
	{RENOGY_ERROR_FLAGS_2, 0, "Aux batt overdischarged" }
};

struct RENOGY_OPTIONS_TABLE {
	int registerAddress;
	int option;
	const char * optionName;
};

const RENOGY_OPTIONS_TABLE renogyOptions[] {
	{RENOGY_CHARGING_MODE, 0, "No charging activated" },
	{RENOGY_CHARGING_MODE, 1, "Reserved" },
	{RENOGY_CHARGING_MODE, 2, "MPPT charging (solar)" },
	{RENOGY_CHARGING_MODE, 3, "Equalization charging (Solar/Alternator)" },
	{RENOGY_CHARGING_MODE, 4, "Boost charging (Solar/Alternator)" },
	{RENOGY_CHARGING_MODE, 5, "Float charging (Solar/Alternator)" },
	{RENOGY_CHARGING_MODE, 6, "Current limted charging (Solar/Alternator)" },
	{RENOGY_CHARGING_MODE, 7, "Reserved" },
	{RENOGY_CHARGING_MODE, 8, "Direct charging (Alternator)" },

	{RENOGY_AUX_BATT_TYPE, 0, "User mode" },
	{RENOGY_AUX_BATT_TYPE, 1, "Open cell Lead Acid" },
	{RENOGY_AUX_BATT_TYPE, 8, "Sealed Lead Acid" },
	{RENOGY_AUX_BATT_TYPE, 8, "AGM Lead Acid" },
	{RENOGY_AUX_BATT_TYPE, 8, "Lithium Iron Phosphate" }
};

	struct REGISTER_VALUE {
		uint16_t registerAddress;
		uint16_t value;
		uint32_t lastUpdateMillis = 0;
	};

	struct DEVICE {
		uint16_t handle;
		char peerName[20];
		uint8_t peerAddress[6];
		boolean slotNamed = false;

		uint8_t dataReceived[DEFAULT_DATA_BUFFER_LENGTH];
		int dataReceivedLength = 0;
		boolean dataError = false;
		int registerExpected;
		boolean newDataAvailable;

		REGISTER_VALUE * registerValues;

		BLEClientService txService = BLEClientService("0000ffD0-0000-1000-8000-00805f9b34fb");				// Renogy service
		BLEClientCharacteristic txCharacteristic = BLEClientCharacteristic("0000ffD1-0000-1000-8000-00805f9b34fb");		// Renogy Tx and Rx service

		BLEClientService rxService = BLEClientService("0000ffF0-0000-1000-8000-00805f9b34fb");				// Renogy service
		BLEClientCharacteristic rxCharacteristic = BLEClientCharacteristic("0000ffF1-0000-1000-8000-00805f9b34fb");		// Renogy Tx and Rx service
	};

	


class BT2Reader {

public:

	int setDeviceTableSize(int i);
	boolean addTargetBT2Device(char * peerName);
	boolean addTargetBT2Device(uint8_t * peerAddress);
	void begin();

	boolean scanCallback(ble_gap_evt_adv_report_t* report);
	boolean connectCallback(uint16_t connectionHandle);
	boolean disconnectCallback(uint16_t connectionHandle, uint8_t reason);

	static void notifyCallbackWrapper(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
	void notifyCallback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);

	int getDeviceIndex(char * name);
	int getDeviceIndex(uint8_t * address);
	int getDeviceIndex(uint16_t connectionHandle);
	int getDeviceIndex(BLEClientCharacteristic * characteristic);
	
	DEVICE * getDevice(char * name);
	DEVICE * getDevice(uint8_t * address);
	DEVICE * getDevice(uint16_t connectionHandle);
	DEVICE * getDevice(int index);
	
	REGISTER_VALUE * getRegister(char * name, uint16_t registerAddress);
	REGISTER_VALUE * getRegister(uint8_t * address, uint16_t registerAddress);
	REGISTER_VALUE * getRegister(uint16_t connectionHandle, uint16_t registerAddress);
	REGISTER_VALUE * getRegister(int deviceIndex, uint16_t registerAddress);
	//REGISTER_VALUE * getRegister(DEVICE * device, uint16_t registerAddress);
	
	int printRegister(char * name, uint16_t registerAddress);
	int printRegister(uint8_t * device, uint16_t registerAddress);
	int printRegister(uint16_t connectionHandle, uint16_t registerAddress);
	int printRegister(DEVICE * device, uint16_t registerAddress);

	void printHex(uint8_t * data, int datalen);
	void printHex(uint8_t * data, int datalen, boolean reverse);
	void printUuid(uint8_t * data, int datalen);
	
	void sendReadCommand(uint8_t * address, uint16_t startRegister, uint16_t numberOfRegisters);
	void sendReadCommand(char * name, uint16_t startRegister, uint16_t numberOfRegisters);
	void sendReadCommand(uint16_t handle, uint16_t startRegister, uint16_t numberOfRegisters);
	void sendReadCommand(int index, uint16_t startRegister, uint16_t numberOfRegisters);
		
	boolean getIsNewDataAvailable(uint8_t * name);
	boolean getIsNewDataAvailable(char * address);
	boolean getIsNewDataAvailable(uint16_t connectionHandle);
	boolean getIsNewDataAvailable(int index);

	void setLoggingLevel(int i);

private:

	const uint16_t BT2_TX_SERVICE = 0xFFD0;
	const uint16_t BT2_MANUFACTURER_ID = 0x7DE0;

	const char HEX_LOWER_CASE[17] = "0123456789abcdef";
	const char HEX_UPPER_CASE[17] = "0123456789ABCDEF";
	const uint8_t UUID_DASHES[16] = {0,0,0,1,0,1,0,1,0,1,0,0,0,0,0,0};				//inserts dashes at different positions when printing uuids
	const uint8_t BLANK_MACID[6] = {0,0,0,0,0,0};									//useful to check whether a BT2 Device slot has a valid peer Mac Address or not
	const char * LOGGING_LEVEL_TEXT[3] = { "QUIET", "ERROR", "VERBOSE"};

	REGISTER_VALUE invalidRegister;
	static BT2Reader * _pointerToBT2ReaderClass;
	int numberOfConnections = 0;
	
	DEVICE * deviceTable;
	int deviceTableSize = 0;
	int registerDescriptionSize = 0;
	int registerValueSize = 0;
	int loggingLevel = BT2READER_QUIET;

	boolean appendRenogyPacket(DEVICE * device, uint8_t * data, int dataLen);
	uint16_t getProvidedModbusChecksum(uint8_t * data);
	uint16_t getCalculatedModbusChecksum(uint8_t * data);
	uint16_t getCalculatedModbusChecksum(uint8_t * data, int start, int end);
	boolean getIsReceivedDataValid(uint8_t * data);
	int getExpectedLength(uint8_t * data);
	void processDataReceived(DEVICE * device);

	int getRegisterDescriptionIndex(uint16_t registerAddress);
	int getRegisterValueIndex(DEVICE * device, uint16_t registerAddress);

	void log(const char * fsh, ...);
	void logprintf(const char * fsh, ...);
	void logerror(const char * fsh, ...);
	void logstub(const char * fsh, va_list * args);

};



#endif