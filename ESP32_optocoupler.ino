/*
 ESP32_optocoupler.ino
 Requirements:
  1. Hardware: ESP32, tower style optocoupler with GND, 3,3 V VCC,  digital output pins, and momentary button. 
  2. libraries: ESPTime, SPI
 
 At startup ESP32 is waiting for interrupt on GPIO15 from button to start the interrupt to trigger data reading on the 
 sensor data pin on GPIO33 with internal pullup resistor. 
  Keep in mind: An interrupt should have no time reading functions, e.g., millis() or micros(). Initially, did this and 
  time data was often corrupted.
  Data dumping to micro sdcard is here, but all commented out. 
 */
 
 #include <WiFi.h>
 #include "time.h"
 #include "sys/time.h"
 
 #define optocouplerPIN 33
 #define btnPIN 15

/*
#include<SPI.h>
#include<sd.h>
#include <FS.h>
#define SD_CS_PIN  6
//In case you want to change the default SPI pins (e.g. for ESP32), uncomment and adjust: 
#define MOSI_PIN 13 //22
#define MISO_PIN 12 //17
#define SCK_PIN 14 //16
//If you use an ESP8266, the standard CS Pin (e.g. D8/GPIO15 on a WEMOS D1 mini or NodeMCU) might not work since the CS pin of 
//  the ADXL345 has a pull-up resistor on most modules. If D8 is high at reset, the ESP8266 will not boot. In that case choose a 
//  different pin as CS! 

bool spi = true;    // flag indicating that SPI shall be used
char filename(16);
bool readstart;
char outbuffer = 512; //stand sd buffer size
*/
volatile boolean triggered;
int t_start;
int t_old;
int t_jump;
byte attempts;
char timestr[32];
 boolean in_cycle;
 boolean readState; //set to one to start reading and 0 to dump data. Can toggle as needed.
unsigned int lastButtonTime = 0;
int debounceTime = 300;
char buf[40];
char timebuf[8];
volatile boolean between_cycles;
volatile int t_final;
int tdiff;

//int bedTimes[400][3]; // Array to hold data

// ******************INTERNET CONNECTION STUFF*****************************************
const char* ssid     = "NETGEAR99";      //"<REPLASE_YOUR_WIFI_SSID>"
const char* password = "cloudycream347"; //<REPLASE_YOUR_WIFI_PASSWORD>"
//*******************END INTERNET CONNECTION STUFF*************************************

//********************TIME RELATED VARIABLES*****************************************
const char* ntpServer = "pool.ntp.org"; 
const long  gmtOffset_sec = -6*3600; //CST time offset
const int   daylightOffset_sec = 3600;
struct tm timeinfo; //output from time.h
struct timeval tv_now;
//******************END TIME RELATED VARIABLES*******************************************

void btn_state(){ 
  if (readState == false) {
    Serial.println("start reading data");
    readState = true;
  }
  else if (readState == true) {
  Serial.println("stop reading data");
  readState = false;
  }
}

void IRAM_ATTR btnInterrupt() {
 //btn was pressed
  unsigned long currentTime = millis();
  if (currentTime - lastButtonTime >= debounceTime) {
    lastButtonTime = currentTime;
    btn_state();
    }
}

void dump_data() {
  if (readState) {
    // optocoupler obstacle in puath = lOW, start reading. No obstacle output HIGH; stop reading
    if(digitalRead(optocouplerPIN) == LOW) {
      t_start = esp_timer_get_time();
      in_cycle = true;
      between_cycles = false;
    }
     // light not blocked so pin HIGH; data read finished.
    else if (digitalRead(optocouplerPIN) == HIGH) { 
      t_old = t_final;
      t_final = esp_timer_get_time();
      in_cycle = false;
      if ((!in_cycle) && (!between_cycles)) {
        //Prevent multiple entries.
        between_cycles = true;
        triggered = false; //reset the sensor trigger
        //bypassed to improve timing
        // gettimeofday(&tv_now, NULL);
        //int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
        //getLocalTime(&timeinfo); // Must do to get current time
        //strftime(timebuf, sizeof(timebuf), "%M:%S", &timeinfo);
        t_jump = t_start - t_old;  
        tdiff = t_final-t_start; 
        print_data();       
      }
    }
  }
}  

