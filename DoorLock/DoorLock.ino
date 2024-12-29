

#include "Lock.h"  // HomeSpan sketches always begin by including the HomeSpan library
#include "DebounceEvent.h"

#if CONFIG_FREERTOS_UNICORE
#define TASK_RUNNING_CORE 0
#else
#define TASK_RUNNING_CORE 1
#endif

#define MOTOR_PIN_1 39
#define MOTOR_PIN_2 40

#define DOOR_CLOSE_PIN 33
#define LOCK_PIN 34
#define UNLOCK_PIN 35


#define BUTTON_PIN 36

#define STATUS_LED_PIN 18
#define CONTROL_PIN 0

Lock *lock;

TaskHandle_t lockUnlockTaskHandle;
void TaskLockUnlock(void *pvParameters);

void buttonCallback(uint8_t pin, uint8_t event, uint8_t count, uint16_t length) {
  if (event == 2 && count == 1) {
    lock->toggleTargetState();
  }
}

DebounceEvent button = DebounceEvent(BUTTON_PIN, buttonCallback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);


void setup() {  // Your HomeSpan code should be placed within the standard Arduino setup() function

  Serial.begin(115200);  // Start a serial connection so you can receive HomeSpan diagnostics and control the device using HomeSpan's Command-Line Interface (CLI)

  homeSpan.setStatusPin(STATUS_LED_PIN); 
  homeSpan.setControlPin(CONTROL_PIN);

  homeSpan.enableOTA("syncbyte");

  homeSpan.begin(Category::Locks, "Smart Door Lock");  // initializes a HomeSpan device named "HomeSpan Lightbulb" with Category set to Lighting

  // Next, we construct a simple HAP Accessory Database with a single Accessory containing 3 Services,
  // each with their own required Characteristics.

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Manufacturer("SYNC BYTE");  // Manufacturer of the Accessory (arbitrary text string, and can be the same for every Accessory)
  new Characteristic::SerialNumber("SB-001");     // Serial Number of the Accessory (arbitrary text string, and can be the same for every Accessory)
  new Characteristic::Model("001-Door Lock");     // Model of the Accessory (arbitrary text string, and can be the same for every Accessory)
  new Characteristic::FirmwareRevision("1.0");    // Firmware of the Accessory (arbitrary text string, and can be the same for every Accessory)

  lock = new Lock(MOTOR_PIN_1, MOTOR_PIN_2, false, DOOR_CLOSE_PIN, UNLOCK_PIN, LOCK_PIN);

  homeSpan.autoPoll();

  // This variant of task creation can also specify on which core it will be run (only relevant for multi-core ESPs)
  xTaskCreatePinnedToCore(
    TaskLockUnlock, "Lock Unlock Task", 2048  // Stack size
    ,
    NULL  // When no parameter is used, simply pass NULL
    ,
    1  // Priority
    ,
    &lockUnlockTaskHandle  // With task handle we will be able to manipulate with this task.
    ,
    TASK_RUNNING_CORE  // Core on which the task will run
  );
}


void loop() {
  button.loop();
}

void TaskLockUnlock(void *pvParameters) {
  lock->lockUnlockTask();
}
