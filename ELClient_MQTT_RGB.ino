/**
 * Simple example to demo the esp-link MQTT client
 */
#include <Arduino.h>

#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>

#include <RGBAnimator.hpp>
#include <SetPwmFrequency.h>

#define PIN_RED 10
#define PIN_GREEN 9
#define PIN_BLUE 11
#define PIN_WHITE 6

#define ON_TIME 300
#define PERC_ON 50
#define REPETITIONS 2
#define FADE_TIME 2000

#define DIR_UP 1
#define DIR_DOWN -1

char faster = (char) 0x05;
String fasterstring = (String) faster;
RGBAnimator rgb;
int direction = DIR_UP;
static uint32_t last;
void setup_rgb() {
  //setPwmFrequency(5,32);
  //setPwmFrequency(6,32); //default 2kHz //beware this changes millis()
  setPwmFrequency(9,8); //4kHz
  RGBAnimator rgb;
  //rgb->pwm_pin_w_ = 5;
  rgb.add_flash(color_t(0,0,0),color_t(255,0,0), ON_TIME, PERC_ON, REPETITIONS, true);
  rgb.add_flash(color_t(0,0,0),color_t(0,255,0), ON_TIME, PERC_ON, REPETITIONS, true);
  rgb.add_flash(color_t(0,0,0),color_t(0,0,255), ON_TIME, PERC_ON, REPETITIONS, true);
//  Animation* anim = (Animation*)rgb.AnimationList.tail->item;
//  if((anim->mode)==1)
//  {
//    FlashAnimation* fanim = (FlashAnimation*) anim;
//    fanim->slower(1000);
//  }
//  else
//  {
//    FadeAnimation* fanim = (FadeAnimation*) anim;
//    fanim->slower(10000);
//  }
  //->
  //rgb.slower(300);
  rgb.animate(1);
}

// Initialize a connection to esp-link using the normal hardware serial port both for
// SLIP and for debug messages.
ELClient esp(&Serial, &Serial);

// Initialize CMD client (for GetTime)
ELClientCmd cmd(&esp);

// Initialize the MQTT client
ELClientMqtt mqtt(&esp);

// Callback made from esp-link to notify of wifi status changes
// Here we just print something out for grins
void wifiCb(void* response) {
  ELClientResponse *res = (ELClientResponse*)response;
  if (res->argc() == 1) {
    uint8_t status;
    res->popArg(&status, 1);

    if(status == STATION_GOT_IP) {
    Serial.println("WIFI CONNECTED");
    } else {
     Serial.print("WIFI NOT READY: ");
    Serial.println(status);
    }
  }
}

bool connected;

// Callback when MQTT is connected
void mqttConnected(void* response) {
  Serial.println("MQTT connected!");
  //mqtt.subscribe("/esp-link/1");
  //mqtt.subscribe("/hello/world/#");
  mqtt.subscribe("/rgb");
  //mqtt.subscribe("/esp-link/2", 1);
  //mqtt.publish("/esp-link/0", "test1");
  connected = true;
}

// Callback when MQTT is disconnected
void mqttDisconnected(void* response) {
  Serial.println("MQTT disconnected");
  connected = false;
}

// Callback when an MQTT message arrives for one of our subscriptions
void mqttData(void* response) {
  ELClientResponse *res = (ELClientResponse *)response;

  Serial.print("Received: topic=");
  String topic = res->popString();
  Serial.println(topic);
  //rgb.stringAvailable(res->popString());

  
  Serial.print("data=");
  String data = res->popString();
  Serial.println(data);

  
  if(topic=="/rgb")
  {
  uint8_t myuint = 0;
  for (uint8_t i=0; i<data.length()/2;i++) {
        myuint = 0;
        for (uint8_t j=0; j<2; j++) {
            char firstchar = data[(i*2)+j];
            //printf("myuint[%d] = %3d data[%d+%d] = %c ", i, myuint[i], i, j, mystring[(i*2)+j]);
            if (firstchar >= '0' && firstchar <= '9') {
                //Serial.print("Number");
                myuint = myuint*16 + firstchar - '0';
            } else if (firstchar >= 'A' && firstchar <= 'F') {
                //Serial.print("LETTER");
                myuint = myuint*16 + firstchar - 'A' + 10;
            } else if (firstchar >= 'a' && firstchar <= 'f') {
                //Serial.print("letter");
                myuint = myuint*16 + firstchar - 'a' + 10;
            } else {
                // error
               // Serial.println("NOOOO");
            }
            //printf(" myuint[%d] = %3d\n", i, myuint[i]);
        }
    rgb.process_data(myuint);
    }
  }

  
 // if(topic=="/RGB")
  //{
    //if (data=="faster")
     // mqtt.publish("/rgb", (&char) 0x04);
    //if (data=="slower")
    //mqtt.publish("/rgb", (char) 0x05);
  //}
}

void mqttPublished(void* response) {
  Serial.println("MQTT published");
}

void setup() {
  Serial.begin(115200);
  Serial.println("EL-Client starting!");
 
// Sync-up with esp-link, this is required at the start of any sketch and initializes the
  // callbacks to the wifi status change callback. The callback gets called with the initial
  // status right after Sync() below completes.
  esp.wifiCb.attach(wifiCb); // wifi status change callback, optional (delete if not desired)
  bool ok;
  do {
    ok = esp.Sync();      // sync up with esp-link, blocks for up to 2 seconds
    if (!ok) Serial.println("EL-Client sync failed!");
  } while(!ok);
  Serial.println("EL-Client synced!");
  esp.Process();  
  // Set-up callbacks for events and initialize with es-link.
  mqtt.connectedCb.attach(mqttConnected);
  mqtt.disconnectedCb.attach(mqttDisconnected);
  mqtt.publishedCb.attach(mqttPublished);
  mqtt.dataCb.attach(mqttData);
  mqtt.setup();

  //Serial.println("ARDUINO: setup mqtt lwt");
  //mqtt.lwt("/lwt", "offline", 0, 0); //or mqtt.lwt("/lwt", "offline");

  Serial.println("EL-MQTT ready");
  Serial.println("RGB-Setup");
  setup_rgb();
  last = millis();
}

static int count;

void loop() {
  esp.Process();  
  rgb.animate(millis()-last);
  analogWrite(PIN_RED, rgb.color_current().R);
  analogWrite(PIN_BLUE, rgb.color_current().G);
  analogWrite(PIN_GREEN,rgb.color_current().B);
  last = millis();

  if (!rgb.running()) {
    if(!(rgb.list_empty()))
      {
      rgb.get_animation();
      Serial.println("pop");
      }
      
  }

  //if (connected && (millis()-last) > 4000) {
    //Serial.println("publishing");
    //char buf[12];

    //itoa(count++, buf, 10);
    //mqtt.publish("/esp-link/1", buf);

    //itoa(count+99, buf, 10);
    //mqtt.publish("/hello/world/arduino", "asd");

   // uint32_t t = cmd.GetTime();
    //Serial.print("Time: "); Serial.println(t);

  //  last = millis();
  //}
  
}
