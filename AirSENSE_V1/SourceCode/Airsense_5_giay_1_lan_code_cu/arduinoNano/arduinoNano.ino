#include <SoftwareSerial.h>
#include <DHT.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define DHTPIN 7
#define DHTTYPE DHT22
#define num_byte_send_2_esp 12
#define time2GetData 5000

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial dustSerial =  SoftwareSerial(4,13); // RX, TX
SoftwareSerial espSerial =  SoftwareSerial(12,8);

boolean sendFlag=false;
boolean readDustFlag=false;

uint8_t t,h;
uint8_t data2esp_buff[num_byte_send_2_esp];
uint8_t dust_buff[63]; // Array save dust data
uint8_t dust_i=0;

unsigned long lastGetData=0;

void setup()
{
    Serial.begin(9600);
    espSerial.begin(115200);
    dustSerial.begin(9600);
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
    lcd.begin();
    lcd.setCursor(0,0); //Colum-Row
    lcd.print("PM2.5:---- ug/m3");
    lcd.setCursor(0,1); //Colum-Row
    lcd.print("Hum:--%");
    lcd.setCursor(8,1); //Colum-Row
    lcd.print("Tem:--*C");
    
    data2esp_buff[0]=66;
    data2esp_buff[1]=77;
    
    Serial.println("setup done");
}
void loop()
{
  if(millis()-lastGetData>time2GetData) 
  {
    lastGetData=millis();
    readDustFlag=true;
    t = dht.readTemperature();
    h = dht.readHumidity();
    if(t!=0||h!=0)
    {
      data2esp_buff[2]=t;
      data2esp_buff[3]=h;
      lcd.setCursor(4,1); //Colum-Row
      lcd.print(h);
      lcd.setCursor(12,1); //Colum-Row
      lcd.print(t);
      Serial.println(h);
      Serial.println(t);
    }
  }
  else if(readDustFlag==true&&dustSerial.available() > 0)
  {
    dust_buff[dust_i]=dustSerial.read();
    dust_i++;
    if(dust_i==63)
    {
      dust_i=0;
      sendFlag=true;
      readDustFlag=false;
      for(int i=0;i<63;i++)
      {
        if(dust_buff[i]==66&&dust_buff[i+1]==77)
        {
          //check sum
          int check=0;
          for(int j=i;j<i+30;j++)
          {
            check=check+dust_buff[j];
          }
          int check2=dust_buff[i+30]*256+dust_buff[i+31];
          if(check==check2)
          {
            data2esp_buff[4]=dust_buff[i+10];
            data2esp_buff[5]=dust_buff[i+11];
            data2esp_buff[6]=dust_buff[i+12];
            data2esp_buff[7]=dust_buff[i+13];
            data2esp_buff[8]=dust_buff[i+14];
            data2esp_buff[9]=dust_buff[i+15];
                    
            lcd.setCursor(6,0);
            char pm25[4];
            sprintf(pm25,"%4d",data2esp_buff[6]*256+data2esp_buff[7]);
            lcd.print(pm25);
            Serial.println(data2esp_buff[6]*256+data2esp_buff[7]);
          }
          break;//thoat for
        }
      }
    }
  }
  else if(sendFlag==true)
  {
    sendFlag=false;
    //send2esp
    int c;
    for(int i=0;i<num_byte_send_2_esp-2;i++)
    {
      c=c+data2esp_buff[i];
    }
    data2esp_buff[num_byte_send_2_esp-2]=c/256;
    data2esp_buff[num_byte_send_2_esp-1]=c%256;//check
    for(int i=0;i<num_byte_send_2_esp;i++)
    {
      espSerial.write(data2esp_buff[i]);
      Serial.print(data2esp_buff[i]);
      Serial.print(" ");
    } 
    Serial.println("  end");
  }
}

