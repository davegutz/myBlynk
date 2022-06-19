//
// MIT License
//
// Copyright (C) 2021 - Dave Gutz
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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "myCloud.h"
#include "command.h"
#include "constants.h"


#include <math.h>

// #include "Blynk/BlynkSimpleSerialBLE.h"
// #define BLYNK_PRINT Serial

extern uint8_t debug;
extern CommandPars cp;            // Various parameters to be common at system level (reset on PLC reset)

/* dag 6/18/2022
// extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;     // Time Blynk events
// extern BlynkStream Blynk;       // Blynk object

// Publish1 Blynk
void publish1(void)
{
  if (debug>104) Serial.printf("Blynk write1\n");
  Blynk.virtualWrite(V2,  pp.pubList.Vbatt);
  Blynk.virtualWrite(V3,  pp.pubList.Voc);
  Blynk.virtualWrite(V4,  pp.pubList.Vbatt);
}


// Publish2 Blynk
void publish2(void)
{
  if (debug>104) Serial.printf("Blynk write2\n");
  Blynk.virtualWrite(V6,  pp.pubList.soc);
  Blynk.virtualWrite(V8,  pp.pubList.T);
  Blynk.virtualWrite(V10, pp.pubList.Tbatt);
}


// Publish3 Blynk
void publish3(void)
{
  if (debug>104) Serial.printf("Blynk write3\n");
  Blynk.virtualWrite(V15, pp.pubList.hm_string);
  Blynk.virtualWrite(V16, pp.pubList.tcharge);
}


// Publish4 Blynk
void publish4(void)
{
  if (debug>104) Serial.printf("Blynk write4\n");
  Blynk.virtualWrite(V18, pp.pubList.Ibatt);
  Blynk.virtualWrite(V20, pp.pubList.Wbatt);
  Blynk.virtualWrite(V21, pp.pubList.soc_ekf);
}


// Attach a Slider widget to the Virtual pin 4 IN in your Blynk app
// - and control the web desired temperature.
// Note:  there are separate virtual IN and OUT in Blynk.
BLYNK_WRITE(V4) {
    if (param.asInt() > 0)
    {
        //pubList.webDmd = param.asDouble();
    }
}


// Attach a switch widget to the Virtual pin 6 in your Blynk app - and demand continuous web control
// Note:  there are separate virtual IN and OUT in Blynk.
BLYNK_WRITE(V6) {
//    pubList.webHold = param.asInt();
}
*/

// Assignments
void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string, const int num_timeouts)
{
  pubList->now = now;
  pubList->unit = unit;
  pubList->hm_string =hm_string;
  pubList->control_time = Sen->control_time;
  pubList->Vbatt = Sen->Vbatt;
  pubList->Tbatt = Sen->Tbatt;
  pubList->Tbatt_filt = Sen->Tbatt_filt;
  pubList->Vshunt = Sen->Vshunt;
  pubList->Ibatt = Sen->Ibatt;
  pubList->Wbatt = Sen->Wbatt;
  pubList->num_timeouts = num_timeouts;
  pubList->T = Sen->T;
  if ( debug==-13 ) Serial.printf("Sen->T=%6.3f\n", Sen->T);
  pubList->tcharge = Mon->tcharge();
  pubList->Voc = Mon->Voc();
  pubList->Voc_filt = Mon->Voc_filt();
  pubList->Vsat = Mon->Vsat();
  pubList->sat = Mon->sat();
  pubList->soc_model = Sen->Sim->soc();
  pubList->soc_ekf = Mon->soc_ekf();
  pubList->soc = Mon->soc();
  pubList->soc_wt = Mon->soc_wt();
  pubList->Amp_hrs_remaining_ekf = Mon->Amp_hrs_remaining_ekf();
  pubList->Amp_hrs_remaining_wt = Mon->Amp_hrs_remaining_wt();
  pubList->Vdyn = Mon->Vdyn();
  pubList->Voc_ekf = Mon->Hx();
  pubList->y_ekf = Mon->y_ekf();
}

// Text headers
void print_serial_header(void)
{
  if ( debug==4 || debug==24 )
    Serial.printf("unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,Vdyn,Voc,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,\n");
}
void print_serial_sim_header(void)
{
  if ( debug==24 )
    Serial.printf("unit_m,  c_time,       Tb_m,Tbl_m,  vsat_m, voc_m, vdyn_m, vb_m, ib_m, sat_m, ddq_m, dq_m, q_m, qcap_m, soc_m, reset_m,\n");
}

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(&pp.pubList);
  if ( debug >= 100 ) Serial.printf("serial_print:");
  Serial.println(cp.buffer);
}

// For summary prints
String time_long_2_str(const unsigned long current_time, char *tempStr)
{
    uint32_t year = Time.year(current_time);
    uint8_t month = Time.month(current_time);
    uint8_t day = Time.day(current_time);
    uint8_t hours = Time.hour(current_time);

    // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
    if ( USE_DST)
    {
      uint8_t dayOfWeek = Time.weekday(current_time);     // 1-7
      if (  month>2   && month<12 &&
        !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
        !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
        {
          Time.zone(GMT+1);
          day = Time.day(current_time);
          hours = Time.hour(current_time);
        }
    }
        uint8_t dayOfWeek = Time.weekday(current_time)-1;  // 0-6
        uint8_t minutes   = Time.minute(current_time);
        uint8_t seconds   = Time.second(current_time);
        if ( debug>105 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    sprintf(tempStr, "%4u-%02u-%02uT%02u:%02u:%02u", int(year), month, day, hours, minutes, seconds);
    return ( String(tempStr) );
}
