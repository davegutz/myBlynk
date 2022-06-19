/*
 * Project Blynk_Photon
  * Description:
  * Test Blynk protocols
  * By:  Dave Gutz June 2022
  * 19-Jun-2022   Initial Git committ
  * 
//
// MIT License
//
// Copyright (C) 2022 - Dave Gutz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// See README.md
*/

// For Photon
#if (PLATFORM_ID==6)
  #define PHOTON
  //#define BOOT_CLEAN      // Use this to clear 'lockup' problems introduced during testing using Talk
  #include "application.h"  // Should not be needed if file ino or Arduino
  SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status
  #include <Arduino.h>      // Used instead of Print.h - breaks Serial
#else
  #undef PHOTON
  using namespace std;
  #undef max
  #undef min
#endif

#undef USE_BT             // Change this to #define to use Bluetooth
#define GMT                   -5        // Enter time different to zulu (does not respect DST) (-5)
uint8_t debug = 0;
#include "command.h"
#include "mySync.h"
CommandPars cp = CommandPars(); // Various control parameters commanding at system level
#define PUBLISH_BLYNK_DELAY   10000UL   // Blynk cloud updates, ms (10000UL = 10 sec)
#define PUBLISH_SERIAL_DELAY  400UL     // Serial print interval (400UL = 0.4 sec)
#define ONE_DAY_MILLIS        86400000UL// Number of milliseconds in one day (24*60*60*1000)
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip);
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);


#ifdef USE_BT
  // #define BLYNK_AUTH_TOKEN            "DU9igmWDh6RuwYh6QAI_fWsi-KPkb7Aa"
  #define BLYNK_AUTH_TOKEN  "DU9igmWDh6RuwYh6QAI_fWsi-KPkb7Aa"
  #define BLYNK_USE_DIRECT_CONNECT
  #define BLYNK_PRINT Serial
  #include "Blynk/BlynkSimpleSerialBLE.h"
  char auth[] = BLYNK_AUTH_TOKEN;
#endif

#ifdef USE_BT
  extern BlynkStream Blynk;       // Blynk object
  extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4; // Time Blynk events
  BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;        // Time Blynk events
#endif
extern PublishPars pp;            // For publishing

// Global locals
PublishPars pp = PublishPars();       // Common for publishing
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

int num_timeouts = 0;           // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";     // time, hh:mm

// Setup
void setup()
{
  // Serial
  Serial.begin(115200);
  Serial.flush();
  delay(1000);          // Ensures a clean display on Arduino Serial startup on CoolTerm
  Serial.println("Hi!");

  // Bluetooth Serial1
  Serial1.begin(9600);
  #ifdef USE_BT
    Blynk.begin(Serial1, auth);
  #endif

  // Cloud
  Time.zone(GMT);
  unsigned long now = millis();
  #ifdef USE_BT
    Serial.printf("Set up blynk...");
    blynk_timer_1.setInterval(PUBLISH_BLYNK_DELAY, publish1);
    blynk_timer_2.setTimeout(1*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_2.setInterval(PUBLISH_BLYNK_DELAY, publish2);});
    blynk_timer_3.setTimeout(2*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_3.setInterval(PUBLISH_BLYNK_DELAY, publish3);});
    blynk_timer_4.setTimeout(3*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_4.setInterval(PUBLISH_BLYNK_DELAY, publish4);});
  #endif
  Serial.printf("done CLOUD\n");

  #ifdef PHOTON
    if ( debug>101 ) { sprintf(cp.buffer, "Photon\n"); Serial.print(cp.buffer); }
  #else
    if ( debug>101 ) { sprintf(cp.buffer, "Mega2560\n"); Serial.print(cp.buffer); }
  #endif

  // Determine millis() at turn of Time.now
  long time_begin = Time.now();
  while ( Time.now()==time_begin )
  {
    delay(1);
    millis_flip = millis()%1000;
  }

} // setup


