#include <Wire.h>
#include <INA226.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Ticker.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define WIFI_SSID "Hieu 2G"
#define WIFI_PASSWORD ""
#define VERSION String(__DATE__) + " " + String(__TIME__)
#define OLED_RESET    -1 // Reset không cần thiết với I2C
Adafruit_SSD1306 display(128,64, &Wire, OLED_RESET);
const int BUTTON_UP = 15;
const int BUTTON_DOWN = 4;
const int BUTTON_SELECT =13;
const int LED_BUILTIN =2;
Ticker timer; 
Ticker timer2; 
INA226 ina2(0x40); //ina nap
INA226 ina(0x41);
String dungluongconlai;
String ssudung;
String snapvao = "";
String thongbao="";
float energy_Wh = 0.0;
float energy_Wh_nap = 0.0;
float dongnapmax=0;
float watnapmax=0;
int demsolancbireset=0;
bool canReset = false;
unsigned long lastTime = 0;
WebServer server(80);
void setup() {

  Serial.begin(115200);
  Wire.begin();
  //khởi tạo nút
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  if(ina.begin() )
  {
    ina.reset();
    ina.configure(0.00242,1.0,5.0,10066);
    ina.setAverage(INA226_512_SAMPLES);
  }
  if(ina2.begin() )
  {
    ina2.reset();
    ina2.configure(0.00193,1.0,-4.0,10066);
    ina2.setAverage(INA226_512_SAMPLES);
  }
  // Wi-Fi connection setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi IP:"+WiFi.localIP().toString());
  }
    // Khởi tạo màn hình
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Không tìm thấy màn hình OLED"));
    for (;;);
  }

  setupOTA();
  timer.attach(1.0, getpower);
  timer2.attach(60.0, tatmanhinh);
}
void tatmanhinh()
{
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  timer2.detach();
}
void hienThiOLED(String noidung, int x = 0, int y = 0, int size = 1) {
  display.setTextSize(size);             
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(x, y);              
  display.println(noidung);
  display.display();
}
void kiemTraResetNgayMoi(float currentnap) {
  if (currentnap < 0.01) {
    if (demsolancbireset < 3600) {
      demsolancbireset++;
    }

    if (demsolancbireset >= 3600) {
      canReset = true;
    }
  } else {
    if (canReset) {
      energy_Wh = 0;
      energy_Wh_nap = 0;
      dongnapmax = 0;
      watnapmax = 0;
      canReset = false;
      demsolancbireset = 0;
    } else {
      demsolancbireset = 0;
    }
  }
}
void getpower() {
  thongbao="";
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;
  if(!ina.begin() )
  {
    ssudung="Không tìm thấy module INA226 Xả;";
  }
  else
  {
  float voltage = ina.getBusVoltage();
  float current = ina.getCurrent_mA() / 1000.0;
  thongbao+="Điện áp sử dụng:"+String(voltage,3)+"v\\n";
  thongbao+="Dòng sử dụng:"+String(current,3)+"A\\n";
  float batteryPercent = (voltage - 11.0) / (12.6 - 11.0) * 100.0;
  if(batteryPercent>100.0)batteryPercent=100;
  float power = voltage * current;
  energy_Wh += power * dt / 3600.0;
  dungluongconlai =String(batteryPercent,0)+"% | "+ String(voltage, 2) + "v | "+String(energy_Wh_nap-energy_Wh,2)+"Wh | "+String((energy_Wh_nap-energy_Wh)/power,1)+"h";
  ssudung = String(current,2) + "A | " 
          + String(power,2) + "W | " 
          + String(energy_Wh,2) + "Wh"; 
  }
  if(!ina2.begin() )
  {
    snapvao="Không tìm thấy module INA226 Nạp;";
  }
  else
  {
  float voltagenap = ina2.getBusVoltage();
  float currentnap = ina2.getCurrent_mA() / 1000.0;
  thongbao+="Điện áp nạp:"+String(voltagenap,3)+"v\\n";
  thongbao+="Dòng nạp:"+String(currentnap,3)+"A\\n";
  kiemTraResetNgayMoi(currentnap);
  float powernap = voltagenap * currentnap;
  if(dongnapmax<currentnap)dongnapmax=currentnap;
  if(watnapmax<powernap)watnapmax=powernap;
  thongbao+="Dòng max:"+String(dongnapmax,2)+"A;\\nCông suất max:"+String(watnapmax,0)+"w\\n";
  thongbao+="Đếm số lần cbi reset:"+String(demsolancbireset)+"\\n";
  thongbao+="Có thể reset:"+String(canReset)+"\\n";
  energy_Wh_nap += powernap * dt / 3600.0;
  // Cập nhật chuỗi hiển thị
  snapvao = String(currentnap,2) + "A | " 
          + String(powernap,2) + "W | "
          + String(energy_Wh_nap,2) + "Wh";
  }
  display.clearDisplay();
  hienThiOLED(dungluongconlai,0,0,1);
  hienThiOLED(snapvao,0,16,1);
  hienThiOLED(ssudung,0,32,1);
}
void setupOTA() {
  server.on("/", handleRoot);
  server.on("/caidat",web_caidat);
  server.on("/savesetting1", HTTP_POST, handleSaveSetting1);
  server.on("/savesetting2", HTTP_POST, handleSaveSetting2);
  server.on("/updatefw", updatefw);
  server.on("/data", handleData);
  server.on("/update", HTTP_POST, []() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='3;url=/'>";
  html += "</head><body>";
  html += "<h2>Cập nhật thành công!</h2>";
  html += "<p>Đang khởi động lại...</p>";
  html += "</body></html>";
  server.send(200, "text/html;charset=UTF-8", html);
  delay(2000);
  ESP.restart();
}, handleUpdate);
  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain;charset=UTF-8", "Đang khởi động lại...");
    delay(1000);
    ESP.restart();
  });
  server.begin();
  Serial.println("Máy chủ OTA HTTP đã bắt đầu.");
}
void loop() {
  server.handleClient();
   if (digitalRead(BUTTON_UP) == LOW or digitalRead(BUTTON_DOWN) == LOW or digitalRead(BUTTON_SELECT) == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
    display.ssd1306_command(SSD1306_DISPLAYON);
    timer2.attach(30.0, tatmanhinh);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
  }
  delay(100);
}
// Handle OTA update
void handleUpdate() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: Start uploading file: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update: Successfully uploaded %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
  }
}
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', sans-serif; background: #f5f5f5; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; }";
  html += "header { background: #4CAF50; color: white; padding: 20px; width: 100%; text-align: center; }";
  html += "main { padding: 20px; width: 100%; max-width: 600px; }";
  html += ".card { background: white; box-shadow: 0 2px 5px rgba(0,0,0,0.1); border-radius: 10px; padding: 20px; margin: 10px 0; }";
  html += ".card h2 { margin-top: 0; font-size: 1.2em; color: #333; }";
  html += ".value { font-size: 1.5em; color: #007BFF; font-weight: bold; }";
  html += "footer { margin-top: 30px; font-size: 0.9em; color: #888; text-align: center; }";
  html += "form { margin-top: 20px; text-align: center; }";
  html += "input[type='submit'] { background: #4CAF50; color: white; border: none; padding: 10px 20px; font-size: 1em; border-radius: 5px; cursor: pointer; }";
  html += "input[type='submit']:hover { background: #45a049; }";
  html += "</style>";
  html += "<script>";
  html += "function fetchData() {";
  html += "  fetch('/data')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('dungluongconlai').innerText = data.dungluongconlai;";
  html += "      document.getElementById('ssudung').innerText = data.ssudung;";
  html += "      document.getElementById('snapvao').innerText = data.snapvao;";
  html += "      document.getElementById('thongbao').innerText = data.thongbao;";
  html += "    });";
  html += "}";
  html += "setInterval(fetchData, 2000);";
  html += "</script>";
  html += "</head><body>";
  html += "<header><h1>Hệ thống MLMT mini 12V</h1></header>";
  html += "<main>";
  html += "<div class='card'><h2>Dung lượng còn lại</h2><div class='value' id='dungluongconlai'>Loading</div></div>";
  html += "<div class='card'><h2>Nạp vào</h2><div class='value' id='snapvao'>Loading</div></div>";
  html += "<div class='card'><h2>Sử dụng</h2><div class='value' id='ssudung'>Loading</div></div>";
