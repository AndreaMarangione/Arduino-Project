//Library
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <TM1637TinyDisplay6.h>

//Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0, 100 };
EthernetServer server(2001);

//Display
#define CLK 3
#define DIO 2
TM1637TinyDisplay6 display(CLK, DIO);

//Tag
char receive;
char valueEncoder[6] = " ";
int encoderPointer = 0;
bool positionReached = false;
char internalFault = ' ';
char plcFault = ' ';
bool plcFaultActive = false;
bool plcNoComm = false;
int timerComm = 0;
int displayBlankCount = 0;
int displayShowCount = 0;

void setup() {
  display.begin();
  display.setBrightness(BRIGHT_HIGH);  //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  Serial.begin(9600);
  pinMode(9, OUTPUT);
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
  //Init
  EthernetClient client = server.available();
  internalFault = ' ';

  //..........................Diagnostic..........................//
  //Check for Ethernet cable connection
  if (!client) {
    plcNoComm = true;
  } else {
    plcNoComm = false;
    timerComm = 0;
  }

  if (plcNoComm) {
    timerComm++;
    if (timerComm > 254) {
      timerComm = 255;
    }
  }

  if (timerComm > 100) {
    internalFault = '1';
  }

  //..........................PLC Communication..........................//
  if (client && internalFault == ' ') {
    receive = client.read();

    //Encoder Value Start While Moving
    if (receive == 'e') {
      positionReached = false;
      digitalWrite(9, HIGH);
      plcFault = ' ';
      encoderPointer = 1;
      for (int i = 0; i < 6; i++) {
        valueEncoder[i] = ' ';
      }
    }

    //Position Reached
    if (receive == 't') {
      positionReached = true;
      digitalWrite(9, HIGH);
      plcFault = ' ';
      encoderPointer = 1;
      for (int i = 0; i < 6; i++) {
        valueEncoder[i] = ' ';
      }
    }

    //Data Received
    if (receive == 'f') {
      encoderPointer = 0;
      plcFaultActive = false;
      digitalWrite(9, LOW);
    }

    //Receiving Encoder Value
    if (encoderPointer > 0 && receive != 'e' && receive != 't') {
      valueEncoder[encoderPointer - 1] = receive;
      encoderPointer += 1;
    }

    //Alarm Start
    if (receive == 'a') {
      digitalWrite(9, HIGH);
      plcFaultActive = true;
    }

    //Receiving Alarm
    if (plcFaultActive && receive != 'a' && receive != ' ') {
      plcFault = receive;
    }
  }

  //..........................Display..........................//
  //Show Internal Fault
  if (internalFault != ' ') {
    char displayText[6] = " ";
    displayText[0] = 'n';
    displayText[1] = 'o';
    displayText[2] = ' ';
    displayText[3] = 'C';
    displayText[4] = 'O';
    displayText[5] = 'N';
    display.showString(displayText);
  }

  //Show Plc Fault
  if (plcFault != ' ' && !plcFaultActive && internalFault == ' ') {
    char displayText[6] = " ";
    displayText[0] = 'p';
    displayText[1] = 'l';
    displayText[2] = 'c';
    displayText[3] = 'F';
    displayText[4] = '0';
    displayText[5] = plcFault;
    display.showString(displayText);
  }

  //Show Value Encoder
  if (encoderPointer <= 0 && plcFault == ' ' && internalFault == ' ') {
    char displayText[6] = " ";
    if (positionReached && displayBlankCount < 20 && displayShowCount <= 0) {
      display.showString(displayText);
      displayBlankCount++;
    }
    if (displayBlankCount >= 20 || !positionReached) {
      for (int i = 0; i < 6; i++) {
        if (valueEncoder[i] != ' ') {
          displayText[i] = valueEncoder[i];
        }
      }
      display.showString(displayText);
      displayShowCount++;
    }
    if (positionReached && displayShowCount >= 50) {
      displayBlankCount = 0;
      displayShowCount = 0;
    }
  }
}
