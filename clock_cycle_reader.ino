#define USE_OTA
#define USE_TELNET
//#define USE_BLE
#define USE_WIFI

#include <Arduino.h>
#ifdef USE_WIFI
#include <WiFi.h>
#endif
#include <EEPROM.h>
#include <time.h>

#ifdef USE_OTA
#include <ota.h>
#endif

#ifdef USE_TELNET
#include <telnet.h>
#endif

#ifdef USE_BLE
#include <esp32_bluetooth.h>
#endif

int debug = 0,tdebug = 0,cnt_time = 0,new_cnt = 0,avg_cnt = 0;
float ncnt_result = 0;
unsigned char rxdata = 0,telnetrx = 0;
/*Put your SSID & Pass =word*/
char ssid[32] = "starwars24";  // Enter SSID here
char password[32] = "0609606096";  //Enter Password here
const char* ntpServer = "pool.ntp.org";

struct timespec end_time;
struct timespec start_time;

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 29);

// wl
IPAddress gate_IP(192, 168, 1, 249);
IPAddress dns_IP(192, 168, 1, 249);
IPAddress sub_IP(255, 255, 255, 0);
// WebServer display_server(8820);
//maine
// IPAddress gate_IP(192, 168, 1, 1);
// IPAddress dns_IP(192, 168, 1, 1);
// IPAddress sub_IP(255, 255, 255, 0);

#define CLOCK_SWINGS 60
int sample[100];
unsigned long startTime;
int cycle = 0;
int swings = 300;
int debounce = 25;
int connected = 0;
int read_cycle = 0;
int led = 8;
int off_avg = 0;

int irsensor = 4;

void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);   // change state of the LED by setting the pin to the LOW voltage level

  if(connect_to_wifi() == false){
#ifdef USE_OTA
    otasetup();
#endif
    connected = 1;
#ifdef USE_TELNET
    setupTelnet();
#endif
  }
  EEPROM.begin(4096);

#ifdef USE_BLE
  blsetup();
#endif
//  blsetup();

  //set the resolution to 12 bits (0-4095)
  analogReadResolution(12);
  cycle = get_last_cycle();
  swings = get_swings_level();
  debounce = get_debounce_level();
  Serial.printf("last cycle = %d\r\n",cycle);
  Serial.printf("last swings = %d\r\n",swings);
  read_cycle = cycle;
  //asetup();
    pinMode(irsensor,INPUT_PULLUP);

}
 
int debounce_signal(int rawSignal, int requiredStableCount)
{
    static int stableState = 0;      // debounced output
    static int lastRawState = 0;     // previous raw sample
    static int counter = 0;               // counts consecutive stable samples

    // If raw signal changed, reset counter
    if (rawSignal != lastRawState)
    {
        lastRawState = rawSignal;
        counter = 0;
    }
    else
    {
        // Raw signal is same as last sample → increment stability counter
        if (counter < requiredStableCount)
            counter++;
    }

    // If stable long enough, accept new state
    if (counter >= requiredStableCount)
        stableState = rawSignal;

    return stableState;
}

void loop() {
  unsigned long nowTime = micros();
  static bool first_loop = true;
  static int ir_sensor_state = 0,last_ir_sensor_state = 0,ncnt = 0,loops = 0;
  static float ncnt_sum = 0;
  if(first_loop){
    first_loop = false;
    startTime = micros();
  }
  if(nowTime > startTime + 100 ){
    startTime = micros();
    ir_sensor_state = digitalRead(irsensor);
    digitalWrite(led, ir_sensor_state);   // change state of the LED by setting the pin to the LOW voltage level
    if(ncnt == 0)clock_gettime(CLOCK_REALTIME, &start_time) ;//gettimeofday(&start_time, NULL);
    ncnt++;
    ir_sensor_state = debounce_signal(ir_sensor_state,debounce);
    if(last_ir_sensor_state == 0 && ir_sensor_state == 1){
      if(debug > 1 ){
        clock_gettime(CLOCK_REALTIME, &end_time) ;//       gettimeofday(&end_time, NULL);
        float result = end_time.tv_sec - start_time.tv_sec + (float)end_time.tv_nsec/1000000000 - (float)start_time.tv_nsec/1000000000;
        String mess =  String("cnt =") + String((float)ncnt/10000,4);
        mess += String(" sec,  ") + String(result) + String("sec \r\n");
#ifdef USE_BLE
       blloop(mess);//update BLE
#endif
#ifdef USE_TELNET
        telnet.print(mess);
#endif
//        blloop(mess);
//        if(tdebug)telnet.print(mess);
      }
      if(ncnt > 8000){
        if(loops){//don't use first loop
          new_cnt = ncnt;
          ncnt_sum += ncnt;
        }
        loops++;
        if(loops >= swings + 1){
          ncnt_result = ncnt_sum / (float)swings;
          store_avg(cycle++,new_cnt,ncnt_result);

          if(debug){
            String mess =  String("avg=") + String((float)ncnt_result/(10000),4) +  String(" sec\r\n");
#ifdef USE_BLE
            blloop(mess);//update BLE
#endif
#ifdef USE_TELNET
           telnet.print(mess);
#endif
//            blloop(mess);
//            if(tdebug)telnet.print(mess);
          }
          loops = ncnt_sum = 0;
        }
      }
      ncnt = 0;
    }//end of trigger processing
    sprocess_input();
#ifdef USE_OTA
    if(connected)otaloop();
#endif 
    last_ir_sensor_state = ir_sensor_state;
  }//end of 100 usec loop
 }

