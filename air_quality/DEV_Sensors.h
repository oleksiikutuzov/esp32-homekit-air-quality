#include "HomeSpan.h"
////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

struct DEV_CO2Sensor : Service::CarbonDioxideSensor { // A standalone Temperature sensor

	SpanCharacteristic *co2Detected;
	SpanCharacteristic *co2Level;
	SpanCharacteristic *co2PeakLevel;

	DEV_CO2Sensor() : Service::CarbonDioxideSensor() { // constructor() method

		co2Detected	 = new Characteristic::CarbonDioxideDetected(false);
		co2Level	 = new Characteristic::CarbonDioxideLevel(400);
		co2PeakLevel = new Characteristic::CarbonDioxidePeakLevel(400);

		Serial.print("Configuring Carbon Dioxide Sensor"); // initialization message
		Serial.print("\n");
	}

	void loop() {

		if (co2Level->timeVal() > 5000) {				// check time elapsed since last update and proceed only if greater than 5 seconds
			float co2 = co2Level->getVal<float>() + 50; // "simulate" a half-degree temperature change...
			if (co2 > 900)								// ...but cap the maximum at 35C before starting over at -30C
				co2 = 400;

			co2Level->setVal(co2); // set the new temperature; this generates an Event Notification and also resets the elapsed time

			LOG1("Temperature Update: ");
			LOG1((int)co2Level);
			LOG1("\n");
		}
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