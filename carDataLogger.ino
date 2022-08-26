#include <EEPROM.h>
//#include <SPI.h>
//#include <SD.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



char receivedChar;
boolean newData = false;
boolean calibration = false;
boolean calTPS = false;
//int filename = 0;
//char csvFile[8];

///////////////
// Pin Setup //
///////////////

const int TPSPin = A0;
const int fuelPressureInput = A1;
const int IATPin = A2;
const int lambdaInput = A3;
const int MAPPin = A6;
const int coolantPin = A7;
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

const bool narrowbandLambda = true;
//const int narrowbandThreshold = 100; //mV
const int narrowbandThreshold = 512; //mV

//const int chipSelect = 4;

int isrCounter = 0;
volatile byte state = LOW;

int page = 4;

void setup() {

    pinMode(led, OUTPUT);
    pinMode(button, INPUT);
    attachInterrupt(digitalPinToInterrupt(button), my_interrupt_handler, FALLING);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(2000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();

    // Draw a single pixel in white
    display.drawPixel(10, 10, SSD1306_WHITE);

    // Show the display buffer on the screen. You MUST call display() after
    // drawing commands to make them visible on screen!
    display.display();
    delay(2000);
  
    Serial.begin(115200);
    Serial.println("<Arduino is ready>");
    Serial.println("Reading Values from EEPROM");

    TPSMax = EEPROM.read(TPSMaxAddress) * 4;
    TPSMin = EEPROM.read(TPSMinAddress) * 4;

    Serial.print("TPS Max : ");
    Serial.println(TPSMax);

    Serial.print("TPS Min : ");
    Serial.println(TPSMin);

//    // see if the card is present and can be initialized:
//    if (!SD.begin(chipSelect)) {
//      Serial.println("Card failed, or not present");
//      // don't do anything more:
//      while (1);
//    }
//    Serial.println("card initialized.");
//
//    for(int x=0; x<1000; x++){
//      // Check to see if the file exists: 
//      char filename[8];
//      sprintf(filename, "%03d.CSV", x);
//      Serial.println(filename);
//      if (SD.exists(filename)) {
//        Serial.println(filename);
//      }
//      else {
//        sprintf(csvFile, "%03d.CSV", x);
//        break;
//      }
//    }
//    
//    
//    File dataFile = SD.open(csvFile, FILE_WRITE);
//    if (dataFile) {
//      dataFile.println("TPS,FuelP,IAT,Lambda,MAP,Alert Level");
//      dataFile.close();
//    }
//
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
        
        if ( receivedChar == '0' ) {
          page = 0;
        }

        if ( receivedChar == '1' ) {
          page = 1;
        }
        if ( receivedChar == '2' ) {
          page = 2;
        }
        if ( receivedChar == '3' ) {
          page = 3;
        }
        if ( receivedChar == '4' ) {
          page = 4;
        }
        if ( receivedChar == '5' ) {
          page = 5;
        }
        if ( receivedChar == '6' ) {
          page = 6;
        }
        if ( receivedChar == '7' ) {
          page = 7;
        }
        
        newData = false;
    }
}

void analog_gauge(int sensor) {
    // Credit for the pointer from this page
    // https://forum.arduino.cc/t/cos-sin-analog-needle-gauge-line-code-display-using-adafruit_ssd1306-128x64/529669
    
    float angle  = (PI/1023) * sensor;                        // map analog in 0,1023, to 0.00,3.14
    int length = 40;                                       // line height Ratio of Screen 0-64
    const byte x0 = 64;                                    // x0 Line Start 0-128
    const byte y0 = 63;                                    // y0 Line Start 0-64
                             // y0 Line Start 0-64
    
    display.drawCircle(x0, y0, 45, WHITE);
    
    byte x1 = x0 - length * cos (angle);
    byte y1 = y0 - length * sin (angle);
    
    display.drawLine(x0, y0, x1, y1, WHITE); // write to screen
     
}

void processData() {
    if (calibration == true) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.setTextSize(2); // Draw 2X-scale text
        display.println("TPS Cali");
        display.println();
        display.println("Full Accel");
        //display.println("Reading will be taken in 2 seconds");
        display.display();
        delay(2000);
        TPSMax = analogRead(TPSPin);
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("TPS Cali");
        display.println();
        display.println("No Accel");
        //display.println("Reading will be taken in 2 seconds");
        display.display();
        delay(2000);
        display.clearDisplay();
        display.display();
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
      //Serial.println(dataString);

      //display the pages
      if ( page == 0 ){
        //Display the full output,
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0,0);
        display.print("TPS:  "); display.println(tpsPercent);
        display.print("Fuel: "); display.println(fuelPressurePSi);
        display.print("IAT:  "); display.println(iatPercent);
        display.print("O2:   "); display.println(lambda);
        display.print("MAP:  "); display.println(mapV);
        display.print("CLT:  "); display.println("0");
        display.display();
      }

      if ( page == 1 ) {
        // its the TPS on this one
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("TPS");
        display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
        display.setTextSize(4);
        display.setCursor(0,20);
        display.println(tpsPercent);
        display.display();
      }

      if ( page == 2 ) {
        // its the Fuel Pressure on this one
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("Fuel Pres");
        display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
        display.setTextSize(4);
        display.setCursor(0,20);
        display.println(fuelPressurePSi);
        display.display();
      }

      if ( page == 3 ) {
        // its the IAT on this one
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("Air Temp");
        display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
        display.setTextSize(4);
        display.setCursor(0,20);
        display.println(iatPercent);
        display.display();
      }

      if ( page == 4 ) {
        // its the Lambda on this one
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("Lambda");
        display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
        display.setTextSize(4);
        display.setCursor(0,45);
        display.drawRect(2, 20, 124, 20, SSD1306_WHITE);
        display.drawFastVLine(64, 20, 20, SSD1306_WHITE);
        // rich / lean on narrowband
        if (narrowbandLambda == true) {
          if ( lambda < narrowbandThreshold ) {
            //lean
            display.fillRect(4, 22, 60, 16, SSD1306_WHITE);
          }
          else {
            //rich
            display.fillRect(64, 22, 60, 16, SSD1306_WHITE);
          }
        }
        display.println(lambda);
        display.display();
      }

      if ( page == 5 ) {
        // its the Map on this one
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("Map");
        display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
        display.setTextSize(4);
        display.setCursor(0,20);
        display.println(mapV);
        display.display();
      }

      if ( page == 6 ) {
        // its the Coolant Temp on this one
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("Coolant");
        display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
        display.setTextSize(4);
        display.setCursor(0,20);
        display.println(96);
        display.display();
      }

      if ( page == 7 ) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("TPS");
        display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
        analog_gauge(tps);
        display.display();
      }

//      File dataFile = SD.open(csvFile, FILE_WRITE);
//      if (dataFile) {
//        dataFile.println(dataString);
//        dataFile.close();
//        Serial.println(dataString);
//      }
//
//      
//
//      else {
//        Serial.print ("Error opening SD Card File");
//    
//      }
      
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
