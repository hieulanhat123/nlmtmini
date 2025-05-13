#include <Wire.h>
#include <INA226.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Ticker.h>
#define WIFI_SSID "Hieu 2G"
#define WIFI_PASSWORD ""
#define VERSION String(__DATE__) + " " + String(__TIME__)

Ticker timer; 
INA226 ina(0x40);
String dungluongconlai;
String ssudung = "đang cập nhật";
String snapvao = "đang cập nhật";
String thongbao="đang cập nhật";
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  // Wi-Fi connection setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi IP:"+WiFi.localIP().toString());
  }
  ina.setMaxCurrentShunt(30, 0.00215);
  setupOTA();
  timer.attach(1.0, getpower);
}
void getpower()
{
  dungluongconlai=String(ina.getBusVoltage(),2) +"v";
  ssudung=String(ina.getBusVoltage(),2) +"v | "+String(ina.getCurrent_mA()/1000,2)+"A | "+ String(ina.getPower_mW()/1000,2)+"W";

  Serial.print("Điện áp Bus (V): ");
  Serial.println(ina.getBusVoltage(), 3);

  Serial.print("Điện áp Shunt (mV): ");
  Serial.println(ina.getShuntVoltage_mV(), 3);

  Serial.print("Dòng (mA): ");
  Serial.println(ina.getCurrent_mA(), 3);

  Serial.print("Công suất (mW): ");
  Serial.println(ina.getPower_mW(), 3);
}
void setupOTA() {
  // Trang gốc với biểu mẫu OTA và biểu mẫu Khởi động lại
  server.on("/", handleRoot);
  server.on("/updatefw", updatefw);
   server.on("/trangchu2", handleRoot2);
  // Thêm router cho yêu cầu dữ liệu
  server.on("/data", handleData);
  // Xử lý OTA trên /update
  server.on("/update", HTTP_POST, []() {
  // Sau khi upload xong, ở đây phản hồi cho client
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
  // Tuyến đường cho việc khởi động lại ESP32
  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain;charset=UTF-8", "Đang khởi động lại...");
    delay(1000); // Đợi một giây để cho phép tin nhắn được gửi
    ESP.restart(); // Khởi động lại ESP32
  });
  server.begin();
  Serial.println("Máy chủ OTA HTTP đã bắt đầu.");
}
void loop() {
  server.handleClient();
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
      // KHÔNG server.send() ở đây nữa!
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
  }
}
void handleRoot() {
  String html = "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"; // Thiết lập viewport cho mobile
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; display: flex; justify-content: flex-start; align-items: center; height: 100vh; flex-direction: column; text-align: center; padding-top: 20px; }"; // Căn giữa theo chiều dọc và chiều ngang
  html += "h1 { margin: 0; font-size: 1.5em; }"; // Kích thước chữ lớn hơn cho tiêu đề
  html += "h2 { margin: 10px 0; font-size: 1.5em; }"; // Kích thước chữ cho h2
  html += "div { font-size: 1.2em; margin: 5px 0; }"; // Kích thước chữ cho các div
  html += "form { margin-top: 20px; }"; // Thêm khoảng cách trên form
  html += "input[type='submit'] { padding: 10px 20px; font-size: 1em; }"; // Định dạng nút submit
  html += "</style>";
  html += "<script>";
  // Thêm một đoạn script để định kỳ gọi thông tin
  html += "function fetchData() {";
  html += "  fetch('/data')"; // Gọi đến /data để lấy thông số
  html += "    .then(response => response.json())"; // Nhận dữ liệu JSON
  html += "    .then(data => {";
  html += "      document.getElementById('dungluongconlai').innerText = data.dungluongconlai;";
  html += "      document.getElementById('ssudung').innerText = data.ssudung;";
  html += "      document.getElementById('snapvao').innerText = data.snapvao;";
  html += "      document.getElementById('thongbao').innerText = data.thongbao;";
  html += "    });";
  html += "}";
  html += "setInterval(fetchData, 1000);"; // Cập nhật mỗi 1 giây
  html += "</script>";
  html += "</head><body>";
  html += "<h1>Hệ thống MLMT mini 12v</h1>";
  html += "<br><br><br>";
  html += "<h2>Dung lượng còn lại</h2>";
  html += "<div id='dungluongconlai'>Dung lượng còn lại: N/A</div>";
  html += "<h2>Nạp vào</h2>";
  html += "<div id='snapvao'>Sạc vào: N/A</div>";
  html += "<h2>Sử dụng</h2>";
  html += "<div id='ssudung'>Sử dụng: N/A</div>";
  html += "<h2>Thông báo</h2>";
  html += "<div id='thongbao'>Thông báo: N/A</div>";
  html += "<br><br><br>";
  html += "<div>Ver: " + VERSION + "<div>";
  html += "<div>IP: " + WiFi.localIP().toString() + "</div>";
  html += "<form action='/updatefw' method='GET'>";
  html += "<input type='submit' value='Cập nhật FW'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}
void handleRoot2() {
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
  html += "setInterval(fetchData, 1000);";
  html += "</script>";
  html += "</head><body>";
  html += "<header><h1>Hệ thống MLMT mini 12V</h1></header>";
  html += "<main>";
  html += "<div class='card'><h2>Dung lượng còn lại</h2><div class='value' id='dungluongconlai'>N/A</div></div>";
  html += "<div class='card'><h2>Nạp vào</h2><div class='value' id='snapvao'>N/A</div></div>";
  html += "<div class='card'><h2>Sử dụng</h2><div class='value' id='ssudung'>N/A</div></div>";
  html += "<div class='card'><h2>Thông báo</h2><div class='value' id='thongbao'>N/A</div></div>";
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
  html += "h1 { margin: 0; font-size: 2em; }"; // Kích thước tiêu đề
  html += "form { margin: 20px 0; }"; // Khoảng cách giữa các form
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Cập nhật phần mềm</h1>"; // Tiêu đề
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>"; // Form cho việc tải lên file
  html += "<input type='file' name='update'>"; // Input file
  html += "<input type='submit' value='Cập nhật'>"; // Nút gửi cho cập nhật
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}
