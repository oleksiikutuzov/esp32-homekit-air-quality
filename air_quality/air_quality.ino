/*********************************************************************************
 *  MIT License
 *
 *  Copyright (c) 2020 Gregg E. Berman
 *
 *  https://github.com/HomeSpan/HomeSpan
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 ********************************************************************************/

/*
			 ESP-WROOM-32 Utilized pins
			  ╔═════════════════════════════╗
			  ║┌─┬─┐  ┌──┐  ┌─┐				║
			  ║│ | └──┘  └──┘ |				║
			  ║│ |            |				║
			  ╠═════════════════════════════╣
		  +++ ║GND         				 GND║ +++
		  +++ ║3.3V      			    IO23║ USED_FOR_NOTHING
			  ║     			    	IO22║
			  ║IO36    				     IO1║ TX
			  ║IO39   				     IO3║ RX
			  ║IO34      	  		    IO21║
			  ║IO35     			    	║ NC
			  ║IO32     				IO19║ MHZ TX
			  ║IO33     			    IO18║ MHZ RX
 LIGHT_SENSOR ║IO25      			     IO5║
			  ║IO26     			    IO17║
			  ║IO27     			    IO16║
 VINDRIKTNING ║IO14    				     IO4║
			  ║IO12      		  	     IO0║ +++, BUTTON
			  ╚═════════════════════════════╝

*/

#include <HomeSpan.h>
#include "DEV_Identify.h"
#include "DEV_Sensors.h"
#include <ErriezMHZ19B.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "SerialCom.h"
#include "Types.h"
#include <WebServer.h>

WebServer webServer(80);

DEV_CO2Sensor		  *CO2; // GLOBAL POINTER TO STORE SERVICE
DEV_AirQualitySensor *AQI; // GLOBAL POINTER TO STORE SERVICE

void setup() {

	Serial.begin(115200);

	homeSpan.setLogLevel(1);
	homeSpan.setPortNum(1201);			// change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setMaxConnections(6);		// reduce max connection to 5 (default is 8) since WebServer and a connecting client will need 2
	homeSpan.setWifiCallback(setupWeb); // need to start Web Server after WiFi is established

	homeSpan.begin(Category::Bridges, "HomeSpan Bridge");

	new SpanAccessory();
	new DEV_Identify("Bridge #1", "HomeSpan", "123-ABC", "HS Bridge", "0.9", 3);
	new Service::HAPProtocolInformation();
	new Characteristic::Version("1.1.0");

	new SpanAccessory();
	new DEV_Identify("Carbon Dioxide Sensor", "HomeSpan", "123-ABC", "Sensor", "0.9", 0);
	CO2 = new DEV_CO2Sensor(); // Create a CO2 Sensor (see DEV_Sensors.h for definition)

	new SpanAccessory();
	new DEV_Identify("Air Quality", "HomeSpan", "123-ABC", "Sensor", "0.9", 0);
	AQI = new DEV_AirQualitySensor(); // Create an Air Quality Sensor (see DEV_Sensors.h for definition)
}

void loop() {

	homeSpan.poll();
	webServer.handleClient(); // need to process webServer once each loop
}

void setupWeb() {
	Serial.print("Starting Light Server Hub...\n\n");
	webServer.begin();

	// Create web routines inline

	webServer.on("/metrics", []() {
		double airQuality = AQI->pm25->getVal();
		double co2		  = CO2->co2Level->getVal();
		if (co2 > 399) { // exclude when it is still warming up
			// double lightness		= neopixelAutoBrightness();
			String airQualityMetric = "# HELP air_quality PM2.5 Density\nair_quality{device=\"air_sensor\",location=\"home\"} " + String(airQuality);
			String CO2Metric		= "# HELP co2 Carbon Dioxide\ncarbon_dioxide{device=\"air_sensor\",location=\"home\"} " + String(co2);
			// String uptimeMetric		= "# HELP uptime Sensor uptime\nuptime{device=\"air_sensor\",location=\"home\"} " + String(uptime);
			// String lightnessMetric	= "# HELP lightness Lightness\nlightness{device=\"air_sensor\",location=\"home\"} " + String(lightness);
			Serial.println(airQualityMetric);
			Serial.println(CO2Metric);
			// Serial.println(uptimeMetric);
			// Serial.println(lightnessMetric);

			webServer.send(200, "text/plain", airQualityMetric + "\n" + CO2Metric);
		}
	});

	webServer.on("/reboot", []() {
		String content = "<html><body>Rebooting!  Will return to configuration page in 10 seconds.<br><br>";
		content += "<meta http-equiv = \"refresh\" content = \"10; url = /\" />";
		webServer.send(200, "text/html", content);

		ESP.restart();
	});

} // setupWeb
