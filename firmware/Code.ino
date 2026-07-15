// ─── Fill these before uploading ──────────────────────────────────────────────
#define BLYNK_TEMPLATE_ID   "TMPL3WQxjP_4v"
#define BLYNK_TEMPLATE_NAME "Medicine Box"
#define BLYNK_AUTH_TOKEN    "vD0ncWHf2FZPts3tdaRiMIXqL36o_ivP"
#define BLYNK_PRINT Serial

const char* WIFI_SSID     = "<wifi_name>";
const char* WIFI_PASSWORD = "<wifi_password>";

// ─── Telegram Config ──────────────────────────────────────────────────────────
const char* BOT_TOKEN      = "<bot_token_from_godfather>";
const char* PATIENT_CHATID = "<patient_chat_id";
const char* FAMILY_CHATID  = "<family_chat_id";
// ─────────────────────────────────────────────────────────────────────────────

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <time.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
const int BUZZER_PIN = 25;
const int FSR_PIN    = 34;

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'}, {'4','5','6','B'},
  {'7','8','9','C'}, {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 33, 32, 19};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
BlynkTimer timer;

// ─── Password ─────────────────────────────────────────────────────────────────
String PASSWORD        = "1234";
String enteredPassword = "";
String newPassword     = "";
int    failedAttempts  = 0;
const  int MAX_ATTEMPTS = 3;
bool   isLocked        = false;
unsigned long lockStartTime = 0;
const unsigned long LOCK_DURATION = 10000;

// ─── Clock ────────────────────────────────────────────────────────────────────
int   clockHour=0, clockMin=0, clockSec=0, clockDay=0;
int   clockDate=1, clockMonth=3, clockYear=2026;
unsigned long lastMillis = 0;
bool  clockSet = false;
String dayNames[]   = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
String monthNames[] = {"Jan","Feb","Mar","Apr","May","Jun",
                       "Jul","Aug","Sep","Oct","Nov","Dec"};

// ─── Medicine Times ───────────────────────────────────────────────────────────
int medTimes[7][4] = {
  {8,0,21,0},{8,0,21,0},{8,0,21,0},{8,0,21,0},
  {8,0,21,0},{8,0,21,0},{8,0,21,0}
};

// ─── Medicine Log ─────────────────────────────────────────────────────────────
struct MedLog { String date, type, status, time; };
MedLog medHistory[30];
int    medHistoryCount = 0;

// ─── Alarm State ──────────────────────────────────────────────────────────────
const unsigned long MED_BUZZ_DURATION = 10UL * 60UL * 1000UL;
bool  morningDone=false, eveningDone=false;
bool  alarmRinging=false;
String alarmType="";
unsigned long alarmStartTime=0, lastBuzzToggle=0;
bool  buzzState=false;
bool  medicineTaken=false, familyAlertSent=false, patientAlertSent=false;

String morningTakenTime="--:--", eveningTakenTime="--:--";
String morningStatus="Pending",  eveningStatus="Pending";

bool wifiConnected = false;

// ─── NTP ──────────────────────────────────────────────────────────────────────
unsigned long lastNTPSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 3600000;

// ─── Modes ────────────────────────────────────────────────────────────────────
enum Mode {
  MODE_CLOCK, MODE_NORMAL,
  MODE_CHANGE_VERIFY, MODE_CHANGE_NEW, MODE_CHANGE_CONFIRM,
  MODE_MED_VERIFY, MODE_MED_SHOW, MODE_MED_SELECT_DAY,
  MODE_MED_SELECT_TYPE, MODE_MED_SET_HOUR, MODE_MED_SET_MIN
};
Mode currentMode = MODE_CLOCK;

int    medEditDay=0, medEditType=0;
String medInput="";
int    medVerifyFails=0, medShowDay=0;

// ─── Forward Declarations ─────────────────────────────────────────────────────
void showMessage(String l1, String l2, int ms = 0);
String twoDigitStr(int n);
String getMasked(String s);
void redrawLine(String prefix, String content);
void resetNormal();
void showTriesOnRow0();
void beepKey(); void beepBackspace(); void beepLimit();
void beepNoInput(); void beepSuccess(); void beepError();
void beepWarning(); void beepLocked(); void beepChanged();
void displayClock();
void showMedTimesForDay(int day);
void updateBlynkDashboard();
void addMedLog(String type, String status, String t);
void sendMonthlyLog();
void sendTelegram(String chatId, String message);
void telegramPatient(String msg);
void telegramFamily(String msg);
void telegramBoth(String msg);
void syncNTP();
void scanAndConnect();

