// https://learn.sparkfun.com/tutorials/voltage-dividers
// https://forum.arduino.cc/index.php?topic=420527.0

float volts = 0;         // Voltage reading
const float ARef = 3.3;  // 3.3v reference default for 3.3v mini
const int R1 = 100;      // 100K
const int R2 = 100;      // 100K
//const int A0 = 0;        // Analog Pin 0

void setup() {
  // Sets internal refernece to 1.1v.  Would need 1/4 divider for 1 cell lipo
  //analogReference(INTERNAL);
  
  Serial.begin(115200);
  Serial.println("Starting Voltage Reader");
}

void loop() {
  // put your main code here, to run repeatedly:
  volts = (analogRead(A0)*ARef/1024.0) * (R1+R2)/R2 * 1.008;
  Serial.println(volts);
  delay(2000);
}
