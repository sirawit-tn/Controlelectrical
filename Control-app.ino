#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>
#include <MicroGear.h>
#include <time.h>


// -------- Library Initial --------
WiFiClient client;
Scheduler TaskRunner;
MicroGear microgear(client);
// ---------------------------------


// ---------- Config Time -----------
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
int timezone = 7;
char ntp_server1[20] = "ntp.ku.ac.th";
char ntp_server2[20] = "fw.eng.ku.ac.th";
char ntp_server3[20] = "time.uni.net.th";

String getTime() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);
  String tmpNow = "";
  tmpNow += String(newtime->tm_year + 1900);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mon + 1);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mday);
  tmpNow += " ";
  tmpNow += String(newtime->tm_hour);
  tmpNow += ":";
  tmpNow += String(newtime->tm_min);
  tmpNow += ":";
  tmpNow += String(newtime->tm_sec);
  return tmpNow;
}

String getTimeNow() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);
  String tmpNow = ":";
  tmpNow += String(newtime->tm_hour);
  tmpNow += ":";
  tmpNow += String(newtime->tm_min);
  return tmpNow;
}
// ---------------------------------


// ---------------- Wifi Variable ----------------
const char* ssid     = "iPhone_T";
const char* password = "028463215";
String ipAddr = "";
String RMUTi_Username = "";
String RMUTi_Password = "";
//------------------------------------------------


// ---------------- Program ----------------
char *timeData;
int timeOpenDelay[][2] = {{ -1, -1}, { -1, -1}, { -1, -1}, { -1, -1}}; // HH mm
int timeCloseDelay[][2] = {{ -1, -1}, { -1, -1}, { -1, -1}, { -1, -1}}; // HH mm
int timeNowData[] = { -1, -1};
bool soleStatus = false;

void dataFormMicrogear(String topic, char* message) {
  int countTime = 0;
  // Serial.println("Data : " + topic + " " + message);
  if (topic == "open1") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeOpenDelay[0][countTime++] = ((String)timeData).toInt();
    }
  } else if (topic == "open2") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeOpenDelay[1][countTime++] = ((String)timeData).toInt();
    }
  } else if (topic == "open3") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeOpenDelay[2][countTime++] = ((String)timeData).toInt();
    }
  } else if (topic == "open4") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeOpenDelay[3][countTime++] = ((String)timeData).toInt();
    }
  } else if (topic == "close1") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeCloseDelay[0][countTime++] = ((String)timeData).toInt();
    }
  } else if (topic == "close2") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeCloseDelay[1][countTime++] = ((String)timeData).toInt();
    }
  } else if (topic == "close3") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeCloseDelay[2][countTime++] = ((String)timeData).toInt();
    }
  } else if (topic == "close4") {
    while ((timeData = strtok_r(message, ":", &message)) != NULL) {
      timeCloseDelay[3][countTime++] = ((String)timeData).toInt();
    }
  }

  //  for (int ii = 0; ii < 4; ii++) {
  //    Serial.print("Open " + (String)(ii + 1));
  //    for (int jj = 0; jj < 2; jj++) {
  //      Serial.print(" " + (String)timeOpenDelay[ii][jj] + " ");
  //    }
  //    Serial.println();
  //  }
  //
  //  for (int ii = 0; ii < 4; ii++) {
  //    Serial.print("Close " + (String)(ii + 1));
  //    for (int jj = 0; jj < 2; jj++) {
  //      Serial.print(" " + (String)timeCloseDelay[ii][jj] + " ");
  //    }
  //    Serial.println();
  //  }

}
// -----------------------------------------


// -------------- Microgear Initial -------------
#define APPID   "IONICAPP"
#define KEY     "pRMJEX71lwN4qWm"
#define SECRET  "vIxtO8p2H4rb9wn96WfAbktxg"
#define ALIAS   "esp8266"
int countTopic = 0;
char *mTopic;
String mMessage;

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  msg[msglen] = '\0';
  mMessage = (String)(char *)msg;
  while ((mTopic = strtok_r(topic, "/", &topic)) != NULL) {
    if (++countTopic == 3) {
      countTopic = 0;
      break;
    }
  }
  dataFormMicrogear((String)mTopic, (char *)msg);
  Serial.println((String)mTopic + " --> " + mMessage);
  if ((String)mTopic == "pump") {
    soleStatus = mMessage == "true";
    //    microgear.publish("/node/Solenoid", (String)(mMessage == "true"));
    //    digitalWrite(D0, mMessage == "true" ? LOW : HIGH);
  }
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.subscribe("/app/+");
  microgear.setAlias(ALIAS);
}
// -----------------------------------------------


