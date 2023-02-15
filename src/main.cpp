#include <stdint.h>
#include <LittleFS.h>
FS* filesystem =      &LittleFS;
#define FileFS        LittleFS
#define FS_Name       "LittleFS"

/* Including WiFi utilities */
#include <ESP8266WiFi.h>
#include <ESPAsyncDNSServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <ESPAsync_WiFiManager.h>
#include <SoftwareSerial.h>

#define HTTP_PORT           80

/* Philips2200 SPECIFIC DEFINES */
#if defined(ESP8266) && !defined(D5)
#define D5 (14) // Rx from display
#define D6 (12) // Tx not used
#define D7 (13) // Gnd for display (to switch it on and off)
#endif
/* Philips2200 SPECIFIC DEFINES END */

#define ASCII_ZERO 0x30

// WiFi
// const char *ssid = "Bbox-3DA63A9A"; /* Add your router's SSID */
// const char *password = "EQzC17ff95WvGq7hGD"; /*Add the password */

SoftwareSerial swSer (D5, D6);

/* 80 is the Port Number for HTTP Web Server */
AsyncWebServer webServer(HTTP_PORT);
AsyncDNSServer dnsServer;
ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer);
// WiFiManager wifiManager;

/* Philips2200 SPECIFIC VARIABLES */
// serial Input
uint8_t serInCommand[39];
uint8_t serInCommand_old[39];
unsigned long timestampLastSerialMsg;
uint8_t serInIdx = 0;

/* Status buttons */
uint8_t powerOn[] = {0xD5, 0x55, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x35, 0x05};
uint8_t powerOff[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x0D, 0x19};
uint8_t requestInfo[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x14};
uint8_t startPause[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x09, 0x10};

/* Coffe buttons */
uint8_t espresso[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x19, 0x0F};
uint8_t coffee[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x29, 0x3E};
uint8_t coffeePulver[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x19, 0x0D};
uint8_t coffeeWater[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x30, 0x27};

uint8_t hotWater[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x31, 0x23};
uint8_t steam[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x19, 0x04};
uint8_t aquaClean[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x1D, 0x14};
uint8_t calcNclean[] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x38, 0x15};


uint8_t oldStatus[19] = "";
/* Philips2200 SPECIFIC VARIABLES END */

/* FUNCTIONS PROTOTYPES */
uint8_t convertHexCharToInt(uint8_t _char);
/* FUNCTIONS PROTOTYPES END */

uint8_t *commandPointer = NULL;

void serialSend(uint8_t command[], int32_t sendCount)
{
  for (int i = 0; i <= sendCount; i++)
  {
    Serial.write(command, 12);
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  Serial.println("Starting the LittleFS Webserver..");
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  Serial.print("\n");
  // WiFi.mode(WIFI_STA); /* Configure ESP8266 in STA Mode */
  // WiFi.begin(ssid, password);
  // Serial.println("Connecting to network...");

  // int i = 0;
  // while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
  //   delay(500);
  //   Serial.print('.');
  // }
  // Serial.println();

  // Serial.print("[+] Connected to WiFi with IP : ");
  // Serial.println(WiFi.localIP());

  // Serial.print("\n");

  ESPAsync_wifiManager.startConfigPortal();

  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
  #ifndef DEBUG
    request->send(LittleFS, "/web/index.html", "text/html");
  #else
    request->send(LittleFS, "/web/debugIndex.html", "text/html");
  #endif
  });
  webServer.on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/web/css/style.css", "text/css"); });
  webServer.on("/js/globals.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/web/js/globals.js", "text/javascript"); });

  webServer.on("/pushed", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String buttonName = "None";

    if(request->hasParam("name"))
    {
      buttonName = request->getParam("name")->value();

      if(commandPointer == NULL)
      {
        if(buttonName == "1") commandPointer = espresso;
        else if(buttonName == "2") commandPointer = coffee;
        else if(buttonName == "3") commandPointer = coffeePulver;
        else if(buttonName == "4") commandPointer = coffeeWater;
        else if(buttonName == "5") commandPointer = hotWater;
        else if(buttonName == "6") commandPointer = steam;
        else if(buttonName == "7") commandPointer = aquaClean;
        else if(buttonName == "8") commandPointer = calcNclean;
        else if(buttonName == "9") commandPointer = coffee;
        else if(buttonName == "10") commandPointer = coffee;
      }
    }

    // WebSerial.println(buttonName);

    request->send(200, "text/plain", "Received"); });

  // Start ElegantOTA
  AsyncElegantOTA.begin(&webServer);

  // Start WebServer
  webServer.begin();

  for (uint8_t x = ASCII_ZERO; x < ASCII_ZERO + 10; x++)
  {
    Serial.println(convertHexCharToInt(x));
  }

  /* PIN INIT */
  // pinMode(D7, OUTPUT);
  // digitalWrite(D7, HIGH);
}

void loop()
{
  if(commandPointer != NULL)
  {
    serialSend(commandPointer, 5);
    commandPointer = NULL;
  }
}

uint8_t convertHexCharToInt(uint8_t _char)
{
  uint8_t retVal;

  switch (_char)
  {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    retVal = _char - ASCII_ZERO;
    break;

  case 'A':
  case 'a':
    retVal = 10;
    break;

  case 'B':
  case 'b':
    retVal = 11;
    break;

  case 'C':
  case 'c':
    retVal = 12;
    break;

  case 'D':
  case 'd':
    retVal = 13;
    break;

  case 'E':
  case 'e':
    retVal = 14;
    break;

  case 'F':
  case 'f':
    retVal = 14;
    break;

  default:
    retVal = 0;
    break;
  }
  return retVal;
}