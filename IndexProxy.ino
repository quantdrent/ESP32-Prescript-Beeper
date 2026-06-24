#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);

unsigned long oledTimeout = 0;
bool oledActive = false;

const char scrambleChars[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"0123456789"
"!@#$%^&*";

char randomChar() {
  int len = strlen(scrambleChars);
  return scrambleChars[random(len)];
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{
  background:#000;
  color:#99edff;
  font-family:Arial;
  text-align:center;
  margin-top:50px;
}
input{
  background:transparent;
  border:2px solid #99edff;
  color:#99edff;
  padding:15px;
  font-size:16px;
  width:80%;
  max-width:300px;
}
button{
  margin-top:20px;
  padding:10px 20px;
  background:transparent;
  border:2px solid #99edff;
  color:#99edff;
  cursor:pointer;
}
#status{
  margin-top:20px;
}
</style>
</head>
<body>

<h2>Prescript Sender</h2>

<input type="text" id="cmdInput" placeholder="Type text"><br>

<button onclick="sendData()">Send</button>

<p id="status"></p>

<script>
function sendData(){

  let txt = document.getElementById("cmdInput").value.trim();

  if(!txt){
    document.getElementById("status").innerText = "Enter text";
    return;
  }

  fetch('/send?val=' + encodeURIComponent(txt))
  .then(r => r.text())
  .then(data => {
      document.getElementById("status").innerText = "Sent!";
      document.getElementById("cmdInput").value = "";
  });
}
</script>

</body>
</html>
)rawliteral";

void drawCenteredText(String text) {

  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int x = (SCREEN_WIDTH - w) / 2;
  int y = (SCREEN_HEIGHT - h) / 2;

  if (x < 0) x = 0;

  display.setCursor(x, y);
  display.print(text);

  display.display();
}

void showMessage(String text) {

  int len = text.length();

  // Scramble animation
  for (int frame = 0; frame < 25; frame++) {

    String out = "";

    for (int i = 0; i < len; i++) {
      out += randomChar();
    }

    drawCenteredText(out);

    delay(40);
  }

  // Reveal animation
  for (int reveal = 0; reveal <= len; reveal++) {

    String out = "";

    for (int i = 0; i < len; i++) {

      if (i < reveal)
        out += text[i];
      else
        out += randomChar();
    }

    drawCenteredText(out);

    delay(50);
  }

  drawCenteredText(text);

  oledTimeout = millis() + 5000;
  oledActive = true;
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleSend() {

  if (!server.hasArg("val")) {
    server.send(400, "text/plain", "Missing");
    return;
  }

  String msg = server.arg("val");

  showMessage(msg);

  server.send(200, "text/plain", "OK");
}

void setup() {

  Wire.begin(3, 4);   // SDA=3, SCL=4 (ESP32-C3)

  randomSeed(micros());

  // SH1106 initialization
  display.begin(0x3C, true);

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  drawCenteredText("Standby");

  WiFi.mode(WIFI_AP);
  WiFi.softAP("LarpMachine");

  server.on("/", handleRoot);
  server.on("/send", handleSend);

  server.begin();
}

void loop() {

  server.handleClient();

  if (oledActive && millis() > oledTimeout) {

    display.clearDisplay();
    display.display();

    oledActive = false;
  }
}
