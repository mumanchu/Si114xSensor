#pragma once

///////////////////////////////////////////////////////////////////////////////
// Si1145/6/7 ALS, IR and Proximity Sensor with and UV Index calculator
// Copyright (C) https://muman.ch and https://github.com/mumanchu, 2015, 2026
// If you re-use any of this code, please include the above copyright notice
// See https://github.com/mumanchu/Si114xSensor 

#include <Wire.h>

class Si114xSensor
{
protected:
	TwoWire* wire = NULL;
	byte i2cAdds = 0x60;		// fixed I2C slave address 0x60

public:
	// Register Numbers, p29
	typedef enum : byte
	{
		// Hard-wired IDs
		PART_ID = 0x00,			// p31
		REV_ID = 0x01,			// p31
		SEQ_ID = 0x02,			// p31
		// Configuration
		INT_CFG = 0x03,			// p32
		IRQ_ENABLE = 0x04,
		HW_KEY = 0x07,
		MEAS_RATE0 = 0x08,
		MEAS_RATE1 = 0x09,
		PS_RATE = 0x0A,
		PS_LED21 = 0x0F,
		PS_LED3 = 0x10,
		// UV configuration coefficient registers
		UCOEF0 = 0x13,
		UCOEF1 = 0x14,
		UCOEF2 = 0x15,
		UCOEF3 = 0x16,
		// Misc
		PARAM_RD = 0x2E,
		PARAM_WR = 0x17,
		COMMAND = 0x18,
		RESPONSE = 0x20,
		CHIP_STAT = 0x30,
		// Measurement registers
		IRQ_STATUS = 0x21,
		ALS_VIS_DATA0 = 0x22,
		ALS_VIS_DATA1 = 0x23,
		ALS_IR_DATA0 = 0x24,
		ALS_IR_DATA1 = 0x25,
		PS1_DATA0 = 0x26,
		PS1_DATA1 = 0x27,
		PS2_DATA0 = 0x28,
		PS2_DATA1 = 0x29,
		PS3_DATA0 = 0x2A,
		PS3_DATA1 = 0x2B,
		AUX_DATA0_UVINDEX0 = 0x2C,
		AUX_DATA1_UVINDEX1 = 0x2D,

	} Si114x_REG;

	// Parameter RAM Addresses, p45
	typedef enum : byte
	{
		I2C_ADDR = 0x00,			// I2C address, activated by BUSADDR command
		CHLIST = 0x01,				// Channel enabled list, EN_xxxx flags, p47

		PSLED12_SELECT = 0x02,		// LED pin driven during PS1 and PS2 measurements, p48
		PSLED3_SELECT = 0x03,		// LED pin driven during PS3 measurements, p49

		PS_ENCODING = 0x05,			// 16/17 bit alignment for PS1/2/3 measurements, p49
		ALS_ENCODING = 0x06,		// 16/17 bit alignment for AI and VIS measurements, p50

		PS1_ADCMUX = 0x07,			// ADC input for PS1 measurement, p51 
		PS2_ADCMUX = 0x08,			// ADC input for PS2 measurement, p52 
		PS3_ADCMUX = 0x09,			// ADC input for PS3 measurement, p53 
		ALS_IR_ADCMUX = 0x0E,		// ADC input for IR measurement, SMALL_IR or LARGE_IR, p55
		AUX_ADCMUX = 0x0F,			// ADC input for AUX measurement, p56

		PS_ADC_COUNTER = 0x0A,		// ADC recovery period before making PS measurement, p53
		PS_ADC_GAIN = 0x0B,			// IR LED pulse width and ADC integration time, p54
		PS_ADC_MISC = 0x0C,			// PS_RANGE (normal/high) and PS_ADC_MODE (raw/normal proximity), p55

		ALS_VIS_ADC_COUNTER = 0x10,	// ADC recovery period before making ALS-VIS measurement, p56
		ALS_VIS_ADC_GAIN = 0x11,	// ADC integration time for ALS measurement, p57 
		ALS_VIS_ADC_MISC = 0x12,	// VIS_RANGE (normal/high), p57

		ALS_IR_ADC_COUNTER = 0x1D,	// ADC recovery period before making ALS-IR measurement, p58 (2)
		ALS_IR_ADC_GAIN = 0x1E,		// ADC integration time for IR ambient measurements, p59
		ALS_IR_ADC_MISC = 0x1F		// IR_RANGE (normal/high), p59

	} Si114x_PARAM;

	// Commands, written to COMMAND register, p22
	typedef enum : byte
	{
		NOP = 0x00,					// Set the response register to zero
		RESET = 0x01,				// Software reset
		BUSADDR = 0x02,				// Modify I2C address

		PS_FORCE = 0x05,			// Force a single PS measurement
		ALS_FORCE = 0x06,			// Force a single ALS measurement
		PSALS_FORCE = 0x07,			// Force a single PS and ALS measurement

		PS_PAUSE = 0x09,			// Pause autonomous PS
		ALS_PAUSE = 0x0A,			// Pause autonomous ALS
		PSALS_PAUSE = 0x0B,			// Pause autonomous PS and ALS

		PS_AUTO = 0x0D,				// Start/restart an autonomous PS Loop
		ALS_AUTO = 0x0E,			// Start/restart an autonomous ALS Loop
		PSALS_AUTO = 0x0F,			// Start/restart autonomous ALS and PS loop

		GET_CAL = 0x12,				// Copies calibration data to I2C registers 0x22..0x2D
		PARAM_QUERY = 0x80,			// Read a parameter,  0b100aaaaa, aaaaa = param number
		PARAM_SET = 0xa0			// Write a parameter, 0b101aaaaa, aaaaa = param number

	} Si114x_CMD;


