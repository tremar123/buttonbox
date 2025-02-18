#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Joystick.h>

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
                   34, 0,                  // Button Count, Hat Switch Count
                   false, false, false,    // X, Y, Z Axis
                   false, false, false,    // Rx, Ry, or Rz
                   false, false,           // rudder or throttle
                   false, false, false     // accelerator, brake, or steering
                   );

/*
0 - ignition
1 - start
2-5 - encoder buttons
6-7 - encoder 1 up / down
8-9 - encoder 2 up / down
10-11 - encoder 3 up / down
12-13 - encoder 4 up / down
14-18 - toggle switches
19-23 - buttons row 1
24-28 - buttons row 2
29-33 - buttons row 3
*/
// joystick buttons defs
#define IGNITION 0
#define START 1
const int encoderSwitches[4] = {2,3,4,5};
const int encoderUpButtons[4] = {6,8,10,12};
const int encoderDownButtons[4] = {7,9,11,13};

// Can only be used without using "Serial"
Encoder encoders[4] = {
    Encoder (1, 4),
    Encoder (2, 5),
    Encoder (3, 6),
    Encoder (7, 8)
};

// pin defs
#define IGNITION_PIN 0
#define START_PIN 9

#define ENCODER_BUTTON_1_PIN 15

#define SHIFTER_CLK 10
#define SHIFTER_LE 16
#define SHIFTER_DT 14

const int matrixRowOutputs[4] = {18,19,20,21};

int lastIgnitionState = 0;
int lastStartState = 0;

int lastEncoderButtonState[4] = {0,0,0,0};
int lastButtonState[20] = {
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0
};

struct EncoderState {
    int buttonToRelease;
    unsigned long releaseTime;
    long lastPosition;
};

EncoderState encoderStates[4];

void setup() {
    pinMode(IGNITION_PIN, INPUT_PULLUP);
    pinMode(START_PIN, INPUT);
    pinMode(ENCODER_BUTTON_1_PIN, INPUT_PULLUP);

    pinMode(SHIFTER_CLK, OUTPUT);
    pinMode(SHIFTER_LE, OUTPUT);
    pinMode(SHIFTER_DT, INPUT);

    for (int i = 0; i < 4; i++) {
        pinMode(matrixRowOutputs[i], OUTPUT);
    }

    Joystick.begin();
}

void loop() {
    // IGNITION              // invert - pulled up
    int currentIgnitionState = digitalRead(IGNITION_PIN);
    if (currentIgnitionState != lastIgnitionState) {
        Joystick.setButton(IGNITION, currentIgnitionState);
        lastIgnitionState = currentIgnitionState;
    }

    // START
    int currentStartState = digitalRead(START_PIN);
    if (currentStartState != lastStartState) {
        Joystick.setButton(START, currentStartState);
        lastStartState = currentStartState;
    }

    // ENCODER BUTTON 1 // inver - pulled up
    int currentEncoder1 = !digitalRead(ENCODER_BUTTON_1_PIN);
    if (currentEncoder1 != lastEncoderButtonState[0]) {
        Joystick.setButton(encoderSwitches[0], currentEncoder1);
        lastEncoderButtonState[0] = currentEncoder1;
    }

    unsigned long currentTime = millis();
    // all encoder movements
    for (int i = 0; i < 4; i++) {
        long currentPosition = encoders[i].read();
        long deltaPosition = currentPosition - encoderStates[i].lastPosition;

        if (abs(deltaPosition) >= 4)
        {
            if (deltaPosition > 0) {
                Joystick.pressButton(encoderUpButtons[i]);
                encoderStates[i].buttonToRelease = encoderUpButtons[i];
            } else if (deltaPosition < 0) {
                Joystick.pressButton(encoderDownButtons[i]);
                encoderStates[i].buttonToRelease = encoderDownButtons[i];
            }

            encoderStates[i].releaseTime = currentTime + 10;
            encoderStates[i].lastPosition = 0;
            encoders[i].write(0);
        }

        if (encoderStates[i].buttonToRelease != -1 && currentTime >= encoderStates[i].releaseTime) {
            Joystick.releaseButton(encoderStates[i].buttonToRelease);
            encoderStates[i].buttonToRelease = -1;
        }
    }


    // for every row
    for (int i = 0; i < 4; i++) {
        digitalWrite(matrixRowOutputs[i], HIGH);
        // save state
        digitalWrite(SHIFTER_LE, LOW);
        digitalWrite(SHIFTER_LE, HIGH);

        // shift, check state with last state and update
        for (int j = 7; j >= 0; j--) {
            if (j > 4) {
                if (i == 0) {
                    int encoderSw = !digitalRead(SHIFTER_DT);
                    int encoderSwIndex = j - 4;
                    if (encoderSw != lastEncoderButtonState[encoderSwIndex]) {
                        Joystick.setButton(2 + encoderSwIndex, encoderSw);
                        lastEncoderButtonState[encoderSwIndex] = encoderSw;
                    }
                } else {
                    digitalWrite(SHIFTER_CLK, HIGH);
                    digitalWrite(SHIFTER_CLK, LOW);
                    digitalWrite(SHIFTER_CLK, HIGH);
                    digitalWrite(SHIFTER_CLK, LOW);
                    digitalWrite(SHIFTER_CLK, HIGH);
                    digitalWrite(SHIFTER_CLK, LOW);
                    j = 5; // we want to continue at 4 and for loop will j--
                    continue;
                }
            } else {
                int button = digitalRead(SHIFTER_DT);
                int buttonIndex = i*5+j;
                if (button != lastButtonState[buttonIndex]) {
                    Joystick.setButton(14 + buttonIndex, button);
                    lastButtonState[buttonIndex] = button;
                }
            }
            digitalWrite(SHIFTER_CLK, HIGH);
            digitalWrite(SHIFTER_CLK, LOW);
        }

        digitalWrite(matrixRowOutputs[i], LOW);
    }
}
