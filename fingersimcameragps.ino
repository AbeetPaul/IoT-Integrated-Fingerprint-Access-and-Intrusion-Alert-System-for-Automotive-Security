#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>

// ----------- Camera Configuration -----------
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ----------- WiFi Credentials -----------
const char* ssid = "Airtel_9419448805";
const char* password = "air78777";

// ----------- Web Server -----------
WebServer server(80);

// ----------- Fingerprint Sensor Setup -----------
#define FP_RX 2
#define FP_TX 13
HardwareSerial fpSerial(1); // UART1
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fpSerial);

// ----------- SIM800L GSM Setup -----------
#define SIM800_TX 14
#define SIM800_RX 15
HardwareSerial sim800(2); // UART2
String phoneNumber = "+917889952380";
String alertMessage = "ALERT: Unauthorized fingerprint detected!";

// ----------- GPS Setup (NEO-6M) -----------
#define GPS_RX 12   // ESP32 RX (connect to GPS TX)
#define GPS_TX -1   // Not used
HardwareSerial gpsSerial(0); // Use Serial0 for GPS (RX only)
TinyGPSPlus gps;

// Function prototypes
void configInitCamera();
void startCameraServer();
void handleRoot();
void handleStream();
void sendAlertSMS();
uint8_t checkFingerprint();

void setup() {
  Serial.begin(115200);

  // Initialize camera
  configInitCamera();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("Stream Link: http://");
  Serial.println(WiFi.localIP());

  // Start web server
  startCameraServer();
  Serial.println("HTTP server started");

  // Initialize fingerprint sensor
  fpSerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor OK");
  } else {
    Serial.println("Fingerprint sensor not found!");
    while (1);
  }

  // Initialize SIM800L
  sim800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);
  delay(3000);
  sim800.println("AT");
  delay(1000);
  Serial.println("SIM800L initialized");

  // Initialize GPS
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS initialized");
}

void configInitCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    return;
  }
}

void startCameraServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();
}

void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html",
    "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>body{margin:0;overflow:hidden}img{display:block;width:100%}</style>"
    "</head>"
    "<body><img src=\"/stream\"></body>"
    "</html>");
}

void handleStream() {
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();
  
  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }
    
    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(fb->len));
    client.println();
    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    
    if (!client.connected()) break;
    delay(1);
  }
}

void sendAlertSMS() {
  // Capture photo (optional, not sent via SMS)
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    Serial.println("Intruder photo captured");
    esp_camera_fb_return(fb);
  }

  // Get GPS data
  String gpsData = "";
  if (gps.location.isValid()) {
    gpsData = "\nLocation: http://maps.google.com/?q=" + 
              String(gps.location.lat(), 6) + "," + 
              String(gps.location.lng(), 6);
  } else {
    gpsData = "\nLocation: Unavailable";
  }

  // Send SMS
  sim800.println("AT+CMGF=1");
  delay(500);
  sim800.print("AT+CMGS=\"");
  sim800.print(phoneNumber);
  sim800.println("\"");
  delay(500);
  sim800.print(alertMessage + gpsData);
  delay(500);
  sim800.write(26); // Ctrl+Z to send
  Serial.println("Alert SMS sent!");
}

uint8_t checkFingerprint() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return 255;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return 255;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    return finger.fingerID;
  } else if (p == FINGERPRINT_NOTFOUND) {
    return 0;
  }
  return 255;
}

void loop() {
  server.handleClient();

  // Read GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  uint8_t id = checkFingerprint();
  if (id == 0) { // Unauthorized fingerprint
    Serial.println("Unauthorized fingerprint detected!");
    sendAlertSMS();
    delay(5000); // Prevent multiple alerts
  } 
  else if (id != 255) { // Authorized fingerprint
    Serial.print("Authorized fingerprint ID: ");
    Serial.println(id);
  }
  delay(1000);
}
