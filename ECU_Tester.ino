#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>



char receivedChar;
boolean newData = false;
boolean calibration = false;
boolean calTPS = false;
int filename = 0;
char csvFile[8];

///////////////
// Pin Setup //
///////////////

const int TPSPin = A0;
const int fuelPressureInput = A1;
const int IATPin = A2;
const int lambdaInput = A3;
const int MAPPin = A4;
const int led = 5;
const int button = 2;

/////////////////////////
// Sensor Min Max Vars //
/////////////////////////

int TPSMax = 1023;
int TPSMin = 0;

int TPSMaxAddress = 10;
int TPSMinAddress = 20;
int FuelPressureMaxAddress = 30;
int FuelPressureMinAddress = 40;

const int chipSelect = 4;

int isrCounter = 0;
volatile byte state = LOW;



void setup() {

    pinMode(led, OUTPUT);
    pinMode(button, INPUT);
    attachInterrupt(digitalPinToInterrupt(button), my_interrupt_handler, FALLING);
  
    Serial.begin(115200);
    Serial.println("<Arduino is ready>");
    Serial.println("Reading Values from EEPROM");

    TPSMax = EEPROM.read(TPSMaxAddress) * 4;
    TPSMin = EEPROM.read(TPSMinAddress) * 4;

    Serial.print("TPS Max : ");
    Serial.println(TPSMax);

    Serial.print("TPS Min : ");
    Serial.println(TPSMin);

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      while (1);
    }
    Serial.println("card initialized.");

    for(int x=0; x<1000; x++){
      // Check to see if the file exists: 
      char filename[8];
      sprintf(filename, "%03d.CSV", x);
      Serial.println(filename);
      if (SD.exists(filename)) {
        Serial.println(filename);
      }
      else {
        sprintf(csvFile, "%03d.CSV", x);
        break;
      }
    }
    
    
    File dataFile = SD.open(csvFile, FILE_WRITE);
    if (dataFile) {
      dataFile.println("TPS,FuelP,IAT,Lambda,MAP,Alert Level");
      dataFile.close();
    }

    delay(3000);
}

void loop() {
    recvOneChar();
    showNewData();
    processData();
}

void recvOneChar() {
    if (Serial.available() > 0) {
        receivedChar = Serial.read();
        newData = true;
    }
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChar);
        if ( receivedChar == 'c' ) {
          calibration = true;
        }
        if ( receivedChar == 'x' ) {
          calibration = false;
        }
        newData = false;
    }
}

void processData() {
    if (calibration == true) {
        Serial.println("In Calibration, press x to exit");
        Serial.println("Press Accelerator to max throttle");
        Serial.println("Reading will be taken in 2 seconds");
        delay(2000);
        TPSMax = analogRead(TPSPin);
        Serial.println("Release Accellerator now");
        Serial.println("Reading will be taken in 2 seconds");
        delay(2000);
        TPSMin = analogRead(TPSPin);
        EEPROM.update(TPSMaxAddress, TPSMax/4);
        EEPROM.update(TPSMinAddress, TPSMin/4);
        calibration = false;
    }
    else {
      String dataString = "";
      
      int tps = analogRead(TPSPin);
      int tpsPercent = map(tps,TPSMin, TPSMax, 0, 100);
      int fuelPressure = analogRead(fuelPressureInput);
      int fuelPressurePSi = map(fuelPressure, 0, 1023, 0, 140);
      int iat = analogRead(IATPin);
      int iatPercent = map(iat, 0, 1023, 0, 100);
      int lambda = analogRead(lambdaInput);
      int mapV = analogRead(MAPPin);

      dataString += tpsPercent;
      dataString += ",";
      dataString += fuelPressurePSi;
      dataString += ",";
      dataString += iatPercent;
      dataString += ",";
      dataString += lambda;
      dataString += ",";
      dataString += mapV;
      dataString += ",";
      dataString += isrCounter;
      Serial.println(dataString);

      File dataFile = SD.open(csvFile, FILE_WRITE);
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
        Serial.println(dataString);
      }

      

      else {
        Serial.print ("Error opening SD Card File");
    
      }
      
    }
    digitalWrite(led, state);
    //isrCounter = 0;
}

void my_interrupt_handler()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    state = !state;
    isrCounter = isrCounter + 10;
  }
  last_interrupt_time = interrupt_time;
}
