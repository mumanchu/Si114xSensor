///////////////////////////////////////////////////////////////////////////////
// Example Sketch for Si114xSensor Library
// Copyright (C) 2026.10.09, https://muman.ch and https://github.com/mumanchu
// If you re-use this software, please include the above copyright notice
// (and wire me a huge pile of cash ;-)

#include <Wire.h>

// Comment this out for release mode, to remove the log output
#define DEBUG

#ifdef DEBUG
#define LOGERROR(s) { Serial.println(s); Serial.flush(); }
#else
#define LOGERROR(s)
#endif

typedef unsigned long ulong;    // 32 bits
typedef unsigned int uint;      // 16 or 32 bits
typedef unsigned short ushort;  // 16 bits


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
		// VDD_VOLTAGE does not work on PSx_ADCMUX
		PS1_ADCMUX, NO_PHOTODIODE,			// NO_PHOTODIODE = REF, p28
		PS2_ADCMUX, GND_VOLTAGE,			// how to "reference to GND"?
		PS3_ADCMUX, TEMPERATURE,			// e.g. 0x429E
		//ALS_VIS_ADCMUX is hard-wired to SMALL_VISIBLE
		ALS_IR_ADCMUX, SMALL_IR,
		//AUX_ADCMUX, TEMPERATURE,			// AUX is used for UV Index, EN_UV

		/*
		// MUX selection, see diagram p28
		PS1_ADCMUX, TEMPERATURE,
		PS2_ADCMUX, TEMPERATURE,
		PS3_ADCMUX, TEMPERATURE,
		//ALS_VIS_ADCMUX is hard-wired to SMALL_VISIBLE
		ALS_IR_ADCMUX, SMALL_IR,
		AUX_ADCMUX, TEMPERATURE,
		*/

		//TODO LARGE_IR on PS1..PS3
		//TODO do we need GND_VOLTAGE, VDD_VOLTAGE?
		//TODO how to use TEMPERATURE?
		//TODO re-enable UV index

		// no PS LEDs
		PSLED12_SELECT, 0,

		// PS channels not used, they work as ADC channels
		PS_ADC_MISC, 0b00000000,			// xxrxxmxx, r=PS_RANGE (1=high), m : 0=PS_ADC_MODE
		// Recommended PS_ADC_REC value 'rrr' is 1's complement of PS_ADC_GAIN
		//PS_ADC_COUNTER, 0b00000000,		// xrrrxxxx, ADC recovery period before making PS measurement, p53
		//PS_ADC_GAIN, 0b00000000,			// xxxxxggg, IR LED pulse width and ADC integration time, p54

		// Alignment: 1=LS 16 bits, 0=MS 16 bits (default)
		//PS_ENCODING, 0b01110000,			// x321xxxx, 3=PS3, 2=PS2, 1=PS1
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
	if (!startMeasurements())
		return false;

	return true;
}

MySi114x si1145;



void setup()
{
	// I use different pins for RX/TX logging
	// this is only for the STM32 Nucleo-64 boards
	#ifdef ARDUINO_NUCLEO_64
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);
	#endif

	Serial.begin(115200);
	delay(3000);

	// PuTTY clear screen and scrollback
	Serial.print("\033[2J\033[H\033[3J");

	Serial.println("\n\rStarted\n\r");
	Serial.flush();

	// Some boards have no user LED
	#ifdef LED_BUILTIN
	pinMode(LED_BUILTIN, OUTPUT);
	#endif

	Wire.begin();
	Wire.setClock(400000);

	if (!si1145.begin(&Wire)) {
		Serial.println("si1145.begin() failed");
		Serial.flush();
		while (1) yield();
	}
	if (!si1145.configureChip()) {
		Serial.println("si1145.configureChip() failed");
		Serial.flush();
		while (1) yield();
	}
}


