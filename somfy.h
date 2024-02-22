
#include "config.h"
#define OPENRTS_BOARD_TTGO_LORA32_V21

#define OPENRTS_RADIO_TYPE_SX1278
#define OPENRTS_RADIO_MISO 19
#define OPENRTS_RADIO_MOSI 27
#define OPENRTS_RADIO_SCLK 5
#define OPENRTS_RADIO_CS   18
#define OPENRTS_RADIO_RST  23
#define OPENRTS_RADIO_DATA 32
#define OPENRTS_LED        25
#define OPENRTS_OLED_TYPE_SSD1306

#define MSG_BUFFER_SIZE (80)

#include <openrts.hpp>

RTSRadio_SX1278 radio(OPENRTS_RADIO_CS);
RTSRemoteStore_NVS remoteStore;
RTSRemote remote(new RTSPulseOutput_GPIO(OPENRTS_RADIO_DATA), &remoteStore);

#define numdevs 4

namespace somfyspace {
typedef void (*cb_onsendrf) (uint32_t addr, uint8_t cmd);
typedef void (*cb_onsubscribe) (char * topic);
typedef void (*cb_onpublish) (char * topic, char * payload, bool retain);
typedef void (*cb_onchange) (uint32_t addr, uint value);

cb_onsendrf onsendrf;
cb_onsubscribe  onsubscribe;
cb_onsubscribe  onunsubscribe;
cb_onpublish onpublish;
cb_onchange ontargetchange;
cb_onchange onpositionchange;

} //  end of namespace somfyspace
#include <Arduino_JSON.h>     // https://github.com/arduino-libraries/Arduino_JSON

class SomfyCommand {
  private:
    uint8_t command;
    char Topic[80];
    char Target[2];
    bool enabled;
    uint32_t devAddr;
    char * devId;
    uint8_t string2command(String cmd) {
      uint8_t command = 0;
      if (cmd.indexOf("My") >= 0) command += 1;
      if (cmd.indexOf("Up") >= 0) command += 2;
      if (cmd.indexOf("Down") >= 0) command += 4;
      if (cmd.indexOf("Prog") >= 0) command += 8;
      return command;
    }

  public:
    SomfyCommand(uint32_t Addr, char *sCommand, char *sLocation, char *sDevice, char *sSubTopic) {
      devAddr = Addr;
      devId = sDevice;
      command = string2command(sCommand);
      strcpy(Topic, sLocation);
      strcat(Topic, "/");
      strcat(Topic, sDevice);
      strcat(Topic, "/");
      strcat(Topic, sSubTopic);
      strncpy(Target, sCommand, 1);
      Target[1] = '\0';
      enabled = true;
    }

    void SendRF() {
      if (enabled) {
        static uint8_t lastCommand = 0;
        Serial.printf("Sending %d\n", command);
        radio.setMode(RTS_RADIO_MODE_TRANSMIT);
        digitalWrite(OPENRTS_LED, 1);
        if (somfyspace::onsendrf) somfyspace::onsendrf(devAddr, command);
        for (int i = 0; i < 4; i++) {
          remote.sendCommand(devAddr, (rts_command)command, lastCommand == command);
          lastCommand = command;
          delay(400);
          i++;
        }
        lastCommand = 0;
        digitalWrite(OPENRTS_LED, 0);
        radio.setMode(RTS_RADIO_MODE_RECEIVE);
      }
    }

    void Enable() {
      enabled = true;
    }

    void Disable() {
      enabled = false;
    }

    bool Enabled() {
      return enabled;
    }
    void Publish() {
      if (somfyspace::onpublish)somfyspace::onpublish(Topic, Target, false);
    }

    void Reset() {
      if (somfyspace::onpublish)somfyspace::onpublish(Topic, "-", false);
    }
};

class DEVICE {

