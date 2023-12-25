/****************************************************************************
 ***************************** Header File List *****************************
 ***************************************************************************/
#include<otadrive_esp.h>
#include <FirebaseESP32.h>
#include <addons/RTDBHelper.h>
#include<EEPROM.h>
#include <ESP32Servo.h>



/****************************************************************************
 ************** Wifi Connectivity and Update Functions List *****************
 ***************************************************************************/
void connectWiFiWithEEP(bool t);
void uploadwiFiCrToEEP();
void reloadwiFiCrFromEEP();
void upgrade();
void OnProgress(int progress, int total);
void autoconnect(bool state); /////Need to implement in the code
uint32_t getID();

/****************************************************************************
 *********  Data Processing and Task Handelling Functions List **************
 ***************************************************************************/
void Localtask();
void Cloudtask(bool ConnectCloud);
void process_engine(String s);
void data_engine(String DATA);
void smart_mode(bool state);
void safe_mode(bool state);


/****************************************************************************
 ***************************** Sensors Function List ************************
 ***************************************************************************/
double readUltrasonicDistance(const int trigger, const int echo);
int measure_light(const int pin);
bool is_Anyone_inside(const int trig, const int echo);
bool is_Weather_Clear(int powerpin, int datapin);
void alert(const int buz, bool a);
bool is_Fire_Detected(const int fr);

/****************************************************************************
 ***************************** GSM ************************
 ***************************************************************************/
void alert_sender(String phn_num, String msg, int alert_mode);
void waitForResponse();
void make_call(String phn_num);
void send_sms(String phn_num, String msg);


/****************************************************************************
 ********************************* Variable List ****************************
 ***************************************************************************/
#define FIREBASE_API_KEY "4iroX8Xjc53K1NdoOhMZb4bfIFM7u915fVmn0TgS"
#define DATABASE_URL "dream-palace-37cf0-default-rtdb.firebaseio.com"
String buff;
String SSID = "Sahanaz";
String WIFIPASS = "2024bubt";
String percentage = "0";
String OTA_api_key = "8927a03d-b29e-4ffc-b45f-4359060e7328", version = "1.8.4.7";
String Data = "", FDB = "", prev_fdb = "", updatest = "0~0~0~0~0~0"; //updatest={fan1->0,led1->2,led2->4,window1->6,water_motor1->8,water_percentage->10}
unsigned long DataMillis = 0, wifiMillis = 0, powerMillis = 0, fireMillis = 0, pumpMillis = 0, updateMillis = 0;
double dfml = 6, h = 16.8, p = 0, y = 0, x = 0; //unit in(cm) centimetre
double c = 2000, aw; //unit in(L)
int count = 0, i = 0;
int open_angle = 90, close_angle = 0;
bool ConnectCloud = true;
bool SmartMode = false, SafeMode = false;
bool ON = HIGH;
bool OFF = LOW;
bool beganfrbs = false;
bool Wifi_Con = false;
bool intuder_detected = false;
int prcnt = 1, available_Water = 2, all = 3;
bool tryfill = false;
bool pump_state = false;
int fillamount = 0;
int upperlimit = 100, lowerlimit = 40;
/****************************************************************************
 ********************************* Object List ******************************
 ***************************************************************************/
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Servo window;


/*****************************************************************************************************
************************************ Defining Pin Number *********************************************
******************************************************************************************************
*/
// BT_TX -> RX0 ESP
// BT_RX -> TX0 ESP
#define RoomtriggerPin 33
#define RoomechoPin 32
#define WatertriggerPin 26
#define WaterechoPin 27
#define RainData 15
#define RainPower 4
#define led1 18
#define led2 13
#define fan1 19
#define Window1 12
#define buzzer 22
#define intuderpin 5 //Skipped
#define fire 14
#define mainpower 21 //problematic
#define motor1 25

/*
****************************************
****************************************
****************************************
*/




