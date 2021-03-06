#include <HomeSpan.h>
#include <SoftwareSerial.h>
#include <ErriezMHZ19B.h>
#include <Adafruit_NeoPixel.h>
#include "SerialCom.hpp"
#include "Types.hpp"
#include <Smoothed.h>

#define MHZ19B_TX_PIN		 19
#define MHZ19B_RX_PIN		 18
#define INTERVAL			 10	  // in seconds
#define HOMEKIT_CO2_TRIGGER	 1350 // co2 level, at which HomeKit alarm will be triggered
#define NEOPIXEL_PIN		 16	  // Pin to which NeoPixel strip is connected
#define NUMPIXELS			 1	  // Number of pixels
#define BRIGHTNESS_DEFAULT	 10	  // Default (dimmed) brightness
#define BRIGHTNESS_MAX		 150  // maximum brightness of CO2 indicator led
#define BRIGHTNESS_THRESHOLD 500  // TODO calibrate Threshold value of dimmed brightness
#define ANALOG_PIN			 35	  // Analog pin, to which light sensor is connected
#define SMOOTHING_COEFF		 10	  // Number of elements in the vector of previous values

bool				  needToWarmUp	= true;
bool				  playInitAnim	= true;
int					  tick			= 0;
bool				  airQualityAct = false;
particleSensorState_t state;
Smoothed<float>		  mySensor;
Smoothed<float>		  mySensor_air;

// Declare functions
void detect_mhz();
void fadeIn(int pixel, int r, int g, int b, int brightnessOverride, double duration);
void fadeOut(int pixel, int r, int g, int b, double duration);
void initAnimation();
int	 neopixelAutoBrightness();

////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

// Use software serial
SoftwareSerial mhzSerial(MHZ19B_TX_PIN, MHZ19B_RX_PIN);

// Declare MHZ19B object
ErriezMHZ19B mhz19b(&mhzSerial);

// Create Neopixel object
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

struct DEV_CO2Sensor : Service::CarbonDioxideSensor { // A standalone Temperature sensor

	SpanCharacteristic *co2Detected;
	SpanCharacteristic *co2Level;
	SpanCharacteristic *co2PeakLevel;
	SpanCharacteristic *co2StatusActive;

	DEV_CO2Sensor() : Service::CarbonDioxideSensor() { // constructor() method

		co2Detected		= new Characteristic::CarbonDioxideDetected(false);
		co2Level		= new Characteristic::CarbonDioxideLevel(399);
		co2PeakLevel	= new Characteristic::CarbonDioxidePeakLevel(400);
		co2StatusActive = new Characteristic::StatusActive(false);

		Serial.print("Configuring Carbon Dioxide Sensor\n"); // initialization message

		mhzSerial.begin(9600);

		detect_mhz();

		// Enable auto-calibration
		mhz19b.setAutoCalibration(true);

		pixels.begin();
		pixels.setBrightness(50);
		pixels.show();

		mySensor.begin(SMOOTHED_EXPONENTIAL, SMOOTHING_COEFF); // SMOOTHED_AVERAGE, SMOOTHED_EXPONENTIAL options
	}

	void loop() {

		if (playInitAnim) {
			Serial.println("Init animation");
			initAnimation();
			playInitAnim = false;
		}

		if (co2StatusActive->timeVal() > 5 * 1000 && needToWarmUp) {
			// Serial.println("Need to warm up");

			if (mhz19b.isWarmingUp()) {
				Serial.println("Warming up");
				pixels.setPixelColor(0, pixels.Color(255, 165, 0));
				pixels.setBrightness(neopixelAutoBrightness());
				pixels.show();
				delay(2.5 * 1000);
				pixels.setPixelColor(0, pixels.Color(0, 0, 0));
				pixels.show();
				tick = tick + 5;
				Serial.println((String)tick + " ");
				co2StatusActive->setVal(false);
			} else {
				needToWarmUp = false;
				co2StatusActive->setVal(true);
				Serial.println("Is warmed up");
			}
		}

		if (co2Level->timeVal() > INTERVAL * 1000 && !needToWarmUp) { // check time elapsed since last update and proceed only if greater than 5 seconds

			float co2_value;

			if (mhz19b.isReady()) {
				co2_value = mhz19b.readCO2();
			}

			if (co2_value >= 400) {

				// Read a sensor value
				// print co2 value
				LOG1("CO2: ");
				LOG1(co2_value);
				LOG1(" ppm\n");

				mySensor.add(co2_value);

				co2Level->setVal(mySensor.get()); // set the new co value; this generates an Event Notification and also resets the elapsed time
				LOG1("Carbon Dioxide Update: ");
				LOG1(co2Level->getVal());
				LOG1("\n");

				// Set color indicator
				// 400 - 800    -> green
				// 800 - 1000   -> yellow
				// 1000+        -> red
				if (co2_value >= 1000) {
					LOG1("Red color\n");
					pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // red color
					// pixels.setBrightness(BRIGHTNESS_DEFAULT);
					pixels.setBrightness(neopixelAutoBrightness());
					pixels.show();
				} else if (co2_value >= 800) {
					LOG1("Yellow color\n");
					pixels.setPixelColor(0, pixels.Color(255, 127, 0)); // orange color
					// pixels.setBrightness(BRIGHTNESS_DEFAULT);
					pixels.setBrightness(neopixelAutoBrightness());
					pixels.show();
				} else if (co2_value >= 400) {
					LOG1("Green color\n");
					pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // green color
					// pixels.setBrightness(BRIGHTNESS_DEFAULT);
					pixels.setBrightness(neopixelAutoBrightness());
					pixels.show();
				}

				// Update peak value
				if (co2_value > co2PeakLevel->getVal()) {
					co2PeakLevel->setVal(co2_value);
				}

				// Trigger HomeKit sensor when concentration reaches this level
				if (co2_value > HOMEKIT_CO2_TRIGGER) {
					co2Detected->setVal(true);
				} else {
					co2Detected->setVal(false);
				}
			}
		}

		if (co2Level->timeVal() > 12 * 60 * 60 * 1000) {
			co2PeakLevel->setVal(400);
		}
	}
};

