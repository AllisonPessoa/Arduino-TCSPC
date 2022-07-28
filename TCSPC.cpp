/*
  TCSPC.h - Library Time-Correlated Single Photon Counter with Arduino.
  Created by Allison Pessoa.
*/

#include "Arduino.h"
#include "TCSPC.h"

////////////////////////////////////////
////////////// COUNTER /////////////////
////////////////////////////////////////

TCSPC::TCSPC(int tc_channel)
{
  switch (tc_channel) {
    case 0:
      volatile byte *PMC_CHANNEL = &PMC_PCER0;
      volatile byte *PMC_PORT = &PMC_PCER0_PID27;

      volatile byte *TC = &TC0;
      volatile byte *TC_CHANNEL = &TC_CHANNEL[0];
      volatile byte *TC_COUNTER = &TC_CMR_TCCLKS_XC0;
      volatile byte *TC_EXT_PORT = &TC_BMR_TC0XC0S_TCLK0;
      break;
  }

  PMC->PMC_CHANNEL = PMC_PORT;            // Time Counter - enables the voltage control and management
  TC->TC_CHANNEL.TC_CCR = TC_CCR_CLKDIS ; // Disable the timer for initializing
  TC->TC_CHANNEL.TC_CMR = TC_COUNTER;     // Selected clock counter: XC0 - EXTERNAL CLOCK SIGNAL
  TC->TC_BMR = TC_EXT_PORT;               // External port connected to TC0: TCLK0=PB26 (pg. 858) = Arduino_Due_Digital_Pin_22 https://www.arduino.cc/en/Hacking/PinMappingSAM3X
}

void TCSPC::start_counter(){
  unsigned long timeout = 84000000; // 1s max to the next clock cycle
  unsigned long k=0;
  unsigned long zero;
  // TC_CV only is set after the first external pulse. Wait for this happen. (See page 862, section 36.6.6 of SAM3X manual)
  TC->TC_CHANNEL.TC_CCR = TC_CCR_SWTRG      // sowftware trigger: reset counter, start clock
                        | TC_CCR_CLKEN;     // counter clock enabled
  zero = TC->TC_CHANNEL.TC_CV;
  while((TC->TC_CHANNEL.TC_CV == zero) && (k < timeout)){
    k++;
  }
}

uint32_t TCSPC::get_counts(){
  return TC->TC_CHANNEL.TC_CV;
}
