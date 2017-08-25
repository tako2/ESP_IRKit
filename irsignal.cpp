//
// irsignal.cpp
//
// Copyright (c) 2017 tako2
//

#include <Arduino.h>
#include "irsignal.h"

#define RECV_START_TIMEOUT 10000 // ms
#define RECV_WAVEFORM_TIME 1000000 // us
#define RECV_WAVEFORM_MAX 2000

//=============================================================================
// Send IR Waveform
//=============================================================================
void IRSignal::send_waveform(vector<long> *waveform)
{
  int pulse = HIGH;
  long time;
  long start_time;

  for (int idx = 0; idx < waveform->size(); idx ++) {
    time = waveform->at(idx);
    if (pulse == HIGH) {
      start_time = micros();
      while ((micros() - start_time) < time) {
	digitalWrite(IRLEDPIN, HIGH);
	delayMicroseconds(m_CarrH);
	digitalWrite(IRLEDPIN, LOW);
	delayMicroseconds(m_CarrL);
      }
      pulse = LOW;
    } else {
      digitalWrite(IRLEDPIN, LOW);
      delayMicroseconds(time);
      pulse = HIGH;
    }
  }
  digitalWrite(IRLEDPIN, LOW);
}

//=============================================================================
// Send IR Signal
//=============================================================================
void IRSignal::send(vector<long> *waveform)
{
  if (m_Carrier == 0 || waveform == NULL) {
    return;
  }

  // Calc 1/3 Duty
  m_CarrL = 1000000L / m_Carrier;
  m_CarrH = m_CarrL / 3;
  m_CarrL -= m_CarrH;

  if (m_CarrH <= 0 || m_CarrL <= 0) {
    return;
  }

  // Send Waveform
  send_waveform(waveform);
}

//=============================================================================
// Receive IR Waveform
//=============================================================================
vector<long> *IRSignal::recv_waveform()
{
  int last_pin = LOW;
  int cur_pin = HIGH;

  vector<long> *waveform = new vector<long>();
  int idx = 0;

  long start_time;
  long last_time, cur_time;
  start_time = last_time = micros();

  last_pin = HIGH;
  while ((micros() - start_time) < RECV_WAVEFORM_TIME
	 && idx < RECV_WAVEFORM_MAX) {
    cur_pin = digitalRead(IRPIN);
    if (last_pin != cur_pin) {
      cur_time = micros();
      waveform->push_back(cur_time - last_time);
      idx ++;
      last_time = cur_time;

      last_pin = cur_pin;
    }
  }

  return (waveform);
}

//=============================================================================
// Receive IR Signal
//=============================================================================
vector<long> *IRSignal::recv()
{
  vector<long> *waveform = NULL;
  int last_pin = LOW;
  int cur_pin = LOW;
  long timeout;

  // Wait 10 seconds
  timeout = millis() + RECV_START_TIMEOUT;

  last_pin = digitalRead(IRPIN);

  while (millis() < timeout) {
    cur_pin = digitalRead(IRPIN);
    if ((last_pin != cur_pin) && (cur_pin == HIGH)) {
      waveform = recv_waveform();
      return (waveform);
    }
    last_pin = cur_pin;
    delay(1);
  }

  return (NULL);
}
