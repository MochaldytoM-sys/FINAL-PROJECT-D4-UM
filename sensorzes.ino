#include <SPI.h>
  #include <MFRC522.h>
  #include <SD.h>
  #include <Wire.h>
  #include <RTClib.h>
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESP32Servo.h>
  // =====================================================
  // RFID PIN
  // =====================================================

  #define RFID_SS    4
  #define RFID_RST   5

  // =====================================================
  // SPI PIN
  // =====================================================

  #define SPI_SCK    12
  #define SPI_MISO   13
  #define SPI_MOSI   11

  // =====================================================
  // SD CARD PIN
  // =====================================================

  #define SD_CS      10

  // =====================================================
  // RTC PIN
  // =====================================================

  #define RTC_SDA    21
  #define RTC_SCL    20

// =====================================================
// SENSOR PIN
// =====================================================

// mpx5010
#define MPX5010_PIN   1

// RO2
#define LP_RO2_PIN    40

// RO 1/4
#define LP_RO_PIN      39

// QPM11
#define QPM11_PIN      38

#define ZMPT_PIN   7
#define ZMPT2_PIN  6

#define ZMPT_THRESHOLD_PP   200
#define ZMPT2_THRESHOLD_PP  200

/// =====================================================
// MECHANICAL SENSOR
// =====================================================

// LIMIT SWITCH
#define LIMIT_SWITCH_PIN   42

// KY-040
#define KY_CLK_PIN         15
#define KY_DT_PIN          16
#define KY_SW_PIN          17
//servo
// SERVO
#define SERVO_PIN  18

// =====================================================
// UART HUB75
// =====================================================

#define RXD2 44

#define TXD2 2

  // =====================================================
  // WIFI   
  // =====================================================

  const char* ssid = "ESP32_SD";
  const char* password = "12345678";

  // =====================================================
  // OBJECT
  // =====================================================

  MFRC522 rfid(RFID_SS, RFID_RST);

  RTC_DS3231 rtc;

  WebServer server(80);
Servo zesServo;
  // =====================================================
  // GLOBAL
  // =====================================================

  int nomor = 1;

  String lastUID = "";
  String statusRO     = "SAFE";
String statusMPX = "MPX_SAFE";
  String statusRO2   = "SAFE";

  String statusQPM11  = "SAFE";
  String statusZMPT = "ZMPT_SAFE";
  String statusZMPT2 = "ZMPT2_SAFE";
  String systemStatus = "STANDBY";
  bool aksesGranted = false;
  bool maintenanceMode = false;
  bool adaEnergiSnapshot = false;
  bool standbyMode = true;
  bool selfDiagnosisMode = false;
  bool dangerMode = false;

  unsigned long selfDiagnosisStart = 0;
  String statusLimitSwitch = "SAFE";

String statusEncoder = "SAFE";

int encoderValue = 0;

int adcMPX = 0;
int adcZMPT = 0;
int adcZMPT2 = 0;
int zmpt2PP = 0;

bool sensorError = false;
int lastCLK = HIGH;
unsigned long lastEncoderRead = 0;
unsigned long maintenanceStart = 0;

bool checkingZES = false;
int lastServoAngle = -1;
String maintenanceUID = "";
String lastUARTStatus = "";
String lastUARTData   = "";

unsigned long lastUARTSend = 0;
unsigned long safeDisplayStart = 0;
bool          showingSafe       = false;
// =====================================================
// DAFTAR SENSOR / SUMBER ENERGI YANG AKTIF
// =====================================================

