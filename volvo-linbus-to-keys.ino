#include <SoftwareSerial.h>
#include "Keyboard.h"

// https://github.com/zapta/linbus/tree/master/analyzer/arduino
#include "src/lin_frame.h"

//If this is set true, button events will only be logged
bool debug_mode = false;

bool rti_screen_up = true;
bool any_button_was_clicked = false;
unsigned long last_back_btn_click = 0;
unsigned long back_btn_click_count = 0;
unsigned long clicks_to_close_screen = 4;

// Pins we use for MCP2004
#define RX_PIN 10
#define TX_PIN 11
#define CS_PIN 8
// We dont use this pin
// #define FAULT_PIN 14


#define SYN_FIELD 0x55
#define SWM_ID 0x20

SoftwareSerial LINBusSerial(RX_PIN, TX_PIN);

//Lbus = LIN BUS from Car
//Vss = Ground
//Vbb = +12V

// MCP2004 LIN bus frame:
// ZERO_BYTE SYN_BYTE ID_BYTE DATA_BYTES.. CHECKSUM_BYTE

// Volvo V50 2007 SWM key codes

// BTN_NEXT       20 0 10 0 0 EF
// BTN_PREV       20 0 2 0 0 FD
// BTN_VOL_UP     20 0 0 1 0 FE
// BTN_VOL_DOWN   20 0 80 0 0 7F
// BTN_BACK       20 0 1 0 0 F7
// BTN_ENTER      20 0 8 0 0 FE
// BTN_UP         20 1 0 0 0 FE
// BTN_DOWN       20 2 0 0 0 FD
// BTN_LEFT       20 4 0 0 0 FB
// BTN_RIGHT      20 8 0 0 0 F7

#define JOYSTICK_UP 0x1
#define JOYSTICK_DOWN 0x2
#define JOYSTICK_LEFT 0x4
#define JOYSTICK_RIGHT 0x8
#define BUTTON_BACK 0x1
#define BUTTON_ENTER 0x8
#define BUTTON_NEXT 0x10
#define BUTTON_PREV 0x2

// IGN_KEY_ON     50 E 0 F1
byte b, i, n;
LinFrame frame;
unsigned long currentMillis, lastClick, lastRtiWrite;
byte currentButton;
byte currentJoystickButton;
short rtiStep;

#define RTI_INTERVAL 100

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  //Serial1 is Arduino default TX pin connected to RTI screen
  Serial1.begin(2400);

  // Open serial communications to host (PC) and wait for port to open:
  Serial.begin(9600, SERIAL_8E1);
  Serial.println("LIN Debugging begins");

  LINBusSerial.begin(9600);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  // pinMode(FAULT_PIN, OUTPUT);
  // digitalWrite(FAULT_PIN, HIGH);

  frame = LinFrame();
}

void loop() {
  currentMillis = millis();

  // if (Serial.available()) {
  //   read_raspberry_serial()
  // }

  if (LINBusSerial.available()) {
    b = LINBusSerial.read();
    n = frame.num_bytes();

    if (b == SYN_FIELD && n > 2 && frame.get_byte(n - 1) == 0) {
      digitalWrite(LED_BUILTIN, HIGH);
      frame.pop_byte();
      handle_frame();
      frame.reset();
      digitalWrite(LED_BUILTIN, LOW);
    } else if (n == LinFrame::kMaxBytes) {
      frame.reset();
    } else {
      frame.append_byte(b);
    }
  }

  // USE this to debug on the computer
  // if (Serial.available() > 0) {
  //   // read the incoming byte:
  //   byte incomingByte = Serial.read();
  //   // say what you got:
  //   Serial.print("I received: ");
  //   Serial.println(incomingByte, DEC);
  //   closeRtiScreen();
  //   if(incomingByte == 111) {
  //     rti_screen_up = true;
  //   }
  // }
}

void handle_frame() {
  if (frame.get_byte(0) != SWM_ID) {
    return;
  }

  // skip zero values 20 0 0 0 0 FF
  if (frame.get_byte(5) == 0xFF) {
    return;
  }

  if (!frame.isValid()) {
    return;
  }

  if(!lastClick || currentMillis - 300 > lastClick) {
    handle_buttons();
    handle_joystick();
    lastClick = currentMillis;
  }
}

// ########################################
// BUTTONS
// ########################################

//We use this so we can test click_button(BUTTON_NEXT)
void handle_buttons() {
  byte button = frame.get_byte(2);

  if (!button) {
    return;
  }

  click_button(button);
}

void click_button(byte button) {
  pinMode(LED_BUILTIN, OUTPUT);

  switch (button) {
    case BUTTON_BACK:
      if(debug_mode) {
        Serial.println("Home (h)");
      } else {
        Keyboard.write('h');
      }
      break;  
    case BUTTON_ENTER:
      if(debug_mode) {
        Serial.println("Space");
      } else {
          Keyboard.write(' ');
          Keyboard.write(KEY_UP_ARROW);
      }
      break;  
    case BUTTON_NEXT:
      if(debug_mode) {
        Serial.println("Next (n)");
      } else {
        Keyboard.write('n');
      }
      break;
    case BUTTON_PREV:
      if(debug_mode) {
        Serial.println("Prev (v)");
      } else {
        Keyboard.write('v');
      }
      break;
  }

  any_button_was_clicked = true;
}

// ########################################
// JOYSTICK
// ########################################

void handle_joystick() {
  byte button = frame.get_byte(1);
  if (!button) {
    return;
  }
  click_joystick(button);
}

void click_joystick(byte button) {
  pinMode(LED_BUILTIN, OUTPUT);

  switch (button) {
    case JOYSTICK_UP:
      if(debug_mode) {
         //Serial.println("Up");
          Serial.println("Left");
      } else {
        // Keyboard.write(KEY_UP_ARROW);
        Keyboard.write(KEY_LEFT_ARROW);
      }
      break;
    case JOYSTICK_DOWN:
      if(debug_mode) {
        //Serial.println("Down");
        Serial.println("Right");
      } else {
        // Keyboard.write(KEY_DOWN_ARROW);
        Keyboard.write(KEY_RIGHT_ARROW);
      }
      break;
    case JOYSTICK_LEFT:
      if(debug_mode) {
        Serial.println("Left");
      } else {
        Keyboard.write(KEY_LEFT_ARROW);
      }
      break;
    case JOYSTICK_RIGHT:
      if(debug_mode) {
        Serial.println("Right");
      } else {
        Keyboard.write(KEY_RIGHT_ARROW);
      }
      break;
  }
}