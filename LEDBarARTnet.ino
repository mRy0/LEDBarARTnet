/*
 Name:		LEDBarARTnet.ino
 Created:	10/26/2021 10:22:49 PM
 Author:	Rafael
*/

// the setup function runs once when you press reset or power the board

#include "OTAUpdateServer.h"
#include <FastLED.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include "AsyncUDP.h"


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
#define ANIMATION_WARP_FADE 4
#define ANIMATION_WARPM 5
#define ANIMATION_WARPM_FADE 6
#define ANIMATION_MODULO 7
#define ANIMATION_MODULO_FADE 8
#define ANIMATION_STOBO 9
#define ANIMATION_RAINBOW 10


#define SLOW_SPEED 255

#define ARTNET_PORT 6454
#define ARTNET_UNIVERSE 1

AsyncUDP udpServer;


const char* host = "esp_led_bar_1";
const char* ssid = "RAVENET";
const char* password = "FickDichMitPasswort!";
const char* ota_password = "revoltec";



CRGB leds_1[LED_PER_STRAND];
CRGB leds_2[LED_PER_STRAND];


struct Bar {
	uint FirstLED;
	uint AnimationStep;

	byte Animation;

	byte Hue;
	byte Saturation;
	byte Value; //brightness

	byte Speed;
	uint CyclePerStep;

	bool Direction;

	void SetSpeed(byte speed) {
		AnimationStep = 0;
		Speed = speed;
		if(speed == 0){
			CyclePerStep = 0;
			return;
		}
		float fSpeed= speed;
		fSpeed = SLOW_SPEED / fSpeed;

		CyclePerStep =  round(fSpeed);
	}

	void SetDirection(byte direction) {
		Direction = direction > 127;
	}
};


Bar bars[10];

uint animation_cycle = 0;


void setup() {
	//waaaait for power ??
	delay(5000);
	Serial.begin(115200);
	delay(1000);
	Serial.println("system is starting...");


	SetupWifi();
	SetupOTAServer(host, ota_password);

	if (!SetupARTnet())
		ESP.restart();

	FastLED.addLeds<NEOPIXEL, LED_PIN_1>(leds_1, LED_PER_STRAND);
	FastLED.addLeds<NEOPIXEL, LED_PIN_2>(leds_2, LED_PER_STRAND);

	SetupBars();

	//say hello
	SetupFinished();

}


