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

//------------------------------------------------------------
// This object parses received characters
//   into the gps.fix() data structure

static NMEAGPS gps;

//------------------------------------------------------------
//  Define a set of GPS fix information.  It will
//  hold on to the various pieces as they are received from
//  an RMC sentence.  It can be used anywhere in your sketch.

static gps_fix fix;

// Initialise LED Fire Effect pins

/*
int ledPin1 = 10;
int ledPin2 = 6;
int ledPin3 = 11;
int ledPin4 = 5;
*/

int ledFire[] = {5,6,10,11};
int ledCount = 4;

// Initialise outdoor lighting

int daytimeLights = 13;

// set sunset and sunrise hours (int, 0 - 23)

int sunrise = 7;
int sunset = 18;

// light status variable - used by a few functions
bool lightStatus = false;

// should lights be turned on or off.
bool lightsOn;

// Don't think we're using this; belive pins 2+3 are default tx,rx
//AltSoftSerial gpsPort; // GPS TX to pin 8, GPS RX to pin 9

static void flickerCandles () 
{
  /*analogWrite(ledPin1, random(120)+135);
  analogWrite(ledPin2, random(120)+135);
  analogWrite(ledPin3, random(120)+135);
  analogWrite(ledPin4, random(120)+135);*/
  for (int i=0; i<ledCount; i++) {
    analogWrite(ledFire[i], random(120)+135);
    DEBUG_PORT.print( F("I just flickered Pin") );
    DEBUG_PORT.println(ledFire[i]);
  }
  DEBUG_PORT.print( F("I just finished the flicker loop") );
  // do this more elegantly. Function pointer in a for loop for each pin?
}

static void switchDaytimeLight (bool lightsOn)
{
  if (lightsOn == true) {
    //turn on transistor lights
    digitalWrite(daytimeLights, HIGH); // pull pin high to power transistor
    lightStatus = true;
    DEBUG_PORT.print( F("Transistor Lights: On") );
  }
  if (lightsOn == false) {
    //turn off transistor lights
    digitalWrite(daytimeLights, LOW); // pull pin low to cut power to transistor circuit
    lightStatus = false;
    DEBUG_PORT.print( F("Transistor Lights: Off") );
  }
}

static void killCandles ()
{
  for (int i=0; i<ledCount; i++) {
    digitalWrite(ledFire[i], LOW); // pull pin low to kill power to LED.
    DEBUG_PORT.print( F("I just killed power to") );
    DEBUG_PORT.println(ledFire[i]);
 }
}

static void doLights ()
{
  uint8_t currentHour = 2; //THIS IS FOR DEBUGGING. Remove to get time from GPS.
  if ( currentHour < sunrise || currentHour >= sunset ) {
    flickerCandles();
    if ( lightStatus == false ) {
      lightsOn = true;
      switchDaytimeLight(lightsOn);
      DEBUG_PORT.println( F("Turning Lights On") );
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
      DEBUG_PORT.print( F("Lights: Off") );
    }
    else {
      DEBUG_PORT.print( F("Lights already off") );
    }
  }
}

//----------------------------------------------------------------
//  This function gets called about once per second, during the GPS
//  quiet time.  It's the best place to do anything that might take
//  a while: print a bunch of things, write to SD, send an SMS, etc.
//
//  By doing the "hard" work during the quiet time, the CPU can get back to
//  reading the GPS chars as they come in, so that no chars are lost.

static void doSomeWork()
{
  //uint8_t currentHour = fix.dateTime.hours;
  doLights(); // this is called once per second and currently only flickers candles once in that time.
  trace_all( DEBUG_PORT, gps, fix );

} // doSomeWork

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
  //configure candle flicker pins
  /*
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);
  */
  for (int i=0; i<ledCount; i++) {
    pinMode(ledFire[i], OUTPUT);
    DEBUG_PORT.print( F("Set pin to OUTPUT:") );
    DEBUG_PORT.println(ledFire[i]);
  }
  
  pinMode(daytimeLights, OUTPUT);

  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

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
