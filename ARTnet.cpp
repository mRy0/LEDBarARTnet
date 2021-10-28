// 
// 
// 

#include "ARTnet.h"


void ARTnet::HandleARTnetMessage(uint8_t* data, int len) {

	//min pack len
	if (len < 18)
		return;

	//check ID
	if (data[0] != 'A' &&
		data[1] != 'r' &&
		data[2] != 't' &&
		data[3] != '-' &&
		data[4] != 'N' &&
		data[5] != 'e' &&
		data[6] != 't' &&
		data[7] != 0)
		return;


	uint16_t opCode = (data[9] << 8) + data[8];


	uint8_t procolVersionHi = data[10];
	uint8_t procolVersionLo = data[11];

	switch (opCode)
	{
	case ARTNET_OPCODE_POLL:
		ParseARTnetPollMessage(data, len);
		break;
	case ARTNET_OPCODE_OPOUT:
		ParseARTnetOPOutMessage(data, len);
		break;
	default:
		Serial.print("Recieved unknown ARTnet message - opCode: ");
		Serial.println(opCode);
		break;
	}
}


bool ARTnet::Setup() {
	if (_udpServer.listen(ARTNET_PORT)) {
		_udpServer.onPacket([&](AsyncUDPPacket packet) {
			HandleARTnetMessage(packet.data(), packet.length());
			});
		return true;
	}
	return false;
}



void ARTnet::ParseARTnetPollMessage(uint8_t* data, int len) {

	//not answear each poll
	if (_nextPoll < millis())
		return;
	_nextPoll += ARTNET_POLL_INTERVAL + millis();

	//overflow check
	if (len < 14)
		return;

	uint8_t talkToMe = data[12];
	uint8_t priority = data[13];

	Serial.print("recieved ARTnetPOLL message flags: ");
	PrintBinary(talkToMe);

	Serial.print(" priority: ");
	Serial.println(priority);
}


void ARTnet::ParseARTnetOPOutMessage(uint8_t* data, int len) {

	uint16_t universe = (data[15] << 8) + data[14];




	uint16_t dataLen = (data[16] << 8) + (data[17]);

	//overflow check
	if (dataLen > (len + 18))
		return;



	//uint pos = 18;
	//for (size_t i = 0; i < BARS; i++)
	//{
	//	bars[i].Animation = data[pos++];
	//	bars[i].Hue = data[pos++];
	//	bars[i].Saturation = data[pos++];
	//	bars[i].Value = data[pos++];
	//	bars[i].SetSpeed(data[pos++]);
	//	bars[i].SetDirection(data[pos++]);

	//	pos += 6;
	//}

}

void ARTnet::PrintBinary(uint8_t inByte)
{
	for (int b = 7; b >= 0; b--)
	{
		Serial.print(bitRead(inByte, b));
	}
}

void ARTnet::PrintBinary(uint16_t inByte)
{
	for (int b = 15; b >= 0; b--)
	{
		Serial.print(bitRead(inByte, b));
	}
}