String getDangerSources() {

  String sumber = "";

  if (statusMPX == "MPX_PRESSURE") {
    sumber += "MPX5010";
  }

  if (statusZMPT == "ZMPT_VOLTAGE") {
    if (sumber.length() > 0) sumber += ", ";
    sumber += "ZMPT101B";
  }

 if (statusZMPT2 == "ZMPT2_VOLTAGE") {
  if (sumber.length() > 0) sumber += ", ";
  sumber += "ZMPT101B 2";
}

  if (statusRO == "RO_PRESSURE") {
    if (sumber.length() > 0) sumber += ", ";
    sumber += "RO 1/4";
  }

  if (statusRO2 == "RO2_PRESSURE") {
    if (sumber.length() > 0) sumber += ", ";
    sumber += "RO2";
  }

  if (statusQPM11 == "QPM11_PRESSURE") {
    if (sumber.length() > 0) sumber += ", ";
    sumber += "QPM11";
  }

  if (statusLimitSwitch == "LIMIT_ACTIVE") {
    if (sumber.length() > 0) sumber += ", ";
    sumber += "LIMIT SWITCH";
  }

  if (statusEncoder == "ENCODER_ACTIVE") {
    if (sumber.length() > 0) sumber += ", ";
    sumber += "ENCODER";
  }

  if (sumber.length() == 0) {
    sumber = "UNKNOWN ENERGY";
  }

  return sumber;
}
  // =====================================================
  // CEK USER
  // =====================================================

  String cekUser(String uidCari) {

    File file = SD.open("/users.csv");

    if (!file) {
      return "";
    }

    while (file.available()) {

      String line = file.readStringUntil('\n');

      line.trim();

      if (line.length() == 0) continue;

      if (line.startsWith("UID")) continue;

      int koma = line.indexOf(',');

      if (koma > 0) {

        String uidFile = line.substring(0, koma);

        String namaFile = line.substring(koma + 1);

        uidFile.trim();
        namaFile.trim();

        uidFile.toUpperCase();

        if (uidFile == uidCari) {

          file.close();

          return namaFile;
        }
      }
    }

    file.close();

    return "";
  }

  // =====================================================
  // AUTO DELETE FILE > 14 HARI
  // =====================================================

  void hapusFileLama(DateTime now) {

    File root = SD.open("/");

    while (true) {

      File file = root.openNextFile();

      if (!file) break;

      String nama = file.name();

      if (
        nama.endsWith(".csv") &&
        nama != "users.csv"
      ) {

        int hari;
        int bulan;
        int tahun;

        sscanf(
          nama.c_str(),
          "/%d-%d-%d.csv",
          &hari,
          &bulan,
          &tahun
        );

        DateTime fileDate(
          tahun,
          bulan,
          hari,
          0,
          0,
          0
        );

        TimeSpan selisih =
          now - fileDate;

        if (selisih.days() > 14) {

          SD.remove(nama);
        }
      }

      file.close();
    }

    root.close();
  }

  // =====================================================
  // HOME PAGE
  // =====================================================

  void handleRoot() {

    digitalWrite(RFID_SS, HIGH);

    digitalWrite(SD_CS, LOW);

    File root = SD.open("/");

    String html = R"rawliteral(

    <html>

    <head>

    <title>ESP32 RFID SYSTEM</title>

    <style>

    body{
      font-family:Arial;
      background:#f4f4f4;
      padding:20px;
    }

    h1{
      color:#333;
    }

    table{
      border-collapse:collapse;
      width:100%;
      background:white;
    }

    th,td{
      border:1px solid #ccc;
      padding:10px;
      text-align:center;
    }

    th{
      background:#2196F3;
      color:white;
    }

    a{
      text-decoration:none;
      color:#2196F3;
      font-weight:bold;
      margin:5px;
    }

    button{
      padding:10px 20px;
      background:#2196F3;
      color:white;
      border:none;
      cursor:pointer;
    }

    </style>

    </head>

    <body>

    <h1>RFID LOG FILE</h1>

    <a href='/adduser'>
    <button>TAMBAH USER RFID</button>
    </a>

    <br><br>

    <table>

    <tr>
      <th>FILE</th>
      <th>AKSI</th>
    </tr>

    )rawliteral";

    while (true) {

      File file = root.openNextFile();

      if (!file) break;

      String nama = file.name();

      if (nama.endsWith(".csv")) {

        html += "<tr>";

        html += "<td>";
        html += nama;
        html += "</td>";

        html += "<td>";

        html += "<a href='/view?file=";
        html += nama;
        html += "'>VIEW</a>";

        html += "<a href='/download?file=";
        html += nama;
        html += "'>DOWNLOAD</a>";
        html += "<a href='/delete?file=";
        html += nama;
        html += "' onclick=\"return confirm('Hapus file ini?')\">DELETE</a>";
        html += "</td>";

        html += "</tr>";
      }

      file.close();
    }

    html += "</table>";

    html += "</body></html>";

    server.send(200, "text/html", html);

    root.close();

    digitalWrite(SD_CS, HIGH);
  }

  // =====================================================
  // VIEW FILE
  // =====================================================

  void handleView() {

    if (!server.hasArg("file")) {

      server.send(
        400,
        "text/plain",
        "FILE NOT SELECTED"
      );

      return;
    }

    String namaFile = server.arg("file");

    if (!namaFile.startsWith("/")) {

      namaFile = "/" + namaFile;
    }

    digitalWrite(RFID_SS, HIGH);

    digitalWrite(SD_CS, LOW);

    File file = SD.open(namaFile);

    if (!file) {

      digitalWrite(SD_CS, HIGH);

      server.send(
        404,
        "text/plain",
        "FILE NOT FOUND"
      );

      return;
    }

    String html = R"rawliteral(

    <html>

    <head>

    <style>

    body{
      font-family:Arial;
      background:#f4f4f4;
      padding:20px;
    }

    pre{
      background:white;
      padding:15px;
      border:1px solid #ccc;
      overflow:auto;
    }

    </style>

    </head>

    <body>

    )rawliteral";

    html += "<h2>";
    html += namaFile;
    html += "</h2>";

html += "<table border='1' cellpadding='10' cellspacing='0' style='border-collapse:collapse;width:100%;background:white;'>";

bool header = true;

while (file.available()) {

  String line = file.readStringUntil('\n');

  line.trim();

  if(line.length() == 0) continue;

  html += "<tr>";

  int start = 0;

  while(true){

    int koma = line.indexOf(',', start);

    String data;

    if(koma == -1){

      data = line.substring(start);

    }else{

      data = line.substring(start, koma);
    }

    if(header){

      html += "<th style='background:#2196F3;color:white;'>";
      html += data;
      html += "</th>";

    }else{

      // FORMAT STATUS

if(data.endsWith("_SAFE")){

  data = "SAFE";
}

if(data.endsWith("_PRESSURE")){

  data = "PRESSURE";
}

if(data.endsWith("_ACTIVE")){

  data = "ACTIVE";
}

if(data.endsWith("_VOLTAGE")){

  data = "VOLTAGE";
}

if(data == "SYSTEM_DANGER"){

  data = "DANGER";
}

if(data == "SYSTEM_SAFE"){

  data = "SAFE";
}

// TAMPILKAN DATA

html += "<td>";
html += data;
html += "</td>";
    }

    if(koma == -1) break;

    start = koma + 1;
  }

  html += "</tr>";

  header = false;
}

