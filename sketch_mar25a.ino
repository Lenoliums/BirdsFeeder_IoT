#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           
#include "soc/rtc_cntl_reg.h" 
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <StringArray.h>
#include <SPIFFS.h>
#include <FS.h>
#include <stdio.h>
#include "main.h"

// Replace with your network credentials
const char* ssid = "****";
const char* password = "******";

const char echoPin_food = 2;
const char trigPin_food = 14;

const char echoPin_bird = 15;
const char trigPin_bird = 13;

const float sizeBox=10;
const float minDistToBird=10;
bool isBirdFeeding=false;
int birdCounter=0;

AsyncWebServer server(80);
boolean takeNewPhoto = false;
#define FILE_PHOTO "/photo.jpg"
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

void setup() {
    pinMode(trigPin_food, OUTPUT); 
    pinMode(echoPin_food, INPUT); 
    pinMode(trigPin_bird, OUTPUT); 
    pinMode(echoPin_bird, INPUT); 
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
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
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  // Local bird counter saved
  File birdsFile = SPIFFS.open("/birds.txt", "r", true);
  char birdsFileContent[42];
  while (birdsFile.available()) {
    sprintf(birdsFileContent + strlen(birdsFileContent), "%c", birdsFile.read());
  }
 
  birdsFile.close();
  birdCounter = atoi(birdsFileContent);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    capturePhotoSaveSpiffs();
    request->send_P(200, "text/plain", "Taking Photo");
  });
  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });
  server.on("/food-level", HTTP_GET, [](AsyncWebServerRequest * request) {
    char str[42];
    sprintf(str, "%d", (int)checkFood());
    request->send(200, "text/plain", str);
  });
  server.on("/bird-check", HTTP_GET, [](AsyncWebServerRequest * request) {
    char str[42];
    sprintf(str, "%d", (int)getBirdCounter());
    request->send(200, "text/plain", str);
  });
  // Start server
  server.begin();
}
void loop() {
  checkBird();
  delay(1);
}

float checkRang(const char trigPin, const char echoPin){
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2); 
    digitalWrite(trigPin, HIGH); 
    delayMicroseconds(10); 
    digitalWrite(trigPin, LOW); 
    float dist = pulseIn(echoPin, HIGH)/58;
    return dist;
  }

  float checkFood() {
    float dist=checkRang(trigPin_food,echoPin_food);
    if(dist>=sizeBox+2){
      return 0;
    }
    if(dist<=2){
      return 100;
    }
    return 100*(sizeBox-dist +2)/sizeBox;
  }
  int getBirdCounter(){return birdCounter;}
  
  void checkBird() {
    float dist = checkRang(trigPin_bird, echoPin_bird);
    Serial.println(isBirdFeeding);
    Serial.println(dist);
    if(dist<minDistToBird && !isBirdFeeding){
      isBirdFeeding=1;
      capturePhotoSaveSpiffs();
      birdCounter+=1;
      File birdsFile = SPIFFS.open("/birds.txt", "w");
      char birdCounterStr[42];
      sprintf(birdCounterStr, "%d", birdCounter);
      birdsFile.print(birdCounterStr);
      birdsFile.close();
    }
    if(dist>minDistToBird && isBirdFeeding){
      isBirdFeeding=!isBirdFeeding;
    }
  }

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}
// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL;
  bool ok = 0;
  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}