html += "<div class='card'><h2>Thông báo</h2><div class='value' id='thongbao' style='white-space: pre-line;'>Loading</div></div>";
  html += "<form action='/caidat' method='GET'><input type='submit' value='Cài đặt'></form>";
  html += "<form action='/updatefw' method='GET'><input type='submit' value='Cập nhật Firmware'></form>";
  html += "<footer>";
  html += "Ver: " + VERSION + "<br>";
  html += "IP: " + WiFi.localIP().toString();
  html += "</footer>";
  html += "</main></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

void handleData() {
  String json = "{";
  json += "\"dungluongconlai\": \"" +String(dungluongconlai) + "\",";
  json += "\"ssudung\": \"" + String(ssudung) + "\",";
  json += "\"snapvao\": \"" + String(snapvao) + "\",";
  json += "\"thongbao\": \"" + String(thongbao) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}
void updatefw() {
  String html = "<html><head>";html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"; // Thiết lập viewport cho responsive
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; display: flex; justify-content: flex-start; align-items: center; height: 100vh; flex-direction: column; text-align: center; padding: 20px; }"; // Canh giữa nội dung
  html += "h1 { margin: 0; font-size: 2em; }";
  html += "form { margin: 20px 0; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Cập nhật phần mềm</h1>";
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>"; // Form cho việc tải lên file
  html += "<input type='file' name='update'>";
  html += "<input type='submit' value='Cập nhật'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}