	// Error Codes from the RESPONSE register, p23 & p38
	// See awaitCmdErr() and getOverflowError()
	typedef enum : byte
	{
		NO_ERROR = 0x00,			// suddenly, nothing happened
		INVALID_COMMAND = 0x80,		// clear with NOP command
		// ADC Overflow Errors, these are latched ntil read with getOverflowError()
		PS1_ADC_OVERFLOW = 0x88,
		PS2_ADC_OVERFLOW = 0x89,
		PS3_ADC_OVERFLOW = 0x8A,
		ALS_VIS_ADC_OVERFLOW = 0x8C,
		ALS_IR_ADC_OVERFLOW = 0x8D,
		AUX_ADC_OVERFLOW = 0x8E,
		// my additional errors
		TIMED_OUT = 0xFE,			// awaitCmdCtr timed out
		I2C_FAILED = 0xFF			// I2C communications error

	} Si114x_ERRORCODE;

	// CHLIST channel enable values, p47
	typedef enum : byte
	{
		EN_UV = 0b10000000,			// UV Index (estimated), divide by 100
		EN_AUX = 0b01000000,		// Temperature sensor, effects p8 
		EN_ALS_IR = 0b00100000,		// Infrared sensor
		EN_ALS_VIS = 0b00010000,	// Visual light sensor
		EN_PS3 = 0b00000100,		// Proximity sensor 1
		EN_PS2 = 0b00000010,		// Proximity sensor 2
		EN_PS1 = 0b00000001			// Proximity sensor 3

	} Si114x_CHLIST;

	// ADCMUX values, selects ADC input for each MUX, Area 51 (p51)
	// inputs are not valid for all ADCs, see diagram p28
	// VISIBLE is always connected to ALS_VIS_DATA
	// UVIndex is enabled by CHLIST EN_UV, not via the AUX_ADCMUX
	typedef enum : byte
	{
		SMALL_IR = 0x00,		// ALS_IR_ADCMUX, PSx_ADCMUX
		SMALL_VISIBLE = 0x02,	// ALS_VIS_DATA,  PSx_ADCMUX (ALS_VIS_ADCMUX is hard-wired to SMALL_VISIBLE)
		LARGE_IR = 0x03,		// ALS_IR_ADCMUX, PSx_ADCMUX (LARGE_IR is ~6x more sensitive than SMALL_IR)
		NO_PHOTODIODE = 0x06,	// PSx_ADCMUX (marked as REF on the diagram)
		GND_VOLTAGE = 0x25,		// PSx_ADCMUX
		TEMPERATURE = 0x65,		// AUX_ADCMUX, PSx_ADCMUX, temperature is RELATIVE (to what?)
		VDD_VOLTAGE = 0x75,		// AUX_ADCMUX, PSx_ADCMUX

	} Si114x_ADCMUX;

	#pragma pack(push, 1)
	// Data for writeConfiguration()
	// parameter number/parameter value pairs
	typedef struct
	{
		Si114x_PARAM paramNumber;
		byte paramValue;
	} Si114x_CONFIG;

	// Measurement values, for readAllMeasurements()
	typedef struct
	{
		byte irqStatus;
		uint16_t alsVis;
		uint16_t alsIr;
		uint16_t ps1;
		uint16_t ps2;
		uint16_t ps3;
		uint16_t aux;
	} Si114x_MEASUREMENTS;
	#pragma pack(pop)

	// Override this in a derived class to code your own configuration
	virtual bool configureChip();

	bool begin(TwoWire* twoWire);
	bool softwareReset();
	bool writeConfiguration(const Si114x_CONFIG* config, uint configLength);
	bool forceMeasurement(bool als = true, bool ps = true);
	bool startAutonomousMeasurements(bool als = true, bool ps = true);
	bool pauseAutonomousMeasurements(bool als = true, bool ps = true);
	bool readAllMeasurements(Si114x_MEASUREMENTS* measurements);
	bool readAlsVis(uint* alsVis);
	bool readAlsIr(uint* alsIr);
	bool readPs1(uint* ps1);
	bool readPs2(uint* ps2);
	bool readPs3(uint* ps3);
	bool readUvIndex(uint* uvIndex);
	bool readTemperature(uint* temperature);
	bool writeIRQEnable(byte irqEnable);
	bool readIRQStatus(byte* irqStatus);
	bool readChipStatus(bool* running, bool* sleeping, bool* suspended);

	bool getNormalisedMeasurements(uint alsVis, uint alsIr,
		ulong* normalisedVis, ulong* normalisedIr, byte relativeGain = 0);
	ulong calculateLux(uint alsVis, uint alsIr);
	bool automaticGainControl(uint aslVis, uint alsIr);
	void getGain(byte* visGain, byte* irGain, bool* visHighRange, bool* irHighRange);

public:
	// The PART_ID returned from the connected device
	byte partId = 0;			// Si11xx, xx = 0x45, 0x46, 0x47

	// Default UV Index calibration coefficients for clear glass overlay, p16
	// change these as appropriate (data is public) BEFORE calling begin()
	//byte defaultUcoef[4] = { 0x29, 0x89, 0x02, 0x00 }; // some use these values
	byte defaultUcoef[4] = { 0x7B, 0x6B, 0x01, 0x00 };

