#include <MPU6050_tockn.h>
#include <Wire.h>

MPU6050 mpu6050(Wire);

#define OLED
//#define BMP180
#define HWSERIAL

// Using SPI bus for multiple devices
// https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi

#include <TinyGPS++.h>

/*
 * SDA - A4
 * SCL - A5
 * GND
 * VCC - RAW
 */
#ifdef OLED
  #include "U8glib.h"
  float volts = 0;         // Voltage reading
  const float ARef = 3.3;  // 3.3v reference default for 3.3v mini
  const int R1 = 100;      // 100K
  const int R2 = 100;      // 100K
#endif

/*
 * BMP180
 * SDA - A4
 * SLC - A5
 * GND
 * VCC - RAW
 */
#ifdef BMP180
  #include <Adafruit_BMP085.h>
#endif

/*
 * UBLOX Neo 6m 9600 Baud
 * VCC - 3.3v
 * TX  - RxPin or RX1 
 * RX  - TxPin or TX0
 */

static const int RXPin = 3, TXPin = 4;
static const uint32_t GPSBaud = 9600;
static const int wayPointButton = 2;
static const int ledPin = 6;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
// TODO: Change to hardware serial
#ifndef HWSERIAL
  #include <SoftwareSerial.h>
  SoftwareSerial ss(RXPin, TXPin);
#endif

// Our oled display
#ifdef OLED
  // U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST
  U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST);  // Fast I2C / TWI
  //U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g(U8G2_R0);
#endif

#ifdef BMP180
  // Barameter for temp, pressure, altitude
  Adafruit_BMP085 bmp;
#endif

void setup() {

  pinMode(wayPointButton, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(A0, INPUT);
  
  #ifdef BMP180
    if (! bmp.begin()) {
      while (1) {}
    }
  #endif
   
  #ifdef OLED
    u8g.setColorIndex(1);
    u8g.setFont(u8g_font_7x13);
  #endif

  // Either GPS or for debugging
  Serial.begin(GPSBaud);

  // If using Software Serial (RXPin/TXPin) we can output debugging information
  #ifndef HWSERIAL
    ss.begin(GPSBaud);
    Serial.print("Setup");
  #endif  

  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(false);
}

void loop() {

  if (digitalRead(wayPointButton) == HIGH) {
    // turn LED on so we know the waypoint is getting logged
    digitalWrite(ledPin, HIGH);
    
    // TODO: Log waypoint to sdcard
    #ifndef HWSERIAL
      Serial.println(gps.location.lat());
      Serial.println(gps.location.lng());
    #endif 
    delay(1000); // Delay to indicate we logged waypoint 
  }
  
  // TODO: Log GPS lat/long elevation timestamp to sdcard
  // Review GPS standards for logging https://en.wikipedia.org/wiki/GPS_Exchange_Format

   mpu6050.update();
   
  #ifdef OLED
    u8g.firstPage();
    do {
      // Box for each satellite
      for (int i=0; i<gps.satellites.value(); i++) {
        u8g.drawBox(i*5, 0, 3, 15);
      }

      volts = (analogRead(A0)*ARef/1024.0) * (R1+R2)/R2 * 1.008;
     
      u8g.setPrintPos(90, 15);
      u8g.print(volts);
      u8g.print("V");
      
      u8g.setPrintPos(0, 30);
      u8g.print(gps.location.lat(), 6);
    
      u8g.setPrintPos(0, 43);
      u8g.print(gps.location.lng(), 6);
      
      u8g.setPrintPos(0, 55);

      u8g.print("gyro:");
      u8g.print((int) mpu6050.getGyroX());
      
      
      #ifdef BMP180
        u8g.print("bmp:");
        u8g.print((int) (bmp.readAltitude(103200) * 3.28084));
      #endif
      
      u8g.print(" blox:");
      u8g.print((int) gps.altitude.feet());
    } while( u8g.nextPage() );
  #endif

  digitalWrite(ledPin, LOW); 
  smartDelay(1800);
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



