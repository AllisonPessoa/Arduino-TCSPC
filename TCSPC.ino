/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

///////////////////////////////////////////////////////
//////////////////// COMMUNICATION ////////////////////
///////////////////////////////////////////////////////
#include <TCSPC.h>
#include <CommandHandler.h>

TCSPC counter(0)
CommandHandler command1("*IDN?\n", 1, &get_id)

void get_id(int value){
  Serial.println("r:FEDERAL UNIVERSITY OF PERNAMBUCO");
  Serial.println("r:PHYSICS DEPARTMENT");
  Serial.println("r:DIGITAL TIME-CORRELATED SINGLE PHOTON COUNTER");
  Serial.println(value[0])
}

void setup(){
  SerialUSB.begin(9600); //Native USB port
  while(!SerialUSB);
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

void pwm2(uint16_t value, uint16_t duty){
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

///////////////////////////////////////////////
////////////// VOLATILE DELAY /////////////////
///////////////////////////////////////////////

void volatile_delay(unsigned long time_delay){
  if (time_delay >= 1000){
    delayMicroseconds(time_delay/1000);
  }
  else {
    volatile long j;
    unsigned long cycles = round(time_delay*6.44659/1000); // Converting from nanoseconds to clock cycles
    for(j=0;j<cycles;j++){}
  }
}

/////////////////////////////////////////
////////////// TRIGGERS /////////////////
/////////////////////////////////////////

void TCSPC::set_trigger(int pin_num){
  pinMode(pin, INPUT_PULLUP); // sinal trigger
}

bool TCSPC::get_trigger_status(int pin_num){
  return ((digitalPinToPort(pin_num) -> PIO_PDSR >> digitalPinToBitMask(pin_num)) & 1);
}

/////////////////////////////////////////
////////////// ACTUATORS ////////////////
/////////////////////////////////////////

void TCSPC::LVTTL_pulse(int pin_num, unsigned long period){
  digitalPinToPort(pin_num) -> PIO_SODR = (1<<digitalPinToBitMask(pin_num)); //set C22/Pin8 high
  volatile_delay(period); // approx 500 ns
  digitalPinToPort(pin_num) -> PIO_CODR = (1<<digitalPinToBitMask(pin_num)); //set C22/Pin8 low
}

/////////////////////////////////////////////////////
////////////// DATA ACQUISION MODES /////////////////
/////////////////////////////////////////////////////

void Counter_mode(unsigned long time_delay){
  uint32_t counts = 0;

  start_counter();
  volatile_delay(time_delay);
  counts = get_counts();

  send_data('A', String(counts));
}

void Boxcar_Mode(unsigned long gate_time, unsigned int trigger_delay, unsigned long N_buffer, unsigned  int N_rep){

  volatile uint32_t counts[N_buffer];
  uint32_t zero;
  String single_line_values = "";

  //Zeramento do buffer de contagens
  for (int i=0; i< N_buffer; i++){
    counts[i] = 0;
  }

  for (int j=0; j<N_rep; j++){
    start_counter();

    while(trigger_on() == 1){}
    while(trigger_on() == 0){}                //Aguardar fase com o sinal
    volatile_delay(trigger_delay);

    for (int i=0; i< N_buffer; i++){                           //ciclo neg
        zero = get_counts();                      //leitura do registrador do TC0 - canal 0
        volatile_delay(gate_time);                                 //Aguarda o time definido
        counts[i] += get_counts() - zero;          //alocar na memória
    }
  }

  for(int i=0; i<N_buffer; i++){//salva na string
    single_line_values.concat(counts[i]);
    single_line_values.concat(",");
  }
  send_data('B', single_line_values);
}

void ODMR_Mode(int N_repet){

  uint32_t counts_pos = 0;
  uint32_t counts_neg = 0;
  uint32_t zero;

  start_counter();

  while(trigger_on() == 0){}
  while(trigger_on() == 1){}                  // e aí, comoAguardar fase com o sinal

  for(int i=0;i<N_repet;i++){

    zero = get_counts();
    while(trigger_on() == 0){}
    counts_neg += get_counts() - zero;

    zero = get_counts();
    while(trigger_on() == 1){}                 //ciclo positivo
    counts_pos += get_counts() - zero;
  }

  send_data('P', String(counts_pos));
  send_data('N', String(counts_neg));

}

void PLL_Mode(unsigned long int_time, int N_repet){
  uint32_t counts_neg_up = 0;
  uint32_t counts_neg_down = 0;
  uint32_t counts_pos_up = 0;
  uint32_t counts_pos_down = 0;

  uint32_t zero;

  start_counter();

  while(trigger_on() == 0){}
  while(trigger_on() == 1){}                  // e aí, comoAguardar fase com o sinal

  for(int i=0;i<N_repet;i++){
    while(trigger_on() == 0){                 //ciclo negativo da RF
      TTL_pulse(500);
      zero = get_counts();
      volatile_delay(int_time);
      counts_neg_up += get_counts() - zero;        //leitura do registrador do TC0 - canal 0

      TTL_pulse(500);
      zero = get_counts();
      volatile_delay(int_time);
      counts_neg_down += get_counts() - zero;
    }
    while(trigger_on() == 1){                 //ciclo positivo da RF
      TTL_pulse(500);
      zero = get_counts();
      volatile_delay(int_time);
      counts_pos_up += get_counts() - zero;        //leitura do registrador do TC0 - canal 0

      TTL_pulse(500);
      zero = get_counts();
      volatile_delay(int_time);
      counts_pos_down += get_counts() - zero;
    }
  }
  unsigned long result = counts_pos_up + counts_neg_up - (counts_pos_down + counts_neg_down);
  send_data('R', String(result));
}

//////////////////////////////////////////
////////////// LOOP WORK /////////////////
//////////////////////////////////////////

Command recieved;

void loop (){
  pwm1(2,1); //42 MHz
  if (Serial.available()){
    recieved = process_command();
    Serial.println(recieved.content);
  }

  if (recieved.content == "STDBY\n") {
    delay(10);
  }

  if (recieved.content == "*IDN?\n") {
    Serial.println("r:FEDERAL UNIVERSITY OF PERNAMBUCO");
    Serial.println("r:PHYSICS DEPARTMENT");
    Serial.println("r:DIGITAL TIME-CORRELATED SINGLE PHOTON COUNTER");
    recieved.content = "STDBY\n"; // Return to STANDBY
  }

  if (recieved.content == "CNT()\n") {
    Counter_mode(recieved.value[0]);
    delay(10);
  }

  if (recieved.content == "BXC(,,,)\n") {
    Boxcar_Mode(recieved.value[0], recieved.value[1], recieved.value[2], recieved.value[3]);
    recieved.content = "STDBY\n"; // Return to STANDBY
  }

  if (recieved.content == "ODMR(,,,)\n") {
    ODMR_Mode(recieved.value[0]);
    recieved.content = "STDBY\n"; // Return to STANDBY
  }

  if (recieved.content == "PLL(,,,)\n") {
    PLL_Mode(recieved.value[0], recieved.value[1]);
    recieved.content = "STDBY\n"; // Return to STANDBY
  }

  if (recieved.content == "PWMI(,)\n") {
      if (recieved.value[0] <= 0 || recieved.value[1] >= recieved.value[0]){
        Serial.println("r:Insira values válidos. Consulte 'help?'");
      } else{
        pwm1(recieved.value[0],recieved.value[1]);
        Serial.println("r:PWM(I) ativado com sucesso - PINO DIGITAL 7");
      }
      recieved.content = "STDBY\n"; // Return to STANDBY
  }

  if (recieved.content == "PWMII(,)\n") {
      if (recieved.value[0] <= 0 || recieved.value[1] >= recieved.value[0]){
        Serial.println("r:Insira values válidos. Consulte 'help?'");
      } else{
        pwm2(recieved.value[0],recieved.value[1]);
        Serial.println("r:PWM(I) ativado com sucesso - PINO DIGITAL 7");
      }
      recieved.content = "STDBY\n"; // Return to STANDBY
  }

  if (recieved.content == "TEST\n") {
    for (int i=0; i<2000; i++){
      Counter_mode(2000+i*10);
    }
    recieved.content = "STDBY\n";
  }
}
