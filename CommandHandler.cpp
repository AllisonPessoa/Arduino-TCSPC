
Command::Command(String cmd_string, int num_arg, void (*function)(int))
{
  typedef struct {
    String content;
    volatile uint32_t value[num_arg];
  }Data;

  recieved = this->read_command();
  if (recieved.content == cmd_string)
    function(recieved.value)
}

Data Command::read_command() {
  Data Comm;
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

void Command::send_data(char id, String data){
  while (Serial.availableForWrite() < sizeof(data)){}
  Serial.print(id);
  Serial.println(data);
  delay(50);
}
