
// Using SPI bus for multiple devices
// https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi

/*
 * UBLOX Neo 6m 9600 Baud
 * VCC - BAT
 * TX  - RxPin or RX1 
 * RX  - TxPin or TX0
 * GND
 */
#include <TinyGPS++.h>
TinyGPSPlus gps;

#define SDCARD
#ifdef SDCARD
  #include <SPI.h>
  #include <SD.h>
#endif

/*
 * SDA - A4
 * SCL - A5
 * GND
 * VCC - BAT
 */
#define OLED
#ifdef OLED
  #include "U8glib.h"
  U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST);  // Fast I2C / TWI
#endif

/*
 * BMP180
 * SDA - A4
 * SLC - A5
 * GND
 * VCC - BAT
 */
//#define BMP180
#ifdef BMP180
  #include <Adafruit_BMP085.h>
  Adafruit_BMP085 bmp;
#endif

const int RXPin = 3, TXPin = 4;
const uint32_t GPSBaud = 9600;
const int wayPointButton = 2;
const int ledPin = 6;
float batteryVoltage = 0;
String dataString = "";
int doLog = 0;

// The serial connection to the GPS device
#define HWSERIAL
#ifndef HWSERIAL
  #include <SoftwareSerial.h>
  SoftwareSerial ss(RXPin, TXPin);
#endif

void setup() {

  pinMode(wayPointButton, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(A0, INPUT);

  #ifdef SDCARD
    SD.begin(10);
  #endif
    
  #ifdef BMP180
    if (! bmp.begin()) {
      while (1) {}
    }
  #endif
   
  // Either GPS or for debugging
  Serial.begin(GPSBaud);

  // If using Software Serial (RXPin/TXPin) we can output debugging information on hardware serial via FTDI
  #ifndef HWSERIAL
    ss.begin(GPSBaud);
    Serial.print("Setup");
  #endif

  return;
  // Wait for GPS lock
  int page = 0;
  while(! gps.location.isValid()) {
    // Print page++ up to 3-4 pages and repeat
    if (page > 2) page = 0;
    draw(page++);
    smartDelay(3000);
  }
}

void draw(int page) {
  #ifndef OLED
    return;
  #endif

  u8g.firstPage();
  do {
    switch (page) {
      case 0:
        u8g.setColorIndex(1);
        u8g.setFont(u8g_font_7x13);
        u8g.setPrintPos(0, 13);
        u8g.print("gpstag v0.8.1");
        u8g.setPrintPos(0, 35);
        u8g.print("got satellites? ...");
        break;
      case 1:
        u8g.setPrintPos(0, 50);
        u8g.print("Bueller? ...");
        break;
      case 2:  
        u8g.setPrintPos(0, 50);
        u8g.print("Fry? ..."); 
        break;
      case 9:   
        for (int i=1; i<=gps.satellites.value(); i++) {
          if (i % 2 == 0) {
           u8g.drawBox(i*5, 0, 3, 15);  
          } else {
           u8g.drawBox(i*5, 4, 3, 11); 
          }
        }

        // Last battery voltage reading
        u8g.setPrintPos(90, 15);
        u8g.print(batteryVoltage);
        u8g.print("V");
        u8g.setPrintPos(0, 30);
        u8g.print(gps.location.lat(), 6);
        u8g.setPrintPos(0, 43);
        u8g.print(gps.location.lng(), 6);
        u8g.setPrintPos(0, 55);
        #ifdef BMP180
          u8g.print("bmp:");
          u8g.print((int) (bmp.readAltitude(103200) * 3.28084));
        #endif
      
        u8g.print(" blox:");
        u8g.print((int) gps.altitude.feet());
        break;
    }
  } while( u8g.nextPage() );
}

void loop() {

  // TODO: Log GPS lat/long elevation timestamp to sdcard
  // Review GPS standards for logging https://en.wikipedia.org/wiki/GPS_Exchange_Format
  
  dataString = String(gps.location.lat(), 6);
  dataString += ',';
  dataString += String(gps.location.lng(), 6);
  dataString += ',';
  dataString += String(gps.altitude.feet());
  dataString += ',';
  char GPSDate[10];
  TinyGPSDate d = gps.date;
  sprintf(GPSDate, "%02d-%02d-%02d", d.year(), d.month(), d.day());
  dataString += String(GPSDate);
  TinyGPSTime t = gps.time;
  char GPSTime[8];
  sprintf(GPSTime, " %02d:%02d:%02d ", t.hour(), t.minute(), t.second());
  dataString += String(GPSTime)  ;
      
  if (digitalRead(wayPointButton) == HIGH) {
    // turn LED on so we know the waypoint is getting logged
    digitalWrite(ledPin, HIGH);
    doLog++;
    dataString += ",1";
    
    // TODO: Log waypoint to sdcard
    #ifndef HWSERIAL
      Serial.println(gps.location.lat());
      Serial.println(gps.location.lng());
    #endif  
  }

  #ifdef SDCARD
    if (doLog) {
      File dataFile = SD.open("tracking.txt", FILE_WRITE);
      dataFile.println(dataString);
      dataFile.close();
    }
  #endif

  // Voltage divider circuit R1 = 2K R2 = 5.1K 3.02V at 4.2 volts
  // http://tinyurl.com/y82a9s59
  // Try to smooth out the reading using our loop delay here
  batteryVoltage = 0;
  for(int i=0; i<10; i++) {
    batteryVoltage += (analogRead(A0) * 3.3 /1024.0) * (2000 + 5100) / 5100;
    smartDelay(200);
  }
  batteryVoltage = batteryVoltage / 10;

  // Draw main screen
  draw(9);
  
  // LED Off
  digitalWrite(ledPin, LOW); 
}

// This custom version of delay() ensures that the gps object is being "fed" data while we wait.
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    #ifndef HWSERIAL
      while(ss.available()) {
        gps.encode(ss.read());
      }
    #else
      while(Serial.available()) {
        gps.encode(Serial.read());
      }
    #endif
  }
}



