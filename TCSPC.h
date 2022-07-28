/*
  TCSPC.h - Library Time-Correlated Single Photon Counter with Arduino.
  Created by Allison Pessoa, 2019.
*/
#ifndef TCSPC_h
#define TCSPC_h

#include "Arduino.h"

class TCSPC
{
  public:
    TCSPC(int tc_channel);
    void start_counter();
    void get_counts();
};

#endif
