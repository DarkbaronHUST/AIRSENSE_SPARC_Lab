#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPtimeESP.h>
#include <EEPROM.h>
#include "./smartconfig.h"

#define num_byte_data_from_arduino 12
#define max_num_data 30
#define queue_num (max_num_data+1)
#define data_size 8
#define queue_size (queue_num*data_size)

boolean isGetTime=true;
boolean isSave2Flash=true;

unsigned long lastGetTime=0;
unsigned long lastGetData=0;
unsigned long lastMqttReconnect=0;

uint8_t data[queue_size];
uint8_t front=0;
uint8_t rear=0;
uint8_t _front=0;
uint8_t _rear=0;

const char mqtt_server[15] = "188.68.48.86"; 
const uint16_t mqtt_port = 1883;
char topic[25];
char espID[10];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

NTPtime NTPch("ch.pool.ntp.org");///???
strDateTime dateTime;

uint8_t data_from_arduino_i=0;
uint8_t data_from_arduino_buff[num_byte_data_from_arduino]; // Array save data from arduino

void mqttPublic();
void write_data_to_flash();
boolean checkSum();
void mqtt_begin();
void enQueue(uint8_t *a);
void gpio_begin();
void flash_begin();
void read_data_from_arduino();
void write_data_to_flash()
{
  digitalWrite(PIN_LED, LOW);//BAT
  isSave2Flash=false;
  if(_front!=_rear&&front==rear)
  {
    EEPROM.write(queue_size+1, _front);
  }
  else if(_front==_rear&&front!=rear)
  {
    uint8_t __rear=(rear+queue_size-data_size)%queue_size;
    for(int i=0;i<data_size;i++)
    {
      EEPROM.write(_rear+i, data[__rear+i]);
    }
    _rear=(_rear+data_size)%queue_size;
    EEPROM.write(queue_size+1, _rear);
  }
  else if(_front!=_rear&&front!=rear)
  {
    uint8_t __rear=(rear+queue_size-data_size)%queue_size;
    for(int i=0;i<data_size;i++)
    {
      EEPROM.write(_rear+i, data[__rear+i]);
    }
    _rear=(_rear+data_size)%queue_size;
    EEPROM.write(queue_size+1, _rear);
    EEPROM.write(queue_size, front);
  }
  EEPROM.commit();
  front=rear;
  digitalWrite(PIN_LED, HIGH);//TAT
}
void gpio_begin()
{
  pinMode(PIN_BUTTON, INPUT);// Cấu hình GPIO cho chân button
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);//TAT
}

void flash_begin()
{
  digitalWrite(PIN_LED, LOW);//BAT
  EEPROM.begin(queue_size+2);

  front=EEPROM.read(queue_size);
  rear=EEPROM.read(queue_size+1);
  _front=front;
  _rear=rear;
  if(front%data_size!=0||rear%data_size!=0||front>=queue_size||rear>=queue_size)
  {
    front=0;
    rear=0;
  }
  uint8_t af=front;
  while(front!=rear)
  {
    for(int i=0;i<data_size;i++)
    {
      data[front+i]=EEPROM.read(front+i);
    }
    front=(front+data_size)%queue_size;
  }
  front=af;
  digitalWrite(PIN_LED, HIGH);//TAT
}

void read_data_from_arduino()
{
  data_from_arduino_buff[data_from_arduino_i]=Serial.read();
  data_from_arduino_i++;
  digitalWrite(PIN_LED, LOW);//BAT
  if(data_from_arduino_i==num_byte_data_from_arduino)
  {
    data_from_arduino_i=0;
    lastGetData=millis();
    if(data_from_arduino_buff[0]==66&&data_from_arduino_buff[1]==77)
    {
      if(checkSum())
      {
        enQueue(&data_from_arduino_buff[2]);
        digitalWrite(PIN_LED, HIGH);//TAT
      }
    }
  }
}
void enQueue(uint8_t *a)
{
  if(front==((rear+data_size)%queue_size))
  {
    for(int i=0;i<data_size;i++)
    {
      data[rear+i]=a[i];
    }
    front=(front+data_size)%queue_size;
    rear=(rear+data_size)%queue_size;
  }
  else
  {
    //digitalWrite(PIN_LED, LOW);//BAT
    for(int i=0;i<data_size;i++)
    {
      data[rear+i]=a[i];
    }
    rear=(rear+data_size)%queue_size;
  }
}

void mqtt_begin()
{
  uint8_t macAddess[6];
  WiFi.macAddress(macAddess);
  unsigned long a=macAddess[3]*256*256+macAddess[4]*256+macAddess[5];
  sprintf(topic,"/airsense/ESP_%08d/",a);
  sprintf(espID,"%08d",a);
  
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.connect(espID);
}
boolean checkSum()
{
  unsigned int check=0;
  for(int i=0;i<num_byte_data_from_arduino-2;i++)
  {
    check=check+data_from_arduino_buff[i];
  }
  unsigned int check2=256*data_from_arduino_buff[num_byte_data_from_arduino-2]+data_from_arduino_buff[num_byte_data_from_arduino-1];
  if(check==check2) return true;
  else return false;
}

void mqttPublic()
{
  digitalWrite(PIN_LED, HIGH);//TAT
  digitalWrite(PIN_LED, LOW);//BAT
  char message[256]={0};
  int ai=0;
  if(rear>front)
    ai=((rear-front)/data_size)-1;
  else
    ai=(rear/data_size)+queue_num-(front/data_size)-1;
  unsigned long now_s=dateTime.epochTime;
  if(lastGetTime>lastGetData)
    now_s=now_s-((lastGetTime-lastGetData)/1000);
  else
    now_s=now_s+((lastGetData-lastGetTime)/1000);

  sprintf(message,"{\"data\":{\"tem\":\"%d\",\"humi\":\"%d\",\"pm1\":\"%d\",\"pm2p5\":\"%d\",\"pm10\":\"%d\",\"time\":\"%d\",\"i\":\"%d\"}}",data[front],data[front+1],(data[front+2]*256+data[front+3]),(data[front+4]*256+data[front+5]),(data[front+6]*256+data[front+7]),(now_s-(ai*60)),ai);
  
  if(mqttClient.publish(topic, message,true))
  {
    front=(front+data_size)%queue_size;
  }
  digitalWrite(PIN_LED, HIGH);//TAT
}

