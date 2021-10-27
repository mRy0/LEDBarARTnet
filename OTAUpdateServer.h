// OTAUpdateServer.h

#ifndef _OTAUPDATESERVER_h
#define _OTAUPDATESERVER_h

    #include "arduino.h"

    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>

#endif

void SetupOTAServer(const char* name, const char* pass);
void HandleUpdateServer();
