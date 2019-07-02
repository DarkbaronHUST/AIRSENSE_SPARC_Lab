#include "./esp01.h"
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200); // Tốc độ baud = 115200
  Serial.setDebugOutput(true);
  pinMode(PIN_BUTTON, INPUT);// Cấu hình GPIO cho chân button
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);//TAT
  mqttClient.setServer(mqtt_server, mqtt_port);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.enableSTA(true);
  WiFi.enableAP(true);
  getMqttTopic();
  WiFi.softAP(topic, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.on("/", handleRoot);
  httpServer.begin();
  Serial.println("HTTP server started");
  
  mqttClient.connect(espID);
  
  dateTime.epochTime=0;
  Serial.println("Setup done"); 
}
void handleRoot() {
  httpServer.send(200, "text/html", "<h1>You are connected  "+String(topic)+"</h1>");
}
void loop() 
{
  httpServer.handleClient();
  digitalWrite(PIN_LED, HIGH);//TAT
  if (longPress()) enter_smartconfig(); 
  if (WiFi.status() == WL_CONNECTED && in_smartconfig && WiFi.smartConfigDone()) exit_smart();
  if(Serial.available() > 0)
  {
    data_from_arduino_buff[data_from_arduino_i]=Serial.read();
    data_from_arduino_i++;
  }
  else if((millis()-lastWifiReconnect>1000)&&isWifiReconnect==true)
  {
    lastWifiReconnect=millis();
    digitalWrite(PIN_LED, LOW);//BAT
    if(WiFi.reconnect()) isWifiReconnect=false;
  }
  else if(data_from_arduino_i==num_byte_data_from_arduino)
  {
    data_from_arduino_i=0;
    if(data_from_arduino_buff[0]==66&&data_from_arduino_buff[1]==77)
    {
      if(checkSum())
      {
        saveData();
        if(data_array_i==max_num_data_save-1) deleteData();
      }
    }
  }
  else if(millis()-lastGetMac>time2GetMac)
  {
    lastGetMac=millis();
    WiFi.scanNetworksAsync(saveScanResult);
  }
  else if (WiFi.status() == WL_CONNECTED) 
  {
    if(millis()-lastGetTime>time2GetTime||isGetTime==true)
    {
      if(millis()-lastRequestTime>1000)
      {
        lastRequestTime=millis();
        dateTime = NTPch.getNTPtime(7.0, 0);
        digitalWrite(PIN_LED, LOW);//BAT
      }
      else if(dateTime.valid)
      {
        lastGetTime=millis();
        isGetTime=false;
        NTPch.printDateTime(dateTime);
      }
    }
//    else if(millis()-lastUpdate>time2Update||isUpdate==true)
//    {
//      if(millis()-lastRequestUpdate>1000)
//      {
//        lastRequestUpdate=millis();
//        ret = ESPhttpUpdate.update("192.168.11.17",8888,"/update","1.1.2");
//        digitalWrite(PIN_LED, LOW);//BAT
//      }
//      else if(ret==HTTP_UPDATE_NO_UPDATES)
//      {
//        lastUpdate=millis();
//        isUpdate=false;
//      }
//    }
    else if(isPublic==true&&millis()-lastMqttReconnect>(2000+rand()))
    {
      digitalWrite(PIN_LED, LOW);//BAT
      if (mqttClient.connected()==0) 
      {
        lastMqttReconnect=millis();
        mqttClient.connect(espID);
      }
      else mqttPublic();
      mqttClient.loop();
    }
  }
}
