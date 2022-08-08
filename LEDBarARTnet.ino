/*
 Name:		LEDBarARTnet.ino
 Created:	10/26/2021 10:22:49 PM
 Author:	Rafael
*/

// the setup function runs once when you press reset or power the board

#include "OTAUpdateServer.h"
#include <ESPAsyncE131.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>


#define LED_PIN_1 25
#define LED_PIN_2 26

#define LED_PER_STRAND 300
#define LED_TOTAL (LED_PER_STRAND + LED_PER_STRAND)

#define PIXEL_PER_BAR 60
#define BARS 10

#define ANIMATION_OFF 0
#define ANIMATION_STATIC 1
#define ANIMATION_FADE 2
#define ANIMATION_WARP 3
#define ANIMATION_WARPM 4
#define ANIMATION_MODULO 5
#define ANIMATION_STOBO 6
#define ANIMATION_RAINBOW 7
#define ANIMATION_RAINBOW_MANUAL 8

#define SLOW_SPEED 255

#define UNIVERSE 1
#define UNIVERSE_COUNT 1
#define DMX_START_ADDRESS 1

// ESPAsyncE131 instance with UNIVERSE_COUNT buffer slots
ESPAsyncE131 e131(UNIVERSE_COUNT);


const char* host = "esp_led_bar_1";
const char* ssid = "RAVENET";
const char* password = "FickDichMitPasswort!";
const char* ota_password = "revoltec";


CRGB leds_1[LED_PER_STRAND];
CRGB leds_2[LED_PER_STRAND];


struct Bar {
	bool IsBackground;
	uint FirstLED;
	uint AnimationStep;

	byte Animation;

	byte Red;
	byte Green;
	byte Blue; //brightness

	byte Speed;
	uint CyclePerStep;

	bool Direction;

	void SetSpeed(byte speed) {
		if (Speed == speed)
			return;
		Speed = speed;
		if(speed == 0){
			CyclePerStep = 0;
			return;
		}
		float fSpeed = speed;
		fSpeed = SLOW_SPEED / fSpeed;

		CyclePerStep =  round(fSpeed);
	}

	void SetDirection(byte direction) {
		Direction = direction > 127;
	}

	void ParseE131Packet(e131_packet_t& packet, uint pos) {

		Animation = packet.property_values[pos];
		Red = packet.property_values[pos + 1];
		Green = packet.property_values[pos + 2];
		Blue = packet.property_values[pos + 3];

		SetSpeed(packet.property_values[pos + 4]);
		SetDirection(packet.property_values[pos + 5]);
	}
};


Bar bars[BARS * 2];

uint animation_cycle = 0;


void setup() {
	//waaaait for power ??
	delay(5000);
	Serial.begin(115200);
	delay(1000);
	Serial.println("system is starting...");


	SetupWifi();
	SetupE131();

	OTAUpdateServer::Setup(host, ota_password);


	FastLED.addLeds<NEOPIXEL, LED_PIN_1>(leds_1, LED_PER_STRAND);
	FastLED.addLeds<NEOPIXEL, LED_PIN_2>(leds_2, LED_PER_STRAND);

	SetupBars();

	//say hello
	SetupFinished();

}


void SetupWifi() {

	// Make sure you're in station mode    
	WiFi.mode(WIFI_STA);
	WiFi.setSleep(false);

	//disable sleep

	WiFi.begin(ssid, password);
	Serial.println("");
	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
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



void SetupFinished() {
	Serial.println("ready to rumble...");
	for (size_t i = 0; i < 4; i++)
	{
		delay(50);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 255, 0, 255);
		}
		ShowPixel();
		delay(25);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 0, 0, 0);
		}
		ShowPixel();
	}

	for (size_t hue = 0; hue < 256; hue++)
	{
		for (size_t px = 0; px < LED_PER_STRAND; px++)
		{
			SetPixelHSV(px, CHSV(hue + px, 255, 255));
			SetPixelHSV(px + LED_PER_STRAND, CHSV(hue + px, 255, 255));
		}
		ShowPixel();
	}
	for (size_t px = 0; px < LED_TOTAL; px++)
	{
		SetPixel(px, 0, 0, 0);
	}
	ShowPixel();

	for (size_t i = 0; i < 4; i++)
	{
		delay(50);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 255, 0, 255);
		}
		ShowPixel();
		delay(25);
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, 0, 0, 0);
		}
		ShowPixel();
	}
	Serial.println("!!!!!");
}


