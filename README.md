# Si114xSensor : Arduino Library for Si1145/6/7 
Silicon Labs ALS, IR and Proximity/Motion/Gesture Sensor, with on-chip UV Index calculation.

> [!NOTE]
> The Silicon Labs Si144x chips are now discontinued and are no longer manufactured. (So it's not recommended for new designs :-)
> They also do not contain a UV Sensor, the UV Index is an estimation calculated by the on-chip DSP.

But you can still buy breakout boards like this one on AliExpress, from Shenzhen Module Studio Co Ltd (CHF 14.-) \
https://de.aliexpress.com/item/1005012309826317.html

<img src="assets/si1145-breakout-board.jpg" alt="Picture of Si1145 breakout board" width="300">


The Silicon Labs data sheet says, "Integrated UV index sensor". That is not quite true. It estimates the UV Index from the ambient light and infrared sensor readings.

I updated this old code for the Arduino architecture while playing with UV Sensors. Maybe someone is interested (but probably not). 

Below are some technical details, so read the Data Sheet first to get an overview. Page numbers, e.g. p28, refer to this version of the data sheet, Rev. 1.3 (12/14). \
https://www.waveshare.com/w/upload/9/99/Si1145-46-47.pdf

ALS = Ambient Light Sensor : ALS_VIS = Visible Light, ALS_IR = Infrared \
PS = Position Sensor

## Advantages of this Antique Library

Unlike most of the other old Si114x libraries, this code adjusts the raw measurements using the gain, range setting and 16/17 bit alignment, see `getNormalisedMeasurements()`. Thus the LUX and other calculations automatically adapt themselves according to the configuration, and the `automaticGainControl()` feature uses normalized measurements and the correct max levels (0x3FFF or 0x7FFF depending on the 16/17 bit encoding, see below). 

It is also non-blocking, polling the `IRQ_STATUS` register or INT pin to determine when readings were ready.

## 16 or 17-Bit Encoding

The ADC is 17-bits, but the measurement registers only hold 16-bits. So it must be configured to copy either the MS or the LS 16 bits from the ADC into the measurement registers by using the `PS_ENCODING` and `AS_ENCODING` parameters. 

The default is **MS 16 bits**. This means that the raw values should all be multiplied by 2 (shift left 1). This setting must be taken into account when using the raw measurements. This could be why some users were complaining about the readings being too high.

## Postion Sensing

This library does not handle proximity, motion or gesture sensing. Only the ambient light, IR sensor and UV Index are enabled. If you want to add proximity sensing, Silicon Labs has example code for full gesture sensing in this file, \
https://github.com/x893/SX1231/blob/master/SX12xxDrivers-2.0.0/src/platform/efm32libs/kits/common/drivers/si114x_algorithm.c

The Si1145 supports one IR LED for proximity only. If it's only proximity sensing that you need, use a cheap IR reflective sensor like the TCRT5000 instead. \
https://muman.ch/muman/index.htm?muman-infrared-reflective-sensor.htm

For motion detection you'll need an Si1146 (with 2 x IR LEDs), and for gesture detection you'll need an Si1147 (with 3 x IR LEDS). But there are more recent and better chips out there for this, even using radar signals (mmWave radar sensors). There will be a muman.ch blog post about these soon.

## Input Selection and Configuration

Here is the diagram from the data sheet. Each ADC conversion has a multiplexer (MUX) to select the input, configurable ADC conversion, and finally a 'Sum' operation which adds an offset of 256 to each reading. 

The AUX_ADCMUX at the bottom has no configuration, and only the VDD and Temperature inputs can be selected, or the AUX data registers are used for the UV Index value (not shown on the diagram) if `EN_UV` is used instead of `EN_AUX` in `CHLIST`. 

The diagram makes it look as though there are 6 ADCs. In reality I think there is only one. Readings are not taken simultaneously, they are taken sequentially.

<img src="assets/si114x-signal-path.png" alt="Si114x Signal Path Diagram">


## AUX MUX

As seen above, the chip contains 5 multiplexers (MUX) which define the ADC inputs for each reading. The input for each MUX is selected with the `xxx_ADCMUX` parameters, except `ALS_VIS_DATA` which is hard-wired to the visible light sensor. Each has parameters for defining 16/17 bit alignment, rate, gain and range. But the AUX input does not have any configuration.

The AUX MUX reading is normally used for the UV Index calculation, with readings returned in the `AUX_DATAx_UVINDEXx` registers. Select this with `EN_UV` in `CHLIST`. Alternatively, AUX can be used for TEMPERATURE or VDD_VOLTAGE measurement. 

The AUX input does not have an `xxx_ENCODING` setting, so it's not clear if it's the MS 16 bits or the LS 16 bits when the Temperature or VDD inputs are selected.

The AUX reading also seems to be scaled differently to the others. For example, with TEMPERATURE connected to PS1, PS2 or PS3 I got a reading of 0x429E (for example). When TEMPERATURE is connected to AUX then I got a reading of 0x2C3C. I was not able to find a relationship between these two readings.

Note: `VDD_VOLTAGE` does not work on the `PSx_ADCMUXs` unless PS_RANGE = high, it always returns 0xFFFF.

## Overflow Detection

When a command is sent, the `RESPONSE` register can return an overflow status, but it's normally used for command counting. This causes problems with the software because the command counter value is not returned if an overflow occurs - so did the command work or not?

This library latches the overflow error into the `overflowError` value which is read and cleared with `getOverflowError()`. However, this only detects overflow if commands are being sent, so you should always check the returned measurements for 0xFFFF to detect overflow.

The max. value before overflow is returned is NOT 0xFFFE as you would imagine. This is not clearly described in the data sheet, and other libraries get this wrong.

The range is 0..0x3FFF for MS 16 bits or 0..0x7FFF for LS 16 bits, see the `PS_ENCODING` and `ALS_ENCODING` parameters.

For MS 16 bits, the result is set to 0xFFFF if above **0x3FFF**. For LS 16 bits, the result is set to 0xFFFF if above **0x7FFF**.

The 16/17 bit setting must be taken into account when doing automatic gain control.

## Grey Areas

There were a lot of unanswered questions with this chip. For example, go to p51 (AREA51). I'm sure it has nothing to do with The Grays.

I never figured out what to do with the offset measurements (No Photodiode, GND measurement, VDD voltage, etc), despite playing around for a while. So I just ignored them, as did everyone else, including SiliconLabs. If anyone knows how to use these offset measurements, or how to use the Temperature sensor, please let me know! (info@muman.ch)

From the data sheet, p51...

**0x02: Visible Photodiode** \
A separate 'No Photodiode' measurement should be subtracted from this reading. Note that the result is a negative value. The result should therefore be negated to arrive at the Ambient Visible Light reading.

**0x03: Large IR Photodiode** \
A separate 'No Photodiode' measurement should be subtracted to arrive at Ambient IR reading.

**0x00: Small IR Photodiode** \
A separate 'No Photodiode' measurement should be subtracted to arrive at Ambient IR reading.

**0x06: No Photodiode** \
This is typically used as reference for reading ambient IR or visible light.

**0x25: GND voltage** \
This is typically used as the reference for electrical measurements.

**0x65: Temperature** \
(Should be used only for relative temperature measurement. Absolute temperature not guaranteed) A separate GND measurement should be subtracted from this reading.

**0x75: VDD voltage** \
A separate GND measurement is needed to make the measurement meaningful.


# Class Reference

To find out what each method does, open the `src/Si114xSensor.h` file and read the voluminous comments for each method.  

```cpp
class Si114xSensor
{
public:
	// Override this is a derived class to code your own configuration
	virtual bool configureChip();

	bool begin(TwoWire* twoWire);
	bool writeDefaultConfiguration();
	bool softwareReset();
	bool writeConfiguration(const Si114x_CONFIG* config, uint configLength);

	bool forceMeasurement(bool als = true, bool ps = true);
	bool pauseMeasurements(bool als = true, bool ps = true);
	bool startMeasurements(bool als = true, bool ps = true);

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

	bool getNormalisedMeasurements(uint alsVis, uint alsIr,	ulong* normalisedVis,
		ulong* normalisedIr, byte relativeGain = 0);
	ulong calculateLux(uint alsVis, uint alsIr);
	bool automaticGainControl(uint aslVis, uint alsIr);
	void getGain(byte* visGain, byte* irGain, bool* visHighRange, bool* irHighRange);
};
```

## Overriding `configureChip()` for Custom Configuration

To avoid modifying the library file, and to provide a kind of configuration template, a class can be derived from Si114xSensor and the `configureChip()` method is overridden in the derived class. The derived class also adds calculation of the `pollTime` value so it does not poll the chip faster than is necessary when waiting for new readings.

These are illustrated in the example sketch. 


## References

All page numbers refer to this version of the data sheet (Rev. 1.3 12/14) \
https://www.waveshare.com/w/upload/9/99/Si1145-46-47.pdf

An older version (Rev. 1.1 12/13) \
https://www.mouser.com/datasheet/2/737/Si1145-46-47-932790.pdf

**AN498: Si114x Designers Guide** \
for proximity and gesture detection \
https://www.silabs.com/documents/public/application-notes/AN498.pdf

**AN523: Overlay Considerations for the Si114x Sensor** \
https://www.silabs.com/documents/public/application-notes/AN523.pdf

**AN580: Infrared Gesture Sensing** \
https://www.edn.com/eeweb-content/wp-content/uploads/articles-app-notes-files-infrared-sensing-1319238328.pdf

**AN522: Using the Si1141 for Touchless Lavatory Appliances** \
Thankfully, I couldn't find this one.

**Silicon Labs Code from 2014** \
Look for 'Si114x' \
https://github.com/x893/SX1231/tree/master/SX12xxDrivers-2.0.0/src/platform/efm32libs/kits/common/drivers

**Breakout Boards** \
These are probably no longer available \
https://github.com/Seeed-Studio/Grove_Sunlight_Sensor \
https://learn.adafruit.com/adafruit-si1145-breakout-board-uv-ir-visible-sensor

**Other Libraries** \
For reference. These may not handle certain configuration changes, like 16/17 bit alignment, etc. \
https://github.com/adafruit/Adafruit_SI1145_Library \
https://github.com/wollewald/SI1145_WE/tree/master \
https://github.com/HGrabas/SI1145/tree/master \
https://github.com/Seeed-Studio/Grove_Sunlight_Sensor


## Revision History

| Date  | Revision | Description |
|:---------- |:---------|:----------- |
| 2026.10.09 | 0.0.0	| Preiminary |

<br/>

## Joke of the Week

Pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679... \
Shepherd's Pi = 3

**Homework** \
_Define the physical characteristics of a universe in which Pi is an integer._ \
_Include the changes to Maxwell's Equations and the effects on the speed of light C and the speed of time T._


