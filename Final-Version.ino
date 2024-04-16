#include "funshield.h"
constexpr int displayDigits = 4;

constexpr int LedPins[] = { led4_pin, led3_pin, led2_pin, led1_pin };
constexpr int LedCount = sizeof(LedPins) / sizeof(LedPins[0]);

constexpr byte letter = 0b10100001;  //d
constexpr int letterPosition = 2;
constexpr int throwPosition = 3;

constexpr int diceOffset = 0;
constexpr int throwsOffset = 1;
constexpr int maxFigure = 10;

constexpr int dice[] = { 4, 6, 8, 10, 12, 20, 100 };
constexpr int diceCount = sizeof(dice) / sizeof(dice[0]);

constexpr int Period = 25;
constexpr int animationPeriod = 150;
constexpr int tresholdPeriod = 900;  //6*150 (jeden cyklus ledek na pocatek)

enum Modes {
  normal,
  config,
} Mode;


class Button {
private:
  int pin;
  int increment;
  int lastState;
  int currentState;
public:
  void setup(int pin, int increment) {//
    this->pin = pin;
    this->increment = increment;
    this->lastState = OFF;
  }
  bool isPressed(int curState) { //
    this->currentState = curState;
    if (this->currentState == ON) {
      return true;
    }

    else {
      return false;
    }
  }

  bool singlePress() { //
    if (currentState != lastState) {
      return true;
    } else {
      return false;
    }
  }
  bool isHeld() {//
    if (currentState == lastState) {
      return true;
    } else {
      return false;
    }
  }
  int cycle(int number, int maximum, int offset) {//
    number += this->increment;
    number = (number % maximum);
    if (number == 0)
      number += offset;
    return number;
  }
  int getPin() { //
    return this->pin;
  }
  void updateState() {//
    this->lastState = this->currentState;
  }
};
class Timer {
private:
  unsigned long lastTime;
  unsigned long timer;
  unsigned long tresholdTimer;
public:
  void setup() {//
    this->lastTime = millis();
    this->timer = 0;
    this->tresholdTimer = 0;
  }
  void setLastTime(unsigned long time) {//
    this->lastTime = time;
  }
  bool measurringPeriod(unsigned long nowTime) {//
    this->timer += nowTime - this->lastTime;
    if (timer >= Period) {
      this->timer -= Period;
      return true;
    } else {
      return false;
    }
  }
  bool measureTreshold(unsigned long nowTime) {//
    this->tresholdTimer += nowTime - this->lastTime;
    if (tresholdTimer >= tresholdPeriod) {
      this->tresholdTimer -= tresholdPeriod;
      return true;
    } else {
      return false;
    }
  }
  void resetTimer() {//
    this->timer = 0;
    this->tresholdTimer = 0;
  }
};

class Brain {
private:
  int throws;
  int dicePosition;
  int state;
  int result;
  int changeProbability;
  int divisor;
public:
  void setup() {//
    this->state = config;
    this->dicePosition = 0;
    this->throws = 1;
    this->divisor = 1;
    this->changeProbability = 0;
  }
  bool displayModeConfig() {//
    if (this->state == config)
      return true;
    else
      return false;
  }
  int getDicePosition() {//
    return dicePosition;
  }
  void setDicePosition(int number) {//
    this->dicePosition = number;
  }
  int getThrows() {//
    return this->throws;
  }
  void setThrows(int number) {//
    this->throws = number;
  }
  bool changeConfig() { //
    if (state == normal) {
      this->state = config;
      return false;
    } else {
      return true;
    }
  }
  bool changeNormal() {//
    if (state == config) {
      this->state = normal;
      return false;
    } else {
      return true;
    }
  }
  void generateResult(bool passedTime, bool passedTreshold) { //
    if (passedTime) {
      this->result = random(this->throws, (dice[this->dicePosition] * this->throws + 1));
      this->result = (this->result + this->changeProbability) / this->divisor;
      if (divisor > 1) {
        this->changeProbability = random((dice[this->dicePosition] * this->throws + 1) / this->divisor, (dice[this->dicePosition] * this->throws + 1));
      }
    } else if (passedTreshold) {
      this->divisor += 1;
    }
  }
  int getResult() {//
    return this->result;
  }
  void resetProbability() {//
    this->divisor = 1;
    this->changeProbability = 0;
  }
};


class Display {
private:
  int position;
public:
  void setup() {//
    this->position = 0;
  }