// ─── WiFi Connect ─────────────────────────────────────────────────────────────
void scanAndConnect() {
  Serial.println("\n=== Scanning WiFi Networks ===");
  showMessage(" Scanning WiFi..","  Please wait...",0);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);

  int n = WiFi.scanNetworks();
  Serial.println("Found "+String(n)+" networks:");
  bool found = false;
  for(int i=0; i<n; i++){
    String ssid = WiFi.SSID(i);
    int rssi    = WiFi.RSSI(i);
    int channel = WiFi.channel(i);
    Serial.print(String(i+1)+") "+ssid);
    Serial.print("  "+String(rssi)+"dBm");
    Serial.print("  ch"+String(channel));
    if(channel > 14) Serial.print("  [5GHz!]");
    Serial.println();
    if(ssid == String(WIFI_SSID)){
      found = true;
      Serial.println(">>> FOUND! ch:"+String(channel)+" sig:"+String(rssi));
      if(channel > 14) showMessage("Hotspot is 5GHz!","Need 2.4GHz!    ",3000);
    }
  }
  if(!found){
    Serial.println("!!! '"+String(WIFI_SSID)+"' NOT FOUND!");
    showMessage("Hotspot NotFound","Check name/ON?  ",3000);
  }

  Serial.println("Connecting to: "+String(WIFI_SSID));
  showMessage(" Connecting...  ",String(WIFI_SSID).substring(0,16),0);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries=0;
  while(WiFi.status()!=WL_CONNECTED && tries<40){
    delay(500); tries++; Serial.print(".");
  }

  if(WiFi.status()==WL_CONNECTED){
    wifiConnected=true;
    Serial.println("\nWiFi Connected! IP: "+WiFi.localIP().toString());
    showMessage(" WiFi Connected!",WiFi.localIP().toString(),2000);
  } else {
    wifiConnected=false;
    Serial.println("\nWiFi FAILED!");
    switch(WiFi.status()){
      case WL_NO_SSID_AVAIL:  Serial.println("-> Not found!");    break;
      case WL_CONNECT_FAILED: Serial.println("-> Wrong password!"); break;
      default: Serial.println("-> Error: "+String(WiFi.status())); break;
    }
    showMessage(" WiFi Failed!   "," Offline Mode   ",2000);
  }
}

// ─── NTP Sync ─────────────────────────────────────────────────────────────────
void syncNTP() {
  if(!wifiConnected) return;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){Serial.println("NTP failed");return;}
  int prevDay=clockDay;
  clockHour=timeinfo.tm_hour; clockMin=timeinfo.tm_min; clockSec=timeinfo.tm_sec;
  clockDay=timeinfo.tm_wday; clockDate=timeinfo.tm_mday;
  clockMonth=timeinfo.tm_mon+1; clockYear=timeinfo.tm_year+1900;
  lastMillis=millis(); clockSet=true; lastNTPSync=millis();
  if(prevDay!=clockDay){
    morningDone=false; eveningDone=false;
    medicineTaken=false; familyAlertSent=false; patientAlertSent=false;
    morningTakenTime="--:--"; eveningTakenTime="--:--";
    morningStatus="Pending"; eveningStatus="Pending";
  }
  Serial.println("NTP: "+String(dayNames[clockDay])+" "+
    twoDigitStr(clockHour)+":"+twoDigitStr(clockMin)+":"+twoDigitStr(clockSec));
}

// ─── Telegram ─────────────────────────────────────────────────────────────────
void sendTelegram(String chatId, String message) {
  if (!wifiConnected) { Serial.println("Telegram skipped"); return; }

  Serial.println("Sending to: " + chatId);

  // URL encode message
  String encoded = "";
  for (int i = 0; i < message.length(); i++) {
    char c = message[i];
    if (c == ' ') encoded += '+';
    else if (c == '\n') encoded += "%0A";
    else if (c == ':') encoded += "%3A";
    else if (c == '!') encoded += "%21";
    else if (c == '?') encoded += "%3F";
    else if (c == '#') encoded += "%23";
    else if (c == '&') encoded += "%26";
    else encoded += c;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15);

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Connection to Telegram FAILED!");
    return;
  }

  String request = "GET /bot" + String(BOT_TOKEN) +
                   "/sendMessage?chat_id=" + chatId +
                   "&text=" + encoded +
                   " HTTP/1.1\r\n" +
                   "Host: api.telegram.org\r\n" +
                   "Connection: close\r\n\r\n";

  client.print(request);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println("Telegram timeout!");
      client.stop();
      return;
    }
  }

  String response = "";
  while (client.available()) {
    response += (char)client.read();
  }
  client.stop();

  if (response.indexOf("\"ok\":true") >= 0) {
    Serial.println("Telegram OK! -> " + chatId);
  } else {
    Serial.println("Telegram response: " + response.substring(0, 150));
  }
}

void telegramPatient(String msg){ sendTelegram(String(PATIENT_CHATID), msg); }
void telegramFamily(String msg) { sendTelegram(String(FAMILY_CHATID),  msg); }
void telegramBoth(String msg)   { telegramPatient(msg); delay(1000); telegramFamily(msg); }

// ─── Medicine Log ─────────────────────────────────────────────────────────────
void addMedLog(String type, String status, String takenTime) {
  if(medHistoryCount>=30){for(int i=0;i<29;i++)medHistory[i]=medHistory[i+1];medHistoryCount=29;}
  MedLog entry;
  entry.date=String(clockDate)+" "+monthNames[clockMonth-1];
  entry.type=type; entry.status=status; entry.time=takenTime;
  medHistory[medHistoryCount]=entry; medHistoryCount++;
}

void sendMonthlyLog() {
  if(!wifiConnected) return;
  String log="";
  for(int i=medHistoryCount-1;i>=0;i--){
    String icon=(medHistory[i].status=="Taken")?"✅":"❌";
    log+=icon+" "+medHistory[i].date+" "+medHistory[i].type+" "+medHistory[i].time+"\n";
  }
  if(log=="") log="No records yet";
  Blynk.virtualWrite(V7, log);
}

