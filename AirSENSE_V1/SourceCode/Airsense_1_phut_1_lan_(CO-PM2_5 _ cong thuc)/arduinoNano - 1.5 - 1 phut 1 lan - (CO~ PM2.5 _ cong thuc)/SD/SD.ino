#include<SoftwareSerial.h>
#include<Wire.h>
#include<LiquidCrystal_I2C.h>
#include<DHT.h>
#include<SPI.h>
#include<SD.h>
LiquidCrystal_I2C lcd(0x3f,16,2);
SoftwareSerial dustSerial=SoftwareSerial(4,5);
SoftwareSerial espSerial=SoftwareSerial(3,8);
#define MiDay 86400000
#define MiHours 3600000
#define MiMinute 60000
#define MiSecond 1000
#define debug false
#define DHTPIN 7
#define DHTTYPE DHT22
#define UART_2_DATA_ESP 18
#define TIME_SEND_2_DATA_MS 59500
#define MAX_NUM_DUST_SIZE TIME_SEND_2_DATA_MS/600+1
#define MAX_NUM_DHT_SIZE TIME_SEND_2_DATA_MS/2000+1
#define TIME_DUST_OUT_MS 3000
DHT dht(DHTPIN, DHTTYPE);
File Data;
char Filename[]="Da00.txt";
unsigned int DataESP[UART_2_DATA_ESP];
boolean ResetESP=true;
uint8_t Dust_i=0;
uint8_t Dht_i=0;
uint8_t UPm=0;
uint8_t UPm1=0;
uint8_t UPm10=0;
unsigned long LastTimeSendData=0;
unsigned long LastReadDht=0;
int SumPm25=0;
int SumPm10=0;
int SumPm1=0;
int SumTemp=0;
int SumHum=0;
int SumPm1Standrad=0;
int SumPm25Standrad=0;
int SumPm10Standrad=0;
int aaH;
int aaT;
int Time;
uint8_t UPm1Stan;
uint8_t UPm25Stan;
uint8_t UPm10Stan;
boolean Kt=true;
boolean ResetBoard=true;
void setup() {
  Serial.begin(9600);
  espSerial.begin(115200);
  dustSerial.begin(9600);
  pinMode(2, OUTPUT);
  SD.begin();
  if (debug) Serial.print("Khong begin duoc SD");
  pinMode(10, OUTPUT);
  digitalWrite(2, LOW);
  lcd.begin();
  lcd.setCursor(0,0);
  lcd.print("Pm2.5:---- ug/m3");
  lcd.setCursor(0,1);
  lcd.print("Hum:--%");
  lcd.setCursor(8,1);
  lcd.print("Tem:--*C");
  DataESP[0]=66;
  DataESP[1]=77;
  SD.remove("Da00.txt");
  //Serial.print(millis(),"/t");
  //Data=SD.open("Da00.csv", FILE_WRITE);
 // if (Data) Serial.print("ok");
  //Data.print("Node_id\tPM1\tPM25\tPM10\tTemperature\tHumidity\n");
 // Serial.print("Node_id\tPM1\tPM25\tPM10\tTemperature\tHumidity\n");
 // Serial.print("a\tb");
  //Serial.print("b");
  //Data.print("Node_id, ,Time Stamp, ,Temperature, ,Humidity, ,PM1, ,PM25, ,P10,\n"
  //Serial.println("alo");
}
boolean Kt1=false;
boolean Kt2=false;

