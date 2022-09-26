/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                  UNIVERSIDADE FEDERAL DE PERNAMBUCO - PHYSICS DEPARTMENT - NANO OPTICS LABORATORY
                                      Arduino Due Time-Correlated Single Photon Counter
                                                      Allison Pessoa

This project performs the time-correlated pulse counting from a single-photon detector, with temporal resolution up to 1 microsecond.
This scketch uses an Arduino Due board due to its internal clock speed (84 MHz) and its 32-bit counter.

The 3.3V LVTTL signal from the photodetector must have a maximum frequency of 33.6 MHz[1]; This means a minimum pulse width of ~15ns.
May a signal conditioning circuit be necessary.
[1] -> See ATMEL SAM3X datasheet page 860.

One can use the several Timer/Counter Due channels (nine in total) to create trigger signals or to attach interrupts.
This skecth has two working modes, but it is not limited to it:
----- Counter Mode: 
      Command: 'CNT arg1'. arg1 is the counting time interval in microseconds.
      
      Uses Timer0 for continuously counting from the photodetector
      Uses Timer1 for synchronized interrupts at every arg1 microseconds to save data.

----- BoxCar Mode: 
      Command: 'BXC arg1 arg2 arg3'. 
                arg1 is the gate time interval in microseconds.
                arg2 is the length of the buffer used to save the data.
                arg3 is the number of repetitions of the routine.
      
      Uses Timer0 for continuously counting from the photodetector.
      Uses Timer1 for synchronized interrupts at every arg1 microseconds to save data.
      Uses an External Pin for synchronized interrupts (trigger) at RISING edge.
      

PINO DETECTOR DE PULSO:       PINO DIGITAL 22
PINO ENTRADA DO TRIGGER:      PINO DIGITAL 2
                              
PINO GERADOR DE ONDA:         PINO DIGITAL 7
                              PINO DIGITAL 8
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

#include <DueTimer.h>
#include <TCSPC.h>
#include <CommandHandler.h>

int cmd_idn();
int cmd_counter_start();
int cmd_counter_get();
int cmd_boxCar_start();
int cmd_stop_interrupts();

CommandType Commands[] = {
  {"*IDN?", &cmd_idn, "Device identification"},
  {"CNT_STR", &cmd_counter_start, "Counter Mode"},
  {"CNT_GET", &cmd_counter_get, "Counter Mode"},
  {"BXC", &cmd_boxCar_start, "BoxCar"},
  {"STOP", &cmd_stop_interrupts, "Stop all interrupts"},
};

CLI command_line(Commands, sizeof(Commands)/sizeof(Commands[0]));
TCSPC counter;

#define BXC_PIN_TRIGGER 2

void setup(){
  SerialUSB.begin(115200);
  //while (!SerialUSB.available());
  command_line.begin(&SerialUSB);
  
  counter.start_counter();

  pinMode(BXC_PIN_TRIGGER, INPUT);
  pwm1(84,42); //1MHz
  pwm2(84000000,42000000); //1 Hz (Chopper)
}

void loop(){
  command_line.start_processing();
}

int cmd_idn(){
  SerialUSB.println("Time-Correlated Single-Photon Counter - Lab. Nano Óptica - Dpto de Física UFPE - Allison Pessoa, 2022");
}

////////////////////
/// COUNTER MODE ///
////////////////////

volatile uint32_t prev_tc_cv;
volatile uint32_t diff_tc_cv;

int cmd_counter_start(){
  Timer1.attachInterrupt(handler_counter_mode);
  Timer1.setPeriod(atof(command_line.args[1]));
  Timer1.start();
}

void handler_counter_mode(){
  uint32_t cur_tc_cv = counter.get_counts();
  diff_tc_cv = cur_tc_cv - prev_tc_cv;
  prev_tc_cv = cur_tc_cv;
}

int cmd_counter_get(){
  write_ulong(diff_tc_cv);
}

///////////////////
/// BOXCAR MODE ///
///////////////////
#define BXC_MAX_BUFFER_SIZE 1000

volatile uint16_t cur_index = 0;
volatile uint16_t cur_repet = 0;

volatile uint16_t buffer_size;
volatile uint16_t num_repet;

volatile uint32_t init_counts;
volatile uint32_t counts_vect[BXC_MAX_BUFFER_SIZE];