void web_caidat() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', sans-serif; background: #f5f5f5; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; }";
  html += "header { background: #4CAF50; color: white; padding: 20px; width: 100%; text-align: center; }";
  html += "main { padding: 20px; width: 100%; max-width: 600px; }";
  html += ".card { background: white; box-shadow: 0 2px 5px rgba(0,0,0,0.1); border-radius: 10px; padding: 20px; margin: 10px 0; }";
  html += ".card h2 { margin-top: 0; font-size: 1.2em; color: #333; }";
  html += ".value { font-size: 1.2em; margin-top: 10px; }";
  html += "input[type='text'] { width: 100%; padding: 10px; font-size: 1em; border: 1px solid #ccc; border-radius: 5px; }";
  html += "input[type='submit'] { background: #4CAF50; color: white; border: none; padding: 10px 20px; font-size: 1em; border-radius: 5px; cursor: pointer; margin-top: 10px; }";
  html += "input[type='submit']:hover { background: #45a049; }";
  html += "input[type='text'], input[type='number'] {width: 100%;padding: 10px;font-size: 1em;box-sizing: border-box;border: 1px solid #ccc;border-radius: 5px;}";
  html += "</style>";
  html += "</head><body>";
  html += "<header><h1>Cài đặt hệ thống</h1></header>";
  html += "<main>";

  // Mỗi card là một biểu mẫu riêng
  html += "<form method='POST' action='/savesetting1'><div class='card'>";
  html += "<h2>Cài lại Wh sử dụng</h2>";
  html += "<input type='number' min='0' name='setting1' value='" + String((int)energy_Wh) + "'>";
  html += "<input type='submit' value='Lưu'>";
  html += "</div></form>";

  html += "<form method='POST' action='/savesetting2'><div class='card'>";
  html += "<h2>Cài lại Wh nạp</h2>";
  html += "<input type='number' min='0' name='setting2' value='" + String((int)energy_Wh_nap) + "'>";
  html += "<input type='submit' value='Lưu'>";
  html += "</div></form>";

  html += "</main></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}
void handleSaveSetting1() {
  if (server.hasArg("setting1")) {
    String value = server.arg("setting1");
    Serial.println("Đã lưu setting1: " + value);
    energy_Wh = value.toFloat();
  }
  server.sendHeader("Location", "/caidat");
  server.send(303);
}
void handleSaveSetting2() {
  if (server.hasArg("setting2")) {
    String value = server.arg("setting2");
    Serial.println("Đã lưu setting2: " + value);
    energy_Wh_nap = value.toFloat();
  }
  server.sendHeader("Location", "/caidat");
  server.send(303);  // Redirect về trang settings
}
