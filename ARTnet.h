// ARTnet.h


#ifndef _ARTNET_h
#define _ARTNET_h

    #include "arduino.h"
    #include "AsyncUDP.h"


    #define ARTNET_PORT 6454

    #define ARTNET_OPCODE_POLL      0x2000
    #define ARTNET_OPCODE_POLLREPLY 0x2100
    #define ARTNET_OPCODE_OPOUT     0x5000



    #define ARTNET_POLL_INTERVAL 2000


    class ARTnet
    {
    private:
        AsyncUDP _udpServer;
        uint _nextPoll;
        void HandleARTnetMessage(uint8_t* data, int len);
        void ParseARTnetPollMessage(uint8_t* data, int len);
        void ParseARTnetOPOutMessage(uint8_t* data, int len);
        void PrintBinary(uint8_t inByte);
        void PrintBinary(uint16_t inByte);
    public:
        bool Setup();
    };

#endif