bool connect_to_wifi(){
  int login_cnt = 0,wifi_status;
  bool not_connected = true;
#ifdef USE_WIFI

  if(WiFi.isConnected()){
    WiFi.disconnect();
    Serial.println("WIFi disconnected");
  }
  // Configures static IP address
  if (!WiFi.config(local_IP,gate_IP,sub_IP,dns_IP)){
    Serial.println("STA Failed to configure");
  }
  while(not_connected){
    if(debug == 1)Serial.println("tp2");
    WiFi.begin(ssid, password);

    //    WiFi.setTxPower(WIFI_POWER_15dBm);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    if(debug == 1)Serial.println("tp3");
    while (wifi_status=WiFi.status() != WL_CONNECTED && login_cnt++ < 12) {
      delay(1000);
      Serial.printf("%d.",wifi_status);
    }
    if(login_cnt < 12){
      Serial.println("");
      Serial.println("WiFi connected..!");
      Serial.print("Got IP: ");  
      Serial.println(WiFi.localIP());
      Serial.print("Got gateway: ");  
      Serial.println(WiFi.gatewayIP());
      Serial.print("Got dns: ");  
      Serial.println(WiFi.dnsIP());
      Serial.print("Signal Strngth: ");  
      Serial.println(WiFi.RSSI());
//      times[9] = WiFi.RSSI();
      delay(3000);
      not_connected = false;
//      configTime(-60 * 60 * 5, 0, ntpServer);
      configTime(0, 0, ntpServer);
      int maxRetries = 20,retryCounter = 0;
      struct tm timeinfo1;
      while (!getLocalTime(&timeinfo1) && retryCounter < maxRetries) {
          delay(500);
          Serial.print(".");
          retryCounter++;
      }
    }else{
       Serial.println("wifi failed");
    }
  }
#endif
  return not_connected;
}

