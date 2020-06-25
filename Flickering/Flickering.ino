/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Copyright 2020 Matt & Katy Jolly 
*/

/*
 * Flickering v0.2
 * Arduino sketch to control the the lights for a book nook. 
 * Provides a configurable number of PWM outputs with a 'flickering' effect for candles burning after sunset
 * and 'daylight' lights (to stream light through windows) controlled via a transistor attached to a digital output.
 * 
 * Time information is provided by NeoGPS using AltSoftSerial on pins 8 (gps TX) and 9 (gps RX); stored in the gps fix.
 * all light control functions are performed in the time between NMEA sentences (approx 1 second).
 *  
*/

#include <NMEAGPS.h>
#include <AltSoftSerial.h>
#include <GPSport.h>
#include <time.h>

//------------------------------------------------------------
// For the NeoGPS example programs, "Streamers" is common set
//   of printing and formatting routines for GPS data, in a
//   Comma-Separated Values text format (aka CSV).  The CSV
//   data will be printed to the "debug output device".
// If you don't need these formatters, simply delete this section.

#include <Streamers.h>

/*
 * Create an object to parse received characters 
 * into the gps.fix() data structure
*/

static NMEAGPS gps;

/*
 * Define a set of GPS fix information to hold things as they're received from an RMC sentence.
 * Call this anywhere in your sketch.
*/

static gps_fix fix;

/* 
 *  Initialise LED Fire Effect pins
 *  Additional pins may be added to the ledFire array to be initialised and handled automatically - ensure that only PWM pins are used.
 *  The ledCount must be equal to the number of pins that the Fire effect is to run on.
*/

int ledFire[] = {5,6,10,11};
int ledCount = 4;

// Initialise outdoor lighting (circuit controlled via transistor; ON = HIGH/LOW???)

int daytimeLights = 12;

// set sunset and sunrise hours (int, 0 - 23)
// note that this needs to be in GMT!

int sunrise = 17;
int sunset = 8;

// Light status variable - set when lights (transistor) are turned on or off;
// functions check thet status of this variable before they change the light status - no need to change the pin value every second.

bool lightStatus = false;

// should lights be turned on or off by a function

bool lightsOn;

static void flickerCandles () 
{
  for (int i=0; i<ledCount; i++) {
    analogWrite(ledFire[i], random(120)+135); // PWM value: 0 (off) - 255 (on)
    DEBUG_PORT.print( F("I just flickered Pin ") );
    DEBUG_PORT.println(ledFire[i]);
  }
  DEBUG_PORT.println( F("I just finished the flicker loop") );
}

static void switchDaytimeLight (bool lightsOn)
{
  if (lightsOn == true) {
    //turn on transistor lights
    digitalWrite(daytimeLights, HIGH); // pull pin high to power transistor
    lightStatus = true;
    DEBUG_PORT.println( F("Transistor Lights: On") );
  }
  if (lightsOn == false) {
    //turn off transistor lights
    digitalWrite(daytimeLights, LOW); // pull pin low to cut power to transistor circuit
    lightStatus = false;
    DEBUG_PORT.println( F("Transistor Lights: Off") );
  }
}

static void killCandles ()
{
  for (int i=0; i<ledCount; i++) {
    digitalWrite(ledFire[i], LOW); // pull pin low to kill power to LED.
    DEBUG_PORT.print( F("I just killed power to ") );
    DEBUG_PORT.println(ledFire[i]);
 }
}

static void doLights (uint8_t currentHour)
{
  // currentHour = 2; // THIS IS FOR DEBUGGING - will override the time passed to the function. Comment out to use the time from GPS fix.
  if ( currentHour < sunrise || currentHour >= sunset ) {
    flickerCandles(); // only called once per second/loop. May need to increase the rate for a more convincing flicker effect.
    if ( lightStatus == false ) {
      lightsOn = true;
      DEBUG_PORT.println( F("Lights: On") );
      switchDaytimeLight(lightsOn);
    }
    else {
      DEBUG_PORT.println( F("Lights already on") );
    }
  }
  else {
    if ( lightStatus == true ) {
      lightsOn = false;
      killCandles();
      switchDaytimeLight (lightsOn);
      DEBUG_PORT.println( F("Lights: Off") );
    }
    else {
      DEBUG_PORT.println( F("Lights already off") );
    }
  }
}

/*
 * This function gets called about once per second, during the GPS
 * quiet time.  It's the best place to do anything that might take
 * a while: print a bunch of things, write to SD, send an SMS, etc.
 * 
 * By doing the "hard" work during the quiet time, the CPU can get back to
 * reading the GPS chars as they come in, so that no chars are lost.
*/

static void doSomeWork()
{
  uint8_t currentHour = fix.dateTime.hours; // Pull the hour out of the new GPS fix.
  doLights(currentHour); // Do the light stuff.
  trace_all( DEBUG_PORT, gps, fix ); // Output GPS Fix info.

} 

//------------------------------------
//  This is the main GPS parsing loop.

static void GPSloop()
{
  while (gps.available( gpsPort )) {
    fix = gps.read();
    doSomeWork();
  }

} // GPSloop

void setup()
{
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  for (int i=0; i<ledCount; i++) { // Configure Fire Effect pins.
    pinMode(ledFire[i], OUTPUT);
    DEBUG_PORT.print( F("Set pin to OUTPUT: ") );
    DEBUG_PORT.println(ledFire[i]);
  }
  
  pinMode(daytimeLights, OUTPUT); // Configure transistor lights pin.
  DEBUG_PORT.print( F("Set pin to OUTPUT: ") );
  DEBUG_PORT.println(daytimeLights);

  DEBUG_PORT.print( F("NMEA.INO: started\n") );
  DEBUG_PORT.print( F("  fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("  gps object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " GPS_PORT_NAME) );

  #ifndef NMEAGPS_RECOGNIZE_ALL
    #error You must define NMEAGPS_RECOGNIZE_ALL in NMEAGPS_cfg.h!
  #endif

  #ifdef NMEAGPS_INTERRUPT_PROCESSING
    #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
  #endif

  #if !defined( NMEAGPS_PARSE_GGA ) & !defined( NMEAGPS_PARSE_GLL ) & \
      !defined( NMEAGPS_PARSE_GSA ) & !defined( NMEAGPS_PARSE_GSV ) & \
      !defined( NMEAGPS_PARSE_RMC ) & !defined( NMEAGPS_PARSE_VTG ) & \
      !defined( NMEAGPS_PARSE_ZDA ) & !defined( NMEAGPS_PARSE_GST )

    DEBUG_PORT.println( F("\nWARNING: No NMEA sentences are enabled: no fix data will be displayed.") );

  #else
    if (gps.merging == NMEAGPS::NO_MERGING) {
      DEBUG_PORT.print  ( F("\nWARNING: displaying data from ") );
      DEBUG_PORT.print  ( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
      DEBUG_PORT.print  ( F(" sentences ONLY, and only if ") );
      DEBUG_PORT.print  ( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
      DEBUG_PORT.println( F(" is enabled.\n"
                            "  Other sentences may be parsed, but their data will not be displayed.") );
    }
  #endif

  DEBUG_PORT.print  ( F("\nGPS quiet time is assumed to begin after a ") );
  DEBUG_PORT.print  ( gps.string_for( LAST_SENTENCE_IN_INTERVAL ) );
  DEBUG_PORT.println( F(" sentence is received.\n"
                        "  You should confirm this with NMEAorder.ino\n") );

  trace_header( DEBUG_PORT );
  DEBUG_PORT.flush();

  gpsPort.begin( 9600 );
}

void loop() {
  GPSloop();
}