void SetupWifi() {
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



bool SetupARTnet() {
	if (udpServer.listen(ARTNET_PORT)) {

		udpServer.onPacket([](AsyncUDPPacket packet) {
			HandleARTnetMessage(packet.data(), packet.length());
		});
		return true;
	}	
	return false;
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
	for (size_t hue = 0; hue < 255; hue++)
	{
		for (size_t px = 0; px < LED_TOTAL; px++)
		{
			SetPixel(px, hue, 255, 255);
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

	HandleUpdateServer();
	for (size_t i = 0; i < BARS; i++)
	{
		HandleBar(&bars[i]);
	}

	ShowPixel();

	animation_cycle++;
	delay(1);

}

void HandleBar(Bar* bar) {

	//# calculate next animation step
	//stop if speed is 0
	if (bar->CyclePerStep == 0)
		return;
	else if (animation_cycle % bar->CyclePerStep == 0)
		bar->AnimationStep++;
	else
	//wait for next cycle
		return;


	if (bar->Animation == ANIMATION_OFF)
		AnimateOff(bar);
	else if (bar->Animation == ANIMATION_STATIC)
		AnimateFullColor(bar);
	else if (bar->Animation == ANIMATION_FADE)
		AnimateFade(bar);
	else if (bar->Animation == ANIMATION_WARP)
		AnimateWarp(bar, false);
	else if (bar->Animation == ANIMATION_WARP_FADE)
		AnimateWarp(bar, true);
	else if (bar->Animation == ANIMATION_WARPM)
		AnimateWarpMultiple(bar, false);
	else if (bar->Animation == ANIMATION_WARPM_FADE)
		AnimateWarpMultiple(bar, true);
	else if (bar->Animation == ANIMATION_MODULO)
		AnimateModulo(bar, false);
	else if (bar->Animation == ANIMATION_MODULO_FADE)
		AnimateModulo(bar, true);
	else if (bar->Animation == ANIMATION_STOBO)
		AnimateStrobo(bar);
	else if (bar->Animation == ANIMATION_RAINBOW)
		AnimateRainbow(bar);
}


void AnimateOff(Bar* bar) {
	for (size_t i = 0; i < PIXEL_PER_BAR; i++)
	{
		SetBarPixel(bar, i, 0, 0, 0);
	}
}

void AnimateFullColor(Bar* bar) {
	if (bar->AnimationStep > 255)
		bar->AnimationStep = 0;

	for (size_t i = 0; i < PIXEL_PER_BAR; i++)
	{
		SetBarPixel(bar, i, bar->Hue + bar->AnimationStep, bar->Saturation, bar->Value);
	}
}

void AnimateFade(Bar* bar) {
	//reset to prevent overflow
	bar->AnimationStep = 0;
	bar->Hue++;
	AnimateFullColor(bar);
}

void AnimateModulo(Bar* bar, bool changeColor) {
	if (bar->AnimationStep > PIXEL_PER_BAR)
		bar->AnimationStep = 1;

	if (changeColor)
		bar->Hue++;

	//all off
	for (size_t px = 0; px < PIXEL_PER_BAR; px++) {
		SetBarPixel(bar, px, 0, 0, 0);
	}

	for (size_t px = 1; px < PIXEL_PER_BAR -1; px++)
	{
		if (px % bar->AnimationStep == 0) {

			SetBarPixel(bar, px);
			SetBarPixel(bar, px + 1);
		}
	}
}



void AnimateWarp(Bar* bar, bool fadeColor) {
	if (bar->AnimationStep >= PIXEL_PER_BAR)
		bar->AnimationStep = 0;

	if (fadeColor)
		bar->Hue++;

	//all off
	for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	{
		SetBarPixel(bar, px, 0, 0, 0);
	}

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

void AnimateWarpMultiple(Bar* bar, bool fadeColor) {
	if (bar->AnimationStep >= PIXEL_PER_BAR)
		bar->AnimationStep = 0;

	if (fadeColor)
		bar->Hue++;

	//all off
	for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	{
		SetBarPixel(bar, px, 0, 0, 0);
	}

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
	else {
		AnimateFullColor(bar);
		bar->AnimationStep = 0;
	}
}
void AnimateRainbow(Bar* bar) {
	if (bar->AnimationStep > 255)
		bar->AnimationStep = 0;

	int hueStep = round(((float)255 / PIXEL_PER_BAR));

	for (size_t px = 0; px < PIXEL_PER_BAR; px++)
	{
		SetBarPixel(bar, px, bar->Hue + bar->AnimationStep + (px * hueStep), bar->Saturation, bar->Value);
	}

}


void SetupBars() {
	for (size_t i = 0; i < BARS; i++)
	{
		bars[i].FirstLED = (i * PIXEL_PER_BAR);
		bars[i].Animation = ANIMATION_OFF;
		bars[i].AnimationStep = 0;
		bars[i].Hue = 0;
		bars[i].Saturation = 0;
		bars[i].Direction = false;
		bars[i].Value = 0;
		bars[i].SetSpeed(0);
	}
}


void Demo() {
	for (size_t i = 0; i < BARS; i++)
	{
		bars[i].FirstLED = (i * PIXEL_PER_BAR);
		bars[i].Animation = ANIMATION_RAINBOW;
		bars[i].AnimationStep = 0;
		bars[i].Hue = 0;
		bars[i].Saturation = 255;
		bars[i].Value = 255;
		bars[i].Direction = true;
		bars[i].SetSpeed(255);
	}
}

void SetBarPixel(Bar* bar, byte px) {
	if (px > PIXEL_PER_BAR)
		return;

	if (bar->Direction) 
		SetPixel(bar->FirstLED + px, bar->Hue, bar->Saturation, bar->Value);
	else
		SetPixel(bar->FirstLED + (PIXEL_PER_BAR - px) -1, bar->Hue, bar->Saturation, bar->Value);

}

void SetBarPixel(Bar* bar, byte px, byte h, byte s, byte v) {
	if (px > PIXEL_PER_BAR)
		return;

	if (bar->Direction)
		SetPixel(bar->FirstLED + px, h, s, v);
	else
		SetPixel(bar->FirstLED + (PIXEL_PER_BAR - px) -1, h, s, v);

}

//sets pixel on each stripe - MAIN function
void SetPixel(uint px, byte h, byte s, byte v) {
	if (px < 0)
		return;
	else if (px < LED_PER_STRAND)
		leds_1[px] = CHSV(h, s, v);
	else if(px <= LED_TOTAL)
		leds_2[px - LED_PER_STRAND] = CHSV(h, s, v);	
}

void ShowPixel() {
	FastLED.show();
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
void HandleARTnetMessage(uint8_t* data, int len) {

	//man pack len
	if (len < 18)
		return;
	//atnetflag
	if (data[9] != 0x50)
		return;

	uint16_t universe = (data[15] << 8) + data[14];

	if (universe != ARTNET_UNIVERSE)
		return;


	uint16_t dataLen = (data[16] << 8) + (data[17]);

	//min len expected
	if (dataLen < (12 * BARS))
		return;
	//overflow check
	if (dataLen > (len + 18))
		return;


	uint pos = 18;
	for (size_t i = 0; i < BARS; i++)
	{
		bars[i].Animation = data[pos++];
		bars[i].Hue = data[pos++];
		bars[i].Saturation = data[pos++];
		bars[i].Value = data[pos++];
		bars[i].SetSpeed(data[pos++]);
		bars[i].SetDirection(data[pos++]);

		pos += 6;
	}
}