// the loop function runs over and over again until power down or reset
void loop() {
	OTAUpdateServer::Handle();
	HandleE131();

	for (size_t i = 0; i < BARS; i++)
	{
		HandleBar(&bars[i]);
	}

	for (size_t i = BARS; i < (BARS*2); i++)
	{
		HandleBar(&bars[i]);
	}

	ShowPixel();

	animation_cycle++;
	delay(1);

}

void HandleBar(Bar* bar) {

  //ignore this mode
	if (bar->Animation == ANIMATION_OFF)
  {
		AnimateOff(bar);
    return;
  }
	else if (bar->Animation == ANIMATION_STATIC){
		AnimateFullColor(bar);
    return;
  }
	else if (bar->Animation == ANIMATION_RAINBOW_MANUAL){
		Rainbow(bar);
    return;
  }
  

	//# calculate next animation step
	//stop if speed is 0
	if (bar->CyclePerStep == 0)
		return;
	else if (animation_cycle % bar->CyclePerStep == 0)
		bar->AnimationStep++;
	else
	//wait for next cycle
		return;


	if (bar->Animation == ANIMATION_FADE)
		AnimateFade(bar);
	else if (bar->Animation == ANIMATION_WARP)
		AnimateWarp(bar);
	else if (bar->Animation == ANIMATION_WARPM)
		AnimateWarpMultiple(bar);
	else if (bar->Animation == ANIMATION_MODULO)
		AnimateModulo(bar);
	else if (bar->Animation == ANIMATION_STOBO)
		AnimateStrobo(bar);
	else if (bar->Animation == ANIMATION_RAINBOW)
		AnimateRainbow(bar);
    
}


void AnimateOff(Bar* bar) {
	if (!bar->IsBackground)
		return;
	for (size_t i = 0; i < PIXEL_PER_BAR; i++)
	{
		SetBarPixel(bar, i, 0, 0, 0);
	}
}

void AnimateFullColor(Bar* bar) {

	for (size_t i = 0; i < PIXEL_PER_BAR; i++)
	{
		SetBarPixel(bar, i, bar->Red, bar->Green, bar->Blue);
	}
}

void AnimateFade(Bar* bar) {
	//reset to prevent overflow
	if (bar->AnimationStep > 255)
		bar->AnimationStep = 0;

	CHSV pxCol = CHSV(bar->AnimationStep, 255, 255);

	for (size_t i = 0; i < PIXEL_PER_BAR; i++)
	{
		SetBarPixelHSV(bar, i, pxCol);
	}
}

void AnimateModulo(Bar* bar) {
	if (bar->AnimationStep > PIXEL_PER_BAR)
		bar->AnimationStep = 1;


	for (size_t px = 1; px < PIXEL_PER_BAR -1; px++)
	{
		if (px % bar->AnimationStep == 0) {

			SetBarPixel(bar, px);
			SetBarPixel(bar, px + 1);
		}
	}
}



void AnimateWarp(Bar* bar) {
	if (bar->AnimationStep >= PIXEL_PER_BAR)
		bar->AnimationStep = 0;


	////all off
	//for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	//{
	//	SetBarPixel(bar, px, 0, 0, 0);
	//}

	int realPx = 0;

	for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	{
		if (px == bar->AnimationStep) {
			for (size_t i = 0; i < 3; i++)
			{
				realPx = px + i;

				if (realPx > PIXEL_PER_BAR)
					realPx -= PIXEL_PER_BAR;

				SetBarPixel(bar, realPx);
			}
		}
	}
}