  private:
    enum PosState {
      Positioned,
      MovingUp,
      MovingDown,
      PositioningUp,
      PositioningDown,
      Unknown
    };
    enum button {
      None,
      Up,
      Down,
      My,
      GotoTarget,
      Prog,
      LProg
    };
    char msg[MSG_BUFFER_SIZE];
    int counter = 0;  //For counting the keep-alive-messages
    unsigned long lastMsg = 0;
    unsigned long TimerLED = 0;
    unsigned long TimePositioned = 0;
    unsigned long TimePositionReported = 0;
    PosState State = Unknown;
    PosState LastState = Unknown;
    button BPressed = None;
    int StartPos = 0;
    int LastPositionReported = 0;    //ensures the position is set on mqtt only when it has changed.
    bool TryToInitializePosition = true; //Try to subscribe and get the position once from previous sessions.
    bool TargetEnabled = false;
    SomfyCommand * SomfyUp;
    SomfyCommand * SomfyDown;
    SomfyCommand * SomfyMy;
    SomfyCommand * SomfyProg;
    void subscribe(char * subTopic) {
      sprintf(msg, "%s/%s/%s", mqtt_Location, devId, subTopic);
      if (somfyspace::onsubscribe) somfyspace::onsubscribe(msg);
    }
    void unsubscribe(char * subTopic) {
      sprintf(msg, "%s/%s/%s", mqtt_Location, devId, subTopic);
      if (somfyspace::onunsubscribe) somfyspace::onunsubscribe(msg);
    }
    void publish(char * subTopic, char * payload, bool retain = false) {
      /*
         if target change or position change do ontargetchange resp. onpositionchange
      */
      char topic[80];
      sprintf(topic, "%s/%s/%s", mqtt_Location, devId, subTopic);
      if (somfyspace::onpublish) somfyspace::onpublish(topic, payload, retain);
    }
  public:
    int Position = 0;
    int Target = 0;
    uint32_t devAddr;
    char * devId;
    char * name;
    double upTime;
    double downTime;
    DEVICE(uint32_t _Addr, char * Id, char * _name, double _upTime, double _downTime) {
      devId = Id;
      devAddr = _Addr;
      name = _name;
      upTime = _upTime;
      downTime = _downTime;
      SomfyUp = new SomfyCommand(devAddr, "Up", mqtt_Location, devId, mqtt_ButtonTopic);
      SomfyDown = new SomfyCommand(devAddr, "Down", mqtt_Location, devId, mqtt_ButtonTopic);
      SomfyMy = new SomfyCommand(devAddr, "My", mqtt_Location, devId, mqtt_ButtonTopic);
      SomfyProg = new SomfyCommand(devAddr, "Prog", mqtt_Location, devId, mqtt_ButtonTopic);
    }

    void begin() {
      SomfyMy->Disable();    // No "stop" before movement.
    }

    void init() {
      subscribe(mqtt_TargetTopic);
      subscribe(mqtt_ButtonTopic);
      if (TryToInitializePosition) subscribe(mqtt_PositionTopic);
    }
    void prepareButton(char * payload) {
      if ((char)payload[0] == 'U') {
        if (SomfyUp->Enabled()) BPressed = Up;
      } else if ((char)payload[0] == 'D') {
        if (SomfyDown->Enabled()) BPressed = Down;
      } else if ((char)payload[0] == 'M') {
        if (SomfyMy->Enabled()) BPressed = My;
      } else if ((char)payload[0] == 'P') {
        BPressed = Prog;
      } else if ((char)payload[0] == 'L') {
        BPressed = LProg;
      }
    }

    void prepareTarget(uint16_t _target) {
      Target = _target;
      if (Target != Position) {
        BPressed = GotoTarget;
#ifdef debug_
        snprintf (msg, MSG_BUFFER_SIZE, "Target obtained: %1d \n", Target);
        Serial.println(msg);
#endif
      }
    }

    void preparePosition(uint16_t _position) {
      if (TryToInitializePosition) {
        Position = _position;
        StartPos = Position;
        Target = Position;
        State = Positioned;
        TargetEnabled = true; //Now it is safe to receive the target.
#ifdef debug_
        snprintf (msg, MSG_BUFFER_SIZE, "Position initialized: %1d \n", Position);
        Serial.println(msg);
#endif
      }
      TryToInitializePosition = false;
      unsubscribe(mqtt_PositionTopic);
    }