// ─── Blynk ────────────────────────────────────────────────────────────────────
void updateBlynkDashboard() {
  if(!wifiConnected) return;
  Blynk.virtualWrite(V3, morningTakenTime);
  Blynk.virtualWrite(V4, eveningTakenTime);
  String morStr="MOR "+twoDigitStr(medTimes[clockDay][0])+":"+twoDigitStr(medTimes[clockDay][1])+" > "+morningStatus;
  if(morningStatus=="Taken") morStr+=" at "+morningTakenTime;
  Blynk.virtualWrite(V5, morStr);
  String eveStr="EVE "+twoDigitStr(medTimes[clockDay][2])+":"+twoDigitStr(medTimes[clockDay][3])+" > "+eveningStatus;
  if(eveningStatus=="Taken") eveStr+=" at "+eveningTakenTime;
  Blynk.virtualWrite(V6, eveStr);
  Blynk.virtualWrite(V8, analogRead(FSR_PIN));
  sendMonthlyLog();
}
void sendBlynkFSR(){if(!wifiConnected)return;Blynk.virtualWrite(V8,analogRead(FSR_PIN));}
void notifyPatient(String type){
  if(!wifiConnected||patientAlertSent)return;
  patientAlertSent=true;
  Blynk.logEvent("medicine_alarm","Time to take "+type+" medicine!");
}
void notifyMedicineTaken(String type,String takenTime){
  if(!wifiConnected)return;
  Blynk.logEvent("medicine_taken",type+" taken at "+takenTime);
  Blynk.virtualWrite(V2,1);delay(100);Blynk.virtualWrite(V2,0);
}
void notifyFamily(String type){
  if(!wifiConnected||familyAlertSent)return;
  familyAlertSent=true;
  Blynk.logEvent("family_alert","MISSED! "+type+" not taken!");
}

// ─── Buzzer ───────────────────────────────────────────────────────────────────
void playTone(int freq,int ms){ledcWriteTone(BUZZER_PIN,freq);ledcWrite(BUZZER_PIN,255);delay(ms);ledcWrite(BUZZER_PIN,0);delay(30);}
void beepKey()      {ledcWriteTone(BUZZER_PIN,2700);ledcWrite(BUZZER_PIN,255);delay(60);ledcWrite(BUZZER_PIN,0);delay(20);}
void beepBackspace(){playTone(1800,80);}
void beepLimit()    {playTone(500,300);}
void beepNoInput()  {playTone(800,200);}
void beepSuccess(){
  ledcWriteTone(BUZZER_PIN,1500);ledcWrite(BUZZER_PIN,255);delay(150);ledcWrite(BUZZER_PIN,0);delay(30);
  ledcWriteTone(BUZZER_PIN,2000);ledcWrite(BUZZER_PIN,255);delay(150);ledcWrite(BUZZER_PIN,0);delay(30);
  ledcWriteTone(BUZZER_PIN,2500);ledcWrite(BUZZER_PIN,255);delay(150);ledcWrite(BUZZER_PIN,0);delay(30);
  ledcWriteTone(BUZZER_PIN,3000);ledcWrite(BUZZER_PIN,255);delay(400);ledcWrite(BUZZER_PIN,0);
}
void beepError(){
  ledcWriteTone(BUZZER_PIN,800);ledcWrite(BUZZER_PIN,255);delay(250);ledcWrite(BUZZER_PIN,0);delay(50);
  ledcWriteTone(BUZZER_PIN,500);ledcWrite(BUZZER_PIN,255);delay(250);ledcWrite(BUZZER_PIN,0);delay(50);
  ledcWriteTone(BUZZER_PIN,300);ledcWrite(BUZZER_PIN,255);delay(500);ledcWrite(BUZZER_PIN,0);
}
void beepWarning(){for(int i=0;i<3;i++){ledcWriteTone(BUZZER_PIN,1500);ledcWrite(BUZZER_PIN,255);delay(120);ledcWrite(BUZZER_PIN,0);delay(80);}}
void beepLocked(){for(int i=0;i<5;i++){ledcWriteTone(BUZZER_PIN,2000);ledcWrite(BUZZER_PIN,255);delay(200);ledcWriteTone(BUZZER_PIN,500);ledcWrite(BUZZER_PIN,255);delay(200);}ledcWrite(BUZZER_PIN,0);}
void beepChanged(){
  ledcWriteTone(BUZZER_PIN,2000);ledcWrite(BUZZER_PIN,255);delay(100);ledcWrite(BUZZER_PIN,0);delay(50);
  ledcWriteTone(BUZZER_PIN,2500);ledcWrite(BUZZER_PIN,255);delay(100);ledcWrite(BUZZER_PIN,0);delay(50);
  ledcWriteTone(BUZZER_PIN,3000);ledcWrite(BUZZER_PIN,255);delay(300);ledcWrite(BUZZER_PIN,0);
}

