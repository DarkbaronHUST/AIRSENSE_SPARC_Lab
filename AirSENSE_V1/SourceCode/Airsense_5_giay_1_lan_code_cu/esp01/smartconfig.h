#define PIN_LED 2
#define PIN_BUTTON 0
unsigned long lastPress = 0;
bool in_smartconfig = false; // Biến trạng thái kiểm tra thiết bị có đang trong chế độ smartconfig hay không.

/* Hàm kiểm tra trạng thái của button*/
bool longPress()
{
  if (millis() - lastPress > 3000 && digitalRead(PIN_BUTTON) == 0) 
  { 
    // Nếu button được nhấn và giữ trong 3s thì
    return true;
  } 
  else if (digitalRead(PIN_BUTTON) == 1) 
  { // Nếu button không được nhấn và giữ đủ 3s thì
    lastPress = millis(); // gán biến lastPress bằng thời điểm khi gọi hàm
  }
  return false;
}

/* Vào chế độ Smartconfig*/
void enter_smartconfig()
{
  digitalWrite(PIN_LED, LOW);
  if (in_smartconfig == false) 
  { // Kiểm tra tra biến trạng thái, nếu không ở chế độ smartconfig thì
    in_smartconfig = true; // Gán biến trạng thái bằng "true", nghĩa là đang trong smartconfig
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig(); // Bắt đầu chế độ smartconfig
    Serial.println("Enter smartconfig"); // In thông báo "Enter smartconfig" ra màn hình
  }
}

/* Thoát chế độ smartconfig*/
void exit_smart()
{
    in_smartconfig = false; // Gán biến trạng thái trở về ban đầu.
    Serial.println("Connected, Exit smartconfig"); // In thông báo ra màn hình.
    WiFi.mode(WIFI_AP_STA);
    digitalWrite(PIN_LED, HIGH);
}


