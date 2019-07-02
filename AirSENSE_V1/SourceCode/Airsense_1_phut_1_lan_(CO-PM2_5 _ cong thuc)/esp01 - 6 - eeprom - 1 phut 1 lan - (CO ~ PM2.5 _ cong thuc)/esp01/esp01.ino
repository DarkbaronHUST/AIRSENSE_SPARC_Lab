#include "./esp01.h"
void setup() 
{
	// put your setup code here, to run once:
	Serial.begin(115200); // Tốc độ baud = 115200
	gpio_begin();
	WiFi.mode(WIFI_STA);
	flash_begin();
	mqtt_begin();
}

void loop() 
{
	if(millis()>53000&&isSave2Flash)
	{
		write_data_to_flash();
	}
	if (longPress()) enter_smartconfig(); 
	if (WiFi.status() == WL_CONNECTED && in_smartconfig && WiFi.smartConfigDone()) exit_smart();
	if(Serial.available() > 0)
	{
		read_data_from_arduino();
	}
	else if (WiFi.status() == WL_CONNECTED)
	{
		if(isGetTime)
		{
			dateTime = NTPch.getNTPtime(7.0, 0);
			digitalWrite(PIN_LED, LOW);//BAT	  
			if(dateTime.valid)
			{
				digitalWrite(PIN_LED, HIGH);//TAT
				lastGetTime=millis();
				isGetTime=false;
			}
		}
		else if(front!=rear)
		{
			if(mqttClient.connected())
			{
				mqttPublic();
				mqttClient.loop();
			}
			else if(millis()-lastMqttReconnect>1000)
			{
				digitalWrite(PIN_LED, LOW);//BAT
				lastMqttReconnect=millis();
				mqttClient.connect(espID);
			}
		}
	}
}
