#include <HomeSpan.h>
#include <SoftwareSerial.h>
#include <ErriezMHZ19B.h>

#define MHZ19B_TX_PIN		19
#define MHZ19B_RX_PIN		18
#define INTERVAL			5	 // in seconds
#define HOMEKIT_CO2_TRIGGER 1350 // co2 level, at which HomeKit alarm will be triggered

bool needToWarmUp = true;
int	 tick		  = 0;

// Declare functions
void detect_mhz();
// void warm_mhz();

////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

// Use software serial
SoftwareSerial mhzSerial(MHZ19B_TX_PIN, MHZ19B_RX_PIN);

// Declare MHZ19B object
ErriezMHZ19B mhz19b(&mhzSerial);

struct DEV_CO2Sensor : Service::CarbonDioxideSensor { // A standalone Temperature sensor

	SpanCharacteristic *co2Detected;
	SpanCharacteristic *co2Level;
	SpanCharacteristic *co2PeakLevel;
	SpanCharacteristic *co2StatusActive;

	DEV_CO2Sensor() : Service::CarbonDioxideSensor() { // constructor() method

		co2Detected		= new Characteristic::CarbonDioxideDetected(false);
		co2Level		= new Characteristic::CarbonDioxideLevel(400);
		co2PeakLevel	= new Characteristic::CarbonDioxidePeakLevel(400);
		co2StatusActive = new Characteristic::StatusActive(false);

		Serial.print("Configuring Carbon Dioxide Sensor\n"); // initialization message

		mhzSerial.begin(9600);

		detect_mhz();
	}

	void loop() {

		if (co2StatusActive->timeVal() > 5 * 1000 && needToWarmUp) {
			// Serial.println("Need to warm up");

			if (mhz19b.isWarmingUp()) {
				Serial.println("Warming up");
				// neopixelAutoBrightness();
				// fadeIn(0, 255, 165, 0, 100, 2500);
				// fadeOut(0, 255, 165, 0, 2500);
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

			if (co2_value > 0) {

				co2Level->setVal(co2_value); // set the new co value; this generates an Event Notification and also resets the elapsed time

				LOG1("Carbon Dioxide Update: ");
				LOG1((int)co2Level);
				LOG1("\n");

				// Set color indicator
				// 400 - 800    -> green
				// 800 - 1000   -> yellow
				// 1000+        -> red
				if (co2_value >= 1000) {
					LOG1("Red color\n");
					// pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // red color
					// pixels.setBrightness(neopixelAutoBrightness());
					// pixels.show();
				} else if (co2_value >= 800) {
					LOG1("Yellow color\n");
					// pixels.setPixelColor(0, pixels.Color(255, 127, 0)); // orange color
					// pixels.setBrightness(neopixelAutoBrightness());
					// pixels.show();
				} else if (co2_value >= 400) {
					LOG1("Green color\n");
					// pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // green color
					// pixels.setBrightness(neopixelAutoBrightness());
					// pixels.show();
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

		// TODO reset peak value
	}
};

struct DEV_AirQualitySensor : Service::AirQualitySensor { // A standalone Air Quality sensor

	// An Air Quality Sensor is similar to a Temperature Sensor except that it supports a wide variety of measurements.
	// We will use three of them.  The first is required, the second two are optional.

	SpanCharacteristic *airQuality; // reference to the Air Quality Characteristic, which is an integer from 0 to 5
	SpanCharacteristic *pm25;

	DEV_AirQualitySensor() : Service::AirQualitySensor() { // constructor() method

		airQuality = new Characteristic::AirQuality(1); // instantiate the Air Quality Characteristic and set initial value to 1
		pm25	   = new Characteristic::PM25Density(0);

		Serial.print("Configuring Air Quality Sensor"); // initialization message
		Serial.print("\n");

	} // end constructor

	void loop() {

		if (airQuality->timeVal() > 5000)						// modify the Air Quality Characteristic every 5 seconds
			airQuality->setVal((airQuality->getVal() + 1) % 6); // simulate a change in Air Quality by incrementing the current value by one, and keeping in range 0-5

	} // loop
};

// HELPER FUNCTIONS

void detect_mhz() {
	// Detect sensor
	Serial.println("Detecting MH-Z19B");
	while (!mhz19b.detect()) {
		// neopixelAutoBrightness();
		// fadeIn(0, 255, 0, 0, 100, 1000);
		// fadeOut(0, 255, 0, 0, 1000);
	};
	Serial.println("Sensor detected!");
}

// void warm_mhz() {
// 	// Print a value each 5 seconds
// 	// during 3-minute warmup (for debug)
// 	Serial.println("Warming up");
// 	if (mhz19b.isWarmingUp()) {
// 		// neopixelAutoBrightness();
// 		// fadeIn(0, 255, 165, 0, 100, 2500);
// 		// fadeOut(0, 255, 165, 0, 2500);
// 		tick = tick + 5;
// 		Serial.println((String)tick + " ");
// 		co2StatusActive->setVal(false);
// 	} else {
// 		needToWarmUp = false;
// 		co2StatusActive->setVal(true);
// 	}
// 	// Serial.println("Warmed up!");
// }