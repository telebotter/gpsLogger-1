/*
  Author: MadNerd.org
  Licence: Mit

  Log speed/altitude/gps coordinate in a .csv file (DAYHOURMINUTESSECOND.csv)
  You can reuse theses values in any spreedsheet software (LibreOffice/Excel...)
  or you can convert .csv to a gpx with http://www.gpsies.com/convert.do

  Buzzer sound
  -- Heavy sound : GPS/SD card fatal error
  -- Soft repetitive sound : GPS is calibrating
  -- Soft and short sound : GPS coordinates saved to sd  

  Components:
  * Ublox GPS module (or compatible with tinygps++)
  * Micro SD to SD card Adapter
  * A buzzer
  * An Arduino Pro Mini 3V
  * Batteries

  Tools:
  ftdi 3V/5V programmer

  Wiring:
  SD card 
  1 --> X
  2 --> 12
  3 --> GND
  4 --> 13
  5 --> VCC
  6 --> GND
  7 --> 11
  8 --> 10 (Chip select)

  GPS
  RX --> 2
  TX --> 3
  VCC --> VCC
  GND --> GND

  You will need 
  * TinyGPS++ library
  * LowPower library

  Previous Readme
  This example uses software serial and the TinyGPS++ library by Mikal Hart
  Based on TinyGPSPlus/DeviceExample.ino by Mikal Hart
*/

//GPS
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "LowPower.h"
//SD
#include <SPI.h>
#include <SD.h>

//Interval between gps position log
const int gps_interval = 2; //Multiples of 8 seconds (here 8x2sec = 16s)

//GPS TX/RX 
//Don't use TX/RX pins on the arduino or you won't be able to upload code on it
//or debug it.
const int RXPin = 2;
const int TXPin = 3;

const int timezone = 1;
//France +1
//England +0
//etc...

// Ublox run at 9600
const int GPSBaud = 9600;

//SD
const int chipSelect = 10;

//Buzzer
const int buzzer = 9;


bool timeOK = false;
bool posOK = false;
bool sdOK = true;
String string_filename;
char* filename;

String s_day;
String s_month;
String s_year;
String s_hour;
String s_minute;
String s_second;
int last_minute;


// Create a TinyGPS++ object called "gps"
TinyGPSPlus gps;

// Create a software serial port called "gpsSerial"
// We can't use TX/RX pins if we don't want to lose the ability to debug/upload code
SoftwareSerial gpsSerial(RXPin, TXPin);


void sleep8sec(int multiples){
  for (int i = 0; i < multiples; i++){
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

void setup()
{
  // Start the Arduino hardware serial port at 9600 baud
  
  Serial.begin(9600);
  buzz(800,200);
  // Start the software serial port at the GPS's default baud
  gpsSerial.begin(GPSBaud);


  // see if the SD card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("SD : ERROR");
    sdOK = false;
    // don't do anything more:
    return;
  }
  Serial.println("SD : OK");
}

void loop()
{
  // This sketch displays information every time a new sentence is correctly encoded.
  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      displayInfo();

  // If 5000 milliseconds pass and there are no characters coming in
  // over the software serial port, show a "No GPS detected" error
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("GPS : ERROR"));
    buzz(700,2000); //GPS error panic
  }

}

void writeHeaders() {
  Serial.println("Writing header");
  Serial.println(filename);
  File gpsFile = SD.open(filename, FILE_WRITE);
  if (gpsFile) {
    gpsFile.println("Latitude,Longitude,Elevation,Time,Speed");
  } else {
    Serial.println("SD : ERROR WRITING");
  }
  gpsFile.close();
}

