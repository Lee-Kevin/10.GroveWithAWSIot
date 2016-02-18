/*
 * GroveWithAWSIot.ino
 *
 * Copyright (c) 2016 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Wuruibin
 * Created Time: Feb 2016
 * Modified Time:
 */
 
#include <aws_iot_mqtt.h>
#include <aws_iot_version.h>
#include "aws_iot_config.h"
#include <math.h>

int a;
float temperature;
int B=3975;                  //B value of the thermistor
float resistance;

aws_iot_mqtt_client myClient;
char JSON_buf[100];
char float_buf[5];
float reportedTemp = 70.0;
float desiredTemp = 70.0;
int cnt = 0;
int rc = 1;
bool success_connect = false;

bool print_log(char* src, int code) {
  bool ret = true;
  if(code == 0) {
    Serial.print("[LOG] command: ");
    Serial.print(src);
    Serial.println(" completed.");
    ret = true;
  }
  else {
    Serial.print("[ERR] command: ");
    Serial.print(src);
    Serial.print(" code: ");
    Serial.println(code);
    ret = false;
  }
  return ret;
}

void msg_callback_delta(char* src, int len) {
  String data = String(src);
  int st = data.indexOf("\"state\":") + strlen("\"state\":");
  int ed = data.indexOf(",\"metadata\":");
  String delta = data.substring(st, ed);
  st = delta.indexOf("\"Temp\":") + strlen("\"Temp\":");
  ed = delta.indexOf("}");
  String delta_data = delta.substring(st, ed);
  desiredTemp = delta_data.toFloat();
}

void setup() {
  Serial.begin(115200);
  while(!Serial);

  char curr_version[80];
  sprintf(curr_version, "AWS IoT SDK Version(dev) %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
  Serial.println(curr_version);

  if(print_log("setup", myClient.setup(AWS_IOT_CLIENT_ID))) {
    if(print_log("config", myClient.config(AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, AWS_IOT_ROOT_CA_PATH, AWS_IOT_PRIVATE_KEY_PATH, AWS_IOT_CERTIFICATE_PATH))) {
      if(print_log("connect", myClient.connect())) {
        success_connect = true;
        print_log("shadow init", myClient.shadow_init(AWS_IOT_MY_THING_NAME));
        print_log("register thing shadow delta function", myClient.shadow_register_delta_func(AWS_IOT_MY_THING_NAME, msg_callback_delta));
      }
    }
  }
}

void loop() {
  if(success_connect) {
    a=analogRead(0);
    resistance=(float)(1023-a)*10000/a; //get the resistance of the sensor;
    temperature=1/(log(resistance/10000)/B+1/298.15)-273.15;//convert to temperature via datasheet&nbsp;;
    Serial.print("Current temperature is ");
    Serial.println(temperature);
  
    dtostrf(temperature, 4, 1, float_buf);
    float_buf[4] = '\0';
    sprintf(JSON_buf, "{\"state\":{\"reported\":{\"Temp\":%s}}}", float_buf);
    print_log("shadow update", myClient.shadow_update(AWS_IOT_MY_THING_NAME, JSON_buf, strlen(JSON_buf), NULL, 5));
    if(myClient.yield()) {
      Serial.println("Yield failed.");
    }
    delay(1000); // check for incoming delta per 1000 ms
  }
}
