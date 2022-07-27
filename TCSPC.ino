/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

///////////////////////////////////////////////////////
//////////////////// COMMUNICATION ////////////////////
///////////////////////////////////////////////////////

typedef struct {
  String content;
  volatile uint32_t value[4];
}Command;

Command process_command() {
    Command Comm;
    char number_char[10];
    char *eprt;
    int ascii;
    int i=0, j=0;

    while(Serial.available()>0) {
      ascii=Serial.peek();
      if(ascii != 13 or ascii != 10) { //end of the command string

        if (ascii > 47 && ascii < 58) { //In the char is a number
          delay(50);
            do {
              number_char[i]= Serial.read(); //save the number char neatly
              ascii = Serial.peek();
              i++;
            } while (ascii > 47 && ascii < 58); //while there are numbers

          i=0;
          long int var = strtol(number_char,&eprt,10);
          Comm.value[j] = var; // Save the number string as a 16-bit integer

          for (int k=0; k<10;k++){
            number_char[k]=0;
          }
          j++;
        }
        else { // In case the char is a letter
          char carac = Serial.read();
          if (carac > 96 && carac < 123){
            carac -= 32; //Keep only upper-case letters
          }
          Comm.content.concat(carac); //concatenate the chars
        }
      }
      delay(10);
    }

    return Comm;
}

void send_data(char id, String data){
  while (Serial.availableForWrite() < sizeof(data)){}
  Serial.print(id);
  Serial.println(data);
  delay(50);
}

//////////////////////////////////////////////
////////////// GENERAL CONFIG ////////////////
//////////////////////////////////////////////

void setup(){
  Serial.begin(115200);
  pinMode(2,INPUT_PULLUP); // sinal chopper
  //CONFIGURAÇÕES DO PWM
  PMC->PMC_PCER0 = PMC_PCER0_PID27;                // interrupção TC0 selecionada - habilita o controle e gerencialmento de tensão
  TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS ;      // lock do timer desabilitado para inicialização
  TC0->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_XC0;   // clock do contador selecionado : XC0
  TC0->TC_BMR = TC_BMR_TC0XC0S_TCLK0;              // sinal conectado ao XC0: TCLK0=PB26=Arduino_Due_Digital_Pin_22 https://www.arduino.cc/en/Hacking/PinMappingSAM3X
}

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

void start_counter(){
  unsigned long timeout = 84000000; // 1s max para o próximo pulso de clock
  unsigned long k=0;
  unsigned long zero;
  // TC_CV só zera após o primeiro pulso externo após o sinal de reset. Aguarda até que isto aconteça.
  // Ver pag. 862, 36.6.6 (Trigger) no manual do SAM3X
  TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG      // sowftware trigger: reset counter, start clock
                            | TC_CCR_CLKEN;     // counter clock enabled
  zero = TC0->TC_CHANNEL[0].TC_CV;
  while((TC0->TC_CHANNEL[0].TC_CV == zero)&&(k<timeout)){
    k++;
  }//contador zerado
}

uint32_t get_counts(){
  return TC0->TC_CHANNEL[0].TC_CV;
}

////////////////////////////////////////
////////////// TRIGGERS ////////////////
////////////////////////////////////////

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

void TTL_pulse(unsigned long period){
  PIOC->PIO_SODR = (1<<22); //set C22/Pin8 high
  volatile_delay(period); // approx 500 ns
  PIOC->PIO_CODR = (1<<22); //set C22/Pin8 low
}

/////////////////////////////////////////////
////////////// PHASE LOCKERS ////////////////
/////////////////////////////////////////////

bool trigger_on(){
  return ((PIOB->PIO_PDSR >> 25 ) & 1);
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