void sprocess_input() {
  String mess;
  int reboot = 0;
  if(telnetrx != 0)rxdata = telnetrx;
  if (rxdata != 0) {
    if (rxdata == 'h'){
      if(telnetrx == 0)mess = "1 = debug,R=reset cycle,t=adjust swings\r\nc or C=read+,c=read-,e=read cycle,L=print all cycles,y=debounce,x=reboot,s=setup\r\n";
      else mess = "1=read,2=read next,3=read prev,4=toggle debug \n\r5=adjust swings,6=reset cycle,7=help\n\r8=print all cycles,9=debounce\n\rbye=quit,reboot=reboot,0=setup\r\n";
    }
    //  else if (rxdata == 'c'){ mess = "clearing counter\r\n";counter = 0;}
    // else if (rxdata == 'c'){ mess = "clearing counter\r\n";counter = 0;}
     else if (rxdata == 'C'){read_cycle++;if(read_cycle>=500)read_cycle=499; mess = String("increased read_cycle to ") + String(read_cycle) +String("\r\n");}
     else if (rxdata == 'e'){mess =  String("cycle = ") + String(read_cycle) + String(",single cnt =") + String(get_cnt(read_cycle))+ String(",avg =") + String(get_avg(read_cycle)/10000,4) + String("\r\n");}
     else if (rxdata == 'c'){read_cycle--;if(read_cycle<0)read_cycle=0; mess = String("decreased read_cycle to ") + String(read_cycle) +String("\r\n");}
     else if (rxdata == 'x'){reboot = 1;String("rebooting in 2 seconds ") + String("\r\n");}
     else if (rxdata == 'L'){print_cycles();}
     else if (rxdata == 't'){swings*=2;if(swings>2100)swings=1; mess = String(" swings set to ") + String(swings) +String("\r\n");store_swings(swings);}
     else if (rxdata == 'y'){debounce +=1;if(debounce>25)debounce=1; mess = String(" debounce set to ") + String(debounce) +String("\r\n");store_debounce(debounce);}
    // else if (rxdata == 'p'){photo-=100;if(photo<100)photo=100; mess = String("decreased photo to ") + String(photo) +String("\r\n");}
    // else if (rxdata == 'M'){motion_delta+=100;if(motion_delta>2000)motion_delta=2000; mess = String("increased motion_delta to ") + String(motion_delta) +String("\r\n");}
    // else if (rxdata == 'm'){motion_delta-=100;if(motion_delta<100)motion_delta=100; mess = String("decreased motion_delta to ") + String(motion_delta) +String("\r\n");}
    // else if (rxdata == 'Y'){yellow_limit+=5;if(yellow_limit>80)yellow_limit=80; mess = String("increased yellow_limit to ") + String(yellow_limit) +String("\r\n");}
    // else if (rxdata == 'y'){yellow_limit-=5;if(yellow_limit<red_limi + 5)yellow_limit=red_limit + 5; mess = String("decreased yellow_limit to ") + String(yellow_limit) +String("\r\n");}
    // else if (rxdata == 'R'){red_limit+=5;if(red_limit>yellow_limit - 5)red_limit=yellow_limit - 5; mess = String("increased red_limit to ") + String(red_limit) +String("\r\n");}
    // else if (rxdata == 'r'){red_limit-=5;if(red_limit<5)red_limit=5; mess = String("decreased red_limit to ") + String(red_limit) +String("\r\n");}
    // else if (rxdata == 'T'){motion_time_limit+=2;if(motion_time_limit>60)motion_time_limit=60; mess = String("increased motion_time_limit to ") + String(motion_time_limit) +String("\r\n");}
    // else if (rxdata == 't'){motion_time_limit-=2;if(motion_time_limit<4)motion_time_limit=4; mess = String("decreased motion_time_limit to ") + String(motion_time_limit) +String("\r\n");}
    // else if (rxdata == 'w'){mess = "update eeprom\r\n";load_up_eeprom(false,true);}
     else if (rxdata == 's') {mess =  String("single =") + String((float)new_cnt/10000,4) +  String( " sec,swings=") + String(swings)  +  String( ",debounce=") + String(debounce) + String(",result =") + String((float)ncnt_result/10000,4) +  String(" sec\r\n");}
     else if (rxdata == 'R'){cycle = 0;store_avg(cycle,1,2);} //send setup
     else if (rxdata == '1'){debug++;if(debug > 2)debug = 0; mess = String("debug set to ") + String(debug) + String("\r\n time = ");
                            mess += String(printInternetTime()) + String("\r\n");}
    // else if (rxdata == '2'){test_radar = !test_radar;if(test_radar) mess = String("test_radar on\r\n");else  mess = String("test_radar off\r\n");}
     else mess = "invalid data send 'h' for help\n";
    // setup_ld016();  printInternetTime()
#ifdef USE_BLE
    if(telnetrx == 0)blloop(mess);
    //blloop(mess);//update BLE
#endif
#ifdef USE_TELNET
    if(telnetrx)telnet.print(mess);
#endif
    if(telnetrx)tdebug = debug;
    // if(telnetrx == 0)blloop(mess);
    // else{
    //   telnet.print(mess);
    //   tdebug = debug;
    // }
    if(rxdata == 'c' || rxdata == 'C'){
      mess =  String("cycle = ") + String(read_cycle) + String(",cnt =") + String(get_cnt(read_cycle))+String(",swings =") + String(swings)+ String(",avg =") + String(get_avg(read_cycle)/10000,4) + String("\r\n");
#ifdef USE_BLE
    if(telnetrx == 0)blloop(mess);
    //blloop(mess);//update BLE
#endif
#ifdef USE_TELNET
    if(telnetrx)telnet.print(mess);
#endif
      //  if(telnetrx == 0)blloop(mess);
      //  else telnet.print(mess);
    }
    rxdata = 0;
    mess = String("last cycle =") + String(get_last_cycle()) + String(",single cnt =") + String((float)new_cnt/10000,4)+String(",swings =") + String(swings)+  String(",avg =") + String(get_last_avg()/10000,4) + String("\r\n");
#ifdef USE_BLE
    if(telnetrx == 0)blloop(mess);//update BLE
#endif
#ifdef USE_TELNET
    if(telnetrx) telnet.print(mess);
#endif
    telnetrx = 0;
  }else{
    mess =  String("");
#ifdef USE_BLE
    if(telnetrx == 0)blloop(mess);//update BLE
#endif
  }
#ifdef USE_TELNET
  if(connected)teloop();
#endif
  if(reboot == 1){delay(2000);ESP.restart();delay(2000);}
}