html += "</table>";

    html += "</body></html>";

    server.send(200, "text/html", html);

    file.close();

    digitalWrite(SD_CS, HIGH);
  }

  // =====================================================
  // DOWNLOAD FILE
  // =====================================================

  void handleDownload() {

    if (!server.hasArg("file")) {

      server.send(
        400,
        "text/plain",
        "FILE NOT SELECTED"
      );

      return;
    }

    String namaFile = server.arg("file");

    if (!namaFile.startsWith("/")) {

      namaFile = "/" + namaFile;
    }

    digitalWrite(RFID_SS, HIGH);

    digitalWrite(SD_CS, LOW);

    File file = SD.open(namaFile);

    if (!file) {

      digitalWrite(SD_CS, HIGH);

      server.send(
        404,
        "text/plain",
        "FILE NOT FOUND"
      );

      return;
    }

    server.sendHeader(
      "Content-Type",
      "text/csv"
    );

    server.sendHeader(
      "Content-Disposition",
      "attachment; filename=" + namaFile
    );

    server.streamFile(
      file,
      "text/csv"
    );

    file.close();

    digitalWrite(SD_CS, HIGH);
  }

  // =====================================================
  // ADD USER PAGE
  // =====================================================
  // =====================================================
// DELETE FILE
// =====================================================

void handleDelete() {

  if (!server.hasArg("file")) {

    server.send(
      400,
      "text/plain",
      "FILE NOT SELECTED"
    );

    return;
  }

  String namaFile = server.arg("file");

  if (!namaFile.startsWith("/")) {

    namaFile = "/" + namaFile;
  }

  digitalWrite(RFID_SS, HIGH);

  digitalWrite(SD_CS, LOW);

  if (SD.exists(namaFile)) {

    SD.remove(namaFile);

    digitalWrite(SD_CS, HIGH);

    server.send(
      200,
      "text/html",
      "<h1>FILE BERHASIL DIHAPUS</h1><a href='/'>KEMBALI</a>"
    );

  } else {

    digitalWrite(SD_CS, HIGH);

    server.send(
      404,
      "text/plain",
      "FILE NOT FOUND"
    );
  }
}
  void handleAddUserPage() {

    String html = R"rawliteral(

    <html>

    <head>

    <title>ADD USER</title>

    <style>

    body{
      font-family:Arial;
      background:#f4f4f4;
      padding:20px;
    }

    input{
      width:100%;
      padding:10px;
      margin-top:10px;
      margin-bottom:15px;
    }

    button{
      padding:10px 20px;
      background:#2196F3;
      color:white;
      border:none;
      cursor:pointer;
    }

    </style>

    </head>

    <body>

    <h1>TAMBAH USER RFID</h1>

    <form action='/saveuser' method='GET'>

    UID RFID:

    <input 
    type='text' 
    name='uid' 
    value='
    )rawliteral";

    html += lastUID;

    html += R"rawliteral(
    '
    placeholder='Tempel kartu RFID atau ketik manual'>

    NAMA USER:

    <input 
    type='text' 
    name='nama' 
    placeholder='Masukkan nama user'
    required>

    <button type='submit'>
    SIMPAN USER
    </button>

    </form>

    </body>

    </html>

    )rawliteral";

    server.send(200, "text/html", html);
  }

  // =====================================================
  // SAVE USER
  // =====================================================

  void handleSaveUser() {

    if (
      !server.hasArg("uid") ||
      !server.hasArg("nama")
    ) {

      server.send(
        400,
        "text/plain",
        "DATA TIDAK LENGKAP"
      );

      return;
    }

    String uid = server.arg("uid");

    String nama = server.arg("nama");

    uid.trim();
    nama.trim();

    uid.toUpperCase();

    digitalWrite(RFID_SS, HIGH);

    digitalWrite(SD_CS, LOW);

    // =====================================================
    // CEK UID DUPLIKAT
    // =====================================================

    File cekFile = SD.open("/users.csv");

    bool duplicate = false;

    while (cekFile.available()) {

      String line = cekFile.readStringUntil('\n');

      line.trim();

      if (line.startsWith("UID")) continue;

      int koma = line.indexOf(',');

      if (koma > 0) {

        String uidFile =
          line.substring(0, koma);

        uidFile.trim();

        uidFile.toUpperCase();

        if (uidFile == uid) {

          duplicate = true;

          break;
        }
      }
    }

    cekFile.close();

    if (duplicate) {

      digitalWrite(SD_CS, HIGH);

      server.send(
        200,
        "text/html",
        "<h1>UID SUDAH TERDAFTAR</h1><a href='/adduser'>KEMBALI</a>"
      );

      return;
    }

    // =====================================================
    // SAVE USER
    // =====================================================

    File file = SD.open(
      "/users.csv",
      FILE_APPEND
    );

    if (!file) {

      digitalWrite(SD_CS, HIGH);

      server.send(
        500,
        "text/plain",
        "GAGAL SIMPAN USER"
      );

      return;
    }

    file.print(uid);

    file.print(",");

    file.println(nama);

    file.close();

    digitalWrite(SD_CS, HIGH);

    server.send(
      200,
      "text/html",
      "<h1>USER BERHASIL DISIMPAN</h1><a href='/'>KEMBALI</a>"
    );
  }

  // =====================================================
  // SETUP
  // =====================================================

  void setup() {

   Serial.begin(115200);

Serial2.begin(
  2400,
  SERIAL_8N1,
  RXD2,
  TXD2
);

delay(1000);

    Serial.println("SYSTEM START");
    // =====================================================
  // DIGITAL INPUT
  // =====================================================
// KODE BARU
pinMode(LP_RO_PIN,  INPUT_PULLUP);
pinMode(LP_RO2_PIN, INPUT_PULLUP);
pinMode(QPM11_PIN,  INPUT_PULLUP);
  // =====================================================
// MECHANICAL SENSOR
// =====================================================

pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);

