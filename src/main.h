#include "Arduino.h"
#include "bluefruit.h"
#include "BT2Reader.h"


struct COMMANDS {
	uint16_t startRegister;
	uint16_t numberOfRegisters;
};

const COMMANDS bt2Commands[8] = {
	{0x000C, 2},												// Startup; this is always the first command send on connection
	{0x000C, 8},												// Product model
	{0x0014, 4},												// Software, hardware version
	{0x0018, 3},												// Serial number, unit address
	{0x0100, 7},												// Aux batt, alternator
	{0x0107, 4},												// solar
	{0x0120, 3},												// flags for charging state, error condition
	{0xE001, 0x21}												// battery type and other settings
};


void setup();
void loop();
void scanCallback(ble_gap_evt_adv_report_t* report);
void connectCallback(uint16_t connectionHandle);
void disconnectCallback(uint16_t connectionHandle, uint8_t reason);









