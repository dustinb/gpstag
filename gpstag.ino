
#define OLED
#define BMP180

// Using SPI bus for multiple devices
// https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi

#include <TinyGPS++.h>

#include <SoftwareSerial.h>

#ifdef OLED
  #include "U8glib.h"
#endif

#ifdef BMP180
  #include <Adafruit_BMP085.h>
#endif

# define DEBUG

/*
 * UBLOX Neo 6m 9600 Baud
 * VCC - 3.3v
 * TX  - RxPin
 * RX  - TxPin
 */
static const int RXPin = 3, TXPin = 4;
static const uint32_t GPSBaud = 9600;
static const int wayPointButton = 2;
static const int ledPin = 6;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
// TODO: Change to hardware serial
#ifdef DEBUG
  SoftwareSerial ss(RXPin, TXPin);
#endif

// Our oled display
#ifdef OLED
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

  #ifdef BMP180
    if (! bmp.begin()) {
      while (1) {}
    }
  #endif
   
  #ifdef OLED
    u8g.setColorIndex(1);
    u8g.setFont(u8g_font_6x12);
  #endif
  
  ss.begin(GPSBaud);
  Serial.begin(9600);
  Serial.print("Setup");
}

void loop() {

  // Blink LED to indicate loop
  //digitalWrite(ledPin, HIGH);
  
  if (digitalRead(wayPointButton) == HIGH) {
    // turn LED on so we know the waypoint is getting logged
    digitalWrite(ledPin, HIGH);
    delay(1000);
    // TODO: Log waypoint to sdcard
    #ifdef DEBUG
      Serial.print(gps.location.lat());
      Serial.print(gps.location.lng());
      Serial.println();
    #endif  
  } else {
    //delay(50);
  }
  digitalWrite(ledPin, LOW); 

  // TODO: Log GPS lat/long elevation timestamp to sdcard
  // Review GPS standards for logging https://en.wikipedia.org/wiki/GPS_Exchange_Format

  #ifdef OLED
    u8g.firstPage();
    do {
      for (int i=0; i<gps.satellites.value(); i++) {
        u8g.drawBox(i*5, 0, 3, 15);
        //u8g.drawVLine((i+1) * 2, 7-i, i <= 8 ? (i+1) * 2 : 16);
      }
      
      //u8g.drawStr( 4, 12, numSats);
      //u8g.setCursor(0, 30);
      u8g.setPrintPos(0, 30);
      u8g.print(gps.location.lat(), 6);
      //u8g.setCursor(0, 42);
      u8g.setPrintPos(0, 42);
      u8g.print(gps.location.lng(), 6);
      //u8g.setCursor(0, 54);
      u8g.setPrintPos(0, 54);
      
      #ifdef BMP180
        u8g.print("bmp:");
        u8g.print((int) (bmp.readAltitude(103200) * 3.28084));
      #endif
      
      u8g.print(" ublox:");
      u8g.print((int) gps.altitude.feet());
    } while( u8g.nextPage() );
  #endif
  
  Serial.print(gps.location.lat());
  digitalWrite(ledPin, LOW);
  smartDelaySW(3000);

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    //Serial.println(F("No GPS data received: check wiring"));
  }
   
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelayHW(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial.available())
      gps.encode(Serial.read());
  } while (millis() - start < ms);
}

static void smartDelaySW(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}