// ------------ TaskScheduler Initial ------------
void microgearLoop();
void solenoidWork();

Task microgearTask(100, TASK_FOREVER, &microgearLoop, &TaskRunner, true);
Task solenoidTask(1000, TASK_FOREVER, &solenoidWork, &TaskRunner, true);

void microgearLoop() {
  if (microgear.connected()) {
    microgear.loop();
  }
  else {
    Serial.println(connectRMUTi());
    Serial.println("connection lost, reconnect...");
    microgear.connect(APPID);
    delay(2000);
  }
}

void solenoidWork() {
  char *timeNowChar;
  char sz[20];
  int indexTime = 0;
  getTimeNow().toCharArray(sz, getTimeNow().length() + 1);
  sz[getTimeNow().length() + 1] = '\0';
  char *p = sz;

  while ((timeNowChar = strtok_r(p, ":", &p)) != NULL) {
    timeNowData[indexTime++] = ((String)timeNowChar).toInt();
  }

  //  Serial.println("HH : " + (String)timeNowData[0] + " mm :" + (String)timeNowData[1]);

  // Open
  if (timeOpenDelay[0][0] == timeNowData[0] && timeOpenDelay[0][1] == timeNowData[1]) {
    // Open 1
    soleStatus = soleStatus == true ? true : true;
  } else if (timeOpenDelay[1][0] == timeNowData[0] && timeOpenDelay[1][1] == timeNowData[1]) {
    // Open 2
    soleStatus = soleStatus == true ? true : true;
  } else if (timeOpenDelay[2][0] == timeNowData[0] && timeOpenDelay[2][1] == timeNowData[1]) {
    // Open 3
    soleStatus = soleStatus == true ? true : true;
  } else if (timeOpenDelay[3][0] == timeNowData[0] && timeOpenDelay[3][1] == timeNowData[1]) {
    // Open 4
    soleStatus = soleStatus == true ? true : true;
  }

  // Close
  if (timeCloseDelay[0][0] == timeNowData[0] && timeCloseDelay[0][1] == timeNowData[1]) {
    // Close 1
    soleStatus = soleStatus == true ? false : false;
  } else if (timeCloseDelay[1][0] == timeNowData[0] && timeCloseDelay[1][1] == timeNowData[1]) {
    // Close 2
    soleStatus = soleStatus == true ? false : false;
  } else if (timeCloseDelay[2][0] == timeNowData[0] && timeCloseDelay[2][1] == timeNowData[1]) {
    // Close 3
    soleStatus = soleStatus == true ? false : false;
  } else if (timeCloseDelay[3][0] == timeNowData[0] && timeCloseDelay[3][1] == timeNowData[1]) {
    // Close 4
    soleStatus = soleStatus == true ? false : false;
  }

  Serial.println("Solenoid Status : " + (String)soleStatus);
  microgear.publish("/node/Solenoid", (String)soleStatus);
  digitalWrite(D0, soleStatus == true ? LOW : HIGH);
}
// -----------------------------------------------


// ------------- WiFi RMUTI --------------
bool connectRMUTi() {
  HTTPClient http;
  if (http.begin(client, "http://afw-krc.rmuti.ac.th/login.php")) {
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String post = "_u=" + RMUTi_Username + "&_p=" + RMUTi_Password + "&a=a&web_host=afw-krc.rmuti.ac.th&web_host4=afw4-krc.rmuti.ac.th&_ip4=" + ipAddr + "&web_host6=afw6-krc.rmuti.ac.th&_ip6=";
    int httpCode = http.POST(post);
    String payload = http.getString();
    Serial.println(payload);
    Serial.println(httpCode);
    http.end();
    return true;
  }
  return false;
}
// ---------------------------------------


void setup() {
  Serial.begin(115200);
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);
  if (WiFi.begin(ssid, password)) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
  ipAddr = WiFi.localIP().toString();
  Serial.println();
  Serial.println(ipAddr);
  while (!connectRMUTi());
  delay(1000);
  Serial.println("Connect Success");
  configTime(7 * 3600, 0, ntp_server1, ntp_server2, ntp_server3);
  while (!time(nullptr)) {
    Serial.print(".");
    delay(500);
  }
  delay(1000);
  Serial.println("Now: " + getTime());
  microgear.on(MESSAGE, onMsghandler);
  microgear.on(CONNECTED, onConnected);
  microgear.init(KEY, SECRET, ALIAS);
  microgear.connect(APPID);

  TaskRunner.startNow();
}


void loop() {
  TaskRunner.execute();
}
