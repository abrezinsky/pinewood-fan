#include <Servo.h>
#include "YA_FSM.h"

#define FANPIN    6  // PIN 6
#define DROPSW    18 // PIN A1
#define ENABLESW  19 // PIN A0
#define ENABLELED 9  // PIN 9

#define LEDPENDINGFLASHRATE 1000
#define LEDPRIMEDFLASHRATE 50

#define STARTSPEED 0   // When we're at the gate and waiting for the start
#define DROPSPEED  60  // The time when we're coasting down the hill
#define WARPSPEED  120 // Blast off

#define COASTTIME  1500  //How long do we run at DROPSPEED
#define POWEREDTIME 1000 // HOW long do we run at WARPSPEED

enum State {
  PENDING,  // Waiting for the ENABLESW to be pressed while the DROPSW is LOW
  PRIMED,   // ENABLE was pressed and DROPSW is still low 
  COASTING, // DROPSW is now high and we're coasting down the track before we go under power
  POWERED,  // Motor is running
  DONE      // Motor is done running and we won the race
};
const char * const StateName[] PROGMEM = { "PENDING", "PRIMED", "COASTING", "POWERED", "DONE"};

Servo fan;
YA_FSM stateMachine;
uint8_t currentState;

unsigned long previousMillis = millis();

bool enableButton = false;
bool dropButton   = false;
bool statusLed    = false;
int  fanSpeed     = 0;

void setup() {
  pinMode(ENABLELED, OUTPUT);
  
  pinMode(ENABLESW, INPUT_PULLUP);
  pinMode(DROPSW, INPUT_PULLUP);

  pinMode(FANPIN, OUTPUT);
  fan.attach(FANPIN,1000,2000);

  setupStateMachine();
  currentState = stateMachine.GetState();
}

void loop() {

 // Read button inputs
  enableButton = !digitalRead(ENABLESW);
  dropButton   = !digitalRead(DROPSW);

  // Update State Machine
  stateMachine.Update();

  // Set outputs according to FSM
  digitalWrite(ENABLELED, statusLed);
  fan.write(fanSpeed);
}

void onPendingCb() { 
  fanSpeed = 0;
  if (millis() > previousMillis + LEDPENDINGFLASHRATE) {
    previousMillis = millis();
    statusLed = !statusLed;
  }
}

void onPrimedCb() { 
  fanSpeed = STARTSPEED;
  if (millis() > previousMillis + LEDPRIMEDFLASHRATE) {
    previousMillis = millis();
    statusLed = !statusLed;
  }
}

void onCoastCb() { 
  fanSpeed = DROPSPEED;
  statusLed = false;
}

void onPoweredCb() { 
  fanSpeed = WARPSPEED;
  statusLed = false;
}

void onDoneCb() {
  fanSpeed = 0;
  statusLed = true;
}

// Setup the State Machine
void setupStateMachine() {
  stateMachine.AddState(StateName[PENDING],  0,              nullptr,          onPendingCb,     nullptr);
  stateMachine.AddState(StateName[PRIMED],   0,              nullptr,          onPrimedCb,      nullptr);
  stateMachine.AddState(StateName[COASTING], COASTTIME,      nullptr,          onCoastCb,       nullptr);
  stateMachine.AddState(StateName[POWERED],  POWEREDTIME,    nullptr,          onPoweredCb,     nullptr);
  stateMachine.AddState(StateName[DONE],     0,              nullptr,          onDoneCb,        nullptr);

  stateMachine.AddTransition(PENDING, PRIMED,   [](){return (dropButton == true && enableButton == true); } );
  stateMachine.AddTransition(PRIMED,  PENDING,  [](){return (dropButton == false && enableButton == true); } ); // Get us out of this loaded gun situation
  stateMachine.AddTransition(PRIMED,  COASTING, [](){return (dropButton == false); } );
  stateMachine.AddTimedTransition(COASTING, POWERED);
  stateMachine.AddTimedTransition(POWERED,  DONE);
}