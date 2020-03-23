// Adafruit IO Publish Example
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

int NA = 0; //nanoamps value
String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete

int last_millis = millis();

int count = 0;

// set up the 'counter' feed
AdafruitIO_Feed *counter = io.feed("counter");

void setup() {
  Serial.begin(9600);
  Serial.swap();
  Serial.println("Connecting to Adafruit IO");
  io.connect();  // connect to io.adafruit.com
  while (io.status() < AIO_CONNECTED) {
  Serial.print("."); delay(500);
  }
  Serial.println();
  Serial.println(io.statusText());
  //pinMode(D4,1);
}

void loop() {
  io.run();  // io.run(); is required for all sketches.

  if (stringComplete) {
    //digitalWrite(D4,0);
    Serial.print("dingdingding! "); //Serial.print("inputString:\"" + inputString); Serial.println("\"");
    inputString.replace("<NA>", ""); inputString.replace("</NA>", ""); inputString.trim();
    NA = inputString.toInt();
    //Serial.println(inputString + " " + (NA / 1.0) );
    inputString = ""; stringComplete = false;
  }

  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    Serial.print(inChar);
    if ( isDigit(inChar) ) {
      inputString += inChar;
    }
    if (inChar == '\n' ) {
      stringComplete = true;
    }
  }

  if ( millis() > last_millis + 3000 ) {
    last_millis = millis();
    // save count to the 'counter' feed on Adafruit IO
    //count =  random(10, 500);
    count = NA;
    //digitalWrite(D4,1);
    Serial.print("uplink:"); Serial.print(count); Serial.println(":");
    counter->save(count);
  }

}