    void onData(const char * topic, char * payload) {
      Serial.printf("Topic: %s, payload:%s\n", topic, payload);
      if (strcmp(topic, mqtt_ButtonTopic) == 0) {
        prepareButton(payload);
      } else if (strcmp(topic, mqtt_TargetTopic) == 0) {
        payload[sizeof(payload)] = '\0'; // NULL terminate the array
        if ((char)payload[0] != '-') {
          prepareTarget(atoi((char *)payload));
        }
      } else if (strcmp(topic, mqtt_PositionTopic) == 0) {
        payload[sizeof(payload)] = '\0'; // NULL terminate the array
        preparePosition(atoi((char *)payload));
      }
    }

    void loop() {
      unsigned long now = millis();
      if ((BPressed == Up) or ((BPressed == GotoTarget) and (Target == 0))) {
        SomfyUp->SendRF();
        TimePositioned = now;
        StartPos = Position;
        SomfyMy->Enable(); // Stopping is allowed now.
        BPressed = None;
        State = MovingUp;
#ifdef debug_
        Serial.println("Up event scheduled");
#endif
      } else if ((BPressed == Down) or ((BPressed == GotoTarget) and (Target == 100))) {
        SomfyDown->SendRF();
        TimePositioned = now;
        StartPos = Position;
        SomfyMy->Enable(); // Stopping is allowed now.
        BPressed = None;
        State = MovingDown;
#ifdef debug_
        Serial.println("Down event scheduled");
#endif
      } else if (BPressed == My) {
        SomfyMy->SendRF();
        SomfyMy->Disable();  // Stopping is NOT allowed now.
#ifdef debug_
        Serial.println("My event scheduled");
#endif
        SomfyMy->Reset();
      } else if (BPressed == Prog) {
        SomfyProg->SendRF();
        BPressed = None;
#ifdef debug_
        Serial.println("Prog event sent");
#endif
      } else if (BPressed == LProg) {
        while ((millis() - now) < Somfy_LongPressTime) {
          SomfyProg->SendRF();
        }
        BPressed = None;
#ifdef debug_
        Serial.println("Long Prog event sent");
#endif
      } else if (BPressed == GotoTarget) {
        BPressed = None;
        TimePositioned = now;
        StartPos = Position;
        if (Position > Target) {
          SomfyUp->SendRF();
          State = PositioningUp;
          StartPos = Position;
          SomfyMy->Enable(); // Stopping is allowed now.
#ifdef debug_
          snprintf (msg, MSG_BUFFER_SIZE, "Position UP from %1d to %2d \n", StartPos, Target);
          Serial.println(msg);
#endif
        } else if (Position < Target) {
          SomfyDown->SendRF();
          State = PositioningDown;
          StartPos = Position;
          SomfyMy->Enable(); // Stopping is allowed now.
#ifdef debug_
          snprintf (msg, MSG_BUFFER_SIZE, "Position DOWN from %1d to %2d \n", StartPos, Target);
          Serial.println(msg);
#endif
        }
      }
      if ((State == MovingUp) or (State == PositioningUp)) Position = StartPos - 1 - 100.0 * (now - TimePositioned) / (1000.0 * (float)upTime);
      if ((State == MovingDown) or (State == PositioningDown)) Position = StartPos + 100.0 * (now - TimePositioned) / (1000.0 * (float)downTime);
      if (Position < 0) {
        Position = 0;
        State = Positioned;
        SomfyMy->Disable();
      } else if (Position > 100) {
        Position = 100;
        State = Positioned;
        SomfyMy->Disable();
      }
      if (BPressed == My) {
        BPressed = None;
        State = Positioned;
        SomfyMy->Disable();
      }
      if (((State == PositioningDown) and (Position >= Target)) or ((State == PositioningUp) and (Position <= Target))) {
        SomfyMy->SendRF();
        SomfyMy->Disable();
#ifdef debug_
        if (State == PositioningDown) snprintf (msg, MSG_BUFFER_SIZE, "Stopped moving down at Position: %1d, Target %2d \n", Position, Target);
        if (State == PositioningUp)   snprintf (msg, MSG_BUFFER_SIZE, "Stopped moving up at Position: %1d, Target %2d \n", Position, Target);
        Serial.println(msg);
#endif
        State = Positioned;
      }
      if (now - TimePositionReported > mqtt_PublishPositionInterval) {
        TimePositionReported = now;
        if (Position != LastPositionReported) {
          LastPositionReported = Position;
#ifdef debug_
          snprintf (msg, MSG_BUFFER_SIZE, "Position is %ld", Position);
          Serial.println(msg);
#endif
          snprintf (msg, MSG_BUFFER_SIZE, "%ld", Position);
          publish(mqtt_PositionTopic, msg, true);
        }
      }
      if (LastState != State) {
        LastState = State;
        if (State == Positioned) {
          SomfyMy->Reset();;
        }
      }
      if (now - lastMsg > mqtt_PublishInterval) {
        lastMsg = now;
        snprintf (msg, MSG_BUFFER_SIZE, "%s, Device:%s,  #%ld P%ld", HOSTNAME, devId, counter++, Position);
        if (counter >= 1000) counter = 0; // prevent overflow
#ifdef debug_
        Serial.print("Publish message: ");
        Serial.println(msg);
#endif
        publish(mqtt_StatusTopic, msg);
      }
    }
};