  int cyclePosition() {//
    this->position += 1;
    this->position = this->position % displayDigits;
    return this->position;
  }
  void displayLetter() {//
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, letter);
    shiftOut(data_pin, clock_pin, MSBFIRST, 1 << (displayDigits - 1 - this->position));
    digitalWrite(latch_pin, HIGH);
    cyclePosition();
  }

  void displayConfig(int digit, int throws) {//
    if (position == letterPosition) {
      displayLetter();
    } else {
      if (position == throwPosition) {
        digit = throws;
      }
      digitalWrite(latch_pin, LOW);
      shiftOut(data_pin, clock_pin, MSBFIRST, digits[digit]);                              //nastaveni cisla
      shiftOut(data_pin, clock_pin, MSBFIRST, 1 << (displayDigits - 1 - this->position));  // nastaveni pozice
      digitalWrite(latch_pin, HIGH);
      cyclePosition();
    }
  }
  void displayDigit(int digit) {//
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, digits[digit]);                              //nastaveni cisla
    shiftOut(data_pin, clock_pin, MSBFIRST, 1 << (displayDigits - 1 - this->position));  // nastaveni pozice
    digitalWrite(latch_pin, HIGH);
    cyclePosition();
  }
  int computeDigit(int value) {//
    int figure = 1;  //10^0
    for (int i = 0; i < this->position; i++) {
      figure = figure * 10;
    }
    int digit = value / figure;  // ziskam cislo co chci na posledni cifre
    digit = digit % 10;          //zbavim se vsech vysich cifer
    return digit;
  }
};

class LEDS {
private:
  int pin;
  unsigned long lastTime;
  unsigned long timer;
  int increase;
  int position;
public:
  void setup() {//
    for (int i = 0; i < LedCount; i++) {
      pinMode(LedPins[i], OUTPUT);
      digitalWrite(LedPins[i], OFF);
    }
    this->increase = 1;
    this->lastTime = millis();
    this->timer = 0;
    this->position = 0;
  }
  void resetLeds() {//
    for (int i = 0; i < LedCount; i++)
      digitalWrite(LedPins[i], OFF);
  }
  void animation(unsigned long time) {//
    this->timer += time - this->lastTime;
    digitalWrite(LedPins[this->position], ON);
    if (timer >= animationPeriod) {
      this->position += this->increase;
      if (position >= (LedCount - 1) || position <= 0) {
        this->increase = this->increase * -1;
      }
      this->timer -= animationPeriod;
      resetLeds();
      digitalWrite(LedPins[this->position], ON);
    }
  }
  void reset() {//
    resetLeds();
    this->increase = 1;
    this->timer = 0;
    this->position = 0;
  }
  void update(unsigned long nowTime) {//
    this->lastTime = nowTime;
  }
};



Button button1;
Button button2;
Button button3;

Brain brain;
Display display;
Timer stopwatch;
LEDS leds;



void setup() {
  pinMode(latch_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(button3_pin, INPUT);
  button1.setup(button1_pin, 0);
  button2.setup(button2_pin, 1);
  button3.setup(button3_pin, 1);
  brain.setup();
  stopwatch.setup();
  leds.setup();
}

void loop() {
  unsigned long time = millis();
//Buttons
  //reading button3
  if (button3.isPressed(digitalRead(button3.getPin()))) {
    if (button3.singlePress()) {
      if (brain.changeConfig()) {
        int num = button3.cycle(brain.getDicePosition(), diceCount, diceOffset);
        brain.setDicePosition(num);
      }
    }
  }
  button3.updateState();

  //reading button2
  if (button2.isPressed(digitalRead(button2.getPin()))) {
    if (button2.singlePress()) {
      if (brain.changeConfig()) {
        int num = button2.cycle(brain.getThrows(), maxFigure, throwsOffset);
        brain.setThrows(num);
      }
    }
  }
  button2.updateState();

  //reading button1
  if (button1.isPressed(digitalRead(button1.getPin()))) {
    if (button1.isHeld()) {
      if (brain.changeNormal()) {
        brain.generateResult(stopwatch.measurringPeriod(time), stopwatch.measureTreshold(time));
        leds.animation(time);
      }
    }
  } else {
    stopwatch.resetTimer();
    leds.reset();
    brain.resetProbability();
  }
  button1.updateState();
  stopwatch.setLastTime(time);
  leds.update(time);

//Display
  //mode configuration
  if (brain.displayModeConfig()) {
    int digit = display.computeDigit(dice[brain.getDicePosition()]);
    display.displayConfig(digit, brain.getThrows());

  //mode normal
  } else {
    int digit = display.computeDigit(brain.getResult());
    display.displayDigit(digit);
  }
}
