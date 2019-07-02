#include <SoftwareSerial.h>
#include <DHT.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define DHTPIN 7
#define DHTTYPE DHT22
#define num_byte_send_2_esp 12
#define time2SendData 59500
#define debug false
#define debug_dust false
#define debug_send false
#define time_out_dust 3000
#define max_num_data_dust time2SendData/600+1
#define max_num_data_dht time2SendData/2000+1

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial dustSerial =  SoftwareSerial(4,5); // RX, TX
SoftwareSerial espSerial =  SoftwareSerial(3,8);

uint8_t data2esp_buff[num_byte_send_2_esp];

boolean isResetESP=true;
unsigned long lastSendData=0;
unsigned long lastReadDHT=0;

struct data_dust
{
  uint8_t dust[6];
};
struct data_dht
{
  float temp;
  float humi;
};
data_dust datas_dust[max_num_data_dust];
data_dht datas_dht[max_num_data_dht];
uint8_t datas_dust_i=0;
uint8_t datas_dht_i=0;
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
    
    if(debug) Serial.println("setup done");
}
void loop()
{
    if(millis()-lastSendData>time2SendData)
    {
        lastSendData=millis();
       
        // 60s 1 lan: tinh trung binh, gui data cho esp
        // tinh trung binh
        float tem=0;
        float hum=0;
        for(int i=0;i<datas_dht_i;i++)
        {
            tem+=datas_dht[i].temp/datas_dht_i;
            hum+=datas_dht[i].humi/datas_dht_i;
        }
        datas_dht_i=0;
        float pm1=0;
        float pm25=0;
        float pm10=0;
        float _dust=0;
        for(int i=0;i<datas_dust_i;i++)
        {
            _dust=datas_dust[i].dust[0]*256+datas_dust[i].dust[1];
            pm1+=_dust/datas_dust_i;
            _dust=datas_dust[i].dust[2]*256+datas_dust[i].dust[3];
            pm25+=_dust/datas_dust_i;
            _dust=datas_dust[i].dust[4]*256+datas_dust[i].dust[5];
            pm10+=_dust/datas_dust_i;
        }
        datas_dust_i=0;

//        pm1=0.66776*pow(pm1,1.1);
//        pm25=1.33*pow(pm25,0.85);
//        pm10=1.06*pm10;
        
        //chuan bi data de gui cho esp
        unsigned int a=0;
        a=pm1+0.5;
        data2esp_buff[4]=a/256;
        data2esp_buff[5]=a%256;
        a=pm25+0.5;
        data2esp_buff[6]=a/256;
        data2esp_buff[7]=a%256;
        a=pm10+0.5;
        data2esp_buff[8]=a/256;
        data2esp_buff[9]=a%256;
        a=tem+0.5;
        data2esp_buff[2]=a;
        a=hum+0.5;
        data2esp_buff[3]=a;
        a=0;
        for(int i=0;i<num_byte_send_2_esp-2;i++)
        {
            a+=data2esp_buff[i];
        }
        data2esp_buff[num_byte_send_2_esp-2]=a/256;
        data2esp_buff[num_byte_send_2_esp-1]=a%256;
        //gui cho esp
        if(debug) Serial.print("send to ESP: ");
        for(int i=0;i<num_byte_send_2_esp;i++)
        {
            espSerial.write(data2esp_buff[i]);
            if(debug_send) Serial.print(data2esp_buff[i]);
            if(debug_send) Serial.print(" ");
        }
        if(debug) Serial.print("\\\\time: ");
        if(debug_send) Serial.println(millis());
        isResetESP=true;
    }
    else
    {
        if(isResetESP&&(millis()-lastSendData>(time2SendData-10000))) 
        {
            isResetESP=false;
            digitalWrite(2, HIGH);
            delay(100);
            digitalWrite(2, LOW);
        }
        // thoi gian con lai: lay data, hien thi len lcd, luu data
        // lay data
        // dust
        datas_dust[datas_dust_i].dust[0]=0;
        datas_dust[datas_dust_i].dust[1]=0;
        datas_dust[datas_dust_i].dust[2]=0;
        datas_dust[datas_dust_i].dust[3]=0;
        datas_dust[datas_dust_i].dust[4]=0;
        datas_dust[datas_dust_i].dust[5]=0;
        unsigned long start_time=millis();
        uint8_t dust_buff[32]={0}; // Array dust data
        uint8_t dust_i=0;
        while(true)
        {
            if(millis()-start_time>time_out_dust) break;
            if(dustSerial.available())
            {
                dust_buff[dust_i]=dustSerial.read();
                if(debug_dust) Serial.print(dust_buff[dust_i]);
                if(debug_dust) Serial.print(" ");
                if(dust_buff[0]==66) dust_i++;
                if(dust_i==32)
                {
                    if(debug_dust) Serial.println(millis());
                    dust_i=0;
                    if(dust_buff[1]==77)
                    {
                        //checksum
                        unsigned int check=0;
                        for(int i=0;i<30;i++)
                        {
                            check+=dust_buff[i];
                        }
                        if(debug) Serial.print("check: ");
                        if(debug) Serial.println(check);
                        unsigned int check2=dust_buff[30]*256+dust_buff[31];
                        if(debug) Serial.print("check2: ");
                        if(debug) Serial.println(check2);
                        if(check==check2)
                        {
                            // luu data
                            datas_dust[datas_dust_i].dust[0]=dust_buff[10];
                            datas_dust[datas_dust_i].dust[1]=dust_buff[11];
                            datas_dust[datas_dust_i].dust[2]=dust_buff[12];
                            datas_dust[datas_dust_i].dust[3]=dust_buff[13];
                            datas_dust[datas_dust_i].dust[4]=dust_buff[14];
                            datas_dust[datas_dust_i].dust[5]=dust_buff[15];
                            // hien thi len lcd
                            lcd.setCursor(6,0);
                            char _pm25[4];
                            float a_pm25=dust_buff[12]*256+dust_buff[13];
                            a_pm25=1.33*pow(a_pm25,0.85);
                            int b_pm25=a_pm25+0.5;
                            sprintf(_pm25,"%4d",b_pm25);
                            lcd.print(_pm25);
                            if(debug) Serial.print("PM2.5: ");
                            if(debug) Serial.print(dust_buff[12]*256+dust_buff[13]);
                            if(debug) Serial.print(" >> ");
                            if(debug) Serial.println(b_pm25);
                            // tang bien dem
                            if(datas_dust_i<max_num_data_dust) datas_dust_i++;
                        }
                        break;//thoat
                    }
                }
            }
        }
        // dht
      if(millis()-lastReadDHT>2000)
      {
        lastReadDHT=millis();
        datas_dht[datas_dht_i].temp=0;
        datas_dht[datas_dht_i].humi=0;
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        if(t!=0&&h!=0)
        {
          // luu data
          datas_dht[datas_dht_i].temp=t;
          datas_dht[datas_dht_i].humi=h;
          //hien thi len lcd
          int a=h+0.5;
          lcd.setCursor(4,1); //Colum-Row
          lcd.print(a);
          a=t+0.5;
          lcd.setCursor(12,1); //Colum-Row
          lcd.print(a);
          if(debug) Serial.print("do am: ");
          if(debug) Serial.println(h);
          if(debug) Serial.print("nhiet do: ");
          if(debug) Serial.println(t);
          // tang bien dem
          if(datas_dht_i<max_num_data_dht) datas_dht_i++;
        }
      }
    }
}

