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
		  +++ ║3.3V      			    IO23║
			  ║     			    	IO22║
			  ║IO36    				     IO1║ TX
			  ║IO39   				     IO3║ RX
			  ║IO34      	  		    IO21║
			  ║IO35     			    	║ NC
			  ║IO32     				IO19║ MHZ TX
			  ║IO33     			    IO18║ MHZ RX
			  ║IO25      			     IO5║
			  ║IO26     			    IO17║
			  ║IO27     			    IO16║
			  ║IO14    				     IO4║
			  ║IO12      		  	     IO0║ +++, BUTTON
			  ╚═════════════════════════════╝

*/

#include <HomeSpan.h>
#include "DEV_Identify.h"
#include "DEV_Sensors.h"
#include <ErriezMHZ19B.h>
#include <SoftwareSerial.h>

void setup() {

	Serial.begin(115200);

	homeSpan.begin(Category::Bridges, "HomeSpan Bridge");

	new SpanAccessory();
	new DEV_Identify("Bridge #1", "HomeSpan", "123-ABC", "HS Bridge", "0.9", 3);
	new Service::HAPProtocolInformation();
	new Characteristic::Version("1.1.0");

	new SpanAccessory();
	new DEV_Identify("Carbon Dioxide Sensor", "HomeSpan", "123-ABC", "Sensor", "0.9", 0);
	new DEV_CO2Sensor(); // Create a CO2 Sensor (see DEV_Sensors.h for definition)

	new SpanAccessory();
	new DEV_Identify("Air Quality", "HomeSpan", "123-ABC", "Sensor", "0.9", 0);
	new DEV_AirQualitySensor(); // Create an Air Quality Sensor (see DEV_Sensors.h for definition)
}

void loop() {

	homeSpan.poll();
}
