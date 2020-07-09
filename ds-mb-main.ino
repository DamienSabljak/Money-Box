//www.elegoo.com
//2016.12.9

//****WARNING: if at any point too much current is drawn, signals will Voltage sag and some registers will not be properly set****
//adding a serial comms messes stuff up for some reason?

int tDelay = 300;

//first shift register
int dataPin = 2;     // (12) DS [S1] on 74HC595 //serial should be the white wire
int clockPin = 3;      // (9) SH_CP [SCK] on 74HC595 //clock should be the blue wire
int latchPin = 9;      // (11) ST_CP [RCK] on 74HC595 //latch should be the orange wire

//second shift register
int dataPin_2 = 5;
int clockPin_2 = 7;
int latchPin_2 = 11;

//pin used to control motor - should be a white wire
int motorGate = 13;

int billSense = A0;

//sequence of LEDs to light up in their coordinate numbers
byte UNSETBIT = 20;//this number will be used within arrrays to determine a light to not turn on
byte TraceMoney[] = {0, 1, 2, 3, 4, 5, 6, 11, 10, 9, 8, 7, 12, 13, 14, 15, 16, 17, 18};
byte TraceMoney_2[] = {0, 1, 2, UNSETBIT, 4, 5, 6, 11, 10, 9, 8, 7, 12, 13, 14, 15, 16, 17, 18};


//State controllers
int MONEYMODE = 3;

//gives bits needed for first register
byte GetCoordBits1 (int coord) {
  if (coord < 8)
    return B10000000;
  else if (coord < 16)
    return B01000000;
  else
    return B00100000;
}

//gives bits needed for second register
byte GetCoordBits2 (int coord) {
  if (coord < 8)
    return (B10000000 >> coord) ^ 255; //flip bits since register 2 should be normally high
  else if (coord < 16)
    return (B10000000 >> (coord - 8)) ^ 255;
  else
    return (B10000000 >> (coord - 16)) ^ 255;
}

void updateShiftRegisters(byte leds,byte leds_2)
{
  //issues with this method causes flickers in other lights when setting the registers
  //for second shift register (return)
  shiftOut(dataPin_2, clockPin_2, LSBFIRST, leds_2);
  

  //first register (source)
  shiftOut(dataPin, clockPin, LSBFIRST, leds);

  //latch at same time to reduce flicker
  digitalWrite(latchPin, LOW);
  digitalWrite(latchPin_2, LOW);
  digitalWrite(latchPin, HIGH);
  digitalWrite(latchPin_2, HIGH);

}

void RunThroughSet(byte arr[], int arrSize) {
  for (int i = 0; i < arrSize; i++) {
    //if on list, light up in multiplex
    if (arr[i] != UNSETBIT) {
      byte leds   = GetCoordBits1(arr[i]);//replace array name in method with desired sequence
      byte leds_2 = GetCoordBits2(arr[i]);//same here
      updateShiftRegisters(leds,leds_2);

    }
    delayMicroseconds(600);//set large enough so there is adifference in brightnemss but small enough to theres no flickers
  }
}

void LoopSet(byte arr[], int arrSize, int loopNum) {
  for (int j = 0 ; j < loopNum ; j++) { //set jumber of loops to set amount of time display stays in this config
    RunThroughSet(arr, arrSize);
  }
}

void setup()
{
  //Serial debug
 ///*
    Serial.begin(9600);//note when using serial comms it will interfere with register (pins 0,1 are for serial)
    for (int k = 7; k < 8 ; k++) {
    Serial.println(GetCoordBits1(k), BIN);
    Serial.println(GetCoordBits2(k), BIN);
    }

    //Serial.end();
    //*/
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  //second shift register
  pinMode(latchPin_2, OUTPUT);
  pinMode(dataPin_2, OUTPUT);
  pinMode(clockPin_2, OUTPUT);

  //motor control
  pinMode(motorGate, OUTPUT);

  //bill sense
  pinMode(billSense, INPUT);

}

