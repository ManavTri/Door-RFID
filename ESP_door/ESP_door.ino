#include <esp_task_wdt.h>
//#include <WiFi.h> //Wifi library
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks
#define WDT_TIMEOUT 30 //seconds
uint64_t uS_TO_S_FACTOR = 1000000;  // Conversion factor for micro seconds to seconds
// #define EAP_IDENTITY "tad85" //if connecting from another corporation, use identity@organisation.domain in Eduroam 
// #define EAP_USERNAME "tad85" //oftentimes just a repeat of the identity
// #define EAP_PASSWORD "_" //your Eduroam password
// const char* ssid = "WIRELESS-PITTNET"; // Eduroam SSID
//const char* host = "arduino.php5.sk"; //external server domain for HTTP connection after authentification
int counter = 0;

// NOTE: For some systems, various certification keys are required to connect to the wifi system.
//       Usually you are provided these by the IT department of your organization when certs are required
//       and you can't connect with just an identity and password.
//       Most eduroam setups we have seen do not require this level of authentication, but you should contact
//       your IT department to verify.
//       You should uncomment these and populate with the contents of the files if this is required for your scenario (See Example 2 and Example 3 below).
//const char *ca_pem = "insert your CA cert from your .pem file here";
//const char *client_cert = "insert your client cert from your .crt file here";
//const char *client_key = "insert your client key from your .key file here";

// const char* resource = "/trigger/esp32/with/key/eUIeYrP9-v0ERqbmv-noOSSWk-wIitBcYJjeSpmCOIc";
// const char* server = "maker.ifttt.com";

const int RST_PIN = 17;
const int SS_PIN = 5;
const int PICC_id_length = 8;

byte readCard[PICC_id_length];
char tagID[PICC_id_length] = "";
String messageText;
String resetText = "Reset";
String moreText = "More";
boolean successRead = false;
boolean correctTag = false;
boolean doorOpened = false;
LiquidCrystal_I2C lcd(0x27,16,2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// motor vars
const int stepPin = 15;
const int dirPin = 16; 
const int relayPin = 7; 

const int microDelay = 1000;
const int stepSize = 200;
const int motorRots = 12;
int additionalRots = 0;

const int redLed = 26;
const int blueLed = 25;

#define EEPROM_SIZE 512

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
     void onWrite(BLECharacteristic *characteristic) {
          std::string rxValue = characteristic->getValue(); 
          if (resetText.equals(rxValue.c_str())){
            ESP.restart();
          }

          else if (moreText.equals(rxValue.c_str())){
            additionalRots = 1;
          }
          Serial.println(rxValue.c_str());
          
     }//onWrite
};