pinMode(KY_CLK_PIN, INPUT_PULLUP);

pinMode(KY_DT_PIN, INPUT_PULLUP);

pinMode(KY_SW_PIN, INPUT_PULLUP);
    // =====================================================
    // ADC
    // =====================================================

    analogReadResolution(12);

pinMode(ZMPT2_PIN, INPUT);
analogSetPinAttenuation(ZMPT2_PIN, ADC_11db);
    // RTC

    Wire.begin(RTC_SDA, RTC_SCL);

    rtc.begin();

   if(rtc.lostPower()){

  rtc.adjust(
    DateTime(
      F(__DATE__),
      F(__TIME__)
    )
  );
}

    // SPI

    SPI.begin(
      SPI_SCK,
      SPI_MISO,
      SPI_MOSI
    );

    // CS

    pinMode(RFID_SS, OUTPUT);

    pinMode(SD_CS, OUTPUT);

    digitalWrite(RFID_SS, HIGH);

    digitalWrite(SD_CS, HIGH);

    // RFID

    digitalWrite(RFID_SS, LOW);

    rfid.PCD_Init();

byte ver = rfid.PCD_ReadRegister(MFRC522::VersionReg);

Serial.print("RFID VERSION = 0x");
Serial.println(ver, HEX);

digitalWrite(RFID_SS, HIGH);

Serial.println("RFID READY");

    // SD

    digitalWrite(SD_CS, LOW);

    if (!SD.begin(SD_CS)) {

      Serial.println("SD FAILED");

      while (1);
    }

    digitalWrite(SD_CS, HIGH);

    Serial.println("SD READY");

    // USERS DATABASE

    digitalWrite(SD_CS, LOW);

    if (!SD.exists("/users.csv")) {

      File userFile = SD.open(
        "/users.csv",
        FILE_WRITE
      );

      if (userFile) {

        userFile.println("UID,NAMA");

        userFile.close();
      }
    }

    digitalWrite(SD_CS, HIGH);

    // AUTO DELETE

    digitalWrite(SD_CS, LOW);

    hapusFileLama(rtc.now());

    digitalWrite(SD_CS, HIGH);

    // WIFI

    WiFi.softAP(ssid, password);

    Serial.println();

    Serial.println("HOTSPOT AKTIF");

    Serial.print("IP : ");

    Serial.println(WiFi.softAPIP());

    // WEB ROUTE

    server.on("/", handleRoot);

    server.on("/view", handleView);

    server.on("/download", handleDownload);
    server.on("/delete", handleDelete);
    server.on("/adduser", handleAddUserPage);

    server.on("/saveuser", handleSaveUser);

    server.begin();
// =====================================================
// SERVO
// =====================================================

ESP32PWM::allocateTimer(0);

zesServo.attach(SERVO_PIN);

zesServo.write(0);
    Serial.println("WEB SERVER READY");
  }

  // =====================================================
  // LOOP
  // =====================================================
