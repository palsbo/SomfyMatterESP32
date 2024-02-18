// Config file for a Somfy / mqtt bridge.

//=========================================================================================================
#define Somfy_LongPressTime 2500    // The time a longpress will repeat a button (in ms)

// The characteristics of your rollo
#define Somfy_Time2MoveFullyUp   43.1 // Number of seconds for moving the rollo "Up"   (position   0)
#define Somfy_Time2MoveFullyDown 44.3 // Number of seconds for moving the rollo "Down" (position 100)

// Topics used=============================================================================================
#define mqtt_PublishInterval  60000             // Interval (ms) for updating status messages
#define mqtt_PublishPositionInterval 1000           // Interval (ms) for updating the position

#define mqtt_Location       "somfy"     // Represents the location where the sunshade is

#define mqtt_TargetTopic    "target"    // (WRITE) The position to go to
#define mqtt_PositionTopic  "position"  // (READ) Actual position
#define mqtt_ButtonTopic    "button"    // (WRITE) press button Up,Down,My,Prog,LongProg (U,D,M,P,L)
#define mqtt_StatusTopic    "status"    // (READ) Status and debug messages

#define mqtt_ESPstate           mqtt_Location "/"  mqtt_Device "/" mqtt_StatusTopic // Status messages and debug info (when enabled)    

//=============================================================================================================

// Set this debug define to write information to the serial port, will NOT be visible over Wifi, only to the serial port
// When set, debug lines are also written to the test/scherm/ESP topic.
//#define debug_