// ─── LCD Helpers ──────────────────────────────────────────────────────────────
void showMessage(String l1,String l2,int ms){lcd.clear();lcd.setCursor(0,0);lcd.print(l1);lcd.setCursor(0,1);lcd.print(l2);if(ms>0)delay(ms);}
String twoDigitStr(int n){return n<10?"0"+String(n):String(n);}
String getMasked(String s){String m="";for(int i=0;i<(int)s.length();i++)m+='*';return m;}
void redrawLine(String prefix,String content){lcd.setCursor(0,1);lcd.print(prefix);lcd.print("              ");lcd.setCursor(prefix.length(),1);lcd.print(content);}
void resetNormal(){currentMode=MODE_NORMAL;enteredPassword="";newPassword="";lcd.clear();lcd.setCursor(0,0);lcd.print("Enter Password: ");lcd.setCursor(0,1);lcd.print(">               ");lcd.setCursor(2,1);}
void showTriesOnRow0(){lcd.setCursor(0,0);lcd.print("Tries left: ");lcd.print(MAX_ATTEMPTS-failedAttempts);lcd.print("    ");}
void showMedTimesForDay(int day){
  lcd.clear();lcd.setCursor(0,0);
  lcd.print(dayNames[day]);lcd.print(" M:");
  lcd.print(twoDigitStr(medTimes[day][0]));lcd.print(":");lcd.print(twoDigitStr(medTimes[day][1]));
  lcd.print(" E:");lcd.print(twoDigitStr(medTimes[day][2]));lcd.print(":");lcd.print(twoDigitStr(medTimes[day][3]));
  lcd.setCursor(0,1);lcd.print("A/B=nav #=edit *=exit");
}

// ─── Clock ────────────────────────────────────────────────────────────────────
void updateClock(){
  if(!clockSet) return;
  if(wifiConnected&&millis()-lastNTPSync>NTP_SYNC_INTERVAL){syncNTP();return;}
  unsigned long now=millis(),diff=now-lastMillis;
  if(diff>=1000){
    int secs=diff/1000;lastMillis+=secs*1000;clockSec+=secs;
    if(clockSec>=60){clockSec-=60;clockMin++;}
    if(clockMin>=60){clockMin=0;clockHour++;}
    if(clockHour>=24){
      clockHour=0;clockDay=(clockDay+1)%7;clockDate++;
      if(clockDate>30){clockDate=1;clockMonth++;if(clockMonth>12){clockMonth=1;clockYear++;}}
      morningDone=false;eveningDone=false;
      medicineTaken=false;familyAlertSent=false;patientAlertSent=false;
      morningTakenTime="--:--";eveningTakenTime="--:--";
      morningStatus="Pending";eveningStatus="Pending";
      updateBlynkDashboard();
    }
  }
}

void checkMedicineAlarms(){
  if(!clockSet||alarmRinging) return;
  if(clockHour==medTimes[clockDay][0]&&clockMin==medTimes[clockDay][1]&&clockSec==0&&!morningDone){
    morningDone=true;alarmRinging=true;alarmStartTime=millis();lastBuzzToggle=millis();
    buzzState=false;alarmType="MORNING";medicineTaken=false;familyAlertSent=false;patientAlertSent=false;
    morningStatus="Alarm!";lcd.clear();
    telegramPatient("💊 MEDICINE REMINDER\n📅 "+String(clockDate)+" "+monthNames[clockMonth-1]+"\n⏰ "+twoDigitStr(clockHour)+":"+twoDigitStr(clockMin)+"\n🌅 Take your MORNING medicine!");
    notifyPatient("MORNING");updateBlynkDashboard();
  }
  if(clockHour==medTimes[clockDay][2]&&clockMin==medTimes[clockDay][3]&&clockSec==0&&!eveningDone){
    eveningDone=true;alarmRinging=true;alarmStartTime=millis();lastBuzzToggle=millis();
    buzzState=false;alarmType="EVENING";medicineTaken=false;familyAlertSent=false;patientAlertSent=false;
    eveningStatus="Alarm!";lcd.clear();
    telegramPatient("💊 MEDICINE REMINDER\n📅 "+String(clockDate)+" "+monthNames[clockMonth-1]+"\n⏰ "+twoDigitStr(clockHour)+":"+twoDigitStr(clockMin)+"\n🌙 Take your EVENING medicine!");
    notifyPatient("EVENING");updateBlynkDashboard();
  }
}

// ─── FSR Check (NEW LOGIC) ────────────────────────────────────────────────────
void checkFSR(){
  if(!alarmRinging) return;
  int fsrValue=analogRead(FSR_PIN);
  static unsigned long lastPrint=0;
  if(millis()-lastPrint>300){
    Serial.print("FSR: ");Serial.println(fsrValue);
    lastPrint=millis();
  }

  // ✅ NEW: FSR >= 2500 means medicine placed in box → stop alarm
  if(fsrValue >= 2500){
    String takenTime=twoDigitStr(clockHour)+":"+twoDigitStr(clockMin);
    alarmRinging=false;medicineTaken=true;
    ledcWrite(BUZZER_PIN,0);beepSuccess();
    if(alarmType=="MORNING"){morningTakenTime=takenTime;morningStatus="Taken";}
    else{eveningTakenTime=takenTime;eveningStatus="Taken";}
    addMedLog(alarmType,"Taken",takenTime);
    telegramBoth("✅ MEDICINE TAKEN!\n📅 "+String(clockDate)+" "+monthNames[clockMonth-1]+"\n💊 "+alarmType+" taken at "+takenTime+"\n👍 Great job!");
    notifyMedicineTaken(alarmType,takenTime);
    updateBlynkDashboard();
    lcd.clear();
    lcd.setCursor(0,0);lcd.print(" Medicine Taken!");
    lcd.setCursor(0,1);lcd.print("  at ");lcd.print(takenTime);lcd.print("  :) ");
    delay(2000);lcd.clear();
  }
}

