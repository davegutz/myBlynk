/**
 * @file       BlynkSimpleParticle.h
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Mar 2015
 * @brief
 *
 */

#ifndef BlynkSimpleParticle_h
#define BlynkSimpleParticle_h

#include "BlynkParticle.h"

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_BLYNK)
  static BlynkTransportParticle _blynkTransport;
  BlynkParticle Blynk(_blynkTransport);
#else
  extern BlynkParticle Blynk;
#endif

#include "BlynkWidgets.h"

#endif