	// Window Coefficents for LUX calculation, set these accordingly from this table:
	// https://www.silabs.com/documents/public/application-notes/AN523.pdf#page=4
	float visCoefficient = 5.41f;
	float irCoefficient = -0.08f;

	// Zero offset for all ADCs, see diagram p28
	float zeroOffset = 256.0f;

	// errorCode, updated only when a command is done, p23
	// check this value if a method returns false
	Si114x_ERRORCODE errorCode = NO_ERROR;
	// Latched overflow error, see getOverflowError()
	// Tip: it's better just to check for 0xFFFF measurements
	Si114x_ERRORCODE overflowError = NO_ERROR;
	Si114x_ERRORCODE getOverflowError();

protected:
	// Shadow copy of the Si114x's parameter values
	// these may be needed for measurements and calculations
	//TODO pre-process only those values that are needed, see getNormalisedMeasurements()
	byte parameterShadow[ALS_IR_ADC_MISC + 1];
	bool readShadowParameters();

	bool readCalibrationParameters(uint16_t(&calibrationParameters)[6]);
	void calculateUcoef(const byte(&defaultUcoef)[4],
		const uint16_t(&calibrationParameters)[6], byte(&outUcoef)[4]);

	//bool awaitSleep();
	bool resetCmdCtr();
	bool awaitCmdCtr();
	bool writeParam(Si114x_PARAM paramNumber, byte paramValue);
	bool readParam(Si114x_PARAM paramNumber, byte* paramValue);
	bool writeCommand(Si114x_CMD cmd);
	bool writeReg(Si114x_REG reg, byte data);
	bool writeReg(Si114x_REG reg, const byte* data, uint length = 1);
	bool readReg(Si114x_REG reg, byte* data, uint length = 1);
	bool pollReg(Si114x_REG reg, byte* data, bool first);
};


// TODO Override this in a derived class to code your own configuration method
// see the example below
bool Si114xSensor::configureChip()
{
	LOGERROR("TODO override this in a derived class");
	return false;
}


// Here is an example configureChip() override
// you can copy/paste this into your sketch
#if 0
#include "Si114xSensor.h"

// Use inheritance to override configureChip() for your own application
class MySi114x : public Si114xSensor
{
public:
	bool configureChip();
	// poll time in milliseconds according to the MEAS_RATE
	uint pollTime = 0;
};

bool MySi114x::configureChip()
{
	// Configure Parameters

	static const Si114x_CONFIG defaultConfig[] =
	{
		// Enable channels
		CHLIST, EN_UV | EN_PS1 | EN_PS2 | EN_PS3 | EN_ALS_VIS | EN_ALS_IR,

		// MUX selection, see diagram p28
		// VDD_VOLTAGE does not work on PSx_ADCMUX unless PS_RANGE=high
		PS1_ADCMUX, NO_PHOTODIODE,			// NO_PHOTODIODE = REF, p28
		PS2_ADCMUX, GND_VOLTAGE,			// how to "reference to GND"?
		PS3_ADCMUX, TEMPERATURE,			// how to use temperature?
		//ALS_VIS_ADCMUX is hard-wired to SMALL_VISIBLE
		ALS_IR_ADCMUX, SMALL_IR,
		//AUX_ADCMUX, TEMPERATURE,			// AUX is used for UV Index, EN_UV

		// no PS LEDs
		PSLED12_SELECT, 0,

		// PS channels not used, they work as ADC channels
		PS_ADC_MISC, 0b00000000,			// xxrxxmxx, r=PS_RANGE (1=high), m : 0=PS_ADC_MODE
		// Recommended PS_ADC_REC value 'rrr' is 1's complement of PS_ADC_GAIN
		//PS_ADC_COUNTER, 0b00000000,		// xrrrxxxx, ADC recovery period before making PS measurement, p53
		//PS_ADC_GAIN, 0b00000000,			// xxxxxggg, IR LED pulse width and ADC integration time, p54

		// Alignment: 1=LS 16 bits, 0=MS 16 bits (default)
		//PS_ENCODING, 0b00000000,			// x321xxxx, 3=PS3, 2=PS2, 1=PS1
		//ALS_ENCODING, 0b00000000,			// xxivxxxx, i=IR alignment, v=VIS alignment
		//there is no AUX_ENCODING

		// Recommended VIS_ADC_REC value 'rrr' is 1's complement of ALS_VIS_ADC_GAIN
		//ALS_VIS_ADC_COUNTER, 0b01110000,	// xrrrxxxx, rrr=ADC recovery period, p56
		//ALS_VIS_ADC_GAIN, 0b00000111,		// xxxxxggg, ADC clock divisor/integration time

		// VIS high range mode, set high for bright sunlight
		ALS_VIS_ADC_MISC, 0b00000000,		// xxxrxxxx, r=VIS_RANGE : 0=normal 1=high, p57

		// Recommended IR_ADC_REC value 'rrr' 1's complement of ALS_IR_ADC_GAIN
		//ALS_IR_ADC_COUNTER, 0b01110000,	// xrrrxxxx, ADC recovery period before making ALS-IR measurement, p58
		//ALS_IR_ADC_GAIN, 0b00000111,		// xxxxxggg, ADC integration time for IR ambient measurements, p59

		// IR high range mode, set high for bright sunlight
		ALS_IR_ADC_MISC, 0b00000000,		// xxrxxxxx, r=IR_RANGE : 0=normal 1=high, p59
	};

	if (!writeConfiguration(defaultConfig, sizeof(defaultConfig) / sizeof(Si114x_CONFIG)))
		return false;

	// Configure Registers

	// enable IRQ_STATUS
	if (!writeReg(IRQ_ENABLE, 0b00011101))
		return false;

	// MEAS_RATE, 16-bit value x 31.25uS
	uint measRate = 31104;		// 31104 = 1s / 31.25us
	if (!writeReg(MEAS_RATE0, (byte*)&measRate, 2))
		return false;

	// save the poll time in milliseconds, 2x the MEAS_RATE
	pollTime = ((measRate + 1) * 3215) / 200000;

	// start autonomous conversion mode at MEAS_RATE
	// poll with readIRQStatus()
	if (!startAutonomousMeasurements())
		return false;

	return true;
}