void updateMedicineBuzz(){
  if(!alarmRinging) return;
  unsigned long now=millis();
  if(now-alarmStartTime>=MED_BUZZ_DURATION){
    alarmRinging=false;ledcWrite(BUZZER_PIN,0);
    if(!medicineTaken){
      if(alarmType=="MORNING"){morningStatus="Missed";morningTakenTime="--:--";}
      else{eveningStatus="Missed";eveningTakenTime="--:--";}
      addMedLog(alarmType,"Missed","--:--");
      telegramFamily("❌ MEDICINE MISSED!\n📅 "+String(clockDate)+" "+monthNames[clockMonth-1]+"\n💊 "+alarmType+" NOT taken!\n🙏 Please check on the patient!");
      telegramPatient("⚠️ MISSED MEDICINE!\n📅 "+String(clockDate)+" "+monthNames[clockMonth-1]+"\nYou missed your "+alarmType+" medicine!\nPlease take it now. 🙏");
      notifyFamily(alarmType);updateBlynkDashboard();
      lcd.clear();lcd.setCursor(0,0);lcd.print("  ALERT SENT!   ");lcd.setCursor(0,1);lcd.print("Family notified!");delay(3000);
    }
    lcd.clear();return;
  }
  // ✅ NEW: buzz when FSR = 0 (medicine not in box)
  int fsrNow = analogRead(FSR_PIN);
  if(fsrNow < 2500){
    if(now-lastBuzzToggle>=500){
      lastBuzzToggle=now;buzzState=!buzzState;
      if(buzzState){ledcWriteTone(BUZZER_PIN,2500);ledcWrite(BUZZER_PIN,255);}
      else{ledcWrite(BUZZER_PIN,0);}
    }
  } else {
    ledcWrite(BUZZER_PIN,0); // silent if medicine already placed
  }
}

void showMedicineAlarm(){
  unsigned long remaining=MED_BUZZ_DURATION-(millis()-alarmStartTime);
  int minsLeft=remaining/60000,secsLeft=(remaining%60000)/1000;
  static bool blinkState=false;
  static unsigned long lastBlink=0;
  if(millis()-lastBlink>600){blinkState=!blinkState;lastBlink=millis();}
  lcd.setCursor(0,0);
  if(blinkState){if(alarmType=="MORNING")lcd.print("MORNING MEDICINE");else lcd.print("EVENING MEDICINE");}
  else lcd.print("  TAKE YOUR MED!");
  lcd.setCursor(0,1);lcd.print("* off ");
  lcd.print(twoDigitStr(minsLeft));lcd.print(":");lcd.print(twoDigitStr(secsLeft));
  lcd.print(" F:");lcd.print(analogRead(FSR_PIN));lcd.print("   ");
}

void displayClock(){
  lcd.setCursor(0,0);
  lcd.print(dayNames[clockDay]);lcd.print(" ");
  lcd.print(twoDigitStr(clockHour));lcd.print(":");
  lcd.print(twoDigitStr(clockMin));lcd.print(":");
  lcd.print(twoDigitStr(clockSec));lcd.print(" ");
  lcd.setCursor(0,1);
  lcd.print("M:");lcd.print(twoDigitStr(medTimes[clockDay][0]));lcd.print(":");lcd.print(twoDigitStr(medTimes[clockDay][1]));
  lcd.print(" E:");lcd.print(twoDigitStr(medTimes[clockDay][2]));lcd.print(":");lcd.print(twoDigitStr(medTimes[clockDay][3]));
  lcd.print(" #=lk");
}

