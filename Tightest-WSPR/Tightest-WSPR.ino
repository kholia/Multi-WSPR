#include <Wire.h>
#include <si5351.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <JTEncode.h>

#include <stdlib.h> // rand()

// WiFI
#include <WiFi.h>
#include "ESPNtpClient.h"
const char *ssid = "home";
const char *password = "XXXX";

// Si5351 stuff
Si5351 si5351;
int32_t si5351CalibrationFactor = 0;

// long long frequency0 = 14097050 * 100UL; // CHANGE THIS PLEASE
uint64_t frequency0 = 28126075 * 100ULL; // CHANGE THIS PLEASE
uint8_t tones0[255];
int toneDelay;
int symbolCount;
double toneSpacing;

// WSPR properties
#define WSPR_TONE_SPACING 146 // ~1.46 Hz
// #define WSPR_DELAY 683    // Delay value for WSPR
char call0[] = "VU3CER"; // CHANGE THIS PLEASE!
char loc[] = "MK68";
uint8_t dbm = 33; // 2W ERP, https://rfzero.net/examples/beacons/wspr-cw/
JTEncode jtencode;
#define TONE_DELAY_ACCURATE 682667UL

// LED
#include <Adafruit_NeoPixel.h>
#define PIN 21
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

void tx()
{
  uint8_t i;
  unsigned long starttime;

  // Serial.println("TX!");
  // digitalWrite(LED_BUILTIN, HIGH);
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(255, 0, 255));
  pixels.show();
  si5351.set_clock_pwr(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK0, 1);
  int value = rand() % (100 + 1);  // Generate a random number between 0 and 100

  for (i = 0; i < symbolCount; i++) {
    starttime = micros();
    si5351.set_freq(frequency0 + (value * 100ULL) + int(tones0[i] * toneSpacing), SI5351_CLK0);
    // delay(toneDelay);
    // delay(toneDelay - (t2 - t1));
    while ((micros() - starttime) < TONE_DELAY_ACCURATE) {};
  }

  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);
  si5351.output_enable(SI5351_CLK0, 0);
  // digitalWrite(LED_BUILTIN, LOW);
  pixels.clear();
  pixels.show();
}

// Debug helper
void led_flash()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
}

void setClock() {
  NTP.setTimeZone(TZ_Etc_UTC);
  NTP.begin("time.google.com");

  // Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  // We just need the minute and the second fields to be accurate
  // setTime(hr, min, sec, day, mnth, yr);
  // https://cplusplus.com/reference/ctime/tm/
  // https://github.com/PaulStoffregen/Time/blob/master/TimeLib.h#L34
  setTime(now);
  // Serial.print("Current time: ");
  // Serial.print(asctime(&timeinfo));
  // Serial.println(timeStatus());
}

void setup() {
  int ret = 0;

  // Setup I/O pins
  pixels.begin();
  pixels.setBrightness(50);
  pixels.clear();
  pixels.show();
  // Serial.begin(115200);
  // delay(7000);
  // Serial.println("Hello!");

  // I2C pins
  // Wire.setSDA(12);
  // Wire.setSCL(13);
  Wire.setPins(1, 2);
  Wire.begin();

  // Initialize the Si5351
  ret = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 26000000, si5351CalibrationFactor);  // I am using a 26 MHz TCXO!
  if (ret != true) {
    led_flash();
    // watchdog_reboot(0, 0, 1000);
  }

  // Set CLK0 output
  // si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); // Set for minimum power as we have an external amplifier
  si5351.set_clock_pwr(SI5351_CLK0, 0); // safety first

  // WiFI + NTP (https://arduino-pico.readthedocs.io/en/latest/wifintp.html)
  WiFi.begin(ssid, password);
  setClock();

  // Prep for WSPR
  jtencode.wspr_encode(call0, loc, dbm, tones0);
  // toneDelay = WSPR_DELAY;
  // toneSpacing = WSPR_TONE_SPACING;
  toneSpacing = 146.5; // The WSPR tone spacing is 375/256 Hz or 1.465 Hz
  symbolCount = WSPR_SYMBOL_COUNT;

  pixels.setPixelColor(0, pixels.Color(0, 255, 0));
  pixels.show();
}

#define SHOW_TIME_PERIOD 1000

void loop() {
  // static int last = 0;

  /* if ((millis () - last) >= SHOW_TIME_PERIOD) {
    last = millis ();
    Serial.println (NTP.getTimeDateStringUs ());
    } */

  if (minute () % 4 == 0 && second() % 60 == 0)  {
    delay(300); // small compensating delay, empirically calculated
    tx();
    setClock();
  }

  delay(10);
}
