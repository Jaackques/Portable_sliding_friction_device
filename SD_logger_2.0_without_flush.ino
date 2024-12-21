#include <SPI.h>
#include <SD.h>
#include <HX711_ADC.h>


//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin
const int buttonPin = 2;
int buttonState = 0;

const int chipSelect = 10;

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);


unsigned long t = 0;

File dataFile;
bool fileOpen = false;
int writeCount = 0;

void setup() {
  Serial.begin(57600); delay(10);
  Serial.println();
  pinMode(buttonPin, INPUT);

  // Open serial communications and wait for port to open:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

  Serial.println("Starting...");

  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 4300; // uncomment this if you want to set the calibration value in the sketch


  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  } else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }

  // Open the file for writing:
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    fileOpen = true;
    Serial.println("File opened successfully.");
  } else {
    Serial.println("Error opening datalog.txt");
  }
}

void loop() {
  // make a string for assembling the data to log:
  String dataString = "";
  static boolean newDataReady = 0;
  const int serialPrintInterval = 100; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      dataString += String(millis());
      dataString += String(", ");
      dataString += String(i);
      newDataReady = 0;
      t = millis();

        // If the file is open, write to it:
     if (fileOpen) {
    dataFile.println(dataString);

             // print to the serial port too:
             Serial.println(dataString);
  }
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }



  // Check if button is pressed to close the file:
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH && fileOpen) {
    dataFile.close();
    fileOpen = false;
    Serial.println("File closed.");
  }
}