void displayInfo()
{
  //Check if SD card works or stop
  if (sdOK) {
    
    //Date Formatting

    s_year = gps.date.year();

    if (gps.date.day() < 10) {
      s_day = "0" + String(gps.date.day());
    } else {
      s_day = String(gps.date.day());
    }

    if (gps.date.month() < 10) {
      s_month = "0" + String(gps.date.month());
    } else {
      s_month = String(gps.date.month());
    }

    //UTC Adjustement
    int adj_hour = gps.time.hour() + timezone;

    if (adj_hour < 10) {
      s_hour = "0" + String(adj_hour);
    } else {
      s_hour = String(adj_hour);
    }

    if (gps.time.minute() < 10) {
      s_minute = "0" + String(gps.time.minute());
    } else {
      s_minute = String(gps.time.minute());
    }

    if (gps.time.second() < 10) {
      s_second = "0" + String(gps.time.second());
    } else {
      s_second = String(gps.time.second());
    }

    //Display debug information

    //Date
    Serial.print(s_day);
    Serial.print(F("/"));
    Serial.print(s_month);
    Serial.print(F("/"));
    Serial.print(s_year);
    Serial.print(" ");

    //Time
    Serial.print(s_hour);
    Serial.print(F(":"));
    Serial.print(s_minute);
    Serial.print(F(":"));
    Serial.print(s_second);

    //Lat / Long
    Serial.print(F(","));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    Serial.print(F(","));
    Serial.print(gps.altitude.meters());
    Serial.print(F(","));
    Serial.print(gps.speed.kmph());
    Serial.println();

    //If time / lat is set we write to the SD Card
    
  
      //We need to also check date/hour in case of deconnection
      //It seems the date is reset and last gps location is displayed      
      if (gps.location.lat() != 0)
      {
        if (! timeOK) {
         buzz(600,1000);
         string_filename = s_day + s_hour + s_minute + s_second + ".csv";

         //Convert string to char
         unsigned int bufSize = string_filename.length() + 1; //String length + null terminator
         filename = new char[bufSize];
         string_filename.toCharArray(filename, bufSize);

         //filename = "gps_" + s_year;
         //filename =  filename +  "-" + s_month + "-" + s_day + "_" + s_hour + "-" + s_minute + "-" + s_second + ".txt";
         timeOK = true;
         Serial.println("FILENAME");
         Serial.println(filename);
         writeHeaders();
         last_minute = -1;
        }
      } else {
        //Buzz until GPS is OK
        buzz(5,5);
      }

    if (timeOK) {
        //We make the arduino sleep for 16s, each 8 seconds the arduino
        //will wake up to put him to sleep again.
        sleep8sec(gps_interval);      

        //Buzz to tell the user data is logged
        buzz(200,100);
        Serial.println("APPEND");
        File gpsFile = SD.open(filename, FILE_WRITE);
        if (gpsFile) {
          //Latitude / Longitude
          gpsFile.print(gps.location.lat(), 6);
          gpsFile.print(F(","));
          gpsFile.print(gps.location.lng(), 6);
          gpsFile.print(F(","));
          gpsFile.print(gps.altitude.meters());

          //Time
          gpsFile.print(F(","));
          gpsFile.print(s_day);
          gpsFile.print(F("/"));
          gpsFile.print(s_month);
          gpsFile.print(F("/"));
          gpsFile.print(s_year);
          gpsFile.print(" ");
          gpsFile.print(s_hour);
          gpsFile.print(F(":"));
          gpsFile.print(s_minute);
          gpsFile.print(F(":"));
          gpsFile.print(s_second);

          //Speed
          gpsFile.print(F(","));

          gpsFile.print(gps.speed.kmph());
          gpsFile.println();
        } else {
          Serial.println("SD : ERROR WRITING");
        }
        gpsFile.close();
        last_minute = gps.time.minute();
      } else {
        buzz(5,5); //Searching satellite
    }
  } else {
     buzz(600,1000); // No SD card panic
  }

}

void buzz(int freq,int waitTime){
  tone(buzzer,freq);
  delay(waitTime);
  noTone(buzzer);
  delay(waitTime); 
  digitalWrite(13,LOW);
}