void pollSi1145()
{
	// use pollTime poll IRQ status at 2x the MEAS_RATE
	static ulong pollStart = 0;
	ulong tpoll = millis();
	if (tpoll - pollStart < si1145.pollTime)
		return;
	pollStart = tpoll;

	// readings ready yet?
	byte irqStatus;
	if (si1145.readIRQStatus(&irqStatus) && irqStatus == 0)
		return;

	// new reading available, toggle the LED
	#ifdef LED_BUILTIN
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	#endif

	// report time between readings
	// set by MEAS_RATE, see configureChip()
	static ulong tStart = 0;
	ulong t = micros();
	ulong dt = tStart == 0 ? 0 : t - tStart;
	tStart = t;

	Si114xSensor::Si114x_MEASUREMENTS m;
	if (!si1145.readAllMeasurements(&m))
		return;

	// overflow flags from RESPONSE  register, latched until read
	// these are set when automaticGainControl() changes the gain
	Si114xSensor::Si114x_ERRORCODE ovfError = si1145.getOverflowError();
	if (ovfError)
		LOGERROR("ovfError");

	// check the values for overflow
	// The range is 0..0x3FFF for MS 16 bits or 0..0x7FFF for LS 16 bits,
	// see the the PS_ENCODING and ALS_ENCODING parameters.
	// For MS 16 bits, the result is set to 0xFFFF if above 0x3FFF.
	// For LS 16 bits, the result is set to 0xFFFF if above 0x7FFF.
	bool overflow = m.alsVis >= 65535 || m.alsIr >= 65535;
	if (overflow)
		LOGERROR("overflow");

	/*
	// Measurement values, for readAllMeasurements()
	typedef struct {
		byte irqStatus;
		uint16_t alsVis;
		uint16_t alsIr;
		uint16_t ps1;		// NO_PHOTODIODE
		uint16_t ps2;		// GND_VOLTAGE
		uint16_t ps3;		// TEMPERATURE
		uint16_t aux;		// VDD_VOLTAGE
	} Si114x_MEASUREMENTS;
	*/

	ulong lux = si1145.calculateLux(m.alsVis, m.alsIr);

	//TODO temperature calculation?

	char buf[128];
	// comment out line for hex or decimal display
	//sprintf(buf, "dt=%lumS alsVis=%04x alsIr=%04x REF=%04x GND=%04x TEMP=%04x UVI=%04x LUX=%lu",
	sprintf(buf, "dt=%lumS alsVis=%u alsIr=%u REF=%u GND=%u TEMP=%u UVI=%u LUX=%lu",
		dt / 1000,
		m.alsVis,
		m.alsIr,
		m.ps1,		// NO_PHOTODIODE
		m.ps2,		// GND_VOLTAGE
		m.ps3,		// TEMPERATURE
		m.aux,		// VDD_VOLTAGE
		lux);
	Serial.println();
	Serial.println(buf);
	Serial.flush();

	// show current gain and range
	byte visGain, irGain;
	bool visRange, irRange;
	si1145.getGain(&visGain, &irGain, &visRange, &irRange);
	sprintf(buf, "visGain=%u visRange=%u irGain=%u irRange=%u", 1 << visGain, visRange, 1 << irGain, irRange);
	Serial.println(buf);
	Serial.flush();

	// show some normalised readings
	ulong vis, ir;
	si1145.getNormalisedMeasurements(m.alsVis, m.alsIr, &vis, &ir);
	sprintf(buf, "x1   vis=%lu, ir=%lu", vis, ir);
	Serial.println(buf);
	Serial.flush();

	si1145.getNormalisedMeasurements(m.alsVis, m.alsIr, &vis, &ir, 6);
	sprintf(buf, "x64  vis=%lu, ir=%lu", vis, ir);
	Serial.println(buf);
	Serial.flush();

	si1145.getNormalisedMeasurements(m.alsVis, m.alsIr, &vis, &ir, 7);
	sprintf(buf, "x128 vis=%lu, ir=%lu", vis, ir);
	Serial.println(buf);
	Serial.flush();

	// automatic gain control
	if (si1145.automaticGainControl(m.alsVis, m.alsIr)) {
		// in autonomous mode, start a new set of readings
		// this takes a new set of readings immediately, making automaticGainControl() very fast
		pollStart = 0;				// immediate poll on next call
		si1145.startMeasurements();
		// (if using forced mode, call forceMeasurement())
	}
}


void loop()
{
	pollSi1145();
}