void setup() {
  Serial2.begin(9600);//Initializing gsm on serial 2
  Serial.begin(9600);
  Serial2.println("AT");
  //waitForResponse();
  EEPROM.begin(512); //Initialasing EEPROM
  pinMode(mainpower, OUTPUT);
  digitalWrite(mainpower, ON);
  pinMode(RoomtriggerPin, OUTPUT);
  pinMode(RoomechoPin, INPUT);
  pinMode(WatertriggerPin, OUTPUT);
  pinMode(WaterechoPin, INPUT);
  pinMode(RainPower, OUTPUT);
  pinMode(RainData, INPUT);
  pinMode(fire, INPUT);
  pinMode(intuderpin, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(motor1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(fan1, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(fan1, OFF);
  digitalWrite(led1, OFF);
  digitalWrite(led2, OFF);
  digitalWrite(motor1, OFF);
  window.attach(Window1);
  OTADRIVE.setInfo(OTA_api_key, version);
  OTADRIVE.onUpdateFirmwareProgress(OnProgress);
  connectWiFiWithEEP(false);
}


void loop() {
  //alert(buzzer, is_Fire_Detected(fire));
  if (is_Fire_Detected(fire))
  {
    //alert_sender("+8801974682349", "Warning!!! Fire is detected at home!", 3);
  }
  if (digitalRead(intuderpin) == HIGH) {
    //Serial.print("Intuder Detected");
    //alert_sender("+8801974682349", "Warning!!! Intruder is detected at door!", 3);
    intuder_detected = true;
  }
  if (beganfrbs == false && WiFi.status() == WL_CONNECTED) {
    beganfrbs == true;
    config.database_url = DATABASE_URL;
    config.signer.test_mode = true;
    Firebase.begin(&config, &auth);
  }
  if (millis() - wifiMillis > 2000 || wifiMillis == 0) {
    wifiMillis = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Wifi_Con = true;
    }
    else {
      Wifi_Con = false;
    }

  }

  Cloudtask(ConnectCloud);
  Localtask();
  if (millis() - powerMillis > 1000 || wifiMillis == 0) {
    powerMillis = millis();
    percentage = measure_water(WatertriggerPin, WaterechoPin, prcnt);
    Serial.println("?wifi:" + (String)Wifi_Con + " Cloud:" + (String)ConnectCloud + " Clear Weather:" + (String)is_Weather_Clear(RainPower, RainData) + " Water:" + percentage + +"% Avaialable Water"+(String)((double)(aw) / 1000.0) + "L  Anyone inside:" + (String)x + " Fire:" + (String)is_Fire_Detected(fire) + " Version:" + version);

  }

  smart_mode(SmartMode);
  safe_mode(SafeMode);
  if (SmartMode == false) {
    if (tryfill && !WaterFilled(fillamount)) {
      if (updatest[8] == '0') {
        digitalWrite(motor1, ON); updatest[8] == '1';
      }
      Serial.println("trying to fill.." + (String)fillamount + "%");
    }

    if (tryfill && WaterFilled(fillamount)) {
      Serial.println("done");
      tryfill = false;
      digitalWrite(motor1, OFF); updatest[8] == '0';
    }
  }
  //update database device status
  if (millis() - updateMillis > 1000 || updateMillis == 0) {
    updatest.remove(10, 3);
    updatest.concat(percentage);
    updateMillis = millis();
    String temp = Firebase.setString(fbdo, "/test/status", updatest) ? fbdo.to<const char *>() : fbdo.errorReason().c_str();
  }



}







void Cloudtask(bool ConnectCloud) {
  if (ConnectCloud) {
    if (Firebase.ready() && (millis() - DataMillis > 1000 || DataMillis == 0)) {
      DataMillis = millis();
      FDB = Firebase.getString(fbdo, "/test/tag") ? fbdo.to<const char *>() : fbdo.errorReason().c_str();
      FDB.remove(FDB.length() - 2, 2);
      FDB.remove(0, 2);
      if (prev_fdb.compareTo(FDB) != 0) {
        data_engine(FDB);
      }
      prev_fdb = FDB;
      FDB = "";
    }
  }
}



void Localtask() {
  if (Serial.available() > 0) {
    Data = Serial.readString();
    Serial.println(Data);
    Serial.println(Data.compareTo("yobro"));
    data_engine(Data);
    Data = "";
  }
}

//Returns nothing. Recieve,Analyze data
void data_engine(String DATA) {
  int pre_index = 0;
  if (DATA[0] == '?' && DATA[1] == 'S' && DATA[2] == 'P') {
    uploadwiFiCrToEEP(DATA);
    return;
  }
  else{
  for (int i = 0; i < DATA.length(); i++) {

    if (DATA[i] == '~') {
      process_engine(DATA.substring(pre_index, i));
      pre_index = i + 1;

    }
    }
  }

}

void process_engine(String s) {
  
  if ((s[0] == 'f') && (s[1] == 'i') && (s[2] == 'l') && (s[3] == 'l')) {
    tryfill = true;
    s.remove(0, 4);
    fillamount = s.toInt();
  }
  
  if (s.compareTo("update") == 0) upgrade();
  else if (s.compareTo("mainpoweroff") == 0) digitalWrite(mainpower, OFF);
  else if (s.compareTo("mainpoweron") == 0) digitalWrite(mainpower, ON);
  else if (s.compareTo("wifioff") == 0)WiFi.mode(WIFI_OFF);
  else if (s.compareTo("connect") == 0)connectWiFiWithEEP(false);
  else if (s.compareTo("connecteep") == 0)connectWiFiWithEEP(true);
  else if (s.compareTo("smartmodeon") == 0)SmartMode = true;
  else if (s.compareTo("smartmodeoff") == 0)SmartMode = false;
  else if (s.compareTo("safemodeon") == 0)SafeMode = true;
  else if (s.compareTo("safemodeoff") == 0)SafeMode = false;
  else if (s.compareTo("ConnectCloud") == 0)ConnectCloud = true;
  else if (s.compareTo("disConnectCloud") == 0)ConnectCloud = false;
  
  else if (s.compareTo("led1on") == 0) {digitalWrite(led1, ON);updatest[2] = '1';}
  else if (s.compareTo("led1off") == 0) {digitalWrite(led1, OFF);updatest[2] = '0';}
  else if ((s.compareTo("led2on") == 0) && updatest[4] == '0') {digitalWrite(led2, ON);updatest[4] = '1';}
  else if ((s.compareTo("led2off") == 0) && updatest[4] == '1') {digitalWrite(led2, OFF);updatest[4] = '0';}
  else if (s.compareTo("fan1on") == 0){digitalWrite(fan1, ON);updatest[0] = '1';}
  else if (s.compareTo("fan1off") == 0){digitalWrite(fan1, OFF);updatest[0] = '0';}
  else if (s.compareTo("motor1on") == 0){updatest[8] = '1'; digitalWrite(motor1, ON);}
  else if (s.compareTo("motor1off") == 0){updatest[8] = '0'; digitalWrite(motor1, OFF);}
  else if (s.compareTo("window1open") == 0) {window.write(open_angle);updatest[6] = '1';}
  else if (s.compareTo("window1close") == 0) {window.write(close_angle);updatest[6] = '0';}
}


String measure_water(const int trig, const int echo, int mode)
{
  y = readUltrasonicDistance(trig, echo); //trig ,echo
  if (y > dfml + h)
    y = dfml + h;
  x = dfml + h - y;
  if (x > h)
    x = h;
  p = (100 * x) / h;
  aw = (c * x) / h;
  String ress;
  if (mode == prcnt) {
    ress = (String)((int)(p));
  }
  else if (mode == available_Water) {
    ress = (String)((double)(aw) / 1000.0);
  }
  else if (mode == all) {
    ress = (String)((double)(aw) / 1000.0) + "L," + (String)((int)p) + "%";
  }
  return ress;
}
/*
  demand(%) current(%)
  80         90
  80         80
  80         78
*/
bool WaterFilled(int demand) {
  if (percentage.toInt() >= demand)
    return true;
  else
    return false;
}



void connectWiFiWithEEP(bool t) {
  if (t) {
    reloadwiFiCrFromEEP();
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID.c_str() , WIFIPASS.c_str());
    byte tried = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(250);
      tried++;
      if (tried == 20) {
        tried = 0;
        Serial.println("Coudn't connect!");
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Connected to wifi");
    }

  }
  else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID.c_str() , WIFIPASS.c_str());
    byte tried = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(250);
      tried++;
      if (tried == 20) {
        tried = 0;
        Serial.println("Coudn't connect!");
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Connected to wifi");
    }

  }
}
void uploadwiFiCrToEEP(String DATA) {
  SSID = "";
  WIFIPASS = "";
  bool ssidfound = false, passfound = false;
  for (int i = 3; i < DATA.length(); i++) {
    if (ssidfound && DATA[i] == '|')passfound = true;
    if (passfound)break;
    if (DATA[i] == '~') {
      ssidfound = true;
      continue;
    }
    if (!ssidfound)
      SSID += DATA[i];
    if (ssidfound && !passfound) {
      WIFIPASS += DATA[i];
    }
  }

  if (SSID.length() > 0 && WIFIPASS.length() > 0) {
    //WIFIPASS.remove(WIFIPASS.length()-2,2);
    EEPROM.write(90, SSID.length()); EEPROM.write(100, WIFIPASS.length());
    for (int i = 0; i < 65; ++i)EEPROM.write(i, 0);
    for (int i = 0; i < SSID.length(); ++i)EEPROM.write(i, SSID[i]);
    for (int i = 0; i < WIFIPASS.length(); ++i)EEPROM.write(32 + i, WIFIPASS[i]);
    EEPROM.commit();
    Serial.println("UPLoaded wifi credential to eep ssid:" + SSID + " pass:" + WIFIPASS);
  }
}


