/*
  Mini MP3 Player + LCD + MQTT
*/

#include 
#include 
#include 
#include 
#include 

// ===== Settings =====
const char* WIFI_SSID = "WHAT";
const char* WIFI_PASS = "password";
const char* MQTT_HOST = "broker.hivemq.com";
const int   MQTT_PORT = 1883;
const char* BASE      = "mini-mp3-7421";   // must match the web page

// Song names, in the same order as the files 0001.mp3, 0002.mp3, ...
const int NUM_SONGS = 3;
const char* songNames[NUM_SONGS] = {"Song 1", "Song 2", "Song 3"};
// ====================

const int BTN_PREV = 2;
const int BTN_PLAY = 3;
const int BTN_NEXT = 4;
const int POT_PIN  = A0;
const int PIN_LED_RED = 5, PIN_LED_BLUE = 6, PIN_LED_YELLOW = 9;
const int btnPins[3] = {BTN_PREV, BTN_PLAY, BTN_NEXT};

LiquidCrystal_I2C lcd(0x27, 16, 2);   // try 0x3F if the screen stays blank
DFRobotDFPlayerMini player;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char tCmd[40], tStatus[40], tPlaylist[40], tOnline[40];

int currentSong = 0;       // 0-based
bool playing = false;
bool started = false;
int volume = 50;           // 0-100 (shown on LCD)
int eqMode = 0;            // 0=Normal 1=Pop 2=Rock 3=Jazz 4=Classic 5=Bass
bool lastState[3] = {HIGH, HIGH, HIGH};
unsigned long lastPress[3] = {0, 0, 0};
unsigned long lastVolCheck = 0;
unsigned long lastMqttTry = 0;

// ---------- Status LEDs (only one is on at a time) ----------
void setLEDs(bool red, bool blue, bool yellow) {
  digitalWrite(PIN_LED_RED, red ? HIGH : LOW);
  digitalWrite(PIN_LED_BLUE, blue ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
}

void updateLED() {
  if (WiFi.status() != WL_CONNECTED || !mqtt.connected()) setLEDs(1, 0, 0);  // red: not connected
  else if (playing) setLEDs(0, 1, 0);                                        // blue: playing
  else setLEDs(0, 0, 1);                                                     // yellow: paused
}

// ---------- MQTT publish ----------
void publishStatus() {
  if (!mqtt.connected()) return;
  char msg[160];
  snprintf(msg, sizeof(msg),
           "{\"song\":%d,\"name\":\"%s\",\"playing\":%s,\"volume\":%d,\"eq\":%d}",
           currentSong + 1, songNames[currentSong], playing ? "true" : "false", volume, eqMode);
  mqtt.publish(tStatus, msg, true);   // retained
}

void publishPlaylist() {
  char msg[300] = "{\"songs\":[";
  for (int i = 0; i < NUM_SONGS; i++) {
    if (i > 0) strncat(msg, ",", sizeof(msg) - strlen(msg) - 1);
    strncat(msg, "\"", sizeof(msg) - strlen(msg) - 1);
    strncat(msg, songNames[i], sizeof(msg) - strlen(msg) - 1);
    strncat(msg, "\"", sizeof(msg) - strlen(msg) - 1);
  }
  strncat(msg, "]}", sizeof(msg) - strlen(msg) - 1);
  mqtt.publish(tPlaylist, msg, true); // retained
}

// ---------- Player + LCD ----------
void setVolume(int percent) {
  player.volume(map(percent, 0, 100, 0, 30));
}

void setEQ(int mode) {
  eqMode = constrain(mode, 0, 5);
  player.EQ(eqMode);
  Serial.print("EQ ");
  Serial.println(eqMode);
  publishStatus();
}

void showVolume() {
  lcd.setCursor(9, 1);
  lcd.print("Vol:    ");
  lcd.setCursor(13, 1);
  lcd.print(volume);
}

void showScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(songNames[currentSong]);
  lcd.setCursor(0, 1);
  lcd.print(playing ? "Playing" : "Paused");
  showVolume();
}

void playSong(int index) {
  currentSong = (index + NUM_SONGS) % NUM_SONGS;
  player.play(currentSong + 1);
  playing = true;
  started = true;
  Serial.print("Playing song ");
  Serial.println(currentSong + 1);
  showScreen();
  publishStatus();
}

