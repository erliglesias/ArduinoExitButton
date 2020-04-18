#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Thread.h>

#define ARDUINO_CLIENT_ID "arduino_1"                                // Client ID for Arduino pub/sub
#define PUBLISH_DELAY 3000

#define EXIT_COUNT_DOWN_STATUS "arduino_1/cont_down_status"          // MTTQ topic for CONT_DOWN_STATUS

#define SUB_RELAY "arduino_1/relayPin"                               // MTTQ topic for RELAY
#define SUB_RELAY_STATUS "arduino_1/status"                          // MTTQ topic for RELAY_STATUS

#define SUB_RELAY1 "arduino_1/relayPin1"
#define SUB_RELAY_STATUS1 "arduino_1/status1"

#define SUB_RELAY2 "arduino_1/relayPin2"
#define SUB_RELAY_STATUS2 "arduino_1/status2"

//RELAYS
const int relayPin2  = 4;
const int relayPin   = 9;
const int relayPin1  = 5;

const int sensorPin = 7;
const int buttonPin = 6;

// Networking details
byte mac[]    = {  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };           // Ethernet shield (W5100) MAC address
IPAddress ip(192, 168, xx, xx);                                    // Ethernet shield (W5100) IP address
IPAddress server(192, 168, xx, xx);                                // MTTQ server IP address

EthernetClient ethClient;
PubSubClient client(ethClient);
Thread thrReconnect = Thread();

int relayValue = 0;
int relay1Value = 0;
int relay2Value = 0;
int previousStateRelayPin = 2;
int previousStateRelayPin1 = 2;
int previousStateRelayPin2 = 2;

int sensorValue = 0;

int buttonValue = 0;
int contButtonPin = 0;
int previousStateButttonPin = 1;

int cicleCont = 0;
boolean countdown = false;
boolean wait_to_auto = false;
boolean bye = false;
boolean waitWelcome = true;
long cdStartTime, cdFinalTime, actualTime;

int actn = 0 ;

void setup()
{
  Serial.begin(9600);

  pinMode(relayPin, OUTPUT);
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  // MTTQ parameters
  client.setServer(server, 1883);
  client.setCallback(callback);

  //Ethernet shield configuration
  Ethernet.begin(mac, ip);

  // Reconnect Thread
  thrReconnect.enabled = true;
  thrReconnect.setInterval(32000);
  thrReconnect.onRun(reconnect);

  delay(2000); // Allow hardware to stabilize.
}

void loop()
{
  sensorValue = digitalRead(sensorPin);
  buttonValue = digitalRead(buttonPin);
  relayValue = digitalRead(relayPin);
  relay1Value = digitalRead(relayPin1);
  relay2Value = digitalRead(relayPin2);

  if (waitWelcome) {
    allup();
  }

  if (buttonValue != previousStateButttonPin) {
    if (buttonValue == 0) {
      contButtonPin++;
    }
  }
  previousStateButttonPin = buttonValue;

  delay(200);

  if (contButtonPin >= 1 ) { // If the button is pressed the countdown begins

    if (contButtonPin % 2 == 0 ) { //If you press a second time the countdown is canceled
      if (actn != 2) { //If it is not in manual mode we put it in automatic again
        actn = 0 ;
        contButtonPin = 0 ;
        Serial.println("Countdown canceled");
        delay(200);
        client.publish(EXIT_COUNT_DOWN_STATUS, "000");
      }
    }  else {
      actn = 1 ;
    }

  } else { // If the button has not been pressed
    if (contButtonPin == 0 && sensorValue == LOW && bye  == true) {
      allup();
      bye  = false;
    }
    if (actn != 2) { //If it is not in manual mode
      actn = 0 ;
    }

  }

  switch (actn) {
    case 0:   //Pir Sensor Mode
      if (sensorValue == HIGH)
      {
        digitalWrite(relayPin, LOW);
      } else {
        digitalWrite(relayPin, HIGH);
      }
      countdown = false;
      wait_to_auto = false;
      break;
    case 1:   //Countdown start
      digitalWrite(relayPin, LOW);
      client.publish(EXIT_COUNT_DOWN_STATUS, "001");

      cdStartTime = millis();

      if (countdown == false)
      {
        cdFinalTime = cdStartTime + 9000;
        Serial.println("Countdown started");
      }

      countdown = true;
      break;
    case 2: // Manual Mode
      //If we press turn off manually wait for the sensor to turn off and it goes into sensor mode
      if (wait_to_auto == true && sensorValue == HIGH) {
        actn = 0;
      }
      break;
  }

  actualTime = millis();

  if (countdown == true && (actualTime > cdFinalTime) && waitWelcome == false && sensorValue == HIGH)
  {
    alldown();

  }

  if (!client.connected()) {
    cicleCont = cicleCont + 1;
    if (cicleCont >= 15) {
      thrReconnect.run();
      cicleCont = 0;
    }
  } else {
    bool arrRelayValues[3] = {relayValue, relay1Value, relay2Value};
    bool arrPreviousState[3] = {previousStateRelayPin, previousStateRelayPin1, previousStateRelayPin2 };
    char* arrSubStatus[3] = {SUB_RELAY_STATUS, SUB_RELAY_STATUS1, SUB_RELAY_STATUS2};
  
    for (int i=0;i<3;i++){
    if ( arrRelayValues[i] != arrPreviousState[i]){
      if (arrRelayValues[i] == LOW) {
        client.publish(arrSubStatus[i], "000");
      } else {
        client.publish(arrSubStatus[i], "001");   
      }
    }
    }
  
    previousStateRelayPin = relayValue;
    previousStateRelayPin1 = relay1Value;
    previousStateRelayPin2 = relay2Value;
  }

  client.loop();

}