class DEVICES {
  private:
    DEVICE * devs[numdevs];

    String topic2devId(char * topic) {
      char buf[strlen(topic) + 1];
      sprintf(buf, "%s", topic);
      String topicFld[4];
      int fldCnt = 0;
      char * token = strtok(buf, "/");
      while ( token != NULL ) {
        topicFld[fldCnt++] = String(token);
        token = strtok(NULL, "/");
      }
      return topicFld[fldCnt - 2];
    }
  public:
    DEVICE * current;
    DEVICES(DEVICE * _devs[]) : devs{_devs[0], _devs[1], _devs[2], _devs[3]} {
      current = devs[0];
    }

    uint8_t getIndexFromTopic(char * topic) {
      return getIndexFromId(topic2devId(topic));
    }

    uint8_t getIndexFromAddr(uint32_t addr) {
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) {
          if (addr == devs[i]->devAddr) {
            return i;
          }
        }
      }
      return 0;
    }

    uint8_t getIndexFromId(String id) {
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) {
          if (id == devs[i]->devId) {
            return i;
          }
        }
      }
      return 0;
    }

    String getSubTopic(char * topic) {
      char buf[80];
      sprintf(buf, "%s", topic);
      String topicFld[4];
      int fldCnt = 0;
      char * token = strtok(buf, "/");
      while ( token != NULL ) {
        topicFld[fldCnt++] = String(token);
        token = strtok(NULL, "/");
      }
      return topicFld[fldCnt - 1];
    }

    void setCurrent(uint8_t count) {
      current = devs[count];
    }

    void setCurrent(const char * fld) {
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) {
          if (String(fld) == devs[i]->devId) {
            current = devs[i];
            break;
          }
        }
      }
    }

    void setCurrent(uint32_t addr) {
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) {
          if (addr == devs[i]->devAddr) {
            current = devs[i];
            break;
          }
        }
      }
    }

    void onSendRf(somfyspace::cb_onsendrf cb) {
      somfyspace::onsendrf = cb;
    }

    void onPublish(somfyspace::cb_onpublish cb) {
      somfyspace::onpublish = cb;
    }

    void onSubscribe(somfyspace::cb_onsubscribe cb) {
      somfyspace::onsubscribe = cb;
    }

    void onUnsubscribe(somfyspace::cb_onsubscribe cb) {
      somfyspace::onunsubscribe = cb;
    }

    String getSelector() {
      JSONVar doc = {};
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) {
          doc["selector"]["id"][i] = devs[i]->name;
          doc["selector"]["addr"][i] = devs[i]->devAddr;
        }
      }
      return (JSON.stringify(doc));
    }

    uint8_t currentIndex() {
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] == current) return i;
      }
      return 0;
    }

    void onTargetChange(somfyspace::cb_onchange cb) {
      somfyspace::ontargetchange = cb;
    }

    void onPositionChange(somfyspace::cb_onchange cb) {
      somfyspace::onpositionchange = cb;
    }

    void begin() {
      pinMode(OPENRTS_LED, OUTPUT);
      pinMode(OPENRTS_RADIO_DATA, OUTPUT);
      radio.begin();
      radio.setMode(RTS_RADIO_MODE_TRANSMIT);
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) devs[i]->begin();
      }
    }

    void init() {
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) devs[i]->init();
      }
    }

    void loop() {
      for (int i = 0; i < numdevs; i++) {
        if (devs[i] != nullptr) devs[i]->loop();
      }
    }
};
