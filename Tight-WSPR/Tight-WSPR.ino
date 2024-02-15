// Runs on Raspberry Pi Pico W

#include <Wire.h>
#include <si5351.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <JTEncode.h>

// WiFI
#include <WiFi.h>
const char *ssid = "home";
const char *password = "XXX";
WiFiMulti multi;

// Si5351 stuff
Si5351 si5351;
int32_t si5351CalibrationFactor = 0;

// long long frequency0 = 14097050 * 100UL; // CHANGE THIS PLEASE
long long frequency0 = 28126075 * 100UL; // CHANGE THIS PLEASE
long long frequency1 = 14097075 * 100UL; // CHANGE THIS PLEASE
long long frequency2 = 14097125 * 100UL; // CHANGE THIS PLEASE
uint8_t tones0[255];
uint8_t tones1[255];
uint8_t tones2[255];
int toneDelay;
int symbolCount;
int toneSpacing;

// WSPR properties
#define WSPR_TONE_SPACING 146 // ~1.46 Hz
#define WSPR_DELAY 683    // Delay value for WSPR
char call0[] = "VU3CER"; // CHANGE THIS PLEASE!
char call1[] = "VU3FOE"; // CHANGE THIS PLEASE!
char call2[] = "VU2TFG"; // CHANGE THIS PLEASE!
char loc[] = "MK68";
uint8_t dbm = 20;
JTEncode jtencode;

#define TONE_DELAY_ACCURATE 682667UL

void tx()
{
  uint8_t i;
  unsigned long t1, t2, starttime;

  // Serial.println("TX!");
  digitalWrite(LED_BUILTIN, HIGH);
  si5351.set_clock_pwr(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK0, 1);

  for (i = 0; i < symbolCount; i++)
  {
    starttime = micros();
    si5351.set_freq(frequency0 + (tones0[i] * toneSpacing), SI5351_CLK0);
    // delay(toneDelay);
    // delay(toneDelay - (t2 - t1));
    while ((micros() - starttime) < TONE_DELAY_ACCURATE) {};
  }

  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);
  si5351.output_enable(SI5351_CLK0, 0);
  digitalWrite(LED_BUILTIN, LOW);
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
  NTP.begin("time.google.com");

  Serial.print("Waiting for NTP time sync: ");
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
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  Serial.println(timeStatus());
}

void setup() {
  int ret = 0;

  // Setup I/O pins
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(5000);

  // I2C pins
  Wire.setSDA(12);
  Wire.setSCL(13);
  Wire.begin();

  // Initialize the Si5351
  ret = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 26000000, si5351CalibrationFactor);
  if (ret != true) {
    led_flash();
    watchdog_reboot(0, 0, 1000);
  }

  // Set CLK0 output
  // si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); // Set for minimum power as we have an external amplifier
  si5351.set_clock_pwr(SI5351_CLK0, 0); // safety first

  // WiFI + NTP (https://arduino-pico.readthedocs.io/en/latest/wifintp.html)
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  multi.addAP(ssid, password);
  if (multi.run() != WL_CONNECTED) {
    Serial.println("Unable to connect to network, rebooting in 10 seconds...");
    delay(10000);
    rp2040.reboot();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  setClock();

  // Prep for WSPR
  jtencode.wspr_encode(call0, loc, dbm, tones0);
  jtencode.wspr_encode(call1, loc, dbm, tones1);
  jtencode.wspr_encode(call2, loc, dbm, tones2);
  toneDelay = WSPR_DELAY;
  toneSpacing = WSPR_TONE_SPACING;
  symbolCount = WSPR_SYMBOL_COUNT;
}

void loop() {
  if (minute () % 4 == 0 && second() % 60 == 0)  {
    delay(200); // small compensating delay
    tx();
  }

  delay(10);
}