void reloadwiFiCrFromEEP() {
  int address=0;
  SSID = "";
  WIFIPASS = "";
  //SSID=EEPROM.readString(address);
  //address=
  //WIFIPASS=EEPROM.readString(address);
  for (int i = 0; i < EEPROM.read(90); ++i)SSID += char(EEPROM.read(i));
  for (int i = 32; i < 32 + EEPROM.read(100); ++i)WIFIPASS += char(EEPROM.read(i));
  Serial.println("Loaded wifi credential from eep ssid:" + SSID + " pass:" + WIFIPASS);
}


void upgrade() {
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Request For Update!");
    auto inf = OTADRIVE.updateFirmwareInfo();
    if (inf.available)
    {
      Serial.printf("UV%s", inf.version.c_str());
      Serial.println("");
      OTADRIVE.updateFirmware();
    }
    else
      Serial.println("No update available!");
  }
  else
    Serial.println("Sorry device is offline! Please connect to the internet...");

}

void OnProgress(int progress, int total)
{
  static int last_percent = 0;
  int percent = (100 * progress) / total;
  if (percent != last_percent)
  {
    Serial.println("U" + (String)percent + "%");
    last_percent = percent;
  }
}




//Return distance
double readUltrasonicDistance(const int trigger, const int echo)
{
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  return 0.01723 * pulseIn(echo, HIGH);
}