void AnimateWarpMultiple(Bar* bar) {
	if (bar->AnimationStep >= PIXEL_PER_BAR)
		bar->AnimationStep = 0;


	////all off
	//for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	//{
	//	SetBarPixel(bar, px, 0, 0, 0);
	//}

	int realPx = 0;
	
	int div1 = bar->AnimationStep;
	int div2 = bar->AnimationStep + ((PIXEL_PER_BAR / 4) *2) ;
	int div3 = bar->AnimationStep + ((PIXEL_PER_BAR / 4) * 3);
	int div4 = bar->AnimationStep + PIXEL_PER_BAR;
	int div5 = bar->AnimationStep - ((PIXEL_PER_BAR / 4) * 2);
	int div6 = bar->AnimationStep - ((PIXEL_PER_BAR / 4) * 3);
	int div7 = bar->AnimationStep - PIXEL_PER_BAR;
	int div8 = bar->AnimationStep + (PIXEL_PER_BAR / 4);
	int div9 = bar->AnimationStep - (PIXEL_PER_BAR / 4);

	for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	{
		if (px == div1 ||
			px == div2 ||
			px == div3 ||
			px == div4 ||
			px == div5 ||
			px == div6 ||
			px == div7 ||
			px == div8 ||
			px == div9
			)
		{
			for (size_t i = 0; i < 2; i++)
			{
				realPx = px + i;

				if (realPx > PIXEL_PER_BAR)
					realPx -= PIXEL_PER_BAR;

				SetBarPixel(bar, realPx);
			}
		}
	}
}

void AnimateStrobo(Bar* bar) {
	//reset to prevent overflow
	if (bar->AnimationStep < 3)
	{
		//all off
		for (size_t px = 0; px < PIXEL_PER_BAR; px++) {
			SetBarPixel(bar, px, 0, 0, 0);
		}
	}
	else if(bar->Red > 0 && bar->Green > 0 && bar->Blue > 0) {
		//all on
		for (size_t px = 0; px < PIXEL_PER_BAR; px++) {
			SetBarPixel(bar, px, bar->Red, bar->Green, bar->Blue);
		}
		bar->AnimationStep = 0;
	}
	else { bar->AnimationStep = 0; }
}
void AnimateRainbow(Bar* bar) {
	if (bar->AnimationStep > 255)
		bar->AnimationStep = 0;

	int hueStep = round(((float)255 / PIXEL_PER_BAR));
  

	for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	{
		SetBarPixelHSV(bar, px, CHSV(bar->AnimationStep + (px * hueStep), 255,255));
	}
}
void Rainbow(Bar* bar) {
	if (bar->AnimationStep > 255)
		bar->AnimationStep = 0;

	int hueStep = round(((float)255 / PIXEL_PER_BAR));
  
  uint16_t brighteness= (bar->Red + bar->Green + bar->Blue) / 3;

	for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	{
		SetBarPixelHSV(bar, px, CHSV(brighteness + (px * hueStep), 255,255));
	}
}


void SetupBars() {
	for (size_t i = 0; i < BARS; i++)
	{
		bars[i].FirstLED = (i * PIXEL_PER_BAR);
		bars[i].Animation = ANIMATION_OFF;
		bars[i].AnimationStep = 0;
		bars[i].Red = 0;
		bars[i].Green = 0;
		bars[i].Blue = 0;
		bars[i].Direction = false;
		bars[i].IsBackground = true;
		bars[i].SetSpeed(0);
	}
	for (size_t i = 0; i < BARS; i++)
	{
		bars[i + BARS].FirstLED = (i * PIXEL_PER_BAR);
		bars[i + BARS].Animation = ANIMATION_OFF;
		bars[i + BARS].AnimationStep = 0;
		bars[i + BARS].Red = 0;
		bars[i + BARS].Green = 0;
		bars[i + BARS].Blue = 0;
		bars[i + BARS].Direction = false;
		bars[i + BARS].IsBackground = false;
		bars[i + BARS].SetSpeed(0);
	}
}



void Demo() {
	for (size_t i = 0; i < BARS; i++)
	{
		bars[i].FirstLED = (i * PIXEL_PER_BAR);
		bars[i].Animation = ANIMATION_RAINBOW;
		bars[i].AnimationStep = 0;
		bars[i].Red = 255;
		bars[i].Green = 0;
		bars[i].Blue = 0;
		bars[i].Direction = true;
		bars[i].SetSpeed(255);
	}
}