int get_last_cycle(){
  int lcycle;
  EEPROM.get(0,lcycle);
  if(lcycle < 0 || lcycle >= 500)lcycle = 0;
  return lcycle;
}

int get_swings_level(){
  int lcycle;
  EEPROM.get(4,lcycle);
  if(lcycle < 1 || lcycle >= 2000)lcycle = 1;
  return lcycle;
}

int get_debounce_level(){
  int lcycle;
  EEPROM.get(8,lcycle);
  if(lcycle < 1 || lcycle >= 25)lcycle = 1;
  return lcycle;
}

float get_avg(int cycle){
  float lcycle;
  EEPROM.get(16 + 500 * 4 + 4 * cycle,lcycle);
  return lcycle;
}

int get_cnt(int cycle){
  int lcycle;
  EEPROM.get(16 + 4 * cycle,lcycle);
  return lcycle;
}

void print_cycles(){
  int lcycle,cnt;
  float pk,pk_avg = 0;
  EEPROM.get(0,lcycle);
   String mess1;
  for(int k =0;k < lcycle + 1;k++){
    EEPROM.get(16 + k * 4,cnt);
    EEPROM.get(16 + 500 * 4 + k * 4,pk);
    Serial.printf("cycle = %d, single swing = %d\n\r",k,cnt);
    if(k == 0)mess1 = String("cycle, single sec, avg sec \r\n");
    else mess1 = "";
    String mess =  mess1 + String(k) +  String("  ,") + String((float)cnt/10000,4) +  String("  ,") + String(pk/10000,4) + String(" \r\n");
#ifdef USE_BLE
    blloop(mess);//update BLE
#endif
#ifdef USE_TELNET
    if(telnetrx)telnet.print(mess);
#endif
//    blloop(mess);
//    if(telnetrx)telnet.print(mess);
    pk_avg += pk;
  }
  pk_avg /= (lcycle + 1);
  String mess = String("avg of all cycles =") + String(pk_avg/10000,5) + String(" sec\r\n");
#ifdef USE_BLE
    blloop(mess);//update BLE
#endif
#ifdef USE_TELNET
    if(telnetrx)telnet.print(mess);
#endif
//  blloop(mess);
//  if(telnetrx)telnet.print(mess);
}

float get_last_avg(){
  int lcycle;
  float cnt;
  EEPROM.get(0,lcycle);
  EEPROM.get(16 + 500 *4 + lcycle * 4,cnt);
  return cnt;
}

void store_avg(int cycle,int cnt,float peak){
  if(cycle >= 500)cycle = 0;
  EEPROM.put(16 + cycle * 4,cnt);
  EEPROM.put(16 + 500 * 4 + cycle * 4,peak);
  EEPROM.put(0,cycle);
  EEPROM.commit();
}

void store_swings(int cycle){
  if(cycle >= 2000)cycle = 2000;
  if(cycle <= 1)cycle = 1;
  EEPROM.put(4,cycle);
  EEPROM.commit();
}

void store_debounce(int cycle){
  EEPROM.put(8,cycle);
  EEPROM.commit();
}

 String printInternetTime() {
  struct timeval tv;
  String sttime = "n/a\r\n";
  char stime[100];
  struct timespec ts;
  // Retrieve time from the high-resolution POSIX clock
  // CLOCK_REALTIME tracks absolute time since the Epoch (Jan 1, 1970)
  if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
    struct tm timeinfo;
    ts.tv_sec -= 60 * 60 * 4;//time zone
 // Convert the seconds portion to broken-down local calendar time
    localtime_r(&ts.tv_sec, &timeinfo);
    // Convert nanoseconds to ms
    long ms = (ts.tv_nsec / 1000000) % 1000; 
    sprintf(stime," %02d/%02d/%04d time %02d:%02d:%02d.%03ld\r\n",
            timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_year+1900,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, ms);
//    telnet.print(stime);
    sttime = String(stime);
  }
  return sttime;
}