void setup() {
  Serial.begin(115200);

  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }

  Serial.println("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

  
  /*
  delay(10);
  Serial.println();
  Serial.print("Connecting to network: ");
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode
  
  // Example1 (most common): a cert-file-free eduroam with PEAP (or TTLS)
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);

  // Example 2: a cert-file WPA2 Enterprise with PEAP
  //WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD, ca_pem, client_cert, client_key);
  
  // Example 3: TLS with cert-files and no password
  //WiFi.begin(ssid, WPA2_AUTH_TLS, EAP_IDENTITY, NULL, NULL, ca_pem, client_cert, client_key);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if(counter>= WDT_TIMEOUT/2){ //after 10 seconds timeout - reset board
      break;
      //ESP.restart();
    }
  }
  
  if (counter < WDT_TIMEOUT/2){
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address set: "); 
    Serial.println(WiFi.localIP()); //print LAN IP
  }
  else{
    Serial.println("could not connect to wifi");
  }
 */
  SPI.begin();        // SPI bus
  mfrc522.PCD_Init(); //  MFRC522
  lcd.init();
  lcd.backlight();
  
  pinMode(stepPin,OUTPUT);
  pinMode(dirPin,OUTPUT);
  pinMode(redLed,OUTPUT);
  pinMode(blueLed,OUTPUT);
  Serial.begin(9600);

  digitalWrite(redLed, HIGH);
  delay(1000);
  digitalWrite(redLed, LOW);
  digitalWrite(blueLed, HIGH);
  delay(1000);
  digitalWrite(blueLed, LOW);
  

  reset_display();

  

  Serial.println("Starting BLE work!");

  BLEDevice::init("SB Door ESP");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("use: 'Reset' to restart ESP");
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined!");
  


  //makeIFTTTRequest(" ", "ESP Reset", " ");
}
void loop() {

  esp_task_wdt_reset();



  for ( uint8_t i = 0; i < PICC_id_length; i++) {  // Clear uid array
      mfrc522.uid.uidByte[i] = 0;
    }
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return;
  }
  for ( uint8_t i = 0; i < PICC_id_length; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    tagID[i] = (mfrc522.uid.uidByte[i]); // Adds the PICC_id_length bytes in a single String variable

  }\
  // tagID.toUpperCase();
  mfrc522.PICC_HaltA(); // Stop reading
  correctTag = false;
  bool success = false;
  Serial.print("tagID: ");
  Serial.println(tagID);
  /*
  reset_EEPROM_new_master(readCard);
  Serial.println("Reset EEPROM done");
  delay(100000);
  return;
  */
  // Checks whether the scanned tag is the master tag
  if ( is_master_id( readCard ) == true ){
    
    lcd_message("Add/Remove Tag", 0);
    successRead = false;
    while (!successRead) {
      successRead = getID();
      if ( successRead == true) { // If new card is scanned
        
        if (remove_id(readCard)){
          messageText = "Removed Tag";
          lcd_message("Removed Tag", 1);
          delay(1000);
          reset_display();
          success = true;
        }
        else{
          add_EEPROM_id(readCard); // If card cannot be found and removed, add it
          messageText = "Added Tag";
          lcd_message("Added Tag!", 1);
          delay(1000);
          reset_display();
          success = true;
        }
      
      }
    }
  }
  // Checks whether the scanned tag is authorized
  
  
  if (success == false){
    for (int i = 2; i < EEPROM_SIZE / PICC_id_length; i++){
      
      byte test [PICC_id_length];
      read_EEPROM_id(test, i);
      
      bool is_found = true;
      for (int n = 0; n < PICC_id_length; n++){
        if (test[n] != readCard[n]){
          is_found = false;
          break;
        }
        
      }
  
      if (is_found){
        open_door();
        messageText = "Access Granted";
        success = true;
        break;
      }
      
    }
  }
  
  if (success == false){
    messageText = "Access Denied";
    lcd_message("Access Denied!", 1);
    delay(1000);
    reset_display();
  }





  /*
  if (WiFi.status() == WL_CONNECTED) { //if we are connected to Eduroam network
    counter = 0; //reset counter
    //Serial.println("Wifi is still connected with IP: "); 
    //Serial.println(WiFi.localIP());   //inform user about his IP address
  }else if (WiFi.status() != WL_CONNECTED) { //if we lost connection, retry
    Serial.println("Connection lost, retrying connection...");
    WiFi.begin(ssid);      
  }
  
  while (WiFi.status() != WL_CONNECTED) { //during lost connection, print dots
    delay(500);
    Serial.print(".");
    counter++;
    if(counter>=WDT_TIMEOUT/2){ //30 seconds timeout - reset board
      break;
    //ESP.restart();
    }
  }
  

  makeIFTTTRequest(tagID, messageText, " ");
  */
  Serial.println(messageText);
}



/*void makeIFTTTRequest(String value1, String value2, String value3) {
  Serial.print("Connecting to "); 
  Serial.print(server);
  
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Failed to connect...");
  }
  
  Serial.print("Request resource: "); 
  Serial.println(resource);

  // Temperature in Celsius
  String jsonObject = String("{\"value1\":\"") + value1 + "\",\"value2\":\"" + value2
                      + "\",\"value3\":\"" + value3 + "\"}";
                      
  // Comment the previous line and uncomment the next line to publish temperature readings in Fahrenheit                    
  //String jsonObject = String("{\"value1\":\"") + (1.8 * bme.readTemperature() + 32) + "\",\"value2\":\"" 
                      //+ (bme.readPressure()/100.0F) + "\",\"value3\":\"" + bme.readHumidity() + "\"}";
                      
  client.println(String("POST ") + resource + " HTTP/1.1");
  client.println(String("Host: ") + server); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
        
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop(); 
}
*/
void motorStep(float mult = 1){

  if(mult < 0){ // Positive mult to open, negative to close
    digitalWrite(dirPin, LOW);
  }
  else{
    digitalWrite(dirPin, HIGH);
  }
  delay(5);

  mult = abs(mult) + additionalRots;
  additionalRots = 0;

  for(int i = 0; i < mult*stepSize; i++){
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(microDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(microDelay);
  }
}

void open_door(){
  lcd_message("Access Granted!", 1);
  motorStep(motorRots);
  delay(3000);
  motorStep(-motorRots);
  reset_display();
}

uint8_t getID() {
  for ( uint8_t i = 0; i < PICC_id_length; i++) {  // Clear uid array
      mfrc522.uid.uidByte[i] = 0;
  }
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }

  for ( uint8_t i = 0; i < PICC_id_length; i++) {  // Reads each byte of the ID
    readCard[i] = mfrc522.uid.uidByte[i]; // Adds the bytes in a global byte array
    tagID[i] = (mfrc522.uid.uidByte[i]); // Adds the bytes in a global String variable
    
  }
  //tagID.toUpperCase();
  
  //Serial.print("tagID: ");
  //Serial.println(tagID);
  mfrc522.PICC_HaltA(); // Stop reading
  if (is_master_id( readCard ) == true){
    return 0;    
  }
  return 1;
}

