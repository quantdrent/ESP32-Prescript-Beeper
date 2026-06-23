#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define LIGHT_PIN 8

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
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
body{background:#000;color:#99edff;font-family:Arial;text-align:center;margin-top:50px;}
input{background:transparent;border:2px solid #99edff;color:#99edff;padding:15px;font-size:16px;width:80%;max-width:300px;}
button{margin-top:20px;padding:10px 20px;background:transparent;border:2px solid #99edff;color:#99edff;}
#status{margin-top:20px;}
</style>
</head>
<body>
<h2>Prescript Sender</h2>
<input type="text" id="cmdInput" placeholder="Type text"><br>
<button onclick="sendData()">Send</button>
<p id="status"></p>
<script>
function sendData(){
 let txt=document.getElementById("cmdInput").value.trim();
 if(!txt){document.getElementById("status").innerText="Enter text";return;}
 fetch('/send?val='+encodeURIComponent(txt))
 .then(r=>r.text())
 .then(data=>{
   document.getElementById("status").innerText="Morse:\\n"+data;
   document.getElementById("cmdInput").value="";
 });
}
</script>
</body>
</html>
)rawliteral";

struct MorseCode {
  char letter;
  const char* code;
};

const MorseCode morseTable[] = {
{'A',".-"},{'B',"-..."},{'C',"-.-."},{'D',"-.."},{'E',"."},
{'F',"..-."},{'G',"--."},{'H',"...."},{'I',".."},{'J',".---"},
{'K',"-.-"},{'L',".-.."},{'M',"--"},{'N',"-."},{'O',"---"},
{'P',".--."},{'Q',"--.-"},{'R',".-."},{'S',"..."},{'T',"-"},
{'U',"..-"},{'V',"...-"},{'W',".--"},{'X',"-..-"},{'Y',"-.--"},
{'Z',"--.."},{'0',"-----"},{'1',".----"},{'2',"..---"},
{'3',"...--"},{'4',"....-"},{'5',"....."},{'6',"-...."},
{'7',"--..."},{'8',"---.."},{'9',"----."}
};

const int DOT_TIME = 100;

const char* getMorse(char c){
 c=toupper(c);
 for(const auto &m:morseTable) if(m.letter==c) return m.code;
 return nullptr;
}

String convertToMorse(String text){
 String result="";
 text.toUpperCase();
 for(int i=0;i<text.length();i++){
   if(text[i]==' '){ result+=" / "; continue; }
   const char* code=getMorse(text[i]);
   if(code){ result+=code; result+=" "; }
 }
 return result;
}

void showMessage(String text){
 int len=text.length();

 for(int frame=0; frame<40; frame++){
   String out="";
   for(int i=0;i<len;i++) out+=randomChar();

   display.clearDisplay();
   display.setCursor(0,0);
   display.println("Received:");
   display.setCursor(0,16);
   display.print("_"); display.print(out); display.print("_");
   display.display();
   delay(35);
 }

 for(int reveal=0; reveal<=len; reveal++){
   for(int anim=0; anim<4; anim++){
     String out="";
     for(int i=0;i<len;i++){
       if(i<reveal) out+=text[i];
       else out+=randomChar();
     }

     display.clearDisplay();
     display.setCursor(0,0);
     display.println("Received:");
     display.setCursor(0,16);
     display.print("_"); display.print(out); display.print("_");
     display.display();
     delay(25);
   }
 }

 display.clearDisplay();
 display.setCursor(0,0);
 display.println("Received:");
 display.setCursor(0,16);
 display.print("_"); display.print(text); display.print("_");
 display.display();

 oledTimeout=millis()+5000;
 oledActive=true;
}

void ledOn(){ digitalWrite(LIGHT_PIN, LOW); }
void ledOff(){ digitalWrite(LIGHT_PIN, HIGH); }

void playMorse(String text){
 text.toUpperCase();
 for(int i=0;i<text.length();i++){
   char c=text[i];
   if(c==' '){ delay(DOT_TIME*7); continue; }
   const char* code=getMorse(c);
   if(!code) continue;

   for(int j=0; code[j]!='\0'; j++){
     ledOn();
     delay(code[j]=='.' ? DOT_TIME : DOT_TIME*3);
     ledOff();
     delay(DOT_TIME);
   }
   delay(DOT_TIME*2);
 }
}

void handleRoot(){
 server.send(200,"text/html",index_html);
}

void handleSend(){
 if(!server.hasArg("val")){
   server.send(400,"text/plain","Missing");
   return;
 }

 String msg=server.arg("val");
 String morse=convertToMorse(msg);

 showMessage(msg);
 playMorse(msg);

 server.send(200,"text/plain",morse);
}

void setup(){
 pinMode(LIGHT_PIN,OUTPUT);
 digitalWrite(LIGHT_PIN,HIGH);

 Wire.begin(3,4);
 randomSeed(micros());

 display.begin(SSD1306_SWITCHCAPVCC,0x3C);
 display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(SSD1306_WHITE);
 display.setCursor(0,0);
 display.println("_Standby._");
 display.display();

 WiFi.mode(WIFI_AP);
 WiFi.softAP("LarpMachine");

 server.on("/",handleRoot);
 server.on("/send",handleSend);
 server.begin();
}

void loop(){
 server.handleClient();

 if(oledActive && millis()>oledTimeout){
   display.clearDisplay();
   display.display();
   oledActive=false;
 }
}