void loop()
{
  //initialize registers
  byte leds = 0;
  byte leds_2 = 255;
  //updateShiftRegisters();

  //byte leds = GetCoordBits1(0);
  //byte leds_2 = GetCoordBits2(0);
  updateShiftRegisters(leds,leds_2);

  Serial.println(analogRead(billSense));
  if(analogRead(billSense) > 900 ){ //THIS MUST BE CALIBRATED FOR EACH SPECIFIC LIGHT ENVIRONMENT
    digitalWrite(motorGate, HIGH);
    MONEYMODE = 0;//make lights flash quickly when money is inserted
  }
  else{
    digitalWrite(motorGate, LOW);
    MONEYMODE = 3;//make lights ambiently flash
  }
  
  //Cycle through all leds on money
  if (MONEYMODE == 0) {
    for (int i = 0; i < sizeof(TraceMoney); i++) {
      leds   = GetCoordBits1(TraceMoney[i]);//replace array name in method with desired sequence
      leds_2 = GetCoordBits2(TraceMoney[i]);//same here
      updateShiftRegisters(leds,leds_2);
      updateShiftRegisters(leds,leds_2);
      updateShiftRegisters(leds,leds_2);
      delay(100);
    }
  }

  //inverted money trace
  else if (MONEYMODE == 1) {
    //copy array over (change array in this block to change pattern)
    byte arr[sizeof(TraceMoney_2)];
    for (int k = 0 ; k < sizeof(TraceMoney_2); k++) {
      arr[k] = TraceMoney_2[k];
    }

    int temp1 = 0, temp2 = 0;
    for (int i = 0 ; i < sizeof(arr); i++) {//cycle through void positions
      temp1 = arr[i];//store array values to put back in place after
      arr[i] = UNSETBIT;
      if (i > 0)
        arr[i - 1] = temp2;
      temp2 = temp1;

      for (int j = 0 ; j < 15 ; j++) { //set jumber of loops to set amount of time display stays in this config
        RunThroughSet(arr, sizeof(arr));
      }
      arr[18] = 18;
    }
  }

  //money sign fully on (Careful! this may burn out shift registers if run for long periods)
  else if (MONEYMODE == 2) {
    leds = B11111111;
    leds_2 = B00000000;
    updateShiftRegisters(leds,leds_2);
  }

  //money flush
  else if (MONEYMODE == 3) {
    int pauseTime = 5; //sets pause between transistions

    //copy array over (change array in this block to change pattern)
    byte arr[sizeof(TraceMoney)];
    for (int k = 0 ; k < sizeof(TraceMoney); k++) {
      arr[k] = TraceMoney[k];
    }
    LoopSet(arr, sizeof(arr), pauseTime*5);
    
    //turn lights off then on
    for (int j = 0 ; j < 2; j++) {

      //first light
      if (j == 0)
        arr[0] = UNSETBIT;
      else
        arr[0] = TraceMoney[0];
      LoopSet(arr, sizeof(arr), pauseTime);

      //turn of second row
      for (int i = 1; i <= 5; i++) {
        if (j == 0)
          arr[i] = UNSETBIT;
        else
          arr[i] = TraceMoney[i];
      }
      LoopSet(arr, sizeof(arr), pauseTime);

      //middle left light off
      if (j == 0)
        arr[6] = UNSETBIT;
      else
        arr[6] = TraceMoney[6];
      LoopSet(arr, sizeof(arr), pauseTime);

      //middle row
      for (int i = 7; i <= 11; i++) {
        if (j == 0)
          arr[i] = UNSETBIT;
        else
          arr[i] = TraceMoney[i];
      }
      LoopSet(arr, sizeof(arr), pauseTime);

      //middle right off
      if (j == 0)
        arr[12] = UNSETBIT;
      else
        arr[12] = TraceMoney[12];
      LoopSet(arr, sizeof(arr), pauseTime);

      //bottom row
      for (int i = 13; i <= 17; i++) {
        if (j == 0)
          arr[i] = UNSETBIT;
        else
          arr[i] = TraceMoney[i];
      }
      LoopSet(arr, sizeof(arr), pauseTime);

      //bottom light
      if (j == 0)
        arr[18] = TraceMoney[18];
      else
        arr[18] = TraceMoney[18];
      LoopSet(arr, sizeof(arr), pauseTime);

    }
  }




  //***debug***
  /*
    digitalWrite(latchPin_2, LOW);
    digitalWrite(clockPin_2, HIGH);
  */

}