void alldown()
{
  Serial.println("Bye");
  digitalWrite(relayPin, LOW);
  client.publish(EXIT_COUNT_DOWN_STATUS, "000");
  countdown = false;
  contButtonPin = 0;
  bye = true;


  digitalWrite(relayPin1, LOW);
  digitalWrite(relayPin2, LOW);
}

void allup()
{
  Serial.println("Hello");
  waitWelcome = false;

  digitalWrite(relayPin1, HIGH);
  digitalWrite(relayPin2, HIGH);
}

void reconnect()
{
  // Loop until reconnected
  Serial.print("Attempting MQTT connection ... ");
  // Attempt to connect
  if (client.connect(ARDUINO_CLIENT_ID)) {
    Serial.println("connected");
    // (re)subscribe
    client.subscribe(SUB_RELAY);
    client.subscribe(SUB_RELAY1);
    client.subscribe(SUB_RELAY2);
  } else {
    Serial.print("Connection failed, state: ");
    Serial.print(client.state());
  }
}

// Sub callback function
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("[sub: ");
  Serial.print(topic);
  Serial.print("] ");
  char message[length + 1] = "";
  for (int i = 0; i < length; i++)
    message[i] = (char)payload[i];
  message[length] = '\0';
  Serial.println(message);

  // SUB_RELAY topic section
  if (strcmp(topic, SUB_RELAY) == 0)
  {
    if (strcmp(message, "countdown_on") == 0)
    {
      contButtonPin++;
      actn = 0;
    }
    if (strcmp(message, "countdown_off") == 0)
    {
      contButtonPin++;
      actn = 1;
    }
    if (strcmp(message, "turn_on") == 0)
    {
      digitalWrite(relayPin, HIGH);
      actn = 2;
    }
    if (strcmp(message, "turn_off") == 0)
    {
      digitalWrite(relayPin, LOW);
      actn = 2;
      wait_to_auto = true;
    }
  }

  // SUB_RELAY1 topic section
  if (strcmp(topic, SUB_RELAY1) == 0)
  {
    if (strcmp(message, "turn_on1") == 0)
    {
      digitalWrite(relayPin1, LOW);
    }
    if (strcmp(message, "turn_off1") == 0)
    {
      digitalWrite(relayPin1, HIGH);
    }
  }

  // SUB_RELAY2 topic section
  if (strcmp(topic, SUB_RELAY2) == 0)
  {
    if (strcmp(message, "turn_on2") == 0)
    {
      digitalWrite(relayPin2, LOW);
    }
    if (strcmp(message, "turn_off2") == 0)
    {
      digitalWrite(relayPin2, HIGH);
    }
  }
}
