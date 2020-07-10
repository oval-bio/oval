/*
  CO2 spirometer
  Reads differential pressure data on an analog voltage on pin A0.
  Converts that to flow rate based on the spirometer air channel sizes.
  Reads CO2 concentration from a sensor attached to a software serial port on pin 12.
  Please note, while this code is functional it should be considered a beta.
  It was tested on atmega328 based Arduinos (Uno, Leonardo, Duemilanova, etc)
  This example code is in the public domain.
*/

//differential air pressure sensor input should be connected to A0

#include <SoftwareSerial.h>
SoftwareSerial mySerial(12, 11); // RX, TX

unsigned char hexdata[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

#include <U8x8lib.h>

//oled with v+ gnd scl sda pinout plugs into 7654
U8X8_SSD1306_128X64_NONAME_SW_I2C oled(/* clock=*/ 5, /* data=*/ 4, /* reset=*/ U8X8_PIN_NONE);
//  pinMode(7, 1); digitalWrite(7, 1); pinMode(6, 1); digitalWrite(6, 0); delay(100); //copy this line uncommented into setup()
//U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);  //for hardware i2c

int FlipMode = 1;
int ms_wait = 100 ;
int sensorValue;
float calibration_voltage, voltage, v2, kpa, lps, pa; //psi;
float rho = 1.225; // Density of air in kg/m3
float area_1 = 0.000283528736986; // Surface area in m2 float // larger 9.5mm/1000 = .0095 * .0095 * 3.14159 = 0.000283528736986 m^2
float area_2 = 0.000070882184247; // Surface area in m2 float // smaller 4.75mm/1000 = 0.00475 * 0.00475 * 3.14159 = 0.000070882184247   cm^2
long CO2 = 0; //add 1 min warmup delay?

float previous_breath_volume;
long last_breath_second;
long last_lull_second;
float current_breath_volume = 0;
int else_ran = 0;
byte pchars[] = {0, 0, 0, 0, 0};
int idx;
int ch;
long hi, lo;
long sensor_init = 0;
long old_tenth = 0;
long mills = millis();
long current_second = mills / 1000;
int current_tenth = (mills / 100) - (current_second * 10)  ;

void setup() {
  Serial.begin(115200); delay(100);
  while (!Serial) { // wait for serial port to connect. Needed for native USB port only
  }
  //Serial.println("\r\n<sketch> CO2_spirometer: </sketch>");
  mySerial.begin(9600); delay(100);
  mySerial.write(hexdata, 9); sensor_init = (millis() + 1000);

  pinMode(A0, 0);      pinMode(A1, 1);      pinMode(A2, 1);      pinMode(A3, 0);      //pinMode(A4, 1);
  digitalWrite(A0, 0); digitalWrite(A1, 1); digitalWrite(A2, 0); //digitalWrite(A3, 0); digitalWrite(A4, 1);
  delay(250);
  calibration_voltage = analogRead(A3) * 5.0 / 1023.0;

  //set pins to power oled
  pinMode(7, 1); digitalWrite(7, 1); pinMode(6, 1); digitalWrite(6, 0); delay(100);
  //begin display driver, init setup co2 and v/lps static elements
  oled.begin(); oled.setPowerSave(0); oled.setFlipMode(FlipMode); oled.setFont(u8x8_font_chroma48medium8_r);

  init_display();
  //https://github.com/olikraus/u8g2/wiki/u8x8reference
  //https://github.com/olikraus/u8g2/wiki/fntlist8x8  font list
}

void init_display() { //init static elements
  oled.setCursor( 0, 1); oled.print( "ms:" );
  oled.setCursor( 3, 1); oled.print( ms_wait );
  oled.setCursor( 0, 0); oled.print( "CO2 ppm:" );
  oled.setCursor(0, 4); oled.print("A2:");
  oled.setCursor(8, 3); oled.print("lpb:");
  oled.setCursor(0, 6); oled.print("kp");
  oled.setCursor(8, 6); oled.print("lps:");
}

void loop() {
  if ( millis() > ( sensor_init + 600 ) ) {
    read_CO2();
  }
  mills = millis(); current_second = mills / 1000; current_tenth = (mills / 100) - (current_second * 10);
  if ( (old_tenth != current_tenth)  ) {
    old_tenth = current_tenth;
    check_airflow();
    calculate_airflow();
    serial_report();
  }
  //spread out screen updates to avoid missing serial data
  if ( (current_tenth == 0) ) {
    oled.setCursor( 8, 1); oled.print( CO2 ); oled.print( "  " );
  }
  if ( (current_tenth == 2) ) {
    oled.setCursor(12, 6); oled.print( lps );
  }
  if ( (current_tenth == 4) ) {
    oled.setCursor(3, 4); oled.print( voltage );
  }
  if ( (current_tenth == 6) ) {
    oled.setCursor(3, 6); oled.print( kpa );
  }
  if ( (current_tenth == 7) ) {
    oled.setCursor(12, 6); oled.print( lps );
  }
  if ( (current_tenth == 9) ) {
    oled.setCursor(12, 3); oled.print( ( previous_breath_volume / 10 ) );
  }
}//void loop()


void parse_char() {
  if (millis() > 50000) {
    //delay(50000);
  }
  //delay(25);
  //Serial.println("parse_char:");
  if ( millis() > ( sensor_init + 600 ) ) {
    //Serial.print("tick: "); Serial.println(sensor_init);
    mySerial.write(hexdata, 9); //sensor_init = millis();
    if (mySerial.available() > 0)
    {
      ch = mySerial.read();
      //Serial.print("ch:"); Serial.print(ch); Serial.print(" idx:"); Serial.println(idx);
      if ( idx == 8 ) {
        idx = 0; hi = pchars[2]; lo = pchars[3]; CO2 = ( (hi * 256) + lo );
        mySerial.write(hexdata, 9); sensor_init = millis();
        Serial.print("co2 "); Serial.println(CO2);
      }
      if (  (idx > 4 ) && (idx < 8 )  ) {
        pchars[idx] = ch;
        idx++;
      }
      if ( ( idx == 4 ) && ( ( ch == 69 ) || ( ch == 70 ) ) ) {
        idx++;
        //hi = pchars[2]; lo = pchars[3]; CO2 = ( (hi * 256) + lo );
        //mySerial.write(hexdata, 9); sensor_init = millis();
        //Serial.print("co2 "); Serial.println(CO2);
      }
      if (idx == 3 ) {
        pchars[idx] = ch;
        idx++;
      }
      if (idx == 2 ) {
        pchars[idx] = ch;
        idx++;
      }
      if ( ( idx == 1 ) && ( ch == 134 ) ) {
        pchars[idx] = ch;
        idx++;
      }
      if ( (idx == 0 ) && ( ch == 255 ) ) {
        pchars[idx] = ch;
        idx++;
      }

    }
  }
}


//should do it this way?
void read_CO2() {
  //Serial.println("read_CO2()"); delay(20);
  for (int i = 0, j = 0; i < 9; i++) {
    if (mySerial.available() > 0)
    {
      ch = mySerial.read();
      if (i == 2) {
        hi = ch;    //High concentration
      }
      if (i == 3) {
        lo = ch;    //Low concentration
      }
      if (i == 8) {
        CO2 = hi * 256 + lo; //CO2 concentration
      }
    }
  }
  mySerial.write(hexdata, 9); //delay(600);
  sensor_init = ( millis() );
  //Serial.println("CO2 read");
}//void check_CO2

//seems to work
void read_CO2_works() {
  //Serial.println("read_CO2()"); delay(20);
  mySerial.write(hexdata, 9); //delay(600);
  sensor_init = (millis() + 1000);
  for (int i = 0, j = 0; i < 9; i++) {
    if (mySerial.available() > 0)
    {
      ch = mySerial.read();
      if (i == 2) {
        hi = ch;    //High concentration
      }
      if (i == 3) {
        lo = ch;    //Low concentration
      }
      if (i == 8) {
        CO2 = hi * 256 + lo; //CO2 concentration
      }
    }
  }
  //Serial.println("CO2 read");
}//void check_CO2

void check_airflow() {
  //Serial.println("check_airflow()");
  sensorValue = analogRead(A3); float voltage = sensorValue * 5.0 / 1023.0; float v2 = voltage - calibration_voltage;
  if ( v2 < 0.01 ) {
    v2 = 0;
  }
  // Voltage read in (0 to 1023) voltage = inputvolt * voltage_source_5v * 1023
  //convert voltage to psi  2.53v=0p range .25 to 4.75 //.25v = -25kpa  4.75 = 25kpa // 2.25v = 25kpa
  kpa = v2 * 25 / 2.25;  pa = v2 * 25 * 1000 / 2.25; //psi = v2 * 25 * .145 / 2.25;
  float massFlow = 1000 * sqrt( (abs(pa) * 2 * rho) / ((1 / (pow(area_2, 2))) - (1 / (pow(area_1, 2))))); // Mass flow of air
  float volFlow = massFlow / rho; // Volumetric flow of air
  lps = volFlow / 10;
}

void calculate_airflow() {
  if ( lps > 0.1 ) {
    //Serial.print("lps > 0: ");Serial.println( ( lps + current_breath_volume ) );
    current_breath_volume = ( lps + current_breath_volume );
    else_ran = 0;
  } else {
    if ( else_ran == 0 ) {
      else_ran = 1;
      previous_breath_volume = current_breath_volume;
      current_breath_volume = 0;
    }
  }
}

void serial_report() {
  //Serial.println("serial_report()");
  Serial.print( lps * 1000 ); Serial.print(", ");
  Serial.print(CO2); Serial.print(", ");
  Serial.print( ( 100 * current_breath_volume ) );  Serial.print(", ");
  Serial.println( ( 100 * previous_breath_volume ) );
}