void SetBarPixel(Bar* bar, byte px) {
	if (px > PIXEL_PER_BAR)
		return;

	if (bar->Direction) 
		SetPixel(bar->FirstLED + px, bar->Red, bar->Green, bar->Blue);
	else
		SetPixel(bar->FirstLED + (PIXEL_PER_BAR - px) -1, bar->Red, bar->Green, bar->Blue);

}

void SetBarPixel(Bar* bar, byte px, byte r, byte g, byte b) {
	if (px > PIXEL_PER_BAR)
		return;

	if (bar->Direction)
		SetPixel(bar->FirstLED + px, r, g, b);
	else
		SetPixel(bar->FirstLED + (PIXEL_PER_BAR - px) -1, r, g, b);

}

void SetBarPixelHSV(Bar* bar, byte px, CHSV hsv) {
	if (px > PIXEL_PER_BAR)
		return;

	if (bar->Direction)
		SetPixelHSV(bar->FirstLED + px, hsv);
	else
		SetPixelHSV(bar->FirstLED + (PIXEL_PER_BAR - px) - 1, hsv);

}

CRGB GetBarPixel(Bar* bar, byte px) {
	return GetPixel(bar->FirstLED + px);
}

//sets pixel on each stripe - MAIN function
void SetPixel(uint px, byte r, byte g, byte b) {
	if (px < LED_PER_STRAND)
		leds_1[px] = CRGB(r, g ,b);
	else if(px <= LED_TOTAL)
		leds_2[px - LED_PER_STRAND] = CRGB(r, g, b);
}
//sets pixel on each stripe - MAIN function
void SetPixelHSV(uint px, CHSV hsv) {
	if (px < LED_PER_STRAND)
		leds_1[px] = hsv;
	else if (px <= LED_TOTAL)
		leds_2[px - LED_PER_STRAND] =  hsv;
}

CRGB GetPixel(uint px) {
	if (px < LED_PER_STRAND)
		return leds_1[px];
	else if (px <= LED_TOTAL)
		return leds_2[px - LED_PER_STRAND];
	return CRGB(0, 0, 0);
}

void ShowPixel() {
	FastLED.show();
}

void HandleE131() {
	if (!e131.isEmpty()) {
		e131_packet_t packet;
		e131.pull(&packet);     // Pull packet from ring buffer

		uint16_t universe = htons(packet.universe);
		uint16_t dmxDataLen = htons(packet.property_value_count) - 1;


		if (universe != UNIVERSE)
			return;

		if (dmxDataLen < ((BARS * 6) + (DMX_START_ADDRESS - 1)))
			return;

		for (size_t i = 0; i < (BARS * 2); i++)
		{
			bars[i].ParseE131Packet(packet, (i * 6) + DMX_START_ADDRESS);
		}
	}
}




/*
* Message Diagram:
* 0 = Back Animation Id
* 1 = Back Hue
* 2 = Back Sat
* 3 = Back Val
* 4 = Back Speed 
* 5 = Back Direction  Bool>127
* 6 = Front Animation Id
* 7 = Front Hue
* 8 = Front Sat
* 9 = Front Val
* 10 = Front Speed 
* 11 = Front Direction  Bool>127
*/
//void HandleARTnetMessage(uint8_t* data, int len) {
//
//	//man pack len
//	if (len < 18)
//		return;
//	//atnetflag
//	if (data[9] != 0x50)
//		return;
//
//	uint16_t universe = (data[15] << 8) + data[14];
//
//	if (universe != ARTNET_UNIVERSE)
//		return;
//
//
//	uint16_t dataLen = (data[16] << 8) + (data[17]);
//
//	//min len expected
//	if (dataLen < (12 * BARS))
//		return;
//	//overflow check
//	if (dataLen > (len + 18))
//		return;
//
//
//	uint pos = 18;
//	for (size_t i = 0; i < BARS; i++)
//	{
//		bars[i].Animation = data[pos++];
//		bars[i].Hue = data[pos++];
//		bars[i].Saturation = data[pos++];
//		bars[i].Value = data[pos++];
//		bars[i].SetSpeed(data[pos++]);
//		bars[i].SetDirection(data[pos++]);
//
//		pos += 6;
//	}
//}