//Measure light ambient
int measure_light(const int pin) {
  return 1;
}

//Return true if anyone is inside the room
bool is_Anyone_inside(const int trig, const int echo) {
  if (readUltrasonicDistance(trig, echo) < 16) {
    return true;
  }
  else {
    return false;
  }

}


bool is_Weather_Clear(int powerpin, int datapin) {
  digitalWrite(powerpin, HIGH);  // Turn the sensor ON
  delay(10);// Allow power to settle
  int val = digitalRead(datapin); // Read the sensor output
  digitalWrite(powerpin, LOW);   // Turn the sensor OFF
  if (val == 1)return true;
  else return false;
}

bool is_Fire_Detected(const int fr) {
  if (digitalRead(fr) == 0)return true;
  else if (digitalRead(fr) == 0)return false;
  else return !digitalRead(fr);
}


void alert(const int buz, bool a) {
  if (a)tone(buz, 1000);
  else noTone(buz);
}


void safe_mode(bool state) {
  if (state) {
    if (is_Fire_Detected(fire)) {
      window.write(open_angle); updatest[6] = '1';
      digitalWrite(mainpower, OFF); //cutdown all power
    }
  }
}


void smart_mode(bool state) {
  if (state) {

    if (!is_Fire_Detected(fire)) {
      if (!is_Anyone_inside(RoomtriggerPin, RoomechoPin)) {
        digitalWrite(fan1, OFF); updatest[0] = '0';
        digitalWrite(led1, OFF); updatest[1] = '0';
      }
      else if (is_Weather_Clear(RainPower, RainData)) {
        digitalWrite(fan1, ON); updatest[0] = '1';
        if (updatest[6] == '0') {
          digitalWrite(led1, ON);
          updatest[2] = '1';
        }
        else {
          digitalWrite(led1, OFF);
          updatest[2] = '0';
        }
      }
      else if (!is_Weather_Clear(RainPower, RainData)) {
        digitalWrite(fan1, OFF); updatest[0] = '0';
        digitalWrite(led1, ON); updatest[2] = '1';
        window.write(close_angle); updatest[6] = '0';
      }
    }
    if (!pump_state && percentage.toInt() < lowerlimit) {
      Serial.print("water is on lowlimit. Truning on pump....");
      digitalWrite(motor1, ON); updatest[8] == '1';
      pump_state = true;
    }
    if (pump_state && percentage.toInt() >= upperlimit) {
      Serial.print("water is exceeding UpperLimit Turning off pump....");
      digitalWrite(motor1, OFF); updatest[8] == '0';
      pump_state = false;
    }
  }
}

void send_sms(String phn_num, String msg) {

  Serial2.println("AT+CMGF=1");
  waitForResponse();

  Serial2.println("AT+CNMI=1,2,0,0,0");

  waitForResponse();
  String msg_mode = "AT+CMGS=\"" + phn_num + "\"\r";
  Serial2.print(msg_mode);
  waitForResponse();
  Serial2.print(msg);
  Serial2.write(0x1A);
  waitForResponse();
}


void make_call(String phn_num) {
  phn_num = "ATD" + phn_num + ";";
  Serial2.println(phn_num);
  delay(40000);
  waitForResponse();
}

void waitForResponse() {
  delay(1000);
  while (Serial2.available()) {
    buff = Serial2.readString();
    Serial.print(buff);
  }
  //Serial2.read();
}

void alert_sender(String phn_num, String msg, int alert_mode)
{
  int c = 3;
  if (alert_mode == 1)send_sms(phn_num, msg);
  if (alert_mode == 2)
  {
    while (c--) {
      waitForResponse();
      if (buff.indexOf("NO CARRIER") > 0)break;
      else make_call(phn_num);
    }
  }
  if (alert_mode == 3)
  {
    send_sms(phn_num, msg);
    delay(40000);
    while (c--) {
      waitForResponse();
      if (buff.indexOf("NO CARRIER") > 0)break;
      else make_call(phn_num);
    }
  }

}


uint32_t getID(){
  uint32_t chipId=0;
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  return chipId;
  }