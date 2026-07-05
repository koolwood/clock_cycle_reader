#define USE_OTA
#define USE_TELNET
#define USE_BLE

#include <Arduino.h>
#include <WiFi.h>
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
    setupTelnet();
  }
  EEPROM.begin(4096);


  blsetup();

  //set the resolution to 12 bits (0-4095)
  analogReadResolution(12);
  cycle = get_last_cycle();
  swings = get_swings_level();
  Serial.printf("last cycle = %d\r\n",cycle);
  Serial.printf("last swings = %d\r\n",swings);
  read_cycle = cycle;
  //asetup();
    pinMode(irsensor,INPUT_PULLUP);

}

void loop() {
  //  static bool lights_off = false;
  unsigned long nowTime = micros();
  static bool first_loop = true;
  static int loopcnt = 0;
  static int on_cnt = 0,last_t_peak,start_cycle = 0,cnt = 0;
  static int ir_sensor_state = 0,last_ir_sensor_state = 0,ncnt = 0,loops = 0;
  //static int history[CLOCK_SWINGS];
  static float ncnt_sum = 0;

    if(nowTime > startTime + 100 ){
      startTime = micros();
      ir_sensor_state = digitalRead(irsensor);
      ncnt++;
      if(last_ir_sensor_state == 0 && ir_sensor_state == 1){
        if(ncnt > 8000 && ncnt < 8800){
          new_cnt = ncnt;
          ncnt_sum += ncnt;
          loops++;
      //    history[loops++] = ncnt;
          if(loops == swings){
            // avg_cnt = 0;
            // for(int i = 0;i < CLOCK_SWINGS;i++)avg_cnt += history[i];
            // avg_cnt /= CLOCK_SWINGS;
            ncnt_result = ncnt_sum / swings;
            store_avg(cycle++,new_cnt,ncnt_result);

            if(debug){
              String mess =  String("new_cnt =") + String(new_cnt) +  String("   swings =") + String(swings)+  String("   ncnt_result =") + String(ncnt_result) +  String("\r\n");
              blloop(mess);
              if(tdebug)telnet.println(mess);
            }
            loops = ncnt_sum = 0;
          }
        }
        ncnt = 0;
        sprocess_input();
#ifdef USE_OTA
        if(connected)otaloop();
#endif 
     }
      last_ir_sensor_state = ir_sensor_state;
    }
//   sprocess_input();
//   if(connected)otaloop();
 }


bool connect_to_wifi(){
  int login_cnt = 0,wifi_status;
  bool not_connected = true;

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
    }else{
       Serial.println("wifi failed");
    }
  }
  return not_connected;
}
void sprocess_input() {
  String mess;
  if(telnetrx != 0)rxdata = telnetrx;
  if (rxdata != 0) {
    if (rxdata == 'h'){
      if(telnetrx == 0)mess = "1 = debug,R=reset cycle,t=adjust swings\r\nc or C=read+,c=read-,e=read cycle,L=print all cycles\r\n";
      else mess = "1=read,2=read next,3=read prev,4=toggle debug \n\r5=adjust treg,6=reset cycle,7=help\n\r8=print all cycles, bye=quit";
    }
    //  else if (rxdata == 'c'){ mess = "clearing counter\r\n";counter = 0;}
    // else if (rxdata == 'c'){ mess = "clearing counter\r\n";counter = 0;}
     else if (rxdata == 'C'){read_cycle++;if(read_cycle>=100)read_cycle=499; mess = String("increased read_cycle to ") + String(read_cycle) +String("\r\n");}
     else if (rxdata == 'e'){mess =  String("cycle = ") + String(read_cycle) + String(",single cnt =") + String(get_cnt(read_cycle))+ String(",avg =") + String(get_avg(read_cycle)) + String("\r\n");}
     else if (rxdata == 'c'){read_cycle--;if(read_cycle<0)read_cycle=0; mess = String("decreased read_cycle to ") + String(read_cycle) +String("\r\n");}
    // else if (rxdata == 'L'){light_time++;if(light_time>10)light_time=10; mess = String("increased light_time to ") + String(light_time) +String("\r\n");}
     else if (rxdata == 'L'){print_cycles();}
     else if (rxdata == 't'){swings*=2;if(swings>1200)swings=10; mess = String(" swings set to ") + String(swings) +String("\r\n");store_swings(swings);}
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
     else if (rxdata == 's') {mess =  String("single cnt =") + String(new_cnt) +  String(",swings =") + String(swings) + String(",ncnt_result =") + String(ncnt_result) +  String("\r\n");}
     else if (rxdata == 'R'){cycle = 0;store_avg(cycle,1,2);} //send setup
     else if (rxdata == '1'){debug++;if(debug > 1)debug = 0; if(debug)mess = String("debug on\r\n");else  mess = String("debug off\r\n");}
    // else if (rxdata == '2'){test_radar = !test_radar;if(test_radar) mess = String("test_radar on\r\n");else  mess = String("test_radar off\r\n");}
     else mess = "invalid data send 'h' for help\n";
    // setup_ld016();
    if(telnetrx == 0)blloop(mess);
    else{
      telnet.println(mess);
      tdebug = debug;
    }
    if(rxdata == 'c' || rxdata == 'C'){
      mess =  String("cycle = ") + String(read_cycle) + String(",cnt =") + String(get_cnt(read_cycle))+String(",swings =") + String(swings)+ String(",avg =") + String(get_avg(read_cycle)) + String("\r\n");
       if(telnetrx == 0)blloop(mess);
       else telnet.println(mess);
    }
    rxdata = 0;
    mess = String("last cycle =") + String(get_last_cycle()) + String(",single cnt =") + String(new_cnt)+String(",swings =") + String(swings)+  String(",avg =") + String(get_last_avg()) + String("\r\n");
    if(telnetrx == 0)blloop(mess);
    else telnet.println(mess);
    telnetrx = 0;
  }else{
    mess =  String("");
     if(telnetrx == 0)blloop(mess);//update BLE
  }
  if(connected)teloop();
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
  if(lcycle < 100 || lcycle >= 2000)lcycle = 100;
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
  float pk;
  EEPROM.get(0,lcycle);
  for(int k =0;k < lcycle;k++){
    EEPROM.get(16 + k * 4,cnt);
    EEPROM.get(16 + 500 * 4 + k * 4,pk);
    Serial.printf("cycle = %d, on_cnt = %d\n\r",k,cnt);
    String mess = String("cycle=") + String(k) +  String(", cnt=") + String(cnt) +  String(", peek=") + String(pk) + String("\r\n");
    blloop(mess);
    if(telnetrx)telnet.println(mess);
  }
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
  if(cycle >= 1200)cycle = 1200;
  if(cycle <= 60)cycle = 60;
  EEPROM.put(4,cycle);
  EEPROM.commit();
}


