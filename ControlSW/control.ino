// Pins for controlling the 74HC595 registers
#define CTRL_PIN_DIN  2 // D2
#define CTRL_PIN_SHCP 3 // D3
#define CTRL_PIN_STCP 4 // D4
#define CTRL_PIN_nOE  5 // D5
// Input pin from the selected board. Inverted input, needs pull-up
#define CTRL_PIN_SPARE_IN  6  // D6
// Our output, extra
#define PIN_SPARE_OUT 8 // D8

uint8_t currentInShift[20] = {0};
uint8_t currentAtOut[20] = {0};

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
   currentAtOut[counter] = 0;
   currentInShift[counter] = 0;
  }

  updateEverything();
  transferDataToOutput();
  setOutput(0);
  
}

void loop() {
  int done = 0;
  String tempString = "";

  int a = 0;
  int b = 0;
  int c = 0;
  int d = 0;
  
  
  while (!done){
    if (Serial.available()>0){
       char t = Serial.read();
       if (t == '\n'){
        tempString = tempString + t;
        done = 1;
       }
       else{
        tempString = tempString + t;
       }
    }
  }

  d = sscanf(tempString.c_str(), "%d %d %d", &a, &b);
  Serial.println(a);
  Serial.println(b);
  Serial.println(c);
  Serial.println(d);
  
  Serial.print(tempString); 

  if (d != 2){
    Serial.println("NACK");
  }
  else{
    Serial.println("ACK");
    if (a == 0){
      if (b == 0){
        setOutput(0);
        Serial.println("Turning outputs OFF");
      }
      else if (b == 1){
        setOutput(1);
        Serial.println("Turning outputs ON");
      }
      else if (b == 2){
        updateEverything();
        Serial.println("Updating all SHIFTS, and transferring to output...");
      }
      else if (b == 3){
        transferDataToOutput();
        Serial.println("Updating SHIFT to OUTPUT");
      }
      else if (b == 4){
        Serial.print("Currently currentInShift:");
        for (int counter=0; counter<sizeof(currentInShift); counter++){
          Serial.print(" ");
          Serial.print(currentInShift[counter]);
        }
        Serial.println("");
        
        Serial.print("Currently currentAtOut:");
        for (int counter=0; counter<sizeof(currentAtOut); counter++){
          Serial.print(" ");
          Serial.print(currentAtOut[counter]);
        }
        Serial.println("");
        
      }
      else{
        Serial.println("ERR\n");
      }
    }
    else if (a >= 1 && a <= 20){
      if (b > 255 || b < 0){
        Serial.println("ERR\n");
      }
      else{
        currentInShift[a-1] = b;
      }
    }
    else{
      Serial.println("ERR\n");
    }
    
  }

}
