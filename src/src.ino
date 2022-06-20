#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string.h>

TFT_eSPI tft = TFT_eSPI();

char* USER = "wox"; //Change this for a different user
const uint16_t RESPONSE_TIMEOUT = 6000;
const uint16_t IN_BUFFER_SIZE = 4096; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 4096; //size of buffer to hold HTTP response
const uint16_t JSON_BODY_SIZE = 4096;
char request_buffer[IN_BUFFER_SIZE];
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char json_body[JSON_BODY_SIZE];


//STATE MACHINE VARIABLES----------------------------------------------
const uint8_t BUTTON45 = 45;
const uint8_t BUTTON39 = 39;
bool user_answer = 0; //This variable indicates whether the user answers true or false
bool just_answered = 0; //This variable keeps track of whether the question has been answered or not
const int IDLE = 0;
const int PRESS = 1;
const int RELEASE = 2;
int state_39 = IDLE;
int state_45 = IDLE;

/* Global variables*/
uint8_t button_state; //used for containing button state and detecting edges
char question_copy[4096];
WiFiClientSecure client; //global WiFiClient Secure object
WiFiClient client2; //global WiFiClient Secure object

DynamicJsonDocument doc(4096);
char json[4096];
char user_score[100];

//WIFI-----------------------------------------------------------------------
const char NETWORK[] = "MIT";
const char PASSWORD[] = "";
uint8_t channel = 1; //network channel on 2.4 GHz
//byte bssid[] = {0x5C, 0x5B, 0x35, 0xEF, 0x59, 0xC3}; //6 byte MAC address of AP you're targeting. Next House 5 west
byte bssid[] = {0x5C, 0x5B, 0x35, 0xEF, 0x59, 0x03}; //3C
//byte bssid[] = {0xD4, 0x20, 0xB0, 0xC4, 0x9C, 0xA3}; //quiet side stud

uint32_t timer;

void setup() {
  Serial.begin(115200);               // Set up serial port

  //SET UP SCREEN:
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size, change if you want
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_PINK, TFT_BLACK); //set color of font to hot pink foreground, black background

  //SET UP BUTTON:
  delay(100); //wait a bit (100 ms)
  pinMode(BUTTON45, INPUT_PULLUP);
  pinMode(BUTTON39, INPUT_PULLUP);


  //if using regular connection use line below:
  //WiFi.begin(NETWORK, PASSWORD);
  //if using channel/mac specification for crowded bands use the following:
  WiFi.begin(NETWORK, PASSWORD, 1, bssid);

  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(NETWORK);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  timer = millis();
}

/* yield_question-----------------------------------------------
This function interacts with the user by spitting out the question at the specified index from our buffer and prints it to the screen
In addition, it also prints the score of the user in question. It updates the score once the user has answered and pushes it to the scoreboard.
Parameters:
* int i: index of which question we want
*/
void yield_question(int i){
  //accessing the question and the answer from the doc
  const char* question = doc["results"][i]["question"];
  const char* answer = doc["results"][i]["correct_answer"];
  Serial.println(strlen(question));  
  strcpy(question_copy, question);

  // This chunk of code formats the questions properly to handle the " and ' special characters that are common. There are more uncommon ones but for most questions this should work. 
  memset(request_buffer, 0, sizeof(request_buffer));
  memset(response_buffer, 0, sizeof(response_buffer));
  sprintf(request_buffer,"POST https://608dev-2.net/sandbox/sc/mochan/trivia/triviaformat.py HTTP/1.1\r\n"); //triviaformat.py is my python script that formats the question to get rid of the unicode characters.
  strcat(request_buffer,"Host: 608dev-2.net\r\n");
  strcat(request_buffer,"Content-Type: application/json\r\n");
  sprintf(request_buffer + strlen(request_buffer),"Content-Length: %d\r\n", strlen(question_copy)); //append string formatted to end of request buffer
  strcat(request_buffer,"\r\n"); //new line from header to body
  strcat(request_buffer,question_copy); //body
  strcat(request_buffer,"\r\n"); //new line from header to body
  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  Serial.println(response_buffer);

  //Printing things to the tft. 
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,2);
  tft.println(response_buffer);
  tft.println(user_score);
  answer_question();
  //Checking answer validity
  if(user_answer == 1 && strcmp(answer, "True") == 0){
    Serial.println("RIGHT");
    post_score(1, 0);
  }
  else if(user_answer == 0 && strcmp(answer, "False") == 0){
    Serial.println("RIGHT");
    post_score(1, 0);
  }
  else{
    Serial.println("WRONG");
    post_score(0,1);
  }
  just_answered = 0;
}


/* answer_question-----------------------------------------------
This functino handles whether the question has been answered or not by toggling booleans
*/
void answer_question(){
  while(just_answered == 0){
    sm_45(digitalRead(BUTTON45)); 
    if(just_answered ==1) 
      break;
    else
      sm_39(digitalRead(BUTTON39));            
  } 
}


//I had a lot of fun generating this root cert (jk)
const char* CA_CERT = \
                      "-----BEGIN CERTIFICATE-----\n" \
                      "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
                      "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
                      "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
                      "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
                      "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
                      "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
                      "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
                      "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
                      "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
                      "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
                      "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n" \
                      "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
                      "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
                      "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
                      "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
                      "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
                      "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
                      "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
                      "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
                      "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n" \
                      "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
                      "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
                      "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
                      "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
                      "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n" \
                      "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
                      "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
                      "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
                      "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
                      "-----END CERTIFICATE-----\n";



