#pragma once

#include "wled.h"
#include "Servo.h"


#ifndef WLED_ENABLE_MQTT
#error "This user mod requires MQTT to be enabled."
#endif

static const uint8_t switchPins[] = { 13 };
#define NUM_SWITCH_PINS (sizeof(switchPins))

//ALEXA CHANGE
int servoDegrees = 0;
void onAlexaChange2(uint8_t p)
    {
      servoDegrees = p;
      colorUpdated(NOTIFIER_CALL_MODE_ALEXA);
    }
//ALEXA CHANGE

int random_int(int min, int max)
{
    return min + rand() % (max+1 - min);
}

class UsermodServos: public Usermod
{
private:
    bool mqttInitialized;
    bool switchState[NUM_SWITCH_PINS];
    unsigned long lastTime = 0;
    bool servosmqtt;

    Servo servo1;
    int lastPosServo1 = 0;
    int targetPosServo1;
    int targetSpeed1;

    Servo servo2;
    int lastPosServo2 = 0;
    int targetPosServo2;
    int targetSpeed2;

    int max_speed = 5;
    


public:
    UsermodServos() :
            mqttInitialized(false)
    {
    }

    void setup()
    {
        //ADD DEVICE ALEXA
        espalexa.addDevice("Torretas", onAlexaChange2);
        espalexa.begin(&server);

        servo1.attach(13);
        servo1.write(0);

        servo2.attach(15);
        servo2.write(0);

        for (int pinNr = 0; pinNr < NUM_SWITCH_PINS; pinNr++) {
            setState(pinNr, false);
            pinMode(switchPins[pinNr], OUTPUT);
        }

    }

    void loop()
    {
        espalexa.loop();

        ReadConfigServos();

        if (millis() - lastTime > 30) {
            lastTime = millis();
    
            loopServo1();
            loopServo2();
        }

        if (!mqttInitialized) {
            mqttInit();
            return; // Try again in next loop iteration
        }
    }
 

    void ReadConfigServos()
    {
        if (servoDegrees > 0 && servoDegrees < 255)
        {
            max_speed = (int) 15*servoDegrees/255;
            
            if (max_speed == 0)
                max_speed = 1;

        }

        if (servoDegrees != 0)
        {
            if (targetPosServo1 == lastPosServo1)
            {
                targetPosServo1 = random_int(0,179);
                targetSpeed1 = random_int(1, max_speed);
            }

            if (targetPosServo2 == lastPosServo2)
            {
                targetPosServo2 = random_int(0,179);
                targetSpeed2 = random_int(1, max_speed);
            }
        }
    }

    int random_int(int min, int max)
    {
    return min + rand() % (max+1 - min);
    }
    void loopServo1()
    {
        if (targetPosServo1 == lastPosServo1)
        return;
        
        if (targetSpeed1 == 0)
        {
            lastPosServo1 = targetPosServo1;
        }

        if (targetPosServo1 > lastPosServo1)
        {
            if (targetPosServo1 - lastPosServo1 < targetSpeed1) 
            {
                lastPosServo1 = targetPosServo1;
            }
            else{
                lastPosServo1 += targetSpeed1;
            }

        }
        else if (targetPosServo1 < lastPosServo1)
        {
            if (lastPosServo1 - targetPosServo1 < targetSpeed1) 
            {
                lastPosServo1 = targetPosServo1;
            }
            else {
                lastPosServo1 -=  targetSpeed1;
            }
        }

        servo1.write(lastPosServo1);
    }

    void loopServo2()
    {
        if (targetPosServo2 == lastPosServo2)
        return;
        
        if (targetSpeed2 == 0)
        {
            lastPosServo2 = targetPosServo2;
        }

        if (targetPosServo2 > lastPosServo2)
        {
            if (targetPosServo2 - lastPosServo2 < targetSpeed2) 
            {
                lastPosServo2 = targetPosServo2;
            }
            else{
                lastPosServo2 += targetSpeed2;
            }

        }
        else if (targetPosServo2 < lastPosServo2)
        {
            if (lastPosServo2 - targetPosServo2 < targetSpeed2) 
            {
                lastPosServo2 = targetPosServo2;
            }
            else {
                lastPosServo2 -=  targetSpeed2;
            }
        }

        servo2.write(lastPosServo2);
    }

    void mqttInit()
    {
        if (!mqtt)
            return;
        mqtt->onMessage(
                std::bind(&UsermodServos::onMqttMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                          std::placeholders::_5, std::placeholders::_6));
        mqtt->onConnect(std::bind(&UsermodServos::onMqttConnect, this, std::placeholders::_1));
        mqttInitialized = true;
    }

    void onMqttConnect(bool sessionPresent);

    void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    void updateState(uint8_t pinNr);

    void setState(uint8_t pinNr, bool active)
    {
        if (pinNr > NUM_SWITCH_PINS)
            return;
        switchState[pinNr] = active;
        
        servoDegrees = (active ? 255 : 0);
        
        updateState(pinNr);
    }
};

inline void UsermodServos::onMqttConnect(bool sessionPresent)
{
    if (mqttDeviceTopic[0] == 0)
        return;

    for (int pinNr = 0; pinNr < NUM_SWITCH_PINS; pinNr++) {
        char buf[128];
        StaticJsonDocument<1024> json;
        sprintf(buf, "%s Switch %d", serverDescription, pinNr + 1);
        json[F("name")] = buf;

        sprintf(buf, "%s/switch/%d", mqttDeviceTopic, pinNr);
        json["~"] = buf;
        strcat(buf, "/set");
        mqtt->subscribe(buf, 0);

        json[F("stat_t")] = "~/state";
        json[F("cmd_t")] = "~/set";
        json[F("pl_off")] = F("OFF");
        json[F("pl_on")] = F("ON");

        char uid[16];
        sprintf(uid, "%s_sw%d", escapedMac.c_str(), pinNr);
        json[F("unique_id")] = uid;

        strcpy(buf, mqttDeviceTopic);
        strcat(buf, "/status");
        json[F("avty_t")] = buf;
        json[F("pl_avail")] = F("online");
        json[F("pl_not_avail")] = F("offline");
        //TODO: dev
        sprintf(buf, "homeassistant/switch/%s/config", uid);
        char json_str[1024];
        size_t payload_size = serializeJson(json, json_str);
        mqtt->publish(buf, 0, true, json_str, payload_size);
        updateState(pinNr);
    }
}

inline void UsermodServos::onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    //Note: Payload is not necessarily null terminated. Check "len" instead.
    for (int pinNr = 0; pinNr < NUM_SWITCH_PINS; pinNr++) {
        char buf[64];
        sprintf(buf, "%s/switch/%d/set", mqttDeviceTopic, pinNr);
        if (strcmp(topic, buf) == 0) {
            //Any string starting with "ON" is interpreted as ON, everything else as OFF
            setState(pinNr, len >= 2 && payload[0] == 'O' && payload[1] == 'N');
            break;
        }
    }
}

inline void UsermodServos::updateState(uint8_t pinNr)
{
    if (!mqttInitialized)
        return;

    if (pinNr > NUM_SWITCH_PINS)
        return;

    char buf[64];
    sprintf(buf, "%s/switch/%d/state", mqttDeviceTopic, pinNr);
    if (switchState[pinNr]) {
        mqtt->publish(buf, 0, false, "ON");
    } else {
        mqtt->publish(buf, 0, false, "OFF");
    }
}