// ─── Med Handlers ─────────────────────────────────────────────────────────────
void handleMedVerify(char key){
  if(key=='*'){beepWarning();medVerifyFails=0;enteredPassword="";currentMode=MODE_CLOCK;lcd.clear();return;}
  if(key=='B'){if(enteredPassword.length()>0){beepBackspace();enteredPassword.remove(enteredPassword.length()-1);redrawLine("Password: ",getMasked(enteredPassword));}else beepWarning();return;}
  if(key=='#'){
    if(enteredPassword.length()==0){beepNoInput();return;}
    if(enteredPassword==PASSWORD){medVerifyFails=0;enteredPassword="";beepSuccess();medShowDay=0;currentMode=MODE_MED_SHOW;showMedTimesForDay(medShowDay);}
    else{medVerifyFails++;beepError();if(medVerifyFails>=MAX_ATTEMPTS){medVerifyFails=0;enteredPassword="";beepLocked();showMessage(" Access Denied! "," 3 fails. Back! ",2000);currentMode=MODE_CLOCK;lcd.clear();}else{showMessage(" Wrong Password!"," Tries left: "+String(MAX_ATTEMPTS-medVerifyFails),1500);enteredPassword="";lcd.clear();lcd.setCursor(0,0);lcd.print("Med Settings:   ");lcd.setCursor(0,1);lcd.print("Password: ");}}
    return;
  }
  if(key>='0'&&key<='9'){if(enteredPassword.length()<8){beepKey();enteredPassword+=key;redrawLine("Password: ",getMasked(enteredPassword));}else beepLimit();}
}
void handleMedShow(char key){
  if(key=='*'){beepWarning();currentMode=MODE_CLOCK;lcd.clear();return;}
  if(key=='A'){beepKey();medShowDay=(medShowDay+6)%7;showMedTimesForDay(medShowDay);return;}
  if(key=='B'){beepKey();medShowDay=(medShowDay+1)%7;showMedTimesForDay(medShowDay);return;}
  if(key=='#'){beepKey();medEditDay=medShowDay;medInput="";currentMode=MODE_MED_SELECT_DAY;lcd.clear();lcd.setCursor(0,0);lcd.print("Edit: ");lcd.print(dayNames[medEditDay]);lcd.setCursor(0,1);lcd.print("1=MOR  2=EVE *=back");return;}
}
void handleMedSelectType(char key){
  if(key=='*'){beepWarning();medShowDay=medEditDay;currentMode=MODE_MED_SHOW;showMedTimesForDay(medShowDay);return;}
  if(key=='1'){beepKey();medEditType=0;medInput="";currentMode=MODE_MED_SET_HOUR;lcd.clear();lcd.setCursor(0,0);lcd.print(dayNames[medEditDay]);lcd.print(" MOR Hr:");lcd.setCursor(0,1);lcd.print("(0-23) > ");return;}
  if(key=='2'){beepKey();medEditType=1;medInput="";currentMode=MODE_MED_SET_HOUR;lcd.clear();lcd.setCursor(0,0);lcd.print(dayNames[medEditDay]);lcd.print(" EVE Hr:");lcd.setCursor(0,1);lcd.print("(0-23) > ");return;}
}
void handleMedSetHour(char key){
  if(key=='*'){beepWarning();medInput="";currentMode=MODE_MED_SELECT_DAY;lcd.clear();lcd.setCursor(0,0);lcd.print("Edit: ");lcd.print(dayNames[medEditDay]);lcd.setCursor(0,1);lcd.print("1=MOR  2=EVE *=back");return;}
  if(key=='B'){if(medInput.length()>0){beepBackspace();medInput.remove(medInput.length()-1);redrawLine("(0-23) > ",medInput);}else beepWarning();return;}
  if(key=='#'){
    if(medInput.length()==0){beepNoInput();return;}
    int val=medInput.toInt();
    if(val<0||val>23){beepError();showMessage(" Invalid! 0-23  ","  Try again     ",1000);medInput="";redrawLine("(0-23) > ","");return;}
    if(medEditType==0)medTimes[medEditDay][0]=val;else medTimes[medEditDay][2]=val;
    beepKey();medInput="";currentMode=MODE_MED_SET_MIN;lcd.clear();
    lcd.setCursor(0,0);lcd.print(dayNames[medEditDay]);lcd.print(medEditType==0?" MOR Min:":" EVE Min:");
    lcd.setCursor(0,1);lcd.print("(0-59) > ");return;
  }
  if(key>='0'&&key<='9'){if(medInput.length()<2){beepKey();medInput+=key;redrawLine("(0-23) > ",medInput);}else beepLimit();}
}
void handleMedSetMin(char key){
  if(key=='*'){beepWarning();medInput="";currentMode=MODE_MED_SET_HOUR;lcd.clear();lcd.setCursor(0,0);lcd.print(dayNames[medEditDay]);lcd.print(medEditType==0?" MOR Hr:":" EVE Hr:");lcd.setCursor(0,1);lcd.print("(0-23) > ");return;}
  if(key=='B'){if(medInput.length()>0){beepBackspace();medInput.remove(medInput.length()-1);redrawLine("(0-59) > ",medInput);}else beepWarning();return;}
  if(key=='#'){
    if(medInput.length()==0){beepNoInput();return;}
    int val=medInput.toInt();
    if(val<0||val>59){beepError();showMessage(" Invalid! 0-59  ","  Try again     ",1000);medInput="";redrawLine("(0-59) > ","");return;}
    if(medEditType==0)medTimes[medEditDay][1]=val;else medTimes[medEditDay][3]=val;
    beepChanged();lcd.clear();
    lcd.setCursor(0,0);lcd.print(dayNames[medEditDay]);lcd.print(medEditType==0?" MOR:":" EVE:");
    lcd.setCursor(0,1);
    if(medEditType==0){lcd.print(twoDigitStr(medTimes[medEditDay][0]));lcd.print(":");lcd.print(twoDigitStr(medTimes[medEditDay][1]));}
    else{lcd.print(twoDigitStr(medTimes[medEditDay][2]));lcd.print(":");lcd.print(twoDigitStr(medTimes[medEditDay][3]));}
    lcd.print(" Saved!");delay(2000);
    medInput="";medShowDay=medEditDay;currentMode=MODE_MED_SHOW;showMedTimesForDay(medShowDay);return;
  }
  if(key>='0'&&key<='9'){if(medInput.length()<2){beepKey();medInput+=key;redrawLine("(0-59) > ",medInput);}else beepLimit();}
}
void handleNormalMode(char key){
  if(key=='*'){beepWarning();enteredPassword="";currentMode=MODE_CLOCK;lcd.clear();return;}
  else if(key=='#'){
    if(enteredPassword.length()==0){beepNoInput();showMessage(" No Input Found!","  Type password ",1000);resetNormal();return;}
    beepKey();delay(100);
    if(enteredPassword==PASSWORD){failedAttempts=0;beepSuccess();lcd.clear();lcd.setCursor(0,0);lcd.print(" Access Granted!");lcd.setCursor(0,1);lcd.print("  Welcome!! :)  ");delay(3000);currentMode=MODE_CLOCK;lcd.clear();}
    else{failedAttempts++;beepError();if(failedAttempts>=MAX_ATTEMPTS){isLocked=true;lockStartTime=millis();beepLocked();lcd.clear();lcd.setCursor(0,0);lcd.print(" SYSTEM  LOCKED!");lcd.setCursor(0,1);lcd.print(" Too many tries ");delay(800);}else{lcd.clear();lcd.setCursor(0,0);lcd.print(" Access Denied! ");lcd.setCursor(0,1);lcd.print(" Tries left: ");lcd.print(MAX_ATTEMPTS-failedAttempts);delay(2000);resetNormal();showTriesOnRow0();}}
  }
  else if(key=='B'){if(enteredPassword.length()>0){beepBackspace();enteredPassword.remove(enteredPassword.length()-1);redrawLine("> ",getMasked(enteredPassword));}else beepWarning();}
  else if(key=='C'){beepKey();enteredPassword="";redrawLine("> ","");}
  else if(key=='A'){beepKey();showMessage("Tries Remaining:","      "+String(MAX_ATTEMPTS-failedAttempts)+" left      ",1500);resetNormal();showTriesOnRow0();}
  else if(key=='D'){beepKey();currentMode=MODE_CHANGE_VERIFY;enteredPassword="";showMessage(" Change Password","Old Pass:       ");lcd.setCursor(0,1);lcd.print("Old Pass: ");}
  else{if(enteredPassword.length()<8){beepKey();enteredPassword+=key;redrawLine("> ",getMasked(enteredPassword));}else beepLimit();}
}
void handleChangeVerify(char key){
  if(key=='*'){beepWarning();currentMode=MODE_CLOCK;lcd.clear();return;}
  if(key=='#'){if(enteredPassword==PASSWORD){beepKey();currentMode=MODE_CHANGE_NEW;enteredPassword="";showMessage(" Change Password","New Pass:       ");lcd.setCursor(0,1);lcd.print("New Pass: ");}else{beepError();showMessage(" Wrong Password!"," Try Again      ",1500);enteredPassword="";showMessage(" Change Password","Old Pass:       ");lcd.setCursor(0,1);lcd.print("Old Pass: ");}}
  else if(key=='B'){if(enteredPassword.length()>0){beepBackspace();enteredPassword.remove(enteredPassword.length()-1);redrawLine("Old Pass: ",getMasked(enteredPassword));}else beepWarning();}
  else if(key>='0'&&key<='9'){if(enteredPassword.length()<8){beepKey();enteredPassword+=key;redrawLine("Old Pass: ",getMasked(enteredPassword));}else beepLimit();}
}
void handleChangeNew(char key){
  if(key=='*'){beepWarning();currentMode=MODE_CLOCK;lcd.clear();return;}
  if(key=='#'){if(enteredPassword.length()==0){beepNoInput();return;}beepKey();newPassword=enteredPassword;enteredPassword="";currentMode=MODE_CHANGE_CONFIRM;showMessage(" Confirm New    ","Confirm:        ");lcd.setCursor(0,1);lcd.print("Confirm: ");}
  else if(key=='B'){if(enteredPassword.length()>0){beepBackspace();enteredPassword.remove(enteredPassword.length()-1);redrawLine("New Pass: ",getMasked(enteredPassword));}else beepWarning();}
  else if(key>='0'&&key<='9'){if(enteredPassword.length()<8){beepKey();enteredPassword+=key;redrawLine("New Pass: ",getMasked(enteredPassword));}else beepLimit();}
}
void handleChangeConfirm(char key){
  if(key=='*'){beepWarning();currentMode=MODE_CLOCK;lcd.clear();return;}
  if(key=='#'){if(enteredPassword==newPassword){PASSWORD=newPassword;beepChanged();showMessage("Password Changed","  Successfully! ",2500);currentMode=MODE_CLOCK;lcd.clear();}else{beepError();showMessage("Passwords dont  ","match! Try again",2000);currentMode=MODE_CHANGE_NEW;enteredPassword="";newPassword="";showMessage(" Change Password","New Pass:       ");lcd.setCursor(0,1);lcd.print("New Pass: ");}}
  else if(key=='B'){if(enteredPassword.length()>0){beepBackspace();enteredPassword.remove(enteredPassword.length()-1);redrawLine("Confirm: ",getMasked(enteredPassword));}else beepWarning();}
  else if(key>='0'&&key<='9'){if(enteredPassword.length()<8){beepKey();enteredPassword+=key;redrawLine("Confirm: ",getMasked(enteredPassword));}else beepLimit();}
}

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup(){
  Serial.begin(115200);
  delay(500);
  Wire.begin(21,22);
  lcd.init();
  lcd.backlight();
  pinMode(FSR_PIN,INPUT);
  ledcAttach(BUZZER_PIN,2700,8);
  ledcWrite(BUZZER_PIN,0);
  keypad.setDebounceTime(50);
  keypad.setHoldTime(500);

  ledcWriteTone(BUZZER_PIN,2700);ledcWrite(BUZZER_PIN,255);delay(150);ledcWrite(BUZZER_PIN,0);delay(100);
  ledcWriteTone(BUZZER_PIN,2700);ledcWrite(BUZZER_PIN,255);delay(150);ledcWrite(BUZZER_PIN,0);

  scanAndConnect();

  if(wifiConnected){
    showMessage(" Getting Time.. ","  From Internet ",0);
    configTime(19800,0,"pool.ntp.org","time.nist.gov");
    struct tm timeinfo;
    int ntpTries=0;
    while(!getLocalTime(&timeinfo)&&ntpTries<20){delay(500);ntpTries++;Serial.print("t");}
    if(getLocalTime(&timeinfo)){
      clockHour=timeinfo.tm_hour;clockMin=timeinfo.tm_min;clockSec=timeinfo.tm_sec;
      clockDay=timeinfo.tm_wday;clockDate=timeinfo.tm_mday;
      clockMonth=timeinfo.tm_mon+1;clockYear=timeinfo.tm_year+1900;
      lastMillis=millis();clockSet=true;lastNTPSync=millis();
      Serial.println("\nTime: "+String(dayNames[clockDay])+" "+twoDigitStr(clockHour)+":"+twoDigitStr(clockMin)+":"+twoDigitStr(clockSec));
      lcd.clear();lcd.setCursor(0,0);lcd.print("  Time Synced!  ");
      lcd.setCursor(0,1);lcd.print(dayNames[clockDay]);lcd.print(" ");
      lcd.print(twoDigitStr(clockHour));lcd.print(":");lcd.print(twoDigitStr(clockMin));lcd.print(":");lcd.print(twoDigitStr(clockSec));
      delay(2000);
    } else {
      Serial.println("\nNTP failed!");
      showMessage(" Time Sync Fail!","  Check internet",2000);
    }
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(3000);
    showMessage("  Blynk Ready!  ","                ",1000);
    timer.setInterval(1000L,sendBlynkFSR);
    timer.setInterval(60000L,updateBlynkDashboard);
  }

  showMessage("  Medicine Box  "," v11 + Telegram ",1500);
  currentMode=MODE_CLOCK;
  lcd.clear();
  Serial.println("=== Ready! WiFi:"+String(wifiConnected?"YES":"NO")+" ===");
}

