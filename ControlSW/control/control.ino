// Pins for controlling the 74HC595 registers
#define CTRL_PIN_DIN  2 // D2
#define CTRL_PIN_SHCP 3 // D3
#define CTRL_PIN_STCP 4 // D4
#define CTRL_PIN_nOE  5 // D5
// Input pin from the selected board. Inverted input, needs pull-up
#define CTRL_PIN_SPARE_IN  6  // D6
// Our output, extra
#define PIN_SPARE_OUT 8 // D8

// We're setting pins on the 595 registers. 8 pins per slot => 2 banks with 4 pins per bank
// POWER pins are ACTIVE HIGH
// INTERFACE pins are ACTIVE LOW
// SPARE pin is generic output
// QA/D0 && QE/D4 == POWER ENABLE (active HIGH)
// QB/D1 && QF/D5 == ICSP ENABLE (active LOW)
// QC/D2 && QG/D6 == JTAG ENABLE (active LOW)
// QD/D3 && QH/D7 == generic output
// First bank is QA/D0 to QD/D3, second bank is QE/D4 to QH/D7
#define BIT_PWR   0x01
#define BIT_ICSP  0x02
#define BIT_JTAG  0x04
#define BIT_PIN 0x08

uint8_t currentInShift[20] = {0b01100110}; // Memset in setup
uint8_t currentAtOut[20] = {0b01100110};

void sendByte(uint8_t data){
  // Data is send MSB first
  // Clock (SHCP) is IDLE LOW, Data is latched on rising edge

  for (int counter=7; counter >=0; counter--){
    int theBit = (data >> counter) & 0x01;
    digitalWrite(CTRL_PIN_DIN, theBit);
    delayMicroseconds(10);
    digitalWrite(CTRL_PIN_SHCP, HIGH);
    delayMicroseconds(10);
    digitalWrite(CTRL_PIN_SHCP, LOW);
    delayMicroseconds(10);
  }

}

void updateEverything(){
  for (int counter = sizeof(currentInShift)-1; counter>=0; counter--){
    sendByte(currentInShift[counter]);
  }
  transferDataToOutput();
}

void transferDataToOutput(){
  // Data is transfered to output on rising edge of STCP
  for (int counter=0; counter < sizeof(currentAtOut); counter++){
   currentAtOut[counter] = currentInShift[counter];
  }
  digitalWrite(CTRL_PIN_STCP, HIGH);
  delayMicroseconds(10);
  digitalWrite(CTRL_PIN_STCP, LOW);
  delayMicroseconds(10);
}

void setOutput(int val){
  // Inverted logic
  if (val){
    digitalWrite(CTRL_PIN_nOE, LOW);
    Serial.println("Turning outputs ON");
  }
  else{
    digitalWrite(CTRL_PIN_nOE, HIGH);
    Serial.println("Turning outputs OFF");
  }
}

void setup() {
  // Setup pin modes
  pinMode(CTRL_PIN_DIN, OUTPUT);
  pinMode(CTRL_PIN_SHCP, OUTPUT);
  pinMode(CTRL_PIN_STCP, OUTPUT);
  pinMode(CTRL_PIN_nOE, OUTPUT);
  
  pinMode(CTRL_PIN_SPARE_IN, INPUT_PULLUP);
  
  pinMode(PIN_SPARE_OUT, OUTPUT);

  // Also set outputs to default
  digitalWrite(CTRL_PIN_DIN, LOW);
  digitalWrite(CTRL_PIN_SHCP, LOW);
  digitalWrite(CTRL_PIN_STCP, LOW);
  digitalWrite(CTRL_PIN_nOE, HIGH);

  digitalWrite(PIN_SPARE_OUT, HIGH);

  Serial.begin(9600);

  for (int counter=0; counter < sizeof(currentAtOut); counter++){
   currentAtOut[counter] = 0b01100110;
   currentInShift[counter] = 0b01100110;
  }

  updateEverything();
  transferDataToOutput();
  setOutput(1);
  
}

