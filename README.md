# Arduino - Exit Button

## PIR Sensor + Button + MQTT

 Pressing the exit button starts a countdown so that the relays turn off. When detecting movement, the relays turn on.


## Home Assistant
 

 Add this to configuration.yam

```yaml
mqtt:
  broker: 192.168.xx.xx

switch:
   - platform: mqtt
    name: "PIR Sensor"
    state_topic: "arduino_1/status"
    command_topic: "arduino_1/relayPin"
    payload_on: "turn_on"
    payload_off: "turn_off"
    state_on: "001"
    state_off: "000"
  - platform: mqtt
    name: "Relay 1"
    state_topic: "arduino_1/status1"
    command_topic: "arduino_1/relayPin1"
    payload_on: "turn_off1"
    payload_off: "turn_on1"
    state_on: "001"
    state_off: "000"
  - platform: mqtt
    name: "Relay 2"
    state_topic: "arduino_1/status2"
    command_topic: "arduino_1/relayPin2"
    payload_on: "turn_off2"
    payload_off: "turn_on2"
    state_on: "001"
    state_off: "000" 
```