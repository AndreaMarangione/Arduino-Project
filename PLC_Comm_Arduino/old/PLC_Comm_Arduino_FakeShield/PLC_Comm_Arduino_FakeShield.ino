//Library
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0, 100 };
EthernetServer server(2001);

//Display
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);

//Tag
char receive;
char valueEncoder[10] = " ";
int encoderPointer = 0;
char internalFault = ' ';
char plcFault = ' ';
bool plcFaultActive = false;
char displayPrintUp[20] = " ";
char displayPrintDown[20] = " ";
char oldDisplayPrintUp[20] = " ";
char oldDisplayPrintDown[20] = " ";

void setup() {
  lcd.init();
  lcd.backlight();
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
  //Check for Ethernet hardware
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    // Serial.println("Ethernet shield was not found.");
    internalFault = '1';
  }

  //Check for Ethernet cable connection
  if (Ethernet.linkStatus() == LinkOFF) {
    // Serial.println("Ethernet cable is not connected.");
    internalFault = '2';
  }

  //FARE DIAGNOSTICA COMUNICAZIONE OK CON PLC

  //..........................PLC Communication..........................//
  if (client && internalFault != '1' && internalFault != '2') {
    receive = client.read();

    //Encoder Value Start
    if (receive == 'e') {
      digitalWrite(9, HIGH);
      plcFault = ' ';
      encoderPointer = 1;
      for (int i = 0; i < 10; i++) {
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
    if (encoderPointer > 0 && receive != 'e') {
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
  int checkDescription = 0;

  //Show Internal Fault
  if (internalFault != ' ') {
    for (int i = 0; i < 15; i++) {
      displayPrintUp[i] = ' ';
      displayPrintDown[i] = ' ';
    }
    displayPrintUp[0] = 'I';
    displayPrintUp[1] = 'N';
    displayPrintUp[2] = 'T';
    displayPrintUp[3] = 'E';
    displayPrintUp[4] = 'R';
    displayPrintUp[5] = 'N';
    displayPrintUp[6] = 'A';
    displayPrintUp[7] = 'L';
    displayPrintUp[8] = ' ';
    displayPrintUp[9] = 'F';
    displayPrintUp[10] = ' ';
    displayPrintUp[11] = internalFault;
  }

  //Show Plc Fault
  if (plcFault != ' ' && !plcFaultActive && internalFault == ' ') {
    for (int i = 0; i < 15; i++) {
      displayPrintUp[i] = ' ';
      displayPrintDown[i] = ' ';
    }
    displayPrintUp[0] = 'P';
    displayPrintUp[1] = 'L';
    displayPrintUp[2] = 'C';
    displayPrintUp[3] = ' ';
    displayPrintUp[4] = 'F';
    displayPrintUp[5] = ' ';
    displayPrintUp[6] = plcFault;
  }

  //Show Value Encoder
  if (encoderPointer <= 0 && plcFault == ' ' && internalFault == ' ') {
    for (int i = 0; i < 10; i++) {
      displayPrintUp[i] = ' ';
      displayPrintDown[i] = valueEncoder[i];
    }
    displayPrintUp[11] = ' ';
  }

  //Check What's Showing On
  for (int i = 0; i < 20; i++) {
    if (oldDisplayPrintUp[i] != displayPrintUp[i] || oldDisplayPrintDown[i] != displayPrintDown[i]) {
      checkDescription += 1;
    }
  }

  if (checkDescription > 0) {
    lcd.clear();
  }

  //Output
  lcd.setCursor(0, 1);
  lcd.print(displayPrintUp);
  lcd.setCursor(5, 2);
  lcd.print(displayPrintDown);

  //Save Old Description
  for (int i = 0; i < 20; i++) {
    oldDisplayPrintUp[i] = displayPrintUp[i];
    oldDisplayPrintDown[i] = displayPrintDown[i];
  }
}