struct DEV_AirQualitySensor : Service::AirQualitySensor { // A standalone Air Quality sensor

	// An Air Quality Sensor is similar to a Temperature Sensor except that it supports a wide variety of measurements.
	// We will use three of them.  The first is required, the second two are optional.

	SpanCharacteristic *airQuality; // reference to the Air Quality Characteristic, which is an integer from 0 to 5
	SpanCharacteristic *pm25;
	SpanCharacteristic *airQualityActive;

	DEV_AirQualitySensor() : Service::AirQualitySensor() { // constructor() method

		airQuality		 = new Characteristic::AirQuality(1); // instantiate the Air Quality Characteristic and set initial value to 1
		pm25			 = new Characteristic::PM25Density(0);
		airQualityActive = new Characteristic::StatusActive(false);

		Serial.print("Configuring Air Quality Sensor"); // initialization message
		Serial.print("\n");

		SerialCom::setup();

		mySensor_air.begin(SMOOTHED_AVERAGE, 4); // SMOOTHED_AVERAGE, SMOOTHED_EXPONENTIAL options

	} // end constructor

	void loop() {

		if (pm25->timeVal() > INTERVAL * 1000) { // modify the Air Quality Characteristic every 5 seconds

			SerialCom::handleUart(state);

			if (state.valid) {

				if (!airQualityAct) {
					airQualityActive->setVal(true);
					airQualityAct = true;
				}

				mySensor_air.add(state.avgPM25);

				pm25->setVal(mySensor_air.get());

				int airQualityVal = 0;

				// Set Air Quality level based on PM2.5 value
				if (state.avgPM25 >= 150) {
					airQualityVal = 5;
				} else if (state.avgPM25 >= 55) {
					airQualityVal = 4;
				} else if (state.avgPM25 >= 35) {
					airQualityVal = 3;
				} else if (state.avgPM25 >= 12) {
					airQualityVal = 2;
				} else if (state.avgPM25 >= 0) {
					airQualityVal = 1;
				}
				airQuality->setVal(airQualityVal);
			}
		}

	} // loop
};

// HELPER FUNCTIONS

void detect_mhz() {
	// Detect sensor
	Serial.println("Detecting MH-Z19B");
	while (!mhz19b.detect()) {
		pixels.setPixelColor(0, pixels.Color(255, 0, 0));
		pixels.setBrightness(BRIGHTNESS_DEFAULT);
		delay(2.5 * 1000);
		pixels.setPixelColor(0, pixels.Color(0, 0, 0));
	};
	Serial.println("Sensor detected!");
}

// Fade in to pre-defined color
void fadeIn(int pixel, int r, int g, int b, double duration) {
	int brightness = neopixelAutoBrightness();
	for (int i = 0; i < brightness; i++) {
		pixels.setPixelColor(pixel, pixels.Color(r, g, b));
		pixels.setBrightness(i);
		pixels.show();
		delay(duration / brightness);
	}
}

// Fade in to pre-defined color
void fadeIn(int pixel, int r, int g, int b, int brightnessOverride, double duration) {
	int brightness = brightnessOverride;
	for (int i = 0; i < brightness; i++) {
		pixels.setPixelColor(pixel, pixels.Color(r, g, b));
		pixels.setBrightness(i);
		pixels.show();
		delay(duration / brightness);
	}
}

// Fade out from pre-defined color
void fadeOut(int pixel, int r, int g, int b, double duration) {
	int currentBrightness = pixels.getBrightness();
	for (int i = 0; i < currentBrightness; i++) {
		pixels.setPixelColor(pixel, pixels.Color(r, g, b));
		pixels.setBrightness(currentBrightness - i);
		pixels.show();
		delay(duration / currentBrightness);
	}
	pixels.setPixelColor(pixel, pixels.Color(0, 0, 0));
	pixels.show();
}

void initAnimation() {
	int duration   = 1000;
	int brightness = BRIGHTNESS_DEFAULT;
	// green
	fadeIn(0, 0, 255, 0, brightness, duration);
	fadeOut(0, 0, 255, 0, duration);

	// yellow
	fadeIn(0, 255, 165, 0, brightness, duration);
	fadeOut(0, 255, 165, 0, duration);

	// red
	fadeIn(0, 255, 0, 0, brightness, duration);
	fadeOut(0, 255, 0, 0, duration);

	// yellow
	fadeIn(0, 255, 165, 0, brightness, duration);
	fadeOut(0, 255, 165, 0, duration);

	// green
	fadeIn(0, 0, 255, 0, brightness, duration);
	fadeOut(0, 0, 255, 0, duration);
}

// Function for setting brightness based on light sensor values
int neopixelAutoBrightness() {
	double sensorValue = analogRead(ANALOG_PIN);
	// print the readings in the Serial Monitor
	Serial.println(sensorValue);

	if (sensorValue < BRIGHTNESS_THRESHOLD) {
		return BRIGHTNESS_DEFAULT;
	} else {
		return BRIGHTNESS_MAX;
	}
}