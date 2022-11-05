#include "BT2Reader.h"

/** Prints bms data received.  Just follow the Serial.printf code to reconstruct any voltages, temperatures etc.
 *  https://www.dropbox.com/s/03vfqklw97hziqr/%E9%80%9A%E7%94%A8%E5%8D%8F%E8%AE%AE%20V2%20%28%E6%94%AF%E6%8C%8130%E4%B8%B2%29%28Engrish%29.xlsx?dl=0
 *	^^^ has details on the data formats
 */


int BT2Reader::printRegister(char * name, uint16_t registerAddress) {
	if (getDeviceIndex(name) == -1) { return -1; }
	return (printRegister(&deviceTable[getDeviceIndex(name)], registerAddress));
}

int BT2Reader::printRegister(uint8_t * address, uint16_t registerAddress) {
	if (getDeviceIndex(address) == -1) { return -1; }
	return (printRegister(&deviceTable[getDeviceIndex(address)], registerAddress));
}

int BT2Reader::printRegister(uint16_t connectionHandle, uint16_t registerAddress) {
	if (getDeviceIndex(connectionHandle) == -1) { return -1; }
	return (printRegister(&deviceTable[getDeviceIndex(connectionHandle)], registerAddress));
}

int BT2Reader::printRegister(DEVICE * device, uint16_t registerAddress) {

	int registerDescriptionIndex = getRegisterDescriptionIndex(registerAddress);
	int registerValueIndex = getRegisterValueIndex(device, registerAddress);
	if (registerDescriptionIndex == -1) {
		//Serial.printf("printRegister: invalid register address 0x%04X; not found in description table; aborting\n", registerAddress);
		return (1);
	}
	if (registerValueIndex == -1) {
		//Serial.printf("printRegister: invalid register address 0x%04X; not found in values table; aborting\n", registerAddress);
		return (1);
	}

	

	uint16_t registerValue = device->registerValues[registerValueIndex].value;
	uint8_t msb = (registerValue >> 8) & 0xFF;
	uint8_t lsb = (registerValue) & 0xFF;	

	const REGISTER_DESCRIPTION * rr = &registerDescription[registerDescriptionIndex];
	Serial.print("BT2Reader:");	
	Serial.printf("%10s: ", device->peerName);
	Serial.printf("%35s (%04X): ", rr->name, rr->address);

	switch(rr->type) {
		case RENOGY_BYTES: 
			{
				for (int i = 0; i < rr->bytesUsed / 2; i++) {
					Serial.printf("%02X ", (device->registerValues[registerValueIndex + i].value / 256) &0xFF);
					Serial.printf("%02X ", (device->registerValues[registerValueIndex + i].value) &0xFF);
				}
				break;
			}
		
		case RENOGY_CHARS: 
			{
				for (int i = 0; i < rr->bytesUsed / 2; i++) {
					Serial.printf("%c", (char)((device->registerValues[registerValueIndex + i].value / 256) &0xFF));
					Serial.printf("%c", (char)((device->registerValues[registerValueIndex + i].value) &0xFF));
				}
				break;
			}
		
		case RENOGY_DECIMAL: Serial.printf("%5d", registerValue); break;
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
				Serial.printf(" %04X\n", registerValue);
				Serial.printf("%42s", " ");
				for (int i = 15; i >= 0; i--) { 
					Serial.print(HEX_UPPER_CASE[i]);
					if (i == 8) { Serial.print(' ');}
				}
				Serial.printf("\n%42s", " ");
				for (int i = 15; i >= 0; i--) { 
					Serial.printf("%01d", (registerValue >> i) &0x01);
					if (i == 8) { Serial.print(' ');}
				}
				
				int registerBitFlagIndex = -1;
				int renogyBitFlagsSize = sizeof(renogyBitFlags) / sizeof(renogyBitFlags[0]);
				for (int i = 0; i < renogyBitFlagsSize; i++) {
					if (renogyBitFlags[i].registerAddress == registerAddress) {
						registerBitFlagIndex = i;
						break;
					}
				}

				if (registerBitFlagIndex < 0) { break; }
				do {
					Serial.printf("\n%42s", " ");
					int bit = renogyBitFlags[registerBitFlagIndex].bit;
					for (int i = 15; i > bit; i--) {Serial.print('.'); if (i == 8) { Serial.print(' '); }}
					Serial.print(((registerValue >> bit) &0x01) == 1 ? "1": "0");
					for (int i = bit; i > 0; i--) { Serial.print('.'); if (i == 8) { Serial.print(' '); } }
					Serial.printf("  Bit %2d:", bit);
					Serial.print(renogyBitFlags[registerBitFlagIndex].bitName);
					Serial.print(((registerValue >> bit) &0x01) == 1 ? " TRUE": " FALSE");
					registerBitFlagIndex++;
				} while (registerBitFlagIndex < renogyBitFlagsSize && (renogyBitFlags[registerBitFlagIndex].registerAddress == registerAddress));
				break;
			}
	}
	Serial.println();
	return (rr->bytesUsed / 2);
}

void BT2Reader::setLoggingLevel(int i) { 
	loggingLevel = i;
	log("Setting logging level to %s\n", LOGGING_LEVEL_TEXT[i]);
}

void BT2Reader::log(const char * fsh, ...) {
	if (loggingLevel < BT2READER_VERBOSE) { return; }
	Serial.print("BT2Reader: ");
	va_list args;
	va_start(args, fsh);
	logstub(fsh, &args);
	va_end(args);
}

void BT2Reader::logprintf(const char * fsh, ...) {
	if (loggingLevel < BT2READER_VERBOSE) { return; }
	va_list args;
	va_start(args, fsh);
	logstub(fsh, &args);
	va_end(args);
}

void BT2Reader::logerror(const char * fsh, ...) {
	if (loggingLevel < BT2READER_ERRORS_ONLY) { return; }
	Serial.print("BT2Reader: ");
	va_list args;
	va_start(args, fsh);
	logstub(fsh, &args);
	va_end(args);
}

void BT2Reader::logstub(const char * fsh, va_list * args) {
	char c[255];
	vsnprintf(c, 255, fsh, *args);
	
	int i = 0;
	while (c[i] != 0) {
		if ((c[i] >= 32 && c[i] < 127) || c[i] == '\n') {
		 	Serial.print(c[i]);
		} else {
			Serial.printf(" [0x%02X]", c[i]);
		}
		i++;
	}
}