void loop() {

  server.handleClient();
  static unsigned long lastBlink = 0;

  if(millis() - lastBlink > 1000){
    lastBlink = millis();
  }

  // =====================================================
  // FLAG ERROR SEMENTARA (reset tiap loop, wajar)
  // sensorError (global) TIDAK direset di sini
  // hanya direset manual saat tap RFID
  // =====================================================

  bool sensorErrorLoop = false;

  // =====================================================
  // SENSOR 1 : MPX5010
  // =====================================================

  int totalMPX = 0;

  for(int i = 0; i < 10; i++){
    totalMPX += analogRead(MPX5010_PIN);
    delay(2);
  }

  adcMPX = totalMPX / 10;

  if(adcMPX >= 4090){
    sensorErrorLoop = true;    // ← pakai Loop, bukan global
    statusMPX = "MPX_ERROR";
  // BARU
}else if(adcMPX > 800){
    statusMPX = "MPX_PRESSURE";
  }else{
    statusMPX = "MPX_SAFE";
  }

// =====================================================
// SENSOR TEGANGAN
// =====================================================

int maxVal = 0;
int minVal = 4095;

// KODE BARU - sampling lebih cepat
for(int i=0;i<100;i++){
  int v = analogRead(ZMPT_PIN);
  if(v > maxVal) maxVal = v;
  if(v < minVal) minVal = v;
  delayMicroseconds(200);
}

int pp = maxVal - minVal;

Serial.print("ZMPT PP = ");
Serial.println(pp);

if(pp > ZMPT_THRESHOLD_PP){

  statusZMPT = "ZMPT_VOLTAGE";

}else{

  statusZMPT = "ZMPT_SAFE";
}
 // =====================================================
// SENSOR : ZMPT101B 2
// =====================================================

int zmpt2Max = 0;
int zmpt2Min = 4095;
long totalZMPT2 = 0;

for(int i = 0; i < 1000; i++){
  int v = analogRead(ZMPT2_PIN);

  if(v > zmpt2Max) zmpt2Max = v;
  if(v < zmpt2Min) zmpt2Min = v;

  totalZMPT2 += v;
  delayMicroseconds(200);
}

adcZMPT2 = totalZMPT2 / 1000;
zmpt2PP  = zmpt2Max - zmpt2Min;

Serial.print("ZMPT2 MIN = ");
Serial.print(zmpt2Min);

Serial.print(" | ZMPT2 MAX = ");
Serial.print(zmpt2Max);

Serial.print(" | ZMPT2 AVG = ");
Serial.print(adcZMPT2);

Serial.print(" | ZMPT2 PP = ");
Serial.println(zmpt2PP);

if(zmpt2PP > ZMPT2_THRESHOLD_PP){

  statusZMPT2 = "ZMPT2_VOLTAGE";

}else{

  statusZMPT2 = "ZMPT2_SAFE";
}

Serial.print("ZMPT2 STATUS NOW = ");
Serial.println(statusZMPT2);
// =====================================================
// SENSOR 1 : LOW PRESSURE RO
// =====================================================
// KODE BARU
// RO sampling 5x anti noise
int roCount = 0;
for(int i = 0; i < 5; i++){
  if(digitalRead(LP_RO_PIN) == LOW) roCount++;
  delay(2);
}
int roState = digitalRead(LP_RO_PIN);

Serial.print("RAW PIN39 = ");
Serial.print(roState);

Serial.print(" STATUS = ");

if(roState == LOW){

  statusRO = "RO_PRESSURE";

}else{

  statusRO = "RO_SAFE";
}

Serial.println(statusRO);

// RO2 sampling 5x anti noise
int RO2State = digitalRead(LP_RO2_PIN);

Serial.print("RAW PIN40 = ");
Serial.print(RO2State);

Serial.print(" STATUS = ");

if(RO2State == LOW){

  statusRO2 = "RO2_PRESSURE";

}else{

  statusRO2 = "RO2_SAFE";
}

Serial.println(statusRO2);
// =====================================================
// SENSOR 2 : LOW PRESSURE RO2
// =====================================================



if(RO2State == LOW){

  statusRO2 = "RO2_PRESSURE";

}else{

  statusRO2 = "RO2_SAFE";
}

// =====================================================
// SENSOR 3 : QPM11
// =====================================================

int qpm11State = digitalRead(QPM11_PIN);

if(qpm11State == HIGH){

  statusQPM11 = "QPM11_PRESSURE";

}else{

  statusQPM11 = "QPM11_SAFE";
}
// =====================================================
// LIMIT SWITCH
// =====================================================

int limitState = digitalRead(LIMIT_SWITCH_PIN);

if(limitState == HIGH){

  statusLimitSwitch = "LIMIT_ACTIVE";

}else{

  statusLimitSwitch = "LIMIT_SAFE";
}
// =====================================================
// KY-040 ROTARY ENCODER
// =====================================================
int currentCLK = digitalRead(KY_CLK_PIN);

if(currentCLK != lastCLK &&
   millis() - lastEncoderRead > 50  // ← debounce lebih ketat, 50ms
){
  if(digitalRead(KY_DT_PIN) != currentCLK){
    encoderValue++;
  }else{
    encoderValue--;
  }

  statusEncoder    = "ENCODER_ACTIVE";
  lastEncoderRead  = millis();

  Serial.print("ENCODER VALUE : ");
  Serial.println(encoderValue);
}

lastCLK = currentCLK;

// Reset ENCODER_ACTIVE setelah 200ms tidak ada gerakan
if(statusEncoder == "ENCODER_ACTIVE" &&
   millis() - lastEncoderRead > 4000){
  statusEncoder = "ENCODER_SAFE";
}

// BUTTON ENCODER

if(digitalRead(KY_SW_PIN) == LOW){

  Serial.println("ENCODER BUTTON PRESSED");
}
Serial.println();
Serial.println("===== STATUS SENSOR =====");
// GANTI DENGAN INI
bool adaEnergi =
(
  statusMPX == "MPX_PRESSURE" ||
  statusZMPT == "ZMPT_VOLTAGE" ||
  statusZMPT2 == "ZMPT2_VOLTAGE" ||
  statusRO == "RO_PRESSURE" ||
  statusRO2 == "RO2_PRESSURE" ||
  statusQPM11 == "QPM11_PRESSURE" ||
  statusLimitSwitch == "LIMIT_ACTIVE" ||
  statusEncoder == "ENCODER_ACTIVE"
);
Serial.println();
Serial.println("===== STATUS SENSOR =====");

Serial.print("RO      = ");
Serial.println(statusRO);

Serial.print("RO2     = ");
Serial.println(statusRO2);

Serial.print("QPM11   = ");
Serial.println(statusQPM11);

Serial.print("LIMIT   = ");
Serial.println(statusLimitSwitch);

Serial.print("ENCODER = ");
Serial.println(statusEncoder);

Serial.print("ENERGI  = ");
Serial.println(adaEnergi);

Serial.println("=========================");
// TAMBAH INI - simpan snapshot selama selfDiagnosis
if(selfDiagnosisMode && adaEnergi){
  adaEnergiSnapshot = true;
}
Serial.println("=========================");
static unsigned long lastDebug = 0;

if(millis() - lastDebug > 1000){

  lastDebug = millis();

  Serial.println();
  Serial.println("===== CEK ENERGI =====");

  Serial.print("MPX   : ");
  Serial.println(statusMPX);

  Serial.print("ZMPT  : ");
  Serial.println(statusZMPT);

  Serial.print("ZMPT2 : ");
  Serial.println(statusZMPT2);

  Serial.print("RO    : ");
  Serial.println(statusRO);

  Serial.print("RO2   : ");
  Serial.println(statusRO2);

  Serial.print("QPM11 : ");
  Serial.println(statusQPM11);

  Serial.print("LIMIT : ");
  Serial.println(statusLimitSwitch);

  Serial.print("ENC   : ");
  Serial.println(statusEncoder);

  Serial.print("adaEnergi = ");
  Serial.println(adaEnergi);

  Serial.println("======================");
}

Serial.println("======================");
// =====================================================
// DIGITAL SELF DIAGNOSIS
// =====================================================

if(checkingZES){

  if(millis() - maintenanceStart < 10000){

if(
  roState == LOW ||
  RO2State == LOW ||
  qpm11State == HIGH ||
  statusMPX == "MPX_PRESSURE" ||
  statusZMPT == "ZMPT_VOLTAGE" ||
  statusZMPT2 == "ZMPT2_VOLTAGE"
){

  systemStatus     = "DANGER";
  dangerMode       = true;
  maintenanceMode  = false;
  checkingZES      = false;
  standbyMode      = false;
  showingSafe      = false;

  zesServo.write(0);
  lastServoAngle = 0;

  Serial.println();
  Serial.println("ENERGY STILL DETECTED");
  Serial.println("DIGITAL SELF DIAGNOSIS ERROR");
  Serial.println("MASUK DANGER MODE DARI MAINTENANCE");
}

  }else{

    checkingZES = false;

    Serial.println();

    Serial.println(
    "ZES DIGITAL CHECK COMPLETE");
  }
}
// =====================================================
// SYSTEM STATUS
// =====================================================

if(standbyMode){
  systemStatus = "STANDBY";
  if(!showingSafe && lastServoAngle != 0){
    zesServo.write(0);
    lastServoAngle = 0;
  }
}
else if(selfDiagnosisMode){
  systemStatus = "SELFDIAG";
  if(lastServoAngle != 0){
    zesServo.write(0);
    lastServoAngle = 0;
  }
 if(millis() - selfDiagnosisStart > 3000){
    selfDiagnosisMode = false;
    Serial.println("SELF DIAGNOSIS COMPLETE");

    // simpan sensorErrorLoop ke global
    if(sensorErrorLoop){
      sensorError = true;
    }

    if(sensorError){
      // SENSOR RUSAK → ERROR MODE
      systemStatus = "ERROR";
      standbyMode  = false;
      Serial.println("SENSOR ERROR - TAP RFID TO RESET");
    }
   else if(adaEnergi){
   Serial.println("===== PENYEBAB DANGER =====");
  Serial.println(statusMPX);
  Serial.println(statusZMPT);
  Serial.println(statusZMPT2);
  Serial.println(statusRO);
  Serial.println(statusRO2);
  Serial.println(statusQPM11);
  Serial.println(statusLimitSwitch);
  Serial.println("===========================");
  systemStatus  = "DANGER";
  dangerMode    = true;
  standbyMode   = false;  // ← tambah ini
  Serial.println("ENERGY DETECTED - DANGER MODE");
}
    else{
      // ENERGI KOSONG → SAFE → BUKA SERVO → LANGSUNG MAINTENANCE
      selfDiagnosisMode = false; 
      systemStatus      = "SAFE";
      showingSafe       = true;
      safeDisplayStart  = millis();
      zesServo.write(90);
      lastServoAngle    = 90;
      Serial.println("ZERO ENERGY SAFE - SERVO BUKA");
    }
  }
}
else if(dangerMode){
  systemStatus = "DANGER";
  if(lastServoAngle != 0){
    zesServo.write(0);
    lastServoAngle = 0;
  }
}
else if(maintenanceMode){
  systemStatus = "MAINTENANCE";
  if(lastServoAngle != 90){
    zesServo.write(90);
    lastServoAngle = 90;
  }
}
else{
  if(!dangerMode && !maintenanceMode && !showingSafe){
    systemStatus = "STANDBY";
    standbyMode  = true;
  }
}
// =====================================================
// TRANSISI SAFE → MAINTENANCE setelah 8 detik
// =====================================================
if(showingSafe){
  if(millis() - safeDisplayStart >= 8000){
    showingSafe      = false;
    maintenanceMode  = true;
    maintenanceStart = millis();
    checkingZES      = true;
    Serial.println("MASUK MAINTENANCE MODE");
  }
}
// =====================================================
// SEND STATUS TO HUB75
// =====================================================

String uartStatus = "";

if(systemStatus == "STANDBY"){

  uartStatus = "STANDBY";

}
else if(systemStatus == "SELFDIAG"){

  uartStatus = "SELFDIAG";

}
else if(systemStatus == "ERROR"){

  uartStatus = "ERROR";

}
else if(systemStatus == "DANGER"){

  uartStatus = "DANGER";

}
else if(systemStatus == "MAINTENANCE"){

  uartStatus = "MAINTENANCE";

}
else if(systemStatus == "SAFE"){

  uartStatus = "SAFE";

}
else{

  uartStatus = "SAFE";
}

// =====================================================
// KIRIM STATUS + DETAIL SENSOR + JAM KE PANEL HUB75
//
// FORMAT:
// STATUS|DETAIL SENSOR|JAM
//
// CONTOH:
// DANGER|LIMIT SWITCH, RO 1/4|14:20:10
// SAFE||14:20:15
// =====================================================

DateTime nowUART = rtc.now();

char jamUART[9];

sprintf(
  jamUART,
  "%02d:%02d:%02d",
  nowUART.hour(),
  nowUART.minute(),
  nowUART.second()
);

String detailUART = "";

if (uartStatus == "DANGER") {
  detailUART = getDangerSources();
}

String uartData =
  uartStatus +
  "|" +
  detailUART +
  "|" +
  String(jamUART);

// Kirim jika isi berubah atau setiap satu detik
if (
  uartData != lastUARTData ||
  millis() - lastUARTSend > 1000
) {

  Serial2.println(uartData);

  Serial.print("UART SEND : ");
  Serial.println(uartData);

  lastUARTData   = uartData;
  lastUARTStatus = uartStatus;
  lastUARTSend   = millis();
}

// =====================================================
// SERIAL MONITOR
// =====================================================
  if(maintenanceMode){
Serial.println();

Serial.println("============================");

Serial.print("RO SENSOR     : ");
Serial.println(statusRO);

Serial.print("RO2 SENSOR   : ");
Serial.println(statusRO2);

Serial.print("QPM11 SENSOR  : ");
Serial.println(statusQPM11);
Serial.print("LIMIT SWITCH  : ");
Serial.println(statusLimitSwitch);

Serial.print("ENCODER VALUE : ");
Serial.println(encoderValue);

Serial.print("ENCODER STATUS: ");
Serial.println(statusEncoder);
Serial.println();

Serial.print("SYSTEM STATUS : ");
Serial.println(systemStatus);

// =====================================================
// SENSOR 7
// =====================================================

/*
int sensor7 = analogRead(SENSOR_7_PIN);

if(sensor7 > threshold){

}
*/

// =====================================================
// SENSOR 8
// =====================================================

/*
int sensor8 = analogRead(SENSOR_8_PIN);

if(sensor8 > threshold){

}
*/

// =====================================================
// SERIAL SENSOR
// =====================================================

Serial.println();

Serial.println("========================");

Serial.println();

Serial.print("ZMPT ADC : ");
Serial.println(adcZMPT);

Serial.print("ZMPT2 ADC : ");
Serial.println(adcZMPT2);
Serial.print("ZMPT2 PP : ");
Serial.println(zmpt2PP);

Serial.println();

Serial.print("ZMPT STATUS : ");
Serial.println(statusZMPT);

Serial.print("MPX5010 ADC : ");
Serial.println(adcMPX);

Serial.println();

Serial.print("ZMPT2 STATUS : ");
Serial.println(statusZMPT2);

Serial.print("MPX STATUS : ");
Serial.println(statusMPX);
}


// BARU AKTIFKAN SPI RFID
digitalWrite(RFID_SS, LOW);

SPISettings(
  500000,
  MSBFIRST,
  SPI_MODE0
);



// =====================================================
// WAIT RFID
// =====================================================


delay(5);

bool adaKartu = rfid.PICC_IsNewCardPresent();


if (!adaKartu) {

  SPI.endTransaction();

  delay(50);

  

  return;  


}else{

  Serial.println("KARTU TERDETEKSI");

  if (!rfid.PICC_ReadCardSerial()) {

    SPI.endTransaction();

    return;
  }
// =====================================================
// UID
// =====================================================

String uid = "";

for (byte i = 0; i < rfid.uid.size; i++) {

  if (rfid.uid.uidByte[i] < 0x10) {

    uid += "0";
  }

  uid += String(
    rfid.uid.uidByte[i],
    HEX
  );
}

uid.toUpperCase();

lastUID = uid;

    // RTC

    DateTime now = rtc.now();

    char waktu[15];

    sprintf(
      waktu,
      "%02d:%02d:%02d",
      now.hour(),
      now.minute(),
      now.second()
    );

    // USER CHECK

    digitalWrite(RFID_SS, HIGH);

    digitalWrite(SD_CS, LOW);

    String nama = cekUser(uid);
    // FILE NAME

char namaFile[30];

    String statusAkses;
if (nama != "") {

  statusAkses = "GRANTED";

  aksesGranted = true;
// =====================================================
// BACA ULANG SENSOR SAAT TAP
// =====================================================

// MPX5010
int totalMPX2 = 0;
for(int i = 0; i < 10; i++){
  totalMPX2 += analogRead(MPX5010_PIN);
  delay(2);
}
adcMPX = totalMPX2 / 10;
if(adcMPX >= 4090)      statusMPX = "MPX_ERROR";
// BARU
else if(adcMPX > 800)   statusMPX = "MPX_PRESSURE";
else                    statusMPX = "MPX_SAFE";

// ZMPT101B
int maxV2 = 0, minV2 = 4095;
for(int i = 0; i < 100; i++){
  int v = analogRead(ZMPT_PIN);
  if(v > maxV2) maxV2 = v;
  if(v < minV2) minV2 = v;
  delayMicroseconds(200);
}statusZMPT = (maxV2 - minV2) > ZMPT_THRESHOLD_PP ? "ZMPT_VOLTAGE" : "ZMPT_SAFE";

// ZMPT101B 2
int zmpt2Max2 = 0;
int zmpt2Min2 = 4095;
long totalZMPT2_2 = 0;

for(int i = 0; i < 1000; i++){
  int v = analogRead(ZMPT2_PIN);

  if(v > zmpt2Max2) zmpt2Max2 = v;
  if(v < zmpt2Min2) zmpt2Min2 = v;

  totalZMPT2_2 += v;
  delayMicroseconds(200);
}

adcZMPT2 = totalZMPT2_2 / 1000;
zmpt2PP  = zmpt2Max2 - zmpt2Min2;

if(zmpt2PP > ZMPT2_THRESHOLD_PP){
  statusZMPT2 = "ZMPT2_VOLTAGE";
}else{
  statusZMPT2 = "ZMPT2_SAFE";
}

// RO
statusRO = (digitalRead(LP_RO_PIN) == LOW) ? "RO_PRESSURE" : "RO_SAFE";

// RO2
statusRO2 = (digitalRead(LP_RO2_PIN) == LOW) ? "RO2_PRESSURE" : "RO2_SAFE";

// QPM11
statusQPM11 = (digitalRead(QPM11_PIN) == HIGH) ? "QPM11_PRESSURE" : "QPM11_SAFE";

// LIMIT SWITCH
statusLimitSwitch = (digitalRead(LIMIT_SWITCH_PIN) == HIGH) ? "LIMIT_ACTIVE" : "LIMIT_SAFE";

// =====================================================
// FILE NAME
  // =====================================================
  // TOGGLE MAINTENANCE MODE
  // =====================================================
// =====================================================
// STANDBY → mulai Self Diagnosis
// =====================================================
// GANTI DENGAN INI
if(standbyMode){
  standbyMode        = false;
  selfDiagnosisMode  = true;
  selfDiagnosisStart = millis();
  maintenanceUID     = uid;
  adaEnergiSnapshot  = false;  // reset dulu
  Serial.println("SELF DIAGNOSIS START");
}
// =====================================================
// DANGER → tap untuk reset ke STANDBY
// =====================================================
else if(dangerMode){
  dangerMode     = false;
  standbyMode    = true;
  systemStatus   = "STANDBY";
  zesServo.write(0);
  lastServoAngle = 0;
  Serial.println("DANGER RESET - KEMBALI STANDBY");
}

// =====================================================
// ERROR → tap untuk reset ke STANDBY
// =====================================================
else if(systemStatus == "ERROR"){
  standbyMode  = true;
  sensorError  = false;
  systemStatus = "STANDBY";
  zesServo.write(0);
  lastServoAngle = 0;
  Serial.println("ERROR RESET - KEMBALI KE STANDBY");
}

// =====================================================
// MAINTENANCE → tap selesai → kembali STANDBY
// =====================================================
else if(maintenanceMode && uid == maintenanceUID){
  maintenanceMode = false;
  standbyMode     = true;
  checkingZES     = false;
  maintenanceUID  = "";
  zesServo.write(0);
  lastServoAngle  = 0;
  Serial.println("MAINTENANCE SELESAI - KEMBALI STANDBY");
}
} else {


  nama = "UNKNOWN";

  statusAkses = "DENIED";

  aksesGranted = false;
}
    // SERIAL

    Serial.println();

    Serial.println("===================");

    Serial.print("UID : ");
    Serial.println(uid);

    Serial.print("NAMA : ");
    Serial.println(nama);

    Serial.print("STATUS : ");
    Serial.println(statusAkses);

    // FILE NAME

    

    sprintf(
      namaFile,
      "/%02d-%02d-%04d.csv",
      now.day(),
      now.month(),
      now.year()
    );

    // HEADER FILE

    if (!SD.exists(namaFile)) {

      File headerFile = SD.open(
        namaFile,
        FILE_WRITE
      );

      if (headerFile) {

headerFile.println(
"NO,WAKTU,UID,NAMA,AKSES,MPX,ZMPT,ZMPT2,RO,RO2,QPM11,LIMIT,ENCODER,SYSTEM,MAINTENANCE"
);

        headerFile.close();
      }
    }

    // SAVE LOG

    File logFile = SD.open(
      namaFile,
      FILE_APPEND
    );

    if (logFile) {

      logFile.print(nomor);

      logFile.print(",");

      logFile.print(waktu);

      logFile.print(",");

      logFile.print(uid);

      logFile.print(",");

      logFile.print(nama);

      logFile.print(",");

      logFile.print(statusAkses);

logFile.print(",");
logFile.print(statusMPX);

logFile.print(",");

logFile.print(statusZMPT);
logFile.print(",");

logFile.print(statusZMPT2);
logFile.print(",");
logFile.print(statusRO);

logFile.print(",");

logFile.print(statusRO2);

logFile.print(",");

logFile.print(statusQPM11);

logFile.print(",");

logFile.print(statusLimitSwitch);

logFile.print(",");

logFile.print(statusEncoder);

logFile.print(",");

logFile.print(systemStatus);
logFile.print(",");

if(maintenanceMode){

  logFile.println("MAINTENANCE_ON");

}else{

  logFile.println("MAINTENANCE_OFF");
}
      logFile.close();

      Serial.println("LOG SAVED");

      nomor++;
    }

    digitalWrite(SD_CS, HIGH);

    rfid.PICC_HaltA();

    rfid.PCD_StopCrypto1();
    SPI.endTransaction();
    delay(1000);
}}