void togglePlay() {
  if (!started) {
    playSong(currentSong);
    return;
  }
  if (playing) {
    player.pause();
    playing = false;
    Serial.println("Paused");
  } else {
    player.start();
    playing = true;
    Serial.println("Resumed");
  }
  showScreen();
  publishStatus();
}

// ---------- MQTT receive ----------
void onMessage(char* topic, byte* payload, unsigned int len) {
  char cmd[16];
  len = min(len, (unsigned int)(sizeof(cmd) - 1));
  memcpy(cmd, payload, len);
  cmd[len] = '\0';
  Serial.print("MQTT cmd: ");
  Serial.println(cmd);

  if (strcmp(cmd, "toggle") == 0) togglePlay();
  else if (strcmp(cmd, "prev") == 0) playSong(currentSong - 1);
  else if (strcmp(cmd, "next") == 0) playSong(currentSong + 1);
  else if (strncmp(cmd, "play:", 5) == 0) {
    int n = atoi(cmd + 5);
    if (n >= 1 && n <= NUM_SONGS) playSong(n - 1);
  }
  else if (strncmp(cmd, "eq:", 3) == 0) {
    int n = atoi(cmd + 3);
    if (n >= 0 && n <= 5) setEQ(n);
  }
}

// ---------- Connection (non-blocking) ----------
void ensureConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastMqttTry < 5000) return;
    lastMqttTry = millis();
    Serial.println("Connecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    return;
  }
  if (!mqtt.connected()) {
    if (millis() - lastMqttTry < 5000) return;
    lastMqttTry = millis();
    String id = "uno-r4-" + String(random(0xffff), HEX);
    Serial.println("Connecting MQTT...");
    // last will: publish "0" (retained) to BASE/online if we drop off
    if (mqtt.connect(id.c_str(), NULL, NULL, tOnline, 0, true, "0")) {
      Serial.println("MQTT OK");
      mqtt.publish(tOnline, "1", true);
      mqtt.subscribe(tCmd);
      publishPlaylist();
      publishStatus();
    } else {
      Serial.print("MQTT failed, rc=");
      Serial.println(mqtt.state());
    }
  }
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) pinMode(btnPins[i], INPUT_PULLUP);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  setLEDs(1, 0, 0);                     // red until connected

  snprintf(tCmd, sizeof(tCmd), "%s/cmd", BASE);
  snprintf(tStatus, sizeof(tStatus), "%s/status", BASE);
  snprintf(tPlaylist, sizeof(tPlaylist), "%s/playlist", BASE);
  snprintf(tOnline, sizeof(tOnline), "%s/online", BASE);

  lcd.init();
  lcd.backlight();
  lcd.print("Mini MP3 Player");

  Serial1.begin(9600);
  delay(1000);
  Serial.println("Starting DFPlayer...");
  if (!player.begin(Serial1)) {
    Serial.println("DFPlayer ERROR");
    lcd.clear();
    lcd.print("DFPlayer error");
    lcd.setCursor(0, 1);
    lcd.print("Check wiring/SD");
    setLEDs(1, 0, 0);                   // red = error
    while (true) delay(1000);
  }
  Serial.println("DFPlayer OK");
  setVolume(volume);
  player.EQ(eqMode);                    // start with Normal EQ
  showScreen();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);
}

void loop() {
  ensureConnections();
  mqtt.loop();

  // Buttons
  for (int i = 0; i < 3; i++) {
    bool state = digitalRead(btnPins[i]);
    if (lastState[i] == HIGH && state == LOW && millis() - lastPress[i] > 200) {
      lastPress[i] = millis();
      if (i == 0) playSong(currentSong - 1);
      else if (i == 1) togglePlay();
      else playSong(currentSong + 1);
    }
    lastState[i] = state;
  }

  // Volume from potentiometer (0-100)
  if (millis() - lastVolCheck > 150) {
    lastVolCheck = millis();
    int newVol = map(analogRead(POT_PIN), 0, 1023, 0, 100);
    if (abs(newVol - volume) >= 2) {
      volume = newVol;
      setVolume(volume);
      showVolume();
      publishStatus();
    }
  }

  // Auto-advance when a song ends
  if (player.available()) {
    if (player.readType() == DFPlayerPlayFinished) {
      playSong(currentSong + 1);
    }
  }

  updateLED();
}
