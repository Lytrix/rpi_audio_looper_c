// Button and led pin numbers per ctrl module 
byte buttonPin[4][2] = {
  {0, 1},
  {2, 3},
  {4, 5},
  {6, 7}
};

byte ledPin[4][2] = {
  {8, 9},
  {10, 11},
  {12, 15},
  {14, 21}  // rewired pin 13 to 21 due to also turning reset led on on teensy board when used
};

// Led, buttonstate variables
byte ledState[4][2];
byte lastButtonState[4][2];
byte currentButtonState[4][2];

// Debounce variables
byte lastFlickerableState[4][2];  // the previous flickerable state from the input pin
const int DEBOUNCE_DELAY = 50;   // the debounce time; increase if the output flickers

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled

// Short, long press variables
const int SHORT_PRESS_TIME = 500; // 500 milliseconds
const int LONG_PRESS_TIME  = 1000; // 1000 milliseconds

unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

void buttonState(char *type, char *track, unsigned int ctrl, unsigned int btn) {
  // If the switch/button changed, due to noise or pressing:
  if (currentButtonState[ctrl][btn] != lastFlickerableState[ctrl][btn]) {
    // reset the debouncing timer
    lastDebounceTime = millis();
    // save the the last flickerable state
    lastFlickerableState[ctrl][btn] = currentButtonState[ctrl][btn];
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    lastButtonState[ctrl][btn] = lastFlickerableState[ctrl][btn];      // save the last state
    currentButtonState[ctrl][btn] = digitalRead(buttonPin[ctrl][btn]); // read new state

    if(lastButtonState[ctrl][btn] == HIGH && currentButtonState[ctrl][btn] == LOW) {
      pressedTime = millis();
      isPressing = true;
      isLongDetected = false;
      // invert state of LED
      ledState[ctrl][btn] = !ledState[ctrl][btn];
      digitalWrite(ledPin[ctrl][btn], ledState);
      if (type == "mute") {
        if (ledState[ctrl][btn] == 0) {
          Serial.print("u");
          Serial.print(track);
          Serial.println("00");
        } else 
        if (ledState[ctrl][btn] == 1) {
          Serial.print("m");
          Serial.print(track);
          Serial.println("00");
        }
      } else
      if (type == "recplay") {
        
        if (ledState[ctrl][btn] == 0) {
          Serial.print("p");
          Serial.print(track);
          Serial.println("00r");
        } else 
        if (ledState[ctrl][btn] == 1) {
          Serial.print("r");
          Serial.print(track);
          Serial.print("g1");
        }
      }  
    } else 
    if (lastButtonState[ctrl][btn] == LOW && currentButtonState[ctrl][btn] == HIGH) {
      releasedTime = millis();
      isPressing = false;
      long pressDuration = releasedTime - pressedTime;

      if( pressDuration < SHORT_PRESS_TIME ) {
        // Serial.println("A short press is detected");
      }
    }
    if (isPressing == true && isLongDetected == false) {
      long pressDuration = millis() - pressedTime;
      
      if( pressDuration > LONG_PRESS_TIME ) {
        //Serial.println("A long press is detected");
        isLongDetected = true;
        Serial.print("o");
        Serial.print(track);
        Serial.println("00");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  for (int c = 0; c <= 3; c++) {  
    for (int b = 0; b <= 1; b++) {
      pinMode(buttonPin[c][b], INPUT_PULLUP); // set arduino pin to input pull-up mode
      currentButtonState[c][b] = digitalRead(buttonPin[c][b]);
      pinMode(ledPin[c][b], OUTPUT);          // set arduino pin to output mode
      ledState[c][b] = digitalRead(ledPin[c][b]);
    }
  }
  // Serial.print("started");
}

void loop() {
  buttonState("recplay", "00", 0, 0);
  buttonState("mute", "00", 0, 1);
  buttonState("recplay", "01", 1, 0);
  buttonState("mute", "01", 1, 1);
  buttonState("p0000", "02", 2, 0);
  buttonState("s0000", "02", 2, 1);
  buttonState("r00g1", "03", 3, 0);
  buttonState("o0000", "03", 3, 1);
  // Serial.println("q0000");
}