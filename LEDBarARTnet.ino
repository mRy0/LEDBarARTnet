#include <AsyncUDP_WT32_ETH01.h>
#include "ESPAsyncE131Eth.h"
#include <FastLED.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>



#define LED_PIN_1 32
#define LED_PIN_2 33

// Ethernet port parameters
// ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720.
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
// Pin number of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source).
#define ETH_PHY_POWER 12
// Type of the Ethernet PHY (LAN8720 or TLK110).
#define ETH_PHY_TYPE ETH_PHY_LAN8720
// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110).
#define ETH_PHY_ADDR 0
// Pin number of the I²C clock signal for the Ethernet PHY.
#define ETH_PHY_MDC 23
// Pin number of the I²C IO signal for the Ethernet PHY.
#define ETH_PHY_MDIO 18

#include <ETH.h>



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


const char* host = "esp_led_bar_1.local";
//const char* ssid = "RAVENET";
//const char* password = "FickDichMitPasswort!";
const char* ota_password = "revoltec";


CRGB leds_1[LED_PER_STRAND];
CRGB leds_2[LED_PER_STRAND];


void setup() {
	//waaaait for power ??
	delay(10000);
	Serial.begin(115200);
	delay(1000);
	Serial.println("system is starting...");


	BeginEthernet();



	FastLED.addLeds<NEOPIXEL, LED_PIN_1>(leds_1, LED_PER_STRAND);
	FastLED.addLeds<NEOPIXEL, LED_PIN_2>(leds_2, LED_PER_STRAND);

  
	//say hello
	SetupFinished();

}


void BeginEthernet()
{
 //disable sleep
  esp_wifi_set_ps(WIFI_PS_NONE);
  
  // To be called before ETH.begin()
  WT32_ETH01_onEvent();
  
  //bool begin(uint8_t phy_addr=ETH_PHY_ADDR, int power=ETH_PHY_POWER, int mdc=ETH_PHY_MDC, int mdio=ETH_PHY_MDIO, 
  //           eth_phy_type_t type=ETH_PHY_TYPE, eth_clock_mode_t clk_mode=ETH_CLK_MODE);
  //ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);

  WT32_ETH01_waitForConnect();
  
  // Client address
  Serial.print("AsyncUdpNTPClient started @ IP address: ");
  Serial.println(ETH.localIP());

  
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    ESP.restart();
  }
  
  SetupUpdateServer();
  
  SetupE131();
  ArduinoOTA.begin();
}


// the loop function runs over and over again until power down or reset
void loop() {
    
  ArduinoOTA.handle();
  HandleE131();
	FastLED.show();
  delay(1);
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
