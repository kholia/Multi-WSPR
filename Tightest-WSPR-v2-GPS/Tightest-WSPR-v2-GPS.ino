#include <Wire.h>
#include <si5351.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <JTEncode.h>

#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <stdlib.h>  // rand()

// Si5351 stuff
Si5351 si5351;
int32_t si5351CalibrationFactor = 0;

int GPStoLOCATOR(float gps_long, float gps_lat, char *locator);

// GPS stuff
#include <TinyGPS++.h>
TinyGPSPlus gps;

uint64_t frequency0 = 28126075 * 100ULL;  // Works!
uint8_t tones[255];
int toneDelay;
int symbolCount;
double toneSpacing;

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// WSPR properties
#define WSPR_TONE_SPACING 146  // ~1.46 Hz
// #define WSPR_DELAY 683  // Delay value for WSPR
char call[] = "VU2BGS";   // CHANGE THIS PLEASE!
char loc[8];
char hardcoded_loc[] = "MK83";       // https://www.levinecentral.com/ham/grid_square.php
uint8_t dbm = 32;          // 2W ERP, https://rfzero.net/examples/beacons/wspr-cw/
JTEncode jtencode;
#define TONE_DELAY_ACCURATE 682667UL

// LED
#undef RGB_BRIGHTNESS
#define RGB_BRIGHTNESS 10 // Change white brightness (max 255)
// the setup function runs once when you press reset or power the board
#ifdef RGB_BUILTIN
#undef RGB_BUILTIN
#endif
#define RGB_BUILTIN 21

void tx() {
  uint8_t i;
  unsigned long starttime;

  // Serial.println("TX!");
  // digitalWrite(LED_BUILTIN, HIGH);
  neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, RGB_BRIGHTNESS);

  si5351.set_clock_pwr(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK0, 1);
  // int value = rand() % (100 + 1);  // Generate a random number between 0 and 100

  for (i = 0; i < symbolCount; i++) {
    starttime = micros();
    // si5351.set_freq(frequency0 + (value * 100ULL) + int(tones[i] * toneSpacing), SI5351_CLK0);
    si5351.set_freq(frequency0 + int(tones[i] * toneSpacing), SI5351_CLK0);
    while ((micros() - starttime) < TONE_DELAY_ACCURATE) {};
  }

  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);
  si5351.output_enable(SI5351_CLK0, 0);
  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, RGB_BRIGHTNESS);
}

int GPStoLOCATOR(float gps_long, float gps_lat, char *locator);

void sync_time_with_gps_with_timeout() {
  // digitalWrite(IS_GPS_SYNCED_PIN, LOW);
  bool newData = false;

  Serial.println("GPS Sync Wait...");
  digitalWrite(LED_BUILTIN, LOW);

  Serial1.begin(9600, SERIAL_8N1, 12, 13);

  for (unsigned long start = millis(); millis() - start < 64000;) {
    while (Serial1.available()) {
      char c = Serial1.read();
      Serial.write(c); // Debug GPS stuff
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
    if (newData && gps.time.isUpdated() && gps.date.isUpdated() && (gps.location.isValid() && gps.location.age() < 2000)) {
      byte Year = gps.date.year();
      byte Month = gps.date.month();
      byte Day = gps.date.day();
      byte Hour = gps.time.hour();
      byte Minute = gps.time.minute();
      byte Second = gps.time.second();
      setTime(Hour, Minute, Second, Day, Month, Year);
      Serial.println(gps.location.lng());
      Serial.println(gps.location.lat());
      GPStoLOCATOR(gps.location.lng(), gps.location.lat(), loc);
      Serial.println(loc);
      loc[4] = 0;  // NULL terminate early
      if (!strcmp(loc, "AA00")) {
        strcpy(loc, hardcoded_loc);
      }
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      Serial.println("GPS Sync Done!");
      return;
    }
  }
  Serial.println("GPS Sync Failed!");
}

void resync_time_with_gps_with_timeout() {
  bool newData = false;

  Serial.println("GPS Resync Wait...");
  digitalWrite(LED_BUILTIN, LOW);

  for (unsigned long start = millis(); millis() - start < 64000;) {
    while (Serial1.available()) {
      char c = Serial1.read();
      if (gps.encode(c))  // Did a new valid sentence come in?
        newData = true;
    }
    if (newData && gps.time.isUpdated() && gps.date.isUpdated() && (gps.location.isValid() && gps.location.age() < 2000)) {
      byte Year = gps.date.year();
      byte Month = gps.date.month();
      byte Day = gps.date.day();
      byte Hour = gps.time.hour();
      byte Minute = gps.time.minute();
      byte Second = gps.time.second();
      setTime(Hour, Minute, Second, Day, Month, Year);
      GPStoLOCATOR(gps.location.lng(), gps.location.lat(), loc);
      Serial.println(loc);
      loc[4] = 0;  // NULL terminate early
      if (!strcmp(loc, "AA00")) {
        strcpy(loc, hardcoded_loc);
      }
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      Serial.println("GPS Resync Done!");
      return;
    }
  }
  Serial.println("GPS sync Failed!");
}


void setup() {
  int ret = 0;

  // Setup I/O pins
  Serial.begin(115200);
  // delay(7000);
  Serial.println("Hello!");

  neopixelWrite(RGB_BUILTIN, 0, 0, 0);

  // I2C pins
  Wire.setPins(1, 2);
  Wire.begin();

  // Initialize the Si5351
  ret = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 26000000, si5351CalibrationFactor);  // I am using a 26 MHz TCXO!
  if (ret != true) {
    // led_flash();
    // watchdog_reboot(0, 0, 1000);
  }

  // Set CLK0 output
  // si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); // Set for minimum power as we have an external amplifier
  si5351.set_clock_pwr(SI5351_CLK0, 0);  // safety first

  // Sync. time with GPS module
  sync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();
  delay(1000);
  resync_time_with_gps_with_timeout();

  // Prep for WSPR
  jtencode.wspr_encode(call, loc, dbm, tones);
  // toneDelay = WSPR_DELAY;
  // toneSpacing = WSPR_TONE_SPACING;
  toneSpacing = 146.5;  // The WSPR tone spacing is 375/256 Hz or 1.465 Hz
  symbolCount = WSPR_SYMBOL_COUNT;

  neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, RGB_BRIGHTNESS); // WHITE - ALL OK!
}

#define SHOW_TIME_PERIOD 1000

void loop() {
  if (minute() % 2 == 0 && second() % 60 == 0) {
    // empirically calculated start delay
    delay(400);
    tx();
    resync_time_with_gps_with_timeout();
  }

  delay(10);
}
