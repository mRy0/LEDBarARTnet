// OTAUpdateServer.h

#ifndef _OTAUPDATESERVER_h
#define _OTAUPDATESERVER_h

    #include "arduino.h"

    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>

class OTAUpdateServer
{
public:
    static void Setup(const char* name, const char* pass);
    static void Handle();
};


#endif