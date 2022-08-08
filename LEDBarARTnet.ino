#include <ESPAsyncE131.h>
#include <FastLED.h>
#include <esp_wifi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>



#define LED_PIN_1 25
#define LED_PIN_2 26

#define LED_PER_STRAND 300
#define LED_TOTAL (LED_PER_STRAND + LED_PER_STRAND)


#define PIXEL_PER_BAR 60
#define BARS 10


#define UNIVERSE 1
#define UNIVERSE_COUNT 1

// 4 = 15 per bar 
//15 * 3  * 10 =  450
#define PX_PER_ADDR 4
#define ZONES_PER_BAR 15


#define MIN_DATA_LEN (ZONES_PER_BAR * 3 * BARS)



// ESPAsyncE131 instance with UNIVERSE_COUNT buffer slots
ESPAsyncE131 e131(UNIVERSE_COUNT);


const char* host = "esp_led_bar_1";
const char* ssid = "RAVENET";
const char* password = "FickDichMitPasswort!";
const char* ota_password = "revoltec";


CRGB leds_1[LED_PER_STRAND];
CRGB leds_2[LED_PER_STRAND];


void setup() {
	//waaaait for power ??
	delay(10000);
	Serial.begin(115200);
	delay(1000);
	Serial.println("system is starting...");


	SetupWifi();
	SetupE131();

	SetupUpdateServer();


	FastLED.addLeds<NEOPIXEL, LED_PIN_1>(leds_1, LED_PER_STRAND);
	FastLED.addLeds<NEOPIXEL, LED_PIN_2>(leds_2, LED_PER_STRAND);


	//say hello
	SetupFinished();

}



// the loop function runs over and over again until power down or reset
void loop() {
  ArduinoOTA.handle();
	HandleE131();


	FastLED.show();

	delay(1);
}

void SetupWifi() {

	// Make sure you're in station mode    
	WiFi.mode(WIFI_STA);

	//disable sleep
	esp_wifi_set_ps(WIFI_PS_NONE);

	WiFi.begin(ssid, password);
	Serial.println("");
	// Wait for connection
  size_t endOfWait = 20;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
    endOfWait--;
    if(endOfWait <= 0){      
      ESP.restart();
    }
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	/*use mdns for host name resolution*/
	if (!MDNS.begin(host)) { //http://esp32.local
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}
	Serial.println("mDNS responder started");

	Serial.println("WiFi Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

}

void SetupE131() {
	Serial.println(F("starting sACN..."));
	// Choose one to begin listening for E1.31 data
	//if (e131.begin(E131_UNICAST))                               // Listen via Unicast
	if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT))   // Listen via Multicast
		Serial.println(F("Listening for data..."));
	else
	{
		Serial.println(F("*** e131.begin failed ***"));
		Serial.println(F("restart"));
		ESP.restart();
	}
	Serial.println(F("ACN fin"));
}


void SetupUpdateServer() {

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

}

void SetupFinished() {
	Serial.println("ready to rumble...");
	for (size_t i = 0; i < 4; i++)
	{
		delay(50);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 255, 0, 255);
		}
    FastLED.show();
		delay(25);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 0, 0, 0);
		}
    FastLED.show();
	}

	for (size_t hue = 0; hue < 256; hue++)
	{
		for (size_t px = 0; px < LED_PER_STRAND; px++)
		{
			SetPixelHSV(px, CHSV(hue + px, 255, 255));
			SetPixelHSV(px + LED_PER_STRAND, CHSV(hue + px, 255, 255));
		}
    FastLED.show();
	}
	for (size_t px = 0; px < LED_TOTAL; px++)
	{
		SetPixel(px, 0, 0, 0);
	}
  FastLED.show();

	for (size_t i = 0; i < 4; i++)
	{
		delay(50);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 255, 0, 255);
		}
    FastLED.show();
		delay(25);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 0, 0, 0);
		}
    FastLED.show();
	}
	Serial.println("!!!!!");
}

void HandleE131() {
	if (!e131.isEmpty()) {
		e131_packet_t packet;
		e131.pull(&packet);     // Pull packet from ring buffer

		uint16_t universe = htons(packet.universe);
		uint16_t dmxDataLen = htons(packet.property_value_count) - 1;


		if (universe != UNIVERSE)
			return;

		if (dmxDataLen < MIN_DATA_LEN +1)
			return;

		size_t currentPixel = 0;

		for (size_t dmxIndex = 1; dmxIndex < (MIN_DATA_LEN +1); dmxIndex += 3)
		{
        for(size_t pxAddr = 0; pxAddr < PX_PER_ADDR; pxAddr++){
          SetPixel(currentPixel, packet.property_values[dmxIndex],
            packet.property_values[dmxIndex + 1], packet.property_values[dmxIndex + 2]);
          currentPixel++;
        }

		}
	}
}
//sets pixel on each stripe - MAIN function
void SetPixel(uint px, byte r, byte g, byte b) {
	if (px < LED_PER_STRAND)
		leds_1[px] = CRGB(r, g, b);
	else if (px <= LED_TOTAL)
		leds_2[px - LED_PER_STRAND] = CRGB(r, g, b);
}

//sets pixel on each stripe - MAIN function
void SetPixelHSV(uint px, CHSV hsv) {
  if (px < LED_PER_STRAND)
    leds_1[px] = hsv;
  else if (px <= LED_TOTAL)
    leds_2[px - LED_PER_STRAND] =  hsv;
}