MySi114x si1145;
#endif


// Validate communications, do software reset, read shadow parameters,
// set UV configuration in UCOEFx registers
// After this, call your own configureChip() method in a derived class,
// see example sketch.
bool Si114xSensor::begin(TwoWire* twoWire)
{
	wire = twoWire;
	partId = 0;
	errorCode = NO_ERROR;
	overflowError = NO_ERROR;

	// no access until min. 25mS after power up
	//while (millis() < 30)
	//	yield();

	// check an Si114x is connected, part ids : 0x45, 0x46, 0x47
	if (!readReg(PART_ID, &partId))
		return false;
	if (partId < 0x45 || partId > 0x47)
		return false;

	// read the calibration parameters via the measurement registers
	uint16_t calibrationParameters[6];
	if (!readCalibrationParameters(calibrationParameters))
		return false;

	// software reset
	// sets all parameters and registers to default values shown in the data sheet
	if (!softwareReset())
		return false;

	// read parameter shadow data, the parameter configuration after the reset
	if (!readShadowParameters())
		return false;

	// set the UV index coefficient registers UCOEF0..3,
	// calculate the UV index coefficients from the calibration parameters
	byte ucoef[4];
	calculateUcoef(defaultUcoef, calibrationParameters, ucoef);
	if (!writeReg(UCOEF0, ucoef, 4))
		return false;

	return true;
}

// Software Reset
// sets all parameters and registers to default values shown in the data sheet
bool Si114xSensor::softwareReset()
{
	if (!writeReg(COMMAND, RESET))
		return false;
	delay(30);			// startup sequence delay, min. 25mS

	// write 0x17 to HW_KEY to "begin normal operation"
	// (is this only for the LED drivers?)
	return writeReg(HW_KEY, 0x17);
}

// Write a configuration array to the Parameter Table, [param, data, ...]
bool Si114xSensor::writeConfiguration(const Si114x_CONFIG* config, uint configLength)
{
	for (int i = 0; i < configLength; ++i) {
		if (!writeParam(config[i].paramNumber, config[i].paramValue))
			return false;
	}
	return true;
}

// Read the calibration parameters, they are copied into the measurement 
// registers by the GET_CAL command.
// All I managed to find out about the calibration parameters is this:
// 
// typedef struct {
//    uint16_t vispd_g0;         // Visible photodiode gain correction
//    uint16_t irpd_g0;          // IR photodiode gain correction
//    uint16_t lespd_g0;         // Light sensor response gain
//    // ... matrix coefficients internally unpacked from the 12 bytes
// } SI114X_CAL_S;
// 
bool Si114xSensor::readCalibrationParameters(uint16_t(&calibrationParameters)[6])
{
	// GET_CAL copies calibration data to I2C registers 0x22..0x2D
	if (!writeCommand(GET_CAL))
		return false;
	return readReg(ALS_VIS_DATA0, (byte*)calibrationParameters, 12);
}

// Calculate the UV Index coefficients from defaults and calibration parameters
// see 'readCalibrationParameters()'
// The data sheet states, p16
// "The use of calibration parameters is documented in the file 'Si114x_functions.h', 
// which is part of the Si114x Programmer's Toolkit example source code and is 
// downloadable from Silabs.com." (Actually, it's not.)
void Si114xSensor::calculateUcoef(const byte(&defaultUcoef)[4],
	const uint16_t(&calibrationParameters)[6],
	byte(&outUcoef)[4])
{
	uint vispd_g0 = calibrationParameters[0];
	uint irpd_g0 = calibrationParameters[1];
	if (vispd_g0 == 0 && irpd_g0 == 0) {
		memcpy((byte*)outUcoef, defaultUcoef, sizeof(defaultUcoef));
		return;
	}
	// 0x3F1A is the "internal golden-chip calibration baseline reference constant"
	const ulong refConst = 0x3F1A;
	// evaluate slope modifications based on the hardware profile ratios 
	ulong ucoef[4];
	ucoef[0] = (defaultUcoef[0] * refConst) / vispd_g0;
	ucoef[1] = (defaultUcoef[1] * refConst) / irpd_g0;
	ucoef[2] = (defaultUcoef[2] * refConst) / vispd_g0;
	ucoef[3] = (defaultUcoef[3] * refConst) / irpd_g0;
	// return 8-bit values
	for (int i = 0; i < 4; i++)
		outUcoef[i] = (ucoef[i] > 0xFF) ? 0xFF : (byte)ucoef[i];
}

// Forced Mode, start new measurement
// als = ambient light sensor, ps = position sensor
bool Si114xSensor::forceMeasurement(bool als/*=true*/, bool ps/*=true*/)
{
	if (!als && !ps)
		return false;
	Si114x_CMD cmd = (als && ps) ? PSALS_FORCE : (als ? ALS_FORCE : PS_FORCE);
	return writeCommand(cmd);
}