void do_https_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  client.setHandshakeTimeout(30);
  client.setCACert(CA_CERT); //set cert for https
  if (client.connect(host,443,4000)) { //try to connect to host on port 443
    Serial.println("CONNECTED");
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    response[0] = '\0';
    //memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    Serial.println("oops something went wrong");
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

/* generate_question-----------------------------------------------
This function performs a get request to the trivia server and deserializes the json doc.
*/
void generate_questions(){
  Serial.println("SENDING REQUEST");
  memset(request_buffer, 0, sizeof(request_buffer));
  sprintf(request_buffer,"GET /api.php?command=request&amount=10&type=boolean HTTP/1.1\r\n");
  strcat(request_buffer,"Host: opentdb.com\r\n"); //comment this line if https doesn't work (bad cert?)
  //strcat(request_buffer,"Host: opentdb.com:443\r\n"); //uncomment this line if https doesn't work (bad cert?)
  strcat(request_buffer,"\r\n"); //new line from header to body
  do_https_request("opentdb.com", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false); //comment this line if https doesn't work (bad cert?)
  //do_http_request("opentdb.com", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false); uncomment this line if https doesn't work (bad cert?)

  Serial.println(response_buffer);

  
  char* ptr_open = strchr(response_buffer, '{');
  char* ptr_close = strrchr(response_buffer, '}');
  int ptrlen = ptr_close- ptr_open +1;
  
  memset(json, 0, sizeof(json));
  memcpy(json, ptr_open, ptrlen);
  Serial.println("json");
  Serial.println(json);
  deserializeJson(doc, json);
  
}

/* sm_45-----------------------------------------------
This function is the state machine for Button at pin 45, which is the "TRUE" response button.
Parameters:
*int button_val: High or Low button value.
*/
void sm_45(int button_val){
  switch(state_45){
    case IDLE:      
      if(button_val == 0)
        state_45 = PRESS;
      break;
    case PRESS:
      if(button_val ==1)
        state_45 = RELEASE;
      break;
    case RELEASE:
      user_answer = 1;
      just_answered = 1; //if the button has j been released then the user has just answered the question
      state_45 = IDLE;
      break;
  }  
}

/* sm_39-----------------------------------------------
This function is the state machine for Button at pin 39, which is the "FALSE" response button.
Parameters:
*int button_val: High or Low button value.
*/
void sm_39(int button_val){
  switch(state_39){
    case IDLE:
      if(button_val == 0)
        state_39 = PRESS;
      break;
    case PRESS:
      if(button_val ==1)
        state_39 = RELEASE;
      break;
    case RELEASE:
      user_answer = 0;
      just_answered = 1;
      state_39 = IDLE;
      break;
  }  
}

/*----------------------------------
   char_append Function:
   Arguments:
      char* buff: pointer to character array which we will append a
      char c:
      uint16_t buff_size: size of buffer buff

   Return value:
      boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}


/*----------------------------------
 * do_http_request Function:
 * Arguments:
 *    char* host: null-terminated char-array containing host to connect to
 *    char* request: null-terminated char-arry containing properly formatted HTTP request
 *    char* response: char-array used as output for function to contain response
 *    uint16_t response_size: size of response buffer (in bytes)
 *    uint16_t response_timeout: duration we'll wait (in ms) for a response from server
 *    uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
 * Return value:
 *    void (none)
 */

void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  if (client2.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client2.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client2.connected()) { //while we remain connected read out data coming back
      client2.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client2.available()) { //read out remaining text (body of response)
      char_append(response,client2.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client2.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client2.stop();
  }
}  

/* post_score-----------------------------------------------
This function does a POST request to triviagame.py to increment the score in the database table.
Parameters:
*int correct: The number of correct responses. 
*int incorrect: The number of incorrect responses.
*/
void post_score(int correct, int incorrect){
  char body[100]; //for body
  sprintf(body,"user=%s&correct=%d&incorrect=%d",USER, correct,incorrect);//generate body, posting to User, 1 step
  int body_len = strlen(body); //calculate body length (for header reporting)
  memset(request_buffer, 0, sizeof(request_buffer));
  memset(response_buffer, 0, sizeof(response_buffer));
  sprintf(request_buffer,"POST https://608dev-2.net/sandbox/sc/mochan/trivia/triviagame.py HTTP/1.1\r\n");
  strcat(request_buffer,"Host: 608dev-2.net\r\n");
  strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request_buffer,"\r\n"); //new line from header to body
  strcat(request_buffer,body); //body
  strcat(request_buffer,"\r\n"); //new line from header to body
  do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  strcpy(user_score, response_buffer);
}

/* run_round-----------------------------------------------
This function runs a full round (10 questions) of True False questions. 
*/
void run_round(){
  if(strlen(user_score)==0){ //if we don't have data about the user score (just on startup), do an empty post request to get back the data.
    post_score(0,0); 
    
  }
  generate_questions();
  for(int i = 0; i < 10; i++){
    just_answered = 0;
    yield_question(i);
  }
      
}
void loop(){
  run_round();
}