int cmd_boxCar_start(){
  //Configures and attach the interrupts in the BoxCar mode; Command: 'BXC arg1 arg2 arg3';
  //Ordered Arguments: gate_time, buffer_size, number_of_repetitions;
  if (atof(command_line.args[2]) > BXC_MAX_BUFFER_SIZE){
    SerialUSB.print("The maximum buffer size is ");
    SerialUSB.println(BXC_MAX_BUFFER_SIZE);
    
  } else {
    
    int gate_time = atof(command_line.args[1]);
    buffer_size = atof(command_line.args[2]);
    num_repet = atof(command_line.args[3]);
    
    cur_repet = 0;
    cur_index = 0;
    
    for(int i=0; i<buffer_size; i++){
      counts_vect[i] = 0;
    }
  
    Timer1.attachInterrupt(bxc_save_counts);
    Timer1.setPeriod(gate_time);
    attachInterrupt(BXC_PIN_TRIGGER, handler_boxcar_mode, RISING);
  }
}

void handler_boxcar_mode(){
  //Called on the rising edge of the BXC_PIN_TRIGGER pin
  //Starts measurements if still repeating. Otherwise, send cumulated data in binary.
  if (cur_repet >= num_repet){
    Timer1.stop();
    detachInterrupt(BXC_PIN_TRIGGER);
    for (int i=0; i<buffer_size; i++){
        while (SerialUSB.availableForWrite() < sizeof(counts_vect[i])){}
        write_ulong(counts_vect[i]);
    }
    
  } else {
    
    cur_index = 0;
    cur_repet++;
    counter.start_counter();
    Timer1.start();
    init_counts = counter.get_counts();
  }
}

void bxc_save_counts(){
  //Saves the time-resolved data in the array
  if (cur_index <= buffer_size){
    counts_vect[cur_index] += counter.get_counts() - init_counts;
    cur_index++;
  }
}

///////////////
/// GENERAL ///
///////////////

void write_ulong(uint32_t value){
  byte buf[4];
  buf[0] = value & 255;
  buf[1] = (value >> 8)  & 255;
  buf[2] = (value >> 16) & 255;
  buf[3] = (value >> 24) & 255;

  SerialUSB.write(buf, sizeof(buf));
}

int cmd_stop_interrupts(){
  Timer1.stop();
}

void pwm1(uint16_t value, uint16_t duty){
  // GERADOR DE ONDA: PINO DIGITAL 7
  int32_t mask_PWM_pin = digitalPinToBitMask(7);
  REG_PMC_PCER1 = 1<<4;               // ativa o clocl para o controle do PWM
  REG_PIOC_PDR |= mask_PWM_pin;       // ativa as funções periférias para o pino (desativa todas as func PIO)
  REG_PIOC_ABSR |= mask_PWM_pin;      // define a opção de periférico B
  REG_PWM_CLK = 0;                    // define a taxa do clock, 0 -> máximo MCLK = 84MHz
  REG_PWM_CMR6 = 0<<9;                // define o clock e a polaridade para o canal do PWM (pin7) -> (CPOL = 0)
  REG_PWM_CPRD6 = value;              // define o período -> T = value/84MHz (value: 16bit),EX. value=4 -> 21 MHz
  REG_PWM_CDTY6 = duty;               // define o ciclo de trabalho, REG_PWM_CPRD6 / duty = duty cycle , para 4/2 = 50%
  REG_PWM_ENA = 1<<6;                 // habilita o canal do PWM (pin 7 = PWML6)
}

void pwm2(uint32_t value, uint32_t duty){
  // GERADOR DE ONDA: PINO DIGITAL 8
  int32_t mask_PWM_pin2 = digitalPinToBitMask(8);
  REG_PMC_PCER1 = 1<<4;               // ativa o clocl para o controle do PWM
  REG_PIOC_PDR |= mask_PWM_pin2;      // ativa as funções periférias para o pino (desativa todas as func PIO)
  REG_PIOC_ABSR |= mask_PWM_pin2;     // define a opção de periférico B
  REG_PWM_CLK = 0;                    // define a taxa do clock, 0 -> máximo MCLK = 84MHz
  REG_PWM_CMR5 = 0<<9;                // define o clock e a polaridade para o canal do PWM (pin8) -> (CPOL = 0)
  REG_PWM_CPRD5 = value;              // define o período -> T = value/84MHz (value: 16bit),EX. value=84 -> 1 MHz
  REG_PWM_CDTY5 = duty;               // define o ciclo de trabalho, REG_PWM_CPRD6 / duty = duty cycle , para 84/42 = 50%
  REG_PWM_ENA = 1<<5;                 // habilita o canal do PWM (pin 8 = PWML5)
}