// Autonomous Mode, start or resume continuous measurement
// als = ambient light sensor, ps = position sensor
bool Si114xSensor::startAutonomousMeasurements(bool als/*=true*/, bool ps/*=true*/)
{
	if (!als && !ps)
		return false;
	Si114x_CMD cmd = (als && ps) ? PSALS_AUTO : (als ? ALS_AUTO : PS_AUTO);
	return writeCommand(cmd);
}

// Autonomous Mode, pause continuous measurements
// als = ambient light sensor, ps = position sensor
bool Si114xSensor::pauseAutonomousMeasurements(bool als/*=true*/, bool ps/*=true*/)
{
	if (!als && !ps)
		return false;
	Si114x_CMD cmd = (als && ps) ? PSALS_PAUSE : (als ? ALS_PAUSE : PS_PAUSE);
	return writeCommand(cmd);
}

// Enable interrupts on the irqEnable channels
// irqEnable should match CHLIST bits
inline bool Si114xSensor::writeIRQEnable(byte irqEnable)
{
	//TODO is this detected by the chip INVALID_SETTING?
	if ((irqEnable ^ parameterShadow[CHLIST]) != 0)
		return false;
	return writeReg(IRQ_ENABLE, irqEnable);
}

// Read the IRQ pending status bits and clear them
// returns the same bits as in CHLIST
inline bool Si114xSensor::readIRQStatus(byte* irqStatus)
{
	*irqStatus = 0;
	byte b;
	if (!readReg(IRQ_STATUS, &b))
		return false;
	if (b) {
		*irqStatus = b;
		// clear the status bits by writing it back
		if (!writeReg(IRQ_STATUS, b))
			return false;
	}
	return true;
}

// Check the chip's operating mode
// In general, don't make configuration changes if it's in 'running' mode,
// wait until it's sleeping or suspended.
bool Si114xSensor::readChipStatus(bool* running, bool* sleeping, bool* suspended)
{
	byte chipStat;
	bool ok = readReg(CHIP_STAT, &chipStat);
	*running = (chipStat & 0b00000100) ? true : false;
	*sleeping = (chipStat & 0b00000001) ? true : false;
	*suspended = (chipStat & 0b00000010) ? true : false;
	return ok;
}

// Returns the latched overflowError and clears it
// Tip: it's better just to check the IR and ALS values, if >= 0xFFFF then it's overflowed
Si114xSensor::Si114x_ERRORCODE Si114xSensor::getOverflowError()
{
	// latched from awaitCmdCtr()
	Si114x_ERRORCODE ovf = overflowError;
	overflowError = NO_ERROR;
	return ovf;
}


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// IMPORTANT!
// The chip should be in SLEEP mode (in between readings) when readings are read, 
// so they are not read when they are changing. This is NOT checked for in the code
// below.

// Read IRQ_STATUS and all six 16-bit measurements (even those which are not used)
// uses I2C burst mode, which can be more efficient than reading each one separately
// NOTE!
// These raw ADC values must be adjusted for:
// - 16/17 bit alignment, see ALS_ENCODING and PS_ENCODING
// - high signal range mode, gain is divided by 14.5, see ALS_xxx_ADC_MISC
// - subtract zero offset (256), this is added *after* the ADC sampling, see diagram p28
// See getNormalisedMeasurements()
inline bool Si114xSensor::readAllMeasurements(Si114x_MEASUREMENTS* measurements)
{
	return readReg(IRQ_STATUS, (byte*)measurements, 13);
}

// Read individual measurements
// 0xFFFF = overflow or saturation
bool Si114xSensor::readAlsVis(uint* alsVis)
{
	// set all bytes to zero (uint may be 32-bits)
	*alsVis = 0;
	// load LS 2 bytes with the 16-bit value, LS byte first
	return readReg(ALS_VIS_DATA0, (byte*)alsVis, 2);
}
bool Si114xSensor::readAlsIr(uint* alsIr)
{
	*alsIr = 0;
	return readReg(ALS_IR_DATA0, (byte*)alsIr, 2);
}
bool Si114xSensor::readPs1(uint* ps1)
{
	*ps1 = 0;
	return readReg(PS1_DATA0, (byte*)ps1, 2);
}
bool Si114xSensor::readPs2(uint* ps2)
{
	*ps2 = 0;
	return readReg(PS2_DATA0, (byte*)ps2, 2);
}
bool Si114xSensor::readPs3(uint* ps3)
{
	*ps3 = 0;
	return readReg(PS3_DATA0, (byte*)ps3, 2);
}

// The UV Index is calculated internally
// there is no UV sensor
bool Si114xSensor::readUvIndex(uint* uvIndex)
{
	*uvIndex = 0;
	// enabled by EN_UV in CHLIST 
	if ((parameterShadow[CHLIST] & EN_UV) == 0)
		return false;
	ushort w;
	if (!readReg(AUX_DATA0_UVINDEX0, (byte*)&w, 2))
		return false;
	*uvIndex = w / 100;		// uv index value is x100
	return true;
}

