

#include "HomeSpan.h"  // HomeSpan sketches always begin by including the HomeSpan library

#define UNLOCKED 0
#define LOCKED 1
#define JAMMED 2
#define UNKNOWN 3

#define UNLOCK 0
#define LOCK 1

struct Lock : Service::LockMechanism {  // Dimmable LED
  SpanCharacteristic *currentState;     // reference to the On Characteristic
  SpanCharacteristic *targetState;      // NEW! Create a reference to the Brightness Characteristic instantiated below

  int motorPin1;
  int motorPin2;
  bool dir;
  int doorClosedPin;
  int lockOpenPin;
  int lookClosedPin;

  bool doorClosed;
  bool lockOpened;
  bool lockClosed;

  int oldState;

  bool toggleState;

  Lock(int motorPin1, int motorPin2, bool dir, int doorClosedPin, int lockOpenPin, int lookClosedPin)
    : Service::LockMechanism() {  // constructor() method
    currentState = new Characteristic::LockCurrentState();
    targetState = new Characteristic::LockTargetState(1, true);

    this->motorPin1 = motorPin1;
    this->motorPin2 = motorPin2;
    this->dir = dir;
    this->doorClosedPin = doorClosedPin;
    this->lockOpenPin = lockOpenPin;
    this->lookClosedPin = lookClosedPin;

    this->doorClosed = false;
    this->lockOpened = false;
    this->lockClosed = false;

    this->toggleState = false;

    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);

    pinMode(doorClosedPin, INPUT_PULLUP);
    pinMode(lockOpenPin, INPUT_PULLUP);
    pinMode(lookClosedPin, INPUT_PULLUP);
  }

  boolean update() {
    /**
    UNLOCK (0)
    LOCK (1) 
    **/

    // Serial.print("Target Lock State new val");
    // Serial.println(targetState->getNewVal());
    // Serial.print("Target Lock State val");
    // Serial.println(targetState->getVal());

    // switch (targetState->getNewVal()) {
    //   case 0:  //unlock
    //     // Serial.print("UNLOCK");
    //     // currentState->setVal(UNLOCK);
    //     break;
    //   case 1:  //lock
    //     // Serial.print("LOCK");
    //     // currentState->setVal(LOCK);
    //     break;
    //   default:
    //     // Serial.print("UNKNOWN");
    //     // currentState->setVal(UNKNOWN);
    //     break;
    // }

    // Serial.println();

    return (true);  // return true
  }

  void stopMotor() {
    // Serial.println("stopMotor");
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
  }

  void readSwitches() {
    this->doorClosed = digitalRead(doorClosedPin) == HIGH;
    this->lockOpened = digitalRead(lockOpenPin) == HIGH;
    this->lockClosed = digitalRead(lookClosedPin) == LOW;
    // Serial.printf("doorClose:%d lockOpen:%d lockClose:%d\n", this->doorClosed, this->lockOpened, this->lockClosed);
  }

  void readSwitches(char inp) {
    Serial.print("readSwitches inp ");
    switch (inp) {
      case 'q':
        Serial.print("case q ");
        this->doorClosed ^= 1;
        break;
      case 'w':
        Serial.print("case w ");
        this->lockOpened ^= 1;
        break;
      case 'e':
        Serial.print("case e ");
        this->lockClosed ^= 1;
        break;
      case 'a':
        Serial.print("case a ");
        this->toggleTargetState();
        break;
      default:
        break;
    }

    Serial.printf("doorClose:%d lockOpen:%d lockClose:%d\n", this->doorClosed, this->lockOpened, this->lockClosed);
  }

  int getLockStateEncoded() {
    int x = 0;
    x |= this->doorClosed;
    x <<= 1;
    x |= this->lockOpened;
    x <<= 1;
    x |= this->lockClosed;
    return (x);
  }

  int getState(int stateEncoded) {
    /**
    UNLOCKED (0)
    LOCKED (1) 
    JAMMED (2) 
    UNKNOWN (3)
    **/
    int state;
    // Serial.print("Current Lock State ");

    switch (stateEncoded) {
      case 0:  //open & half-unlocked
        // Serial.print("UNLOCKED");
        state = UNLOCKED;
        break;
      case 1:  //open & locked
        // Serial.print("JAMMED");
        state = JAMMED;
        break;
      case 2:  //open & fully-unlocked
        // Serial.print("UNLOCKED");
        state = UNLOCKED;
        break;
      case 3:  //open & unknown
        // Serial.print("UNKNOWN");
        state = UNKNOWN;
        break;
      case 4:  //close & half-unlocked
        // Serial.print("UNLOCKED");
        state = UNLOCKED;
        break;
      case 5:  //close & locked
        // Serial.print("LOCKED");
        state = LOCKED;
        break;
      case 6:  //close & fully-unlocked
        // Serial.print("UNLOCKED");
        state = UNLOCKED;
        break;
      case 7:  //close & unknown
        // Serial.print("UNKNOWN");
        state = UNKNOWN;
        break;
      default:
        // Serial.print("UNKNOWN");
        state = UNKNOWN;
        break;
    }

    // Serial.println();
    return (state);
  }

  void updateCurrentState(int state) {
    currentState->setVal(state);
  }

  void toggleTargetState() {
    this->toggleState = true;
  }

  void rotateMotor(bool forward) {
    if (forward == this->dir) {
      digitalWrite(motorPin1, HIGH);
      digitalWrite(motorPin2, LOW);
    } else {
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);
    }
  }

  void lockUnlockTask() {
    while (true) {
      int motorState = 0;  //0-stop 1-forward 2-reverse
      this->readSwitches();

      if (this->toggleState) {
        targetState->setVal(targetState->getVal() ^ 1);
        this->toggleState = false;
        // Serial.print("toggleTargetState ");
        // Serial.println(targetState->getVal());
      }

      int currentEncodedState = this->getLockStateEncoded();
      int ts = targetState->getVal();
      int cs = currentState->getVal();

      // Serial.printf("Current State:%d, Target State:%d\n", cs, ts);

      if (ts != cs) {
        switch (ts) {
          case UNLOCK:
            if ((!this->lockOpened) && (cs < 2)) motorState = 2;
            break;
          case LOCK:
            if ((currentEncodedState == 4) || (currentEncodedState == 6)) motorState = 1;
            break;
          default:
            break;
        }
      } else if ((ts == UNLOCK) && !this->lockOpened) {
        motorState = 2;
      }


      if (this->oldState != currentEncodedState) {
        this->updateCurrentState(this->getState(currentEncodedState));
        this->oldState = currentEncodedState;
      }

      switch (motorState) {
        case 1:
          this->rotateMotor(true);
          break;
        case 2:
          this->rotateMotor(false);
          break;
        default:
          this->stopMotor();
          break;
      }
    // delay(1000);
    }
  }
};