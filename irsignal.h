//
// irsignal.h
//
// Copyright (c) 2017 tako2
//

#ifndef _IRSIGNAL_H
#define _IRSIGNAL_H

#include <string>
#include <vector>
#include <map>

#define IRLEDPIN 14
#define IRPIN 5

using namespace std;

class IRSignal {
 public:
  long m_Carrier;

  IRSignal() {}
  ~IRSignal() {}

  void set_carrier(long carrier)
  {
    m_Carrier = carrier;
  }

  void send(vector<long> *waveform);
  vector<long> *recv();

 private:
  int m_CarrH;
  int m_CarrL;
  void send_waveform(vector<long> *waveform);
  vector<long> *recv_waveform();
};

#endif /* !_IRSIGNAL_H */