void print_data() { 
  //snprint would work, but takes more time to process; Is safer in that no buffer overruns, just terminates output.
  sprintf(buf, "%ld,%ld,%ld",t_start,t_jump,tdiff);
  Serial.println(buf);
} 

void IRAM_ATTR sensorInterrupt() {
    // Execute reads if btn readState true
   triggered = true;
   dump_data();
} 
  
void setup() {
  Serial.begin(115200);
  //connect to WiFi
  Serial.println("connecting to WiFi");
 /*
  attempts = 0;
  'WiFi.mode(WIFI_STA);
  'WiFi.begin(ssid, password);
  //Connect to router/lan
  //allow up to 10 attempts to connect  @ 250ms = 5s to get connected.
  'while ((WiFi.status() != WL_CONNECTED) && (attempts <= 20)) {
  '  delay(500); //may be less, but play it safe
   ' Serial.print(".");
   ' ++attempts;
    '}
    //Connection not established in 10s, stop.  
  if (WL_CONNECTED) {
    Serial.print(" CONNECTED IN ");
  Serial.print(attempts);
  Serial.println(" attempts.");
    //init and get the time
    Serial.println("Getting time");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    strftime(timestr, sizeof timestr, "%Y.%m.%d:%H.%M.%S", &timeinfo);     
    Serial.println(timestr);
    }
   else {
      Serial.println("Failed to Connect to router; using default date");
   }
  */
   pinMode(btnPIN, INPUT_PULLUP); // assume using ESP32 PULLUP internal pin.
   //With internal pullup pin->btn->GMD button will go low on press.
   attachInterrupt(btnPIN, btnInterrupt, CHANGE); //RISING TO MINIMIZE BOUNCE ISSUES.
   
   //SET THE SENSOR WATCH
   readState = false;
   in_cycle = false;
   pinMode(optocouplerPIN, INPUT); 
  //attach coupler interrupt, look for change 
  attachInterrupt(optocouplerPIN, sensorInterrupt, CHANGE); // Attach the interrupt
  /*
  //Deal with sdcard
  if (!sd.init(){
    Serial.print("SD CARD not connected");
  } 
  ////create a sequential file name: data00x.csv; open it for continuous input.
  getFilename(); 
  Serial.print("File name created: ");
  Serial.println(filename);
  File acelFile;
  acelFile = SD.open(filename, FILE_WRITE); 
  read_start =  0;
  */
  /*works here
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(timebuf, sizeof(timebuf),"%M:%S",&timeinfo);
  Serial.println(timebuf);
  */
  Serial.println("Waiting for button press to activate sensor watch.");
}
  
 void loop() {
}

/*

void writeDataToSD(long t, float x, float y, float z) {
  // Open a file on the SD card
  //File acelFile;
  //acelFile = SD.open("accel_data.txt", FILE_WRITE);

  // If the file opened successfully, write the data
  if (acelFile) {
    snprintf(outbuffer, 256, "%d,%d,%d,%d", t, x, y, z,);
  acelFile.println(outbuffer); //for continuoud on one line use acelFile.print()
   } else {
    // Handle error if the file could not be opened
    Serial.println("Failed to open file");
  }
}
 
void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    write_fail = true;
    return;
  }
  //save message
  if(file.println(message)){
      Serial.println("Data appended");
  } else {
    Serial.println("Append failed");
    write_fail = true;
  }
  //close file to ensure file integrity when stopping.
  file.close();
}

void getFilename(){
  int n = 0;
  char slsh[2] = "/";
  char fname[16];
  snprintf(fname, sizeof(fname), "/data%03d.csv", n); // includes a three-digit sequence number in the file name
  Serial.print("in getFilename; start name:");
  Serial.println(fname);
  while(SD.exists(fname)) {
    n++;
    snprintf(fname, sizeof(fname), "/data%03d.csv", n);
  }
  strcpy(filename,slsh);
  strcat(filename,fname);
  Serial.print("filename is now:");
  Serial.println(filename);
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
     file = root.openNextFile();
  }
)
*/
  