// Returns "temperature x 100" ???
// AUX_DATA0_UVINDEX0 contains either the temperature or the UV Index
// TODO what is returned? mV, degC, degC - offset? ...
bool Si114xSensor::readTemperature(uint* temperature)
{
	*temperature = 0;
	// enabled by EN_AUX in CHLIST
	if ((parameterShadow[CHLIST] & EN_AUX) == 0)
		return false;
	return readReg(AUX_DATA0_UVINDEX0, (byte*)temperature, 2);
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// Calculate LUX for the sensor cover defined by visCoefficient and irCoefficient
// https://www.silabs.com/documents/public/application-notes/AN523.pdf#page=4
// alsVis and alsIr are the 16-bit readings returned by readAllMeasurements() etc
// (at low light levels the LUX value is all over the place)
ulong Si114xSensor::calculateLux(uint alsVis, uint alsIr)
{
	if (alsVis == 0xFFFF || alsIr == 0xFFFF)
		return (ulong)-1L;			// overflow/saturation

	// normalize measurments relative to gain 0
	ulong vis, ir;
	getNormalisedMeasurements(alsVis, alsIr, &vis, &ir, 0);

	// lux calculation uses coefficients from table AN523 p4 
	return (ulong)((visCoefficient * vis) + (irCoefficient * ir));
}

// The ADC values change according to the gain and the high signal range setting
// Call this to get normalised values for the specified relative gain
// The ADC offset (256) is also removed
// Returns 0..0x7FFFFF, or 0xFFFFFFFF (-1) on overflow
// alsVis, alsIr : raw ADC readings
// relativeGain  : gain (0..7) to which readings will be normalised, 
//                 2^relativeGain, 0=x1, 1=x2, .. 6=x64, 7=x128
bool Si114xSensor::getNormalisedMeasurements(uint alsVis, uint alsIr,
	ulong* normalisedVis, ulong* normalisedIr, byte relativeGain /*= 0*/)
{
	if (relativeGain > 7)
		return false;

	// gain is power-of-2, 0x000=1, 0x001=2, .. 0x110=64, 0x111=128
	float visGain = (float)(1 << (parameterShadow[ALS_VIS_ADC_GAIN] & 0x07));
	float irGain = (float)(1 << (parameterShadow[ALS_IR_ADC_GAIN] & 0x07));

	// adjust for relative gain 
	// e.g. convert a x2 value to the value it would be at x128
	if (relativeGain != 0) {
		float relGain = (float)(1 << relativeGain);
		visGain /= relGain;
		irGain /= relGain;
	}

	// high signal range mode = gain is divided by 14.5
	static const byte rangeMask = 0b00100000;
	if (parameterShadow[ALS_VIS_ADC_MISC] & rangeMask)
		visGain /= 14.5f;
	if (parameterShadow[ALS_IR_ADC_MISC] & rangeMask)
		irGain /= 14.5f;

	// adjust for zero offset (256)
	// offset is added *after* the ADC sampling, see Sum in diagram p28
	uint vis = alsVis < zeroOffset ? 0 : alsVis - zeroOffset;
	uint ir = alsIr < zeroOffset ? 0 : alsIr - zeroOffset;

	// adjust for 16/17 bit alignment
	// 1=LS 16 bits of ADC, 0=MS 16 bits of ADC (default)
	// ALS_ENCODING : xxivxxxx, i=IR alignment, v=VIS alignment
	// if MS 16 bits, it holds bits 16..1, so the actual ADC count is x 2
	byte alsEncoding = parameterShadow[ALS_ENCODING];
	if ((alsEncoding & 0b00010000) == 0)
		vis <<= 1;
	if ((alsEncoding & 0b00100000) == 0)
		ir <<= 1;

	// return 0xFFFFFFFF (-1) on overflow
	*normalisedVis = alsVis >= 0xFFFF ? -1 : (ulong)((float)vis / visGain);
	*normalisedIr = alsIr >= 0xFFFF ? -1 : (ulong)((float)ir / irGain);

	return true;
}

// Automaticaly adapt the gain according to the last VIS and IR measurements 
// to prevent 16-bit overflow/underflow.
// Returns true if the gain was changed
// IF IT RETURNS TRUE you must start the next reading immediately,
// - if in autonomous mode, call startAutonomousMeasurements()
// - if in forced mode, call forceMeasurement()
// YOU MUST USE getNormalisedMeasurements() for the VIS and IR values to
// account for the gain and other factors.
//TODO high range mode handling could be improved (use indoor/outdoor setting?)
bool Si114xSensor::automaticGainControl(uint alsVis, uint alsIr)
{
	// hysteresis
	// overflow occurs at 0x7FFF (NOT 0xFFFF), AN498 p38
	// or 0x3FFF if MS 16 bits is returned (default) !!! 
	// 16/17 bit alignment is handled below
	static const uint alsMax = 30000;
	static const uint alsMin = 3000;

	// gain is power-of-2, 0x000=1 0x001=2 0x010=4 0x011=8 0x100=16 0x101=32 0x110=64 0x111=128
	byte visGain = parameterShadow[ALS_VIS_ADC_GAIN] & 0x07;
	byte irGain = parameterShadow[ALS_IR_ADC_GAIN] & 0x07;

	// high signal range mode = gain divided by 14.5
	static const byte rangeMask = 0b00100000;
	byte visAdcMisc = parameterShadow[ALS_VIS_ADC_MISC]; // xxrxxxxx, r=VIS_RANGE : 0=normal 1=high, p57
	byte irAdcMisc = parameterShadow[ALS_IR_ADC_MISC];   // xxrxxxxx, r=IR_RANGE  : 0=normal 1=high, p59

	// adjust for 16/17 bit alignment, else overflow occurs at 0x3FFF which is wrong!
	// 1=LS 16 bits, 0=MS 16 bits (default)
	// ALS_ENCODING, 0b00110000,		// xxivxxxx, i=IR alignment, v=VIS alignment
	byte alsEncoding = parameterShadow[ALS_ENCODING];
	if ((alsEncoding & 0b00010000) == 0)
		alsVis <<= 1;
	if ((alsEncoding & 0b00100000) == 0)
		alsIr <<= 1;

	bool visGainChanged = false;
	bool visRangeChanged = false;
	if (alsVis > alsMax) {
		if (visGain > 0) {
			// reduce the gain
			--visGain;
			visGainChanged = true;
		}
		else if ((visAdcMisc & rangeMask) == 0) {
			// turn on high range mode
			visAdcMisc |= rangeMask;
			visRangeChanged = true;
		}
	}
	else if (alsVis < alsMin) {
		if (visGain < 7) {
			// increase the gain
			++visGain;
			visGainChanged = true;
		}
		else if (visAdcMisc & rangeMask) {
			// turn off high range mode
			visAdcMisc &= ~rangeMask;
			visRangeChanged = true;
		}
	}

	bool irGainChanged = false;
	bool irRangeChanged = false;
	if (alsIr > alsMax) {
		if (irGain > 0) {
			// reduce the gain
			--irGain;
			irGainChanged = true;
		}
		else if ((irAdcMisc & rangeMask) == 0) {
			// turn on high range mode
			irAdcMisc |= rangeMask;
			irRangeChanged = true;
		}
	}
	else if (alsIr < alsMin) {
		if (irGain < 7) {
			// increase the gain
			++irGain;
			irGainChanged = true;
		}
		else if (irAdcMisc & rangeMask) {
			// turn off high range mode
			irAdcMisc &= ~rangeMask;
			irRangeChanged = true;
		}
	}
	if (!(visGainChanged || visRangeChanged || irGainChanged || irRangeChanged))
		return false;		// gain not changed

	bool ok = false;
	if (visGainChanged)
		ok |= writeParam(ALS_VIS_ADC_GAIN, visGain);
	if (visRangeChanged)
		ok |= writeParam(ALS_VIS_ADC_MISC, visAdcMisc);
	if (irGainChanged)
		ok |= writeParam(ALS_IR_ADC_GAIN, irGain);
	if (irRangeChanged)
		ok |= writeParam(ALS_IR_ADC_MISC, irAdcMisc);

	if (visGainChanged || irGainChanged)
		LOGERROR("gain changed");
	if (visRangeChanged || irRangeChanged)
		LOGERROR("range changed");

	//TODO 
	// if in autonomous mode, on return take a new set of readings by 
	// calling startAutonomousMeasurements()
	// if in forced mode, on return call forceMeasurement()

	return true;			// gain was changed
}

// Get the gains which may have been modified by automaticGainControl() etc.
// visGain  : 0..7, 0=x1, .. 7=x128, gain = 2^visGain
// irGain   : 0..7, 0=x1, .. 7=x128, gain = 2^irGain
// visRange : true=high range, gain /= 14.5, 0=normal
// irRange  : true=high range, gain /= 14.5, 0=normal
void Si114xSensor::getGain(byte* visGain, byte* irGain, bool* visHighRange, bool* irHighRange)
{
	// gain x1..x128
	*visGain = parameterShadow[ALS_VIS_ADC_GAIN] & 0x07;
	*irGain = parameterShadow[ALS_IR_ADC_GAIN] & 0x07;

	// high range mode
	*visHighRange = parameterShadow[ALS_VIS_ADC_MISC] & 0b00100000 ? true : false;
	*irHighRange = parameterShadow[ALS_IR_ADC_MISC] & 0b00100000 ? true : false;

	/*TODO ??? this affects the values too
	// 16/17 bit alignment
	// 1=LS 16 bits, 0=MS 16 bits (default)
	// ALS_ENCODING, 0b00110000,	// xxivxxxx, i=IR alignment, v=VIS alignment
	byte alsEncoding = parameterShadow[ALS_ENCODING];
	if ((alsEncoding & 0b00010000) == 0)
		;// alsVis <<= 1;
	if ((alsEncoding & 0b00100000) == 0)
		;// alsIr <<= 1;
	*/
}


// Local Methods

// Read all parameter values and save them in the public parameterShadow[] array
bool Si114xSensor::readShadowParameters()
{
	for (int i = 0; i <= ALS_IR_ADC_MISC; ++i) {
		if (!readParam((Si114x_PARAM)i, parameterShadow + i))
			return false;
	}
	return true;
}

/*not used
// Wait until the Si114x is in SLEEP mode, with timeout
// so we are sure it's not updating measurement data
bool Si114xSensor::awaitSleep()
{
	ulong t = millis();
	bool first = true;
	while (1) {
		byte chipStat;
		if (!pollReg(CHIP_STAT, &chipStat, first))
			break;
		first = false;
		if (chipStat & 1)			// in SLEEP mode
			return true;
		if (millis() - t >= 1000) {	// timed out
			errorCode = TIMED_OUT;
			break;
		}
		yield();
	}
	return false;
}
*/

// Reset the RESPONSE register to zero with NOP command
// also clears INVALID_SETTING status
// the next valid command increments the CMD_CTR to 1
// poll the CMD_CTR with awaitCmdCtr()
bool Si114xSensor::resetCmdCtr()
{
	errorCode = I2C_FAILED;
	if (!writeReg(COMMAND, NOP))
		return false;
	// why do we need this? 
	// without this, the command counter is NOT reset
	byte response;
	return readReg(RESPONSE, &response);
}

// After every command, poll RESPONSE and check for errors
// Overflow errors are latched in 'overflowError', read and clear with 
// getOverflowError().
// CMD_CTR is incremented when the command completes successfully
// bit 7 is set if an error occurs, with the error number in bits 7..4
bool Si114xSensor::awaitCmdCtr()
{
	errorCode = I2C_FAILED;
	ulong t = millis();
	bool first = true;
	while (1) {
		yield();
		byte response;
		if (!pollReg(RESPONSE, &response, first))
			break;
		first = false;

		// was there an error? (if bit 7 set, it's an error code)
		// 0x80 = Invalid Command
		// 0x88 = PS1 ADC overflow
		// 0x89 = PS2 ADC overflow
		// 0x8A = PS3 ADC overflow
		// 0x8C = ALS_VIS ADC overflow
		// 0x8D = ALS_IR ADC overflow
		// 0x8E = AUX ADC overflow
		if (response & 0x80) {
			errorCode = (Si114x_ERRORCODE)response;
			// overflow error is latched until read by getOverflowError()
			if (errorCode >= 0x88 && errorCode <= 0x8E) {
				LOGERROR("overflow error");
				overflowError = errorCode;
				// "Even if the RESPONSE register has an overflow condition, 
				// commands are still accepted and processed.", p13
				// so I assume the command was executed
				return true;
			}
			if (errorCode == Si114x_ERRORCODE::INVALID_COMMAND)
				LOGERROR("invalid command");
			break;
		}

		// because we did a NOP command, the count should now be 1
		// (other librares just check it's changed, they don't check the value)
		if (response == 1) {
			errorCode = NO_ERROR;
			return true;
		}
		if (response > 1)
			break;
		if (millis() - t > 1000) {
			errorCode = TIMED_OUT;
			break;
		}
	}

	// failed, NOP is sent at start of next command (clears RESPONSE)
	return false;
}

// Write to COMMAND register and poll RESPONSE0 until CMD_CTRL field is incremented
bool Si114xSensor::writeCommand(Si114x_CMD cmd)
{
	// must be in SLEEP mode? SiliconLabs gecko lib does this
	//if (!awaitSleep())
	//	return false;

	// set command counter to 0
	if (!resetCmdCtr())
		return false;
	// write COMMAND register
	if (!writeReg(COMMAND, cmd))
		return false;
	// poll RESPONSE and check the CMD_CTR field was incremented
	// set errorCode to the result
	return awaitCmdCtr();
}

// Write one parameter
bool Si114xSensor::writeParam(Si114x_PARAM paramNumber, byte paramValue)
{
	if (paramNumber > ALS_IR_ADC_MISC)
		return false;

	// must be in SLEEP mode? SiliconLabs gecko lib does this
	//if (!awaitSleep())
	//	return false;

	// set command counter to 0
	if (!resetCmdCtr())
		return false;
	// write data & PARAM_SET bit | parameter number to PARAM_WR & COMMAND registers
	byte b[2] = { paramValue, PARAM_SET | paramNumber };
	if (!writeReg(PARAM_WR, b, 2))
		return false;
	// poll RESPONSE and check the CMD_CTR field was incremented
	// set errorCode to the result
	if (!awaitCmdCtr())
		return false;

	// update parameter shadow data
	parameterShadow[paramNumber] = paramValue;

	#if 1
	// debug test only, check PARAM_RD contains 'paramValue'
	byte paramRd;
	if (!readReg(PARAM_RD, &paramRd))
		return false;
	if (paramRd != paramValue)
		return false;
	#endif

	return true;
}

// Read one parameter
bool Si114xSensor::readParam(Si114x_PARAM paramNumber, byte* paramValue)
{
	*paramValue = 0;
	if (!writeCommand((Si114x_CMD)(Si114x_CMD::PARAM_QUERY | paramNumber)))
		return false;
	// return the data byte
	return readReg(PARAM_RD, paramValue);
}

// Write one register
inline bool Si114xSensor::writeReg(Si114x_REG reg, byte data)
{
	return writeReg(reg, &data);
}

// Write one or more registers
bool Si114xSensor::writeReg(Si114x_REG reg, const byte* data, uint length /*=1*/)
{
	wire->beginTransmission(i2cAdds);
	wire->write((byte)reg);
	if (wire->write(data, length) != length) {
		LOGERROR("write() failed");
		return false;
	}
	if (wire->endTransmission() != 0) {
		LOGERROR("endTransmission() failed");
		return false;
	}
	return true;
}

// Read one or more registers
bool Si114xSensor::readReg(Si114x_REG reg, byte* data, uint length /*=1*/)
{
	memset(data, 0, length);		// return 0s if it fails

	wire->beginTransmission(i2cAdds);
	wire->write((byte)reg);			// default is auto-increment address
	if (wire->endTransmission(false) != 0) {
		LOGERROR("endTransmission() failed");
		return false;
	}
	wire->requestFrom((int)i2cAdds, length);
	if (wire->readBytes((byte*)data, length) != length) {
		LOGERROR("readBytes() failed");
		return false;
	}
	return true;
}

// Poll the same register, sending the address only once when
// first = true, uses "auto increment disable"
bool Si114xSensor::pollReg(Si114x_REG reg, byte* data, bool first)
{
	*data = 0;

	// send address only on first call, with auto increment disabled
	if (first) {
		wire->beginTransmission(i2cAdds);
		// 0x40 = auto increment disable
		wire->write((byte)reg | 0x40);
		if (wire->endTransmission(false) != 0) {
			LOGERROR("endTransmission() failed");
			return false;
		}
	}
	wire->requestFrom((int)i2cAdds, 1);
	int b = wire->read();
	if (b < 0) {
		LOGERROR("read() failed");
		return false;
	}
	*data = (byte)b;
	return true;
}
