// Domino Clock using NTP and RTC module. Time corrected for DST automaticaly

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <time.h>
#include <simpleDSTadjust.h>
#include <Adafruit_NeoPixel.h>
#include <RTClib.h>
#include <Wire.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

RTC_DS1307 RTC;

// -------------- Configuration options -----------------

// Update time from NTP server every 5 hours
#define NTP_UPDATE_INTERVAL_SEC 5 * 3600

// Maximum of 3 servers
#define NTP_SERVERS "us.pool.ntp.org", "pool.ntp.org", "time.nist.gov"

//Define PINS for NeoPixel Displays
#define PIN_1           D4
#define PIN_2           D5
#define PIN_3           D6

//Define LED count of NeoPixel Displays
#define NUMpixels_1     14 //Hour
#define NUMpixels_2     14 //Tens Minute
#define NUMpixels_3     14 //Units

// Daylight Saving Time (DST) rule configuration
// Rules work for most contries that observe DST - see https://en.wikipedia.org/wiki/Daylight_saving_time_by_country for details and exceptions
// See http://www.timeanddate.com/time/zones/ for standard abbreviations and additional information
// Caution: DST rules may change in the future
//Uncomment your time zone or create your time zone
//
//#define timezone -9 // US Alaska Time Zone
//struct dstRule StartRule = {"AKDT", Second, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT -8 hours
//struct dstRule EndRule = {"AKST", First, Sun, Nov, 2, 0};       // Standard time = UTC/GMT -9 hour
//US Pacific Time Zone
#define timezone -8 // US Pacific Time Zone
struct dstRule StartRule = {"PDT", Second, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT -7 hours
struct dstRule EndRule = {"PST", First, Sun, Nov, 2, 0};       // Standard time = UTC/GMT -8 hour
//US Mountain Time Zone
//#define timezone -7 // US Mountain Time Zone
//struct dstRule StartRule = {"MDT", Second, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT -6 hours
//struct dstRule EndRule = {"MST", First, Sun, Nov, 2, 0};       // Standard time = UTC/GMT -7 hour
//US Central Time Zone
//#define timezone -6 // US Central Time Zone
//struct dstRule StartRule = {"CDT", Second, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT -5 hours
//struct dstRule EndRule = {"CST", First, Sun, Nov, 2, 0};       // Standard time = UTC/GMT -6 hour
//US Eastern Time Zone
//#define timezone -5 // US Eastern Time Zone
//struct dstRule StartRule = {"EDT", Second, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT -4 hours
//struct dstRule EndRule = {"EST", First, Sun, Nov, 2, 0};       // Standard time = UTC/GMT -5 hour

const char* ssid = "Your WiFi SSID";
const char* password = "Your WiFi Password";

int led = 4;
int pixels = 1;
int Tens, Units;
int R = 127; // SET RED PIXELS COLOR
int G = 127; // SET GREEN PIXELS COLOR
int B = 127; // SET BLUE PIXELS COLOR
int myARay[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}; // PIXELS ARRAY

Ticker ticker1;
int32_t tick;

// flag changed in the ticker function to start NTP Update
bool readyForNtpUpdate = true;

// Setup simpleDSTadjust Library rules
simpleDSTadjust dstAdjusted(StartRule, EndRule);

//Setup NeoPixel Displays
Adafruit_NeoPixel pixels_1 = Adafruit_NeoPixel(NUMpixels_1, PIN_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_2 = Adafruit_NeoPixel(NUMpixels_2, PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_3 = Adafruit_NeoPixel(NUMpixels_3, PIN_3, NEO_GRB + NEO_KHZ800);

void setup() {

  Wire.begin();
  RTC.begin();

  Serial.begin(115200);

  pixels_1.begin(); // Hours
  pixels_2.begin(); // Tens Minutes
  pixels_3.begin(); // Tens Units

  pixelsClear();

  //Connect to Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    WiFiConnectStatus();
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  updateNTP(); // Init the NTP time



  tick = NTP_UPDATE_INTERVAL_SEC; // Init the NTP update countdown ticker
  ticker1.attach(1, secTicker); // Run a 1 second interval Ticker
}

void loop()
{
  if (readyForNtpUpdate)
  {
    readyForNtpUpdate = false;

    char rtcBuf[30];
    DateTime now = RTC.now();
    Serial.print("\nRTC Time: ");
    sprintf(rtcBuf, "%02d/%2d/%4d %02d:%02d:%02d", now.month(), now.day(), now.year(), now.hour(), now.minute(), now.second());
    Serial.println(rtcBuf);
    updateNTP(); // Update time
    char buf[30];
    char *dstAbbrev;
    time_t t = dstAdjusted.time(&dstAbbrev);
    struct tm *timeinfo = localtime (&t);
    sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d %s\n", timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, dstAbbrev);
    Serial.print("NTP Time: ");
    Serial.print(buf);
  }
  displayTime();
  delay(2000);  // to reduce upload failures
}


void secTicker() // NTP timer update ticker
{
  tick--;
  if (tick <= 0)
  {
    readyForNtpUpdate = true;
    tick = NTP_UPDATE_INTERVAL_SEC; // Re-arm
  }
}

//Update NTP time and RTC module time
void updateNTP() {
  char *dstAbbrev;
  configTime(timezone * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr)) {
    Serial.print("#");
    delay(1000);
  }
  RTC.adjust(dstAdjusted.time(&dstAbbrev));
}

void pixelsClear()
{
  pixels_1.clear();
  pixels_2.clear();
  pixels_3.clear();
  pixels_1.show();
  pixels_2.show();
  pixels_3.show();
}

void WiFiConnectStatus()
{
  if (led == 1) { 
    led = 4; 
    pixels++;
    }
  switch (pixels) {
    case 1:
      pixels_1.setPixelColor(myARay[led], pixels_1.Color(R, G, B));
      pixels_1.show();
      led--;
      break;
    case 2:
      pixels_2.setPixelColor(myARay[led], pixels_2.Color(R, G, B));
      pixels_2.show();
      led--;
      break;
    case 3:
      pixels_3.setPixelColor(myARay[led], pixels_3.Color(R, G, B));
      pixels_3.show();
      led--;
      break;
    case 4:
      pixels = 1;
      pixelsClear();
      delay(1000);
      break;
  }
}
void displayTime()
{
  DateTime now = RTC.now();

  int hour_ = now.hour();

  Tens = (now.minute() / 10);
  Units = (now.minute() - (Tens * 10));

  if (hour_ > 12 && hour_ != 0) //Convert 24H format to 12H format
  {
    hour_ = hour_ - 12;
  }
  else if (hour_ == 0)
  {
    hour_ = 12;
  }

  switch (hour_) {
    case 1:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(10, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 2:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(10, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(3, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 3:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(10, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 4:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 5:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(10, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 6:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(3, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(10, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 7:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(1, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(5, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(10, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 8:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(1, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(5, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(8, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(12, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 9:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(1, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(3, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(5, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(8, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(12, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 10:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(1, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(3, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(5, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(8, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(10, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(12, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 11:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(1, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(3, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(5, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(8, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(9, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(11, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(12, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
    case 12:
      for (byte i = 0; i < 14; i = i + 1) {
        pixels_1.setPixelColor(myARay[i], pixels_1.Color(0, 0, 0));
        delay(5);
      }
      pixels_1.setPixelColor(0, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(1, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(2, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(4, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(5, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(6, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(7, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(8, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(9, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(11, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(12, pixels_1.Color(R, G, B));
      pixels_1.setPixelColor(13, pixels_1.Color(R, G, B));
      pixels_1.show();
      break;
  }
  //TENS UNITS
  switch (Units) {
    case 0:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.show();
      break;
    case 1:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(10, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 2:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(3, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(10, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 3:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(3, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(7, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(13, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 4:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(0, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(6, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(7, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(13, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 5:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(0, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(1, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(3, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(5, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(6, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 6:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(7, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(8, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(9, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(11, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(12, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(13, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 7:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(0, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(1, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(3, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(5, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(6, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(7, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(13, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 8:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(0, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(1, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(5, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(6, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(7, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(8, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(12, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(13, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
    case 9:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_3.setPixelColor(myARay[j], pixels_3.Color(0, 0, 0));
        delay(5);
      }
      pixels_3.setPixelColor(0, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(1, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(5, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(6, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(7, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(8, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(10, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(12, pixels_3.Color(R, G, B));
      pixels_3.setPixelColor(13, pixels_3.Color(R, G, B));
      pixels_3.show();
      break;
  }
  //TENS MINUTES
  switch (Tens) {
    case 0:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_2.setPixelColor(myARay[j], pixels_2.Color(0, 0, 0));
        delay(5);
      }
      pixels_2.show();
      break;
    case 1:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_2.setPixelColor(myARay[j], pixels_2.Color(0, 0, 0));
        delay(5);
      }
      pixels_2.setPixelColor(10, pixels_2.Color(R, G, B));
      pixels_2.show();
      break;
    case 2:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_2.setPixelColor(myARay[j], pixels_2.Color(0, 0, 0));
        delay(5);
      }
      pixels_2.setPixelColor(3, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(10, pixels_2.Color(R, G, B));
      pixels_2.show();
      break;
    case 3:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_2.setPixelColor(myARay[j], pixels_2.Color(0, 0, 0));
        delay(5);
      }
      pixels_2.setPixelColor(7, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(10, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(13, pixels_2.Color(R, G, B));
      pixels_2.show();
      break;
    case 4:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_2.setPixelColor(myARay[j], pixels_2.Color(0, 0, 0));
        delay(5);
      }
      pixels_2.setPixelColor(0, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(6, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(7, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(13, pixels_2.Color(R, G, B));
      pixels_2.show();
      break;
    case 5:
      for (byte j = 0; j < 14; j = j + 1) {
        pixels_2.setPixelColor(myARay[j], pixels_2.Color(0, 0, 0));
        delay(5);
      }
      pixels_2.setPixelColor(7, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(8, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(10, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(12, pixels_2.Color(R, G, B));
      pixels_2.setPixelColor(13, pixels_2.Color(R, G, B));
      pixels_2.show();
      break;
  }
  delay(2000);
}