void loop() {
  int done = 0;
  String tempString = "";

  int numConverted = 0;
  char argOne[16];
  uint8_t argTwo;
  char argThree[16];
  char argFour[16];
  char argFive[16];


  
  
  while (!done){
    if (Serial.available()>0){
       char t = Serial.read();
       if (t == '\n' || t == '\r'){
        //tempString = tempString + t;
        done = 1;
       }
       else{
        tempString = tempString + t;
       }
    }
  }


  tempString.toUpperCase();  // Don't care about case
  tempString.trim();                  // Remove whitespaces etc.

  if (tempString.equals("HELP")){
    Serial.println("Simple backplane controller");
    Serial.println("---------------------------");
    Serial.println("SET [SLOT] [BANK] [WHAT] [ON/OFF]");
    Serial.println("-> SLOT == 1 to 20");
    Serial.println("-> BANK == A or B");
    Serial.println("-> WHAT == PWR, ICSP, JTAG, PIN");
    Serial.println("-> ON == Asserted, OFF == Deasserted (polarities are taken care of)");
    Serial.println("GET [SLOT] [BANK] [WHAT]");
    Serial.println("See above for what is what.");
    Serial.println("");
  }
  
  Serial.println(tempString);
  numConverted = sscanf(tempString.c_str(), "%s %d %s %s %s",
                       argOne, &argTwo, argThree, argFour, argFive);


  if ((String(argOne) == "SET") && (numConverted == 5)){
    if (argTwo < 1 || argTwo > 20){
      Serial.println("NACK");
    }
    else if ((String(argThree) != "A") && (String(argThree) != "B")){
      Serial.println("NACK");
    }
    else if ((String(argFour) != "PWR") && (String(argFour) != "ICSP")
        && (String(argFour) != "JTAG") && (String(argFour) != "PIN")){
      Serial.println("NACK");
    } 
    else if ((String(argFive) != "ON") && (String(argFive) != "OFF")){
      Serial.println("NACK");
    }
    else{

      // Parse data
      uint8_t slot = argTwo - 1;
      
      uint8_t bank = 0; // A == default
      if (String(argThree) == "B"){
        bank = 4;
      }

      uint8_t onOff = 0; // OFF default
      if (String(argFive) == "ON"){
        onOff = 1;
      }        

      uint8_t bit = BIT_PWR;  // PWR == default
      if (String(argFour) == "ICSP"){
        bit = BIT_ICSP;
      }
      else if (String(argFour) == "JTAG"){
        bit = BIT_JTAG;
      }
      else if (String(argFour) == "PIN"){
        bit = BIT_PIN;
      }
      
      // PWR and SPARE bits get SET ON as they are. ICSP and JTAG are INVERTED
      // To set, PWR and SPARE; shift[slot] = shift[slot] | (bit << bank)
      // To clr, PWR and SPARE; shift[slot] = shift[slot] & (~(bit << bank))
      // JTAG and ICSP are inverted.    
      if (onOff){
        if (bit == BIT_PWR || bit == BIT_PIN){
          currentInShift[slot] = currentInShift[slot] | (bit << bank);
        }
        else{
          currentInShift[slot] = currentInShift[slot] & (~(bit << bank));
        }
      }
      else{
        if (bit == BIT_PWR || bit == BIT_PIN){
          currentInShift[slot] = currentInShift[slot] & (~(bit << bank));
        }
        else{
          currentInShift[slot] = currentInShift[slot] | (bit << bank);
        }
      }

      updateEverything();
      transferDataToOutput();
      setOutput(1);

    }
  }

  else if ((String(argOne) == "GET") && (numConverted == 4)){
    if (argTwo < 1 || argTwo > 20){
      Serial.println("NACK");
    }
    else if ((String(argThree) != "A") && (String(argThree) != "B")){
      Serial.println("NACK");
    }
    else if ((String(argFour) != "PWR") && (String(argFour) != "ICSP")
        && (String(argFour) != "JTAG") && (String(argFour) != "PIN")){
      Serial.println("NACK");
    }
    else{
            // Parse data
      uint8_t slot = argTwo - 1;
      
      uint8_t bank = 0; // A == default
      if (String(argThree) == "B"){
        bank = 4;
      }

      uint8_t bit = BIT_PWR;  // PWR == default
      if (String(argFour) == "ICSP"){
        bit = BIT_ICSP;
      }
      else if (String(argFour) == "JTAG"){
        bit = BIT_JTAG;
      }
      else if (String(argFour) == "PIN"){
        bit = BIT_PIN;
      }

      uint8_t info = currentInShift[slot] & (bit >> bank);
      if (info){
        info = 1;
      }
      if (bit == BIT_ICSP || bit == BIT_JTAG){
        info = info ^ 0x01;
      }

      Serial.println("ACK");
      Serial.println("VAL: " + String(info));
        
    }
  }
  else{
    Serial.println("NACK");
  }

}
