#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <NTPtimeESP.h>
//#include <ESP8266httpUpdate.h>
#include "./smartconfig.h"

#define num_net 3
#define max_num_data_save 300
#define max_num_mac_save 20
#define num_byte_data_from_arduino 12

#define time2GetMac 15*60*1000
#define time2GetTime 60*60*1000
#define time2Update 12*60*60*1000

struct mac
{
  int32_t rssi;
  String bssid;
};
struct locat
{
  mac mac_array0[num_net];
  unsigned long real_time;
};
struct data
{
  uint8_t temp;
  uint8_t humi;
  uint16_t dust[3];  
  unsigned long real_time;
};

boolean isPublic=false;
boolean isGetTime=true;
boolean isUpdate=true;
boolean isWifiReconnect=false;

unsigned long lastGetData=0;
unsigned long lastGetMac=0;
unsigned long lastGetTime=0;
unsigned long lastRequestTime=0;
unsigned long lastWifiReconnect=0;
unsigned long lastMqttReconnect=0;
unsigned long last=0;
unsigned long lastUpdate=0;
unsigned long lastRequestUpdate=0;


const char mqtt_server[15] = "188.68.48.86"; 
const uint16_t mqtt_port = 1883;
char topic[25];
char espID[10];


const char *password = "12345678";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";



ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

NTPtime NTPch("ch.pool.ntp.org");
strDateTime dateTime;

//t_httpUpdate_return ret;

uint8_t data_from_arduino_i=0;
uint8_t data_from_arduino_buff[num_byte_data_from_arduino]; // Array save data from arduino

data data_array[max_num_data_save];
uint8_t data_array_i=3;

locat mac_array[max_num_mac_save];
uint8_t mac_array_i=2;

void saveScanResult(int networksFound);
void mqttPublic();
void saveData();
void deleteData();
boolean checkSum();
void getMqttTopic();
void getMqttTopic()
{
  uint8_t macAddess[6];
  WiFi.macAddress(macAddess);
  unsigned long a=macAddess[3]*256*256+macAddess[4]*256+macAddess[5];
  sprintf(topic,"/airsense/ESP_%08d/",a);
  sprintf(espID,"%08d",a);
}
boolean checkSum()
{
  int check=0;
  for(int i=0;i<num_byte_data_from_arduino-2;i++)
  {
    check=check+data_from_arduino_buff[i];
  }
  int check2=256*data_from_arduino_buff[num_byte_data_from_arduino-2]
  +data_from_arduino_buff[num_byte_data_from_arduino-1];
  if(check==check2) return true;
  else return false;
}
void deleteData()
{
  data_array_i=3;
//  for(int i=0;i<data_array_i;i++)
//  {
//    data_array[i]=data_array[i+1];
//  }
}
void saveData()
{
  data_array[data_array_i].temp=data_from_arduino_buff[2];
  data_array[data_array_i].humi=data_from_arduino_buff[3];
  for(int i=0;i<3;i++)
  {
    data_array[data_array_i].dust[i]=256*data_from_arduino_buff[2*i+4]
    +data_from_arduino_buff[2*i+5];
  }
  data_array[data_array_i].real_time=dateTime.epochTime+(millis()-lastGetTime)/1000;
  data_array_i++;
  isPublic=true;
}
void mqttPublic()
{
  digitalWrite(PIN_LED, LOW);//BAT
  Serial.println("Publish message: ");
  String _name[3]={"pm1","pm2p5","pm10"};
  String data = "{\"data\":{\"tem\":\""
  + String(data_array[data_array_i-1].temp) 
  + "\",\"humi\":\""
  + String(data_array[data_array_i-1].humi);
  for(int i=0;i<3;i++)
  {
     data=data+"\",\""+_name[i]+"\":\""
     +String(data_array[data_array_i-1].dust[i]);
  }
  data=data + "\",\"time\":\""
  + String(data_array[data_array_i-1].real_time)
  + "\",\"i\":\""
  + String(data_array_i-1);
  if(mac_array_i>1)
  {
    mac_array_i--;
    data=data+"\"},\"locat\":{\"time\":\""
    + String(mac_array[mac_array_i].real_time);
    for(int i=0;i<num_net;i++)
    {
      data=data+"\",\"bssid"+String(i)+"\":\""
      +mac_array[mac_array_i].mac_array0[i].bssid
      +"\",\"rssi"+String(i)+"\":\""
      +String(mac_array[mac_array_i].mac_array0[i].rssi);
    }
    data=data
      + "\",\"i\":\""
      + String(mac_array_i)
      +"\"}}";
  }
  else
  {
    data=data+"\"}}";
  }
  char * message = new char[data.length() + 1];
  std::copy(data.begin(), data.end(), message);
  message[data.length()] = '\0'; // don't forget the terminating 0
  Serial.println(message);
  if(mqttClient.publish(topic, message,true))
  {
    data_array_i--;
  }
  if(data_array_i<=1)
  {
    isPublic=false;
  }
//  if(data_array_i<=2)
//  {
//    mqttClient.disconnect();
//  }
  
}
void saveScanResult(int networksFound)
{
  Serial.printf("%d network(s) found\n", networksFound);
  if(networksFound>num_net)
  {
    mac mac_array1[networksFound];
    for (int i = 0; i < networksFound; i++)
    {
      mac_array1[i].rssi=WiFi.RSSI(i);
      mac_array1[i].bssid=WiFi.BSSIDstr(i);
    }
    for (int i=0; i<networksFound; i++) 
    {
      for (int j=networksFound-1; j>i; j--)
      {
        if(mac_array1[j - 1].rssi < mac_array1[j].rssi) 
        {
            mac a = mac_array1[j-1];
            mac_array1[j-1] = mac_array1[j];
            mac_array1[j] = a;
        }
      }
    }
    for(int i = 0; i < num_net; i++)
    {
      mac_array[mac_array_i].mac_array0[i]=mac_array1[i];
    }
  }
  else
  {
    for(int i = 0; i < num_net; i++)
    {
      if(i<networksFound)
      {
        mac_array[mac_array_i].mac_array0[i].rssi=WiFi.RSSI(i);
        mac_array[mac_array_i].mac_array0[i].bssid=WiFi.BSSIDstr(i);
      }
      else
      {
        mac_array[mac_array_i].mac_array0[i].bssid="";
        mac_array[mac_array_i].mac_array0[i].rssi=0;
      }
    }
  }
  mac_array[mac_array_i].real_time=dateTime.epochTime+(millis()-lastGetTime)/1000;
  WiFi.scanDelete();
  mac_array_i++;
  isWifiReconnect=true;
}
