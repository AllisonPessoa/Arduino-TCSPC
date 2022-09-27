# ArduinoDue-TCSPC
An Arduino Due Sketch to perform time-resolved measurements from single-photon detectors.  
This project performs the time-correlated pulse counting from a single-photon detector, with temporal resolution up to 1 microsecond, using an Arduino Due board. The single-photon detector can be an [APD](https://en.wikipedia.org/wiki/Avalanche_photodiode) or a [PMT](https://en.wikipedia.org/wiki/Photomultiplier_tube), for example. Depending on the chosen approach, a different signal conditioning circuit may be necessary to send the pulses to the Arduino with the correct specifications (3.3V LVTTL).

This scketch uses an Arduino Due board due to its internal clock speed (84 MHz) and its 32-bit counter. In short, it works by counting the number of income pulses in a given interval (often called integration time. Minimum is 1 microsecond). This project encompasses a library (TCSPC) to wrap up the low-level functions handling the microcontroller registers. 

The program uses the Timer/Counter 0 (TC0) of the Due board working on the maximum speed (84 MHz). The 3.3V LVTTL signal from the photodetector must have a maximum frequency of 33.6 MHz $^{[1]}$; This means a minimum pulse width of ~15ns.
[1] -> See ATMEL SAM3X datasheet page 860.

Multiple external triggers and phase-lockers can be set.

## Installation
1. Download the latest release from GitHub.
2. Unzip the folder on your Arduino's Library folder.
3. Re-open the Arduino Software

## Basic Usage

## Credits
Created by Allison Pessoa 2022.
