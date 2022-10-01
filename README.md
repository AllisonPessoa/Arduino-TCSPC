# ArduinoDue-TCSPC
An Arduino Due Sketch to perform time-resolved measurements from single-photon detectors.  
This project performs the time-correlated pulse counting from a single-photon detector, with temporal resolution up to 1 microsecond, using an Arduino Due board. The single-photon detector can be an [APD](https://en.wikipedia.org/wiki/Avalanche_photodiode) or a [PMT](https://en.wikipedia.org/wiki/Photomultiplier_tube), for example. Depending on the chosen approach, a different signal conditioning circuit may be necessary to send the pulses to the Arduino with the correct specifications (3.3V LVTTL for Due boards).

In short, it works by counting the number of income pulses in a given time interval (often called integration time. Minimum is 1 microsecond). 

## Requirements
This sketch uses some libraries. They must be installed prior skecht upload.
1. TCSPC: Gathers the low-level functions configuring the microcontroller's registers. Files included in this page.
2. [CommandHandler](https://github.com/AllisonPessoa/CommandHandler): Makes an easily-configurable Command Line Interface.
3. [DueTimer](https://github.com/ivanseidel/DueTimer): Configures the Timers/Counters to perform synchronized interrupts.

## Installation
1. Download the latest release from GitHub.
2. Unzip the 'TCSPC' folder on your Arduino's Library folder.
3. Also install the DueTimer library ([here](https://github.com/ivanseidel/DueTimer))
4. Re-open the Arduino Software

## Basic Usage
Basic programming of the Counter Mode. It uses CommandHandler library to facilitate sending commands through Serial port (See Requirements).

```cpp
//Include the necessary libraries
#include <DueTimer.h>
#include <TCSPC.h>
#include <CommandHandler.h>

int cmd_counter_start();
int cmd_counter_get();

CommandType Commands[] = {
  {"CNT_START", &cmd_counter_start, "Counter Mode - Start counting. Arg1 (int): Integration time in microseconds"},
  {"CNT_GET", &cmd_counter_get, "Counter Mode - Return the last stored count value"},
};

CLI command_line(Commands, sizeof(Commands)/sizeof(Commands[0])); // Command Line Interface
TCSPC counter;

void setup(){
  Serial.begin(115200);
  command_line.begin(&Serial);
}

void loop(){
  command_line.start_processing();
}

volatile uint32_t prev_tc_cv; //Global variable to save the Counter register's values. They must be volatile.
volatile uint32_t diff_tc_cv;

int cmd_counter_start(){
  counter.start_counter();
  Timer1.attachInterrupt(handler_counter_mode); //DueTimer library to attach an interrupt on Timer1
  Timer1.setPeriod(atof(command_line.args[1]));
  Timer1.start();
}

void handler_counter_mode(){ //Every XX seconds (defined by the user), this function is called.
  uint32_t cur_tc_cv = counter.get_counts();
  diff_tc_cv = cur_tc_cv - prev_tc_cv;
  prev_tc_cv = cur_tc_cv;
}

int cmd_counter_get(){ //This function is called everytime 'CNT_GET' is called through the Serial, 
  Serial.println(diff_tc_cv);
}

```
When the user enters, for example, `CNT_START 100` trough the Serial, the microcontroller starts counting the incoming pulses on the Digital Pin 22 at each 100 microseconds and keep saving it in a gobal variable. When the user enters `CNT_GET`, the current value from this global value is returned through the Programming USB port. 

## Specifications
The program uses the Timer/Counter 0 (TC0) of the Due board working on the maximum speed (84 MHz). The 3.3V LVTTL signal from the photodetector must have a maximum frequency of 33.6 MHz $^{[1]}$; This means a minimum pulse width of ~15 ns. Multiple external triggers and phase-lockers can be set, for example, by using the other Timer/Counters in Due (there are 9 in total). The DueTimer library is used to this end. Furthermore, all digital pins on an Arduino Due board can be used to attach external interrupts$^{[2]}$, so that multiple external triggers can be set.  
[1] -> See [ATMEL SAM3X datasheet](https://ww1.microchip.com/downloads/en/devicedoc/atmel-11057-32-bit-cortex-m3-microcontroller-sam3x-sam3a_datasheet.pdf) page 860.  
[2] -> See [attachInterrupt()](https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/) function.

The sketch was designed...

## Communication improvements
The Arduino Due board has a NativeUSB port. It allows...

## Credits
Created by Allison Pessoa 2022 originally to perform luminescence lifetime measurements in single dielectric nanoparticles doped with lanthanide ions. See my master's dissertation here.