// ─── Loop ─────────────────────────────────────────────────────────────────────
void loop(){
  if(wifiConnected){Blynk.run();timer.run();}
  updateClock();
  checkMedicineAlarms();
  updateMedicineBuzz();
  checkFSR();

  if(isLocked&&!alarmRinging){
    unsigned long elapsed=millis()-lockStartTime;
    if(elapsed>=LOCK_DURATION){isLocked=false;failedAttempts=0;beepKey();currentMode=MODE_CLOCK;lcd.clear();}
    else{int s=(LOCK_DURATION-elapsed)/1000+1;lcd.setCursor(0,0);lcd.print(" SYSTEM  LOCKED ");lcd.setCursor(0,1);lcd.print("  Wait: ");lcd.print(s);lcd.print(" sec    ");delay(400);return;}
  }

  if(alarmRinging){
    static unsigned long lastDisplay=0;
    if(millis()-lastDisplay>300){showMedicineAlarm();lastDisplay=millis();}
    char key=keypad.getKey();
    if(key=='*'){
      String takenTime=twoDigitStr(clockHour)+":"+twoDigitStr(clockMin);
      alarmRinging=false;medicineTaken=true;ledcWrite(BUZZER_PIN,0);beepKey();
      if(alarmType=="MORNING"){morningTakenTime=takenTime;morningStatus="Taken";}
      else{eveningTakenTime=takenTime;eveningStatus="Taken";}
      addMedLog(alarmType,"Taken",takenTime);
      telegramBoth("✅ MEDICINE TAKEN!\n📅 "+String(clockDate)+" "+monthNames[clockMonth-1]+"\n💊 "+alarmType+" taken at "+takenTime+"\n👍 Great job!");
      notifyMedicineTaken(alarmType,takenTime);updateBlynkDashboard();
      showMessage("  Alarm Stopped "," Take Medicine! ",2000);lcd.clear();
    }
    return;
  }

  if(currentMode==MODE_CLOCK&&clockSet){
    static int lastSec=-1;
    if(clockSec!=lastSec){displayClock();lastSec=clockSec;}
  }

  if(!clockSet){
    lcd.setCursor(0,0);lcd.print(" Waiting for    ");
    lcd.setCursor(0,1);lcd.print(" WiFi + Time... ");
    return;
  }

  char key=keypad.getKey();
  if(!key) return;

  switch(currentMode){
    case MODE_CLOCK:
      if(key=='#'){beepKey();resetNormal();}
      else if(key=='A'){beepKey();medVerifyFails=0;enteredPassword="";currentMode=MODE_MED_VERIFY;lcd.clear();lcd.setCursor(0,0);lcd.print("Med Settings:   ");lcd.setCursor(0,1);lcd.print("Password: ");}
      break;
    case MODE_NORMAL:         handleNormalMode(key);    break;
    case MODE_CHANGE_VERIFY:  handleChangeVerify(key);  break;
    case MODE_CHANGE_NEW:     handleChangeNew(key);     break;
    case MODE_CHANGE_CONFIRM: handleChangeConfirm(key); break;
    case MODE_MED_VERIFY:     handleMedVerify(key);     break;
    case MODE_MED_SHOW:       handleMedShow(key);       break;
    case MODE_MED_SELECT_DAY: handleMedSelectType(key); break;
    case MODE_MED_SET_HOUR:   handleMedSetHour(key);    break;
    case MODE_MED_SET_MIN:    handleMedSetMin(key);     break;
  }
}