void read_EEPROM_id(byte byteArray [], int index){ // reads ID at index and store it in byteArray
  
  for(int i = 0; i < PICC_id_length; i++){byteArray[i] = 0x00;}
  int start_pos = index * PICC_id_length;
  for (int i = start_pos; i < start_pos + PICC_id_length; i++){
    byteArray[i - start_pos] = EEPROM.read(i);
  }
  if (index == 0){
    Serial.println("EEPROM index 0 is reserved");
  }
  
}
void clear_EEPROM_index(int index){
  if (index > 1){
    
    int start_pos = index * PICC_id_length;
    for (int i = start_pos; i <= start_pos + PICC_id_length; i++){
      EEPROM.write(i, 0xFF);
      
    }
    EEPROM.commit();
    
  }
  else{
    Serial.println("EEPROM index 0/1 is reserved");
  }

}
void add_EEPROM_id(byte new_id []){
  for (int i = 2; i < EEPROM_SIZE / PICC_id_length; i++){
    byte test [PICC_id_length];
    read_EEPROM_id(test, i);

    bool is_Empty = true;
    for (int n = 0; n < PICC_id_length; n++){
      if (test[n] != 0xFF){
        is_Empty = false;
        break;
      }
    }

    if (is_Empty){
      write_EEPROM_id(new_id, i);
      break;
    }
    
  }
}
void reset_EEPROM_new_master(byte new_id []){
  write_EEPROM_id(new_id, 1);
  for (int i = 2; i < EEPROM_SIZE / PICC_id_length; i++){
    
    clear_EEPROM_index(i);
    if (i%10 == 0){
      Serial.print("clearing:");
      Serial.println(i);
    }

    
  }
}
bool remove_id(byte remove_id []){
  bool success = false;
  for (int i = 2; i < EEPROM_SIZE / PICC_id_length; i++){
    byte test [PICC_id_length];
    read_EEPROM_id(test, i);

    bool is_found = true;
    for (int n = 0; n < PICC_id_length; n++){
      if (test[n] != remove_id[n]){
        is_found = false;
        break;
      }
    }

    if (is_found){
      clear_EEPROM_index(i);
      success = true;
      break;
    }
    
  }
  return success;
}
void write_EEPROM_id(byte id [], int index){
  if (index > 0){
    
    int start_pos = index * PICC_id_length;
    for (int i = start_pos; i <= start_pos + PICC_id_length; i++){
      EEPROM.write(i, id[i - start_pos]);
    }
    EEPROM.commit();
  }
  else{
    Serial.println("EEPROM index 0 is reserved");
  }
}

bool is_master_id (byte id []){
  
  byte master_key [PICC_id_length];
  read_EEPROM_id(master_key, 1);
  bool is_master = true;
  for (int i = 0; i < PICC_id_length; i++){
    if (master_key[i] != id[i]){
      is_master = false;
      break;
    }
  }
  delay(2000);
  return is_master;
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
    Serial.println("");
}

void print_id(byte id []){
  for (int i = 0; i < PICC_id_length; i++){
    Serial.print(id[i], HEX);
    
  }
  Serial.println("");

}

void lcd_message(String message, int row){
  
  int start = 0;
  if (message.length() < 16){
    start = (16 - message.length())/2;
  }
  lcd.setCursor(start,row);
  
  for (int i = 0; i < message.length() && i <= 16; i++){
    lcd.print(message[i]);
  }
  
  Serial.println("lcd message:" + message);
}

void reset_display(){
  lcd_message("  Scan your ID  ", 0);
  lcd_message("                ", 1);
}