// Loop
void loop()
{
  // Synchronization
  boolean publishB;                           // Particle publish, T/F
  static Sync *PublishBlynk = new Sync(PUBLISH_BLYNK_DELAY);
  boolean publishS;                           // Serial print, T/F
  static Sync *PublishSerial = new Sync(PUBLISH_SERIAL_DELAY);
  static uint8_t last_read_debug = 0;         // Remember first time with new debug to print headers
  static uint8_t last_publishS_debug = 0;     // Remember first time with new debug to print headers
 
  unsigned long current_time;               // Time result
  static unsigned long now = millis();      // Keep track of time
  time32_t time_now;                        // Keep track of time
  static unsigned long start = millis();    // Keep track of time
  unsigned long elapsed = 0;                // Keep track of time
  static boolean reset = true;              // Dynamic reset
  static boolean reset_publish = true;      // Dynamic reset
  
  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////
  // Serial test
  #ifndef USE_BT
    if ( Serial1.available() ) Serial1.write(Serial1.read());
  #endif

  // Start Blynk
  #ifdef USE_BT
    Blynk.run();
    blynk_timer_1.run();
    blynk_timer_2.run();
    blynk_timer_3.run();
    blynk_timer_4.run(); 
  #endif

  // Keep time
  now = millis();
  time_now = Time.now();
  sync_time(now, &last_sync, &millis_flip);      // Refresh time synchronization
  char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
  double control_time = decimalTime(&current_time, tempStr, now, millis_flip);
  hm_string = String(tempStr);
  elapsed = ReadSensors->now() - start;
  publishB = PublishBlynk->update(millis(), false);           //  now || false
  publishS = PublishSerial->update(millis(), reset_publish);  //  now || reset_publish

  // Publish to Particle cloud if desired (different than Blynk)
  // Visit https://console.particle.io/events.   Click on "view events on a terminal"
  // to get a curl command to run
  if ( publishS )
  {
    assign_publist(&pp.pubList, PublishParticle->now(), unit, hm_string, Sen, num_timeouts, Mon);
 
    // Mon for debug
    if ( publishS )
    {
      if ( debug==4 || debug==24 )
      {
        if ( reset_publish || (last_publishS_debug != debug) )
        {
          print_serial_header();
          if ( debug==24 ) print_serial_sim_header();
        }
        serial_print(PublishSerial->now(), Sen->T);
      }

      if ( debug==-4 )
      {
        debug_m4(Mon, Sen);
      }
      last_publishS_debug = debug;
    }

  }

  // Initialize complete once sensors and models started and summary written
  if ( publishS ) reset_publish = false;

} // loop


#ifdef USE_BT
  // Publish1 Blynk
  void publish1(void)
  {
    if (debug==25) Serial.printf("Blynk write1\n");
    Blynk.virtualWrite(V2,  pp.pubList.Vbatt);
    Blynk.virtualWrite(V3,  pp.pubList.Voc);
    Blynk.virtualWrite(V4,  pp.pubList.Vbatt);
  }


  // Publish2 Blynk
  void publish2(void)
  {
    if (debug==25) Serial.printf("Blynk write2\n");
    Blynk.virtualWrite(V6,  pp.pubList.soc);
    Blynk.virtualWrite(V8,  pp.pubList.T);
    Blynk.virtualWrite(V10, pp.pubList.Tbatt);
  }


  // Publish3 Blynk
  void publish3(void)
  {
    if (debug==25) Serial.printf("Blynk write3\n");
    Blynk.virtualWrite(V15, pp.pubList.hm_string);
    Blynk.virtualWrite(V16, pp.pubList.tcharge);
  }


  // Publish4 Blynk
  void publish4(void)
  {
    if (debug==25) Serial.printf("Blynk write4\n");
    Blynk.virtualWrite(V18, pp.pubList.Ibatt);
    Blynk.virtualWrite(V20, pp.pubList.Wbatt);
    Blynk.virtualWrite(V21, pp.pubList.soc_ekf);
  }
#endif

// Time synchro for web information
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip)
{
  if (now - *last_sync > ONE_DAY_MILLIS) 
  {
    *last_sync = millis();

    // Request time synchronization from the Particle Cloud
    if ( Particle.connected() ) Particle.syncTime();

    // Refresh millis() at turn of Time.now
    long time_begin = Time.now();
    while ( Time.now()==time_begin )
    {
      delay(1);
      *millis_flip = millis()%1000;
    }
  }
}

// Convert time to decimal for easy lookup
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip)
{
  *current_time = Time.now();
  uint32_t year = Time.year(*current_time);
  uint8_t month = Time.month(*current_time);
  uint8_t day = Time.day(*current_time);
  uint8_t hours = Time.hour(*current_time);

  // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
  if ( USE_DST)
  {
    uint8_t dayOfWeek = Time.weekday(*current_time);     // 1-7
    if (  month>2   && month<12 &&
      !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
      !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
      {
        Time.zone(GMT+1);
        *current_time = Time.now();
        day = Time.day(*current_time);
        hours = Time.hour(*current_time);
      }
  }
  uint8_t dayOfWeek = Time.weekday(*current_time)-1;  // 0-6
  uint8_t minutes   = Time.minute(*current_time);
  uint8_t seconds   = Time.second(*current_time);

  // Convert the string
  time_long_2_str(*current_time, tempStr);

  // Convert the decimal
  if ( debug>105 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
  static double cTimeInit = ((( (double(year-2021)*12 + double(month))*30.4375 + double(day))*24.0 + double(hours))*60.0 + double(minutes))*60.0 + \
                      double(seconds) + double(now-millis_flip)/1000.;
  double cTime = cTimeInit + double(now-millis_flip)/1000.;
  return ( cTime );
}