void loop() {
  Data=SD.open(Filename, FILE_WRITE);
  //if (Data) Serial.print("ok");
  
  if (Kt)
  {
    Data.print("Node_id\t");Data.print("PM1\t");Data.print("PM1STAN\t");
    Data.print("PM25\t"); Data.print("PM25STAN\t");
    Data.print("PM10\t"); Data.print("PM10STAN\t");
    Data.print("Temperature\t"); Data.print("Humidity\n");
     Serial.print("Node_id\t");Serial.print("PM1\t"); 
     Serial.print("PM1TAN\t");Serial.print("PM25\t"); 
     Serial.print("PM25STAN\t");
     Serial.print("PM10\t");Serial.print("PM10STAN\t");
    Serial.print("Temperature\t"); Serial.print("Humidity\n");
  
  Kt=false;
  }
  //if (Data) Serial.print("ok");
  //Serial.print(dustSerial.available());
  
  //Serial.println(millis());
  //uint8_t xx=millis();
  //Serial.print(xx);
  if (millis()-LastTimeSendData>TIME_SEND_2_DATA_MS)
  {
   //Serial.print("OK");
    LastTimeSendData=millis();
    ResetESP=true;
    float fPm25=SumPm25/Dust_i;
    float fPm10=SumPm10/Dust_i;
    float fPm1=SumPm1/Dust_i;
    float fPm1Stan=SumPm1Standrad/Dust_i;
    float fPm25Stan=SumPm25Standrad/Dust_i;
    float fPm10Stan=SumPm10Standrad/Dust_i;
    float fTemp=SumTemp/Dht_i;
    float fHum=SumHum/Dht_i;
    int a=fTemp+0.5;
    Dust_i=0;
    Dht_i=0;
    DataESP[2]=a;
    a=fHum+0.5;
    DataESP[3]=a;
    a=fPm1+0.5;
    DataESP[4]=a/256;
    DataESP[5]=a%256;
    a=fPm25+0.5;
    DataESP[6]=a/256;
    DataESP[7]=a%256;
    a=fPm10+0.5;
    DataESP[8]=a/256;
    DataESP[9]=a%256;
    a=fPm1Stan+0.5;
    DataESP[10]=a/256;
    DataESP[11]=a%256;
    a=fPm25Stan+0.5;
    DataESP[12]=a/256;
    DataESP[13]=a%256;
    a=fPm10Stan+0.5;
    DataESP[14]=a/256;
    DataESP[15]=a%256;
    a=0;
    for (int i=0;i<=15;i++) a+=DataESP[i];
    DataESP[16]=a/256;
    DataESP[17]=a%256;
    for (int i=0;i<=UART_2_DATA_ESP-1;i++) espSerial.write(DataESP[i]);
    SumPm1=0;
    SumPm25=0;
    SumPm10=0;
    SumTemp=0;
    SumHum=0;
    SumPm1Standrad=0;
    SumPm25Standrad=0;
    SumPm10Standrad=0;
  }
  else
  {
    //Serial.println("Alo");
    //Serial.println(millis()-LastTimeSendData);
    //Serial.println("Ok");
    if (ResetESP&&(millis()-LastTimeSendData>(TIME_SEND_2_DATA_MS-10000)))
    {
      //USerial.println("Hello");
      ResetESP=false;
      digitalWrite(2, HIGH);
      delay(100);
      digitalWrite(2, LOW);
    }
    //Serial.println("KO");
    uint8_t Dust_buff[32]={0};
    //Serial.print(Dust_buff[32]);
    unsigned long StartTime=millis();
    //Serial.println(StartTime);
    uint8_t Dust__i=0;
    while(true)
    {
         //Serial.println("Oki");
            //Serial.println(millis()-StartTime);
      if (millis()-StartTime>TIME_DUST_OUT_MS) 
      {
        //Serial.print(Dust__i);
        //delay(2000);
        break;
      }
      
            if (dustSerial.available())
            {
              //Serial.println("ok1");
              Dust_buff[Dust__i]=dustSerial.read();
             
              if (Dust_buff[0]==66) Dust__i++;
              //Serial.println(Dust__i);
              //delay(500);
              if (Dust__i==32)
               {
               //Serial.println("khong");
                Dust__i=0;
                if (Dust_buff[1]==77)
                {
                  //Serial.println("co");
                  unsigned int check1=0;
                  //Serial.println(check1);
                  for (int i=0;i<=29;i++) 
                  {
                     //Serial.println(Dust_buff[i]);
                    // delay(1000);
                     
                    check1+=Dust_buff[i];
                    
                  }
                  
                  //Serial.println(check1);
                  unsigned int check2=Dust_buff[30]*256+Dust_buff[31];
                  //Serial.println(check2);
                  if(check1==check2)
                  {
                    //Serial.println("ok");
                    SumPm1Standrad+=Dust_buff[4]*256+Dust_buff[5];
                    SumPm25Standrad+=Dust_buff[6]*256+Dust_buff[7];
                    SumPm10Standrad+=Dust_buff[8]*256+Dust_buff[9];
                    SumPm1+=Dust_buff[10]*256+Dust_buff[11];
                    SumPm25+=Dust_buff[12]*256+Dust_buff[13];
                    SumPm10+=Dust_buff[14]*256+Dust_buff[15]; 
                  
                  uint8_t uPm=Dust_buff[12]*256+Dust_buff[13];
                   UPm=uPm+0.5;
                  float fPm=1.33*pow(uPm,0.85);
                  uint8_t uuPm25=fPm+0.5;
                  char Pm25[4];
                  
                  sprintf(Pm25,"%4d",uuPm25);
                  lcd.setCursor(6,0);
                  lcd.print(Pm25);
                  //Data.print(UPm); Data.print("\t");// Data SD khac Data in len lcd
                 // Serial.print(UPm);Serial.print("\t");
                 // Serial.println(Pm25);
                  
                  // in PM1 vs Pm10 sang SD
                  uPm=Dust_buff[10]*256+Dust_buff[11];
                   UPm1=uPm+0.5;
                  //Data.print(UPm1);Data.print("\t");
                  //Serial.print(UPm1);Serial.print("\t");
                  uPm=Dust_buff[14]*256+Dust_buff[15];
                  UPm10=uPm+0.5;
                  //Data.print(UPm10);Data.print("\t");
                  //Serial.print(UPm10);Serial.print("\t");
                   UPm1Stan=(Dust_buff[4]*256+Dust_buff[5])+0.5;
                   UPm25Stan=(Dust_buff[6]*256+Dust_buff[7])+0.5;
                   UPm10Stan=(Dust_buff[8]*256+Dust_buff[9])+0.5;
                  if (Dust_i<MAX_NUM_DUST_SIZE) Dust_i++;
                  Kt1=true;
             //     Serial.println(Kt1);
                  }
                  break;
                }
               }
            
          }
      
    }
    //Serial.print("ok");
    if (millis()-LastReadDht>2000)
    {
      LastReadDht=millis();
      uint8_t t=dht.readTemperature();
      uint8_t h=dht.readHumidity();
      if(t!=0&&h!=0)
      {
        SumTemp+=t;
        SumHum+=h;
         aaT=t+0.5;
        lcd.setCursor(12,1);
        lcd.print(aaT);
        //Data.print(aaT);Data.print("\t");
        //Serial.print(aaT);Serial.print("\t");
         aaH=h+0.5;
        lcd.setCursor(4,1);
        lcd.print(aaH);
        //Data.print(aaH);Data.print("\t");
        //Serial.print(aaH);Serial.print("\t");
        if (Dht_i<MAX_NUM_DHT_SIZE) Dht_i++;
        Kt2=true;
        //Serial.println(Kt2);
      }
    }
    if (Kt1&&Kt2)
    {
      //Serial.println("Da ok");
      Kt1=false;
      Kt2=false;
      Data.print("ESP_00981683");
      Data.print("\t");
      Serial.print("ESP_00981683");
      Serial.print("\t");
      Data.print(UPm1); Data.print("\t");
      Serial.print(UPm1); Serial.print("\t");
      Data.print(UPm1Stan);Data.print("\t");
      Serial.print(UPm1Stan); Serial.print("\t");
      Data.print(UPm); Data.print("\t");
      Serial.print(UPm); Serial.print("\t");
      Data.print(UPm25Stan); Data.print("\t");
      Serial.print(UPm25Stan);Serial.print("\t");
      Data.print(UPm10); Data.print("\t");
      Serial.print(UPm10); Serial.print("\t");
      Data.print(UPm10Stan); Data.print("\t");
      Serial.print(UPm10Stan); Serial.print("\t");
      Data.print(aaT);Data.print("\t");
      Serial.print(aaT);Serial.print("\t");
      Data.print(aaH); Data.print("\t");
      Serial.print(aaH); Serial.print("\t");
       Time=millis();
//      SendTime;
      Data.print("\n");Serial.print("\n");

    }
    //Data.print("\n");
    //Serial.print("\n");
  }
  Data.close();
  
}
