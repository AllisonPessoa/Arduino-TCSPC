# Arduino-TCSPC

This Arduino's script brings time-resolved counting of LVTTL pulses from single-photon detectors by using an Arduino. The temporal resolution ranges from ~500ns to miliseconds. This project in particular uses an Arduino Due board because of its internal clock of 84 MHz.

* Specifications:

The detected signal should operate in a maximum frequency of 40 MHz, with a minimum duty cycle of 25 ns in order to not miss any incoming signal. The system is versatile in a way that the users can adpt it to their needs. There are provided two examples using the TCSPC: 1) BoxCar mode, to measure the luminescent lifetime of single particles through photon-counting; and 2) A plataform to perform magnetometry measurements with Nitrogen-Vacancy Centers in Nanodiamonds.

In counter mode the system performs the counting during a predefined integration time (or gate time), which is defined by the command 'CNT(gate_time)'.
'gate_time' is given in nanoseconds (but the minimum value is 0.2). 

In BoxCar mode the system performs similar counting, but only after
a trigger signal is recieved. After this trigger signal, the system can still wait a predefined time before start counting, which is 
called the 'trigger delay'. The command for activating this mode is 'BXC(gate_time, trigger_delay, N_buffer, N_rep)'. N_buffer refers
to the number of gates the system will integrate, and save separately, after being enabled to count (trigger + delay). N_rep refers the number of
times the routine is repeated. The time-resolved counts accumulates during the routine.

After the routines, the final outputs are sent through serial port (USB), which can be saved, or integrated with another program to be shown 
in real time. The informations are identified as following:
Counter -> ex. 'A123'
BoxCar  -> ex. 'B123'
Info    -> ex. 'r:blabla'

Furthermore, it possess some additional commands:
'*IDN?':        Returns information about this script
'PWMi(x,y)':    Activates a square-wave generator at Digital pin 7. Frequency adjustable.
'PWMii(x,y)':   Activates a square-wave generator at Digital pin 8. Frequency adjustable.

Pinning

Pulse Detector:               PINO DIGITAL 22

Chopper Trigger:              PINO DIGITAL 2
                              
Square-wave generator:        PINO DIGITAL 7
                              PINO DIGITAL 8
