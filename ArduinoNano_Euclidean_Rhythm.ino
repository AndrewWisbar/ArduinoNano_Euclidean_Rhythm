
#include <Adafruit_NeoPixel.h>

const  uint16_t t_load = 0;

//General Symbolic COnstants
#define MAX_LEDS        16
#define NUM_CHANNELS     4
#define BRIGHT          10

//Digital Outputs
#define LED_PIN          2
#define GATE1_PIN        3
#define GATE2_PIN        4
#define GATE3_PIN        5
#define GATE4_PIN        6

//Analog Inputs
#define RATE_IN         A0
#define STEPS_IN        A1
#define HITS_IN         A2
#define ROT_IN          A3

//Digital Inputs
#define CHANNEL_PIN      7

//Rotary Encoder
#define CLK              9
#define DT              10

//Seven Segment Display Pins
/*
#define D_PIN1 8
#define D_PIN2 9
#define D_PIN3 10

#define SEG1 11
#define SEG2 12
#define SEG3 13
#define SEG4 A3
#define SEG5 A4
#define SEG6 A5
#define SEG7 A6
*/

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Channel {
  private:
    // Control Settings
    int chSteps = MAX_LEDS;
    int chHits = 0;
    int chRot = 0;

    int color[3] = {0, 0, 0};

    int posLED;         // Currently active LED index

    uint16_t steps = 0; // Bit Map for active steps
    int chGate;         // Output Pin number

  public:

    Channel(int, int[3]);

    int getHits();
    int getSteps();
    int getRot();

    void setHits(int);
    void setSteps(int);
    void setRot(int);

    void update();
    void draw();

    void calcEuclid();
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

unsigned long period = 7812 / 2; // slightly faster than 1/16 notes 120bpm
bool state = 0;

int lastSteps = 0;
int lastHits = 0;
int lastRot = 0;

int activeChannel = 0;
bool buttonReleased = 1;
bool channelUpdated = 0;

bool CLKState;
bool lastCLKState;

int chColors[][3] = {{72, 163, 15}, {168, 10, 10}, {3, 21, 153}, {189, 154, 0}};

Channel channels[] = {Channel(GATE1_PIN, chColors[0]), Channel(GATE2_PIN, chColors[1]), Channel(GATE3_PIN, chColors[2]), Channel(GATE4_PIN, chColors[3])};

Adafruit_NeoPixel strip(MAX_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup()
{
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(CHANNEL_PIN, INPUT);
  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(BRIGHT);
  strip.show();
  /*
  byte digitPins[] = {D_PIN1, D_PIN2, D_PIN3};
  byte segPins[] = {SEG1, SEG2, SEG3, SEG4, SEG5, SEG6, SEG7};
  
  sevseg.begin(COMMON_ANODE, 3, digitPins, segPins, false, false, false, true);
  */
  // Reset Timer1 control reg a
  TCCR1A = 0;

  TCCR1B &= ~(1 << WGM13);
  TCCR1B |= (1 << WGM12);
  // Prescaler of 256
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B &= ~(1 << CS10);

  //set compare value
  TCNT1 = t_load;
  OCR1A = period;

  //Enable Timer1 compare interrupt
  TIMSK1 = (1 << OCIE1A);

  sei();
}

void loop()
{
  readControls();
  channels[activeChannel].draw();
}

/**
 * Timer interupt to control channel updates and gates
 * 
 * Called twice per step, once to potentially open the gate, and once to close it.
 */
ISR(TIMER1_COMPA_vect) {
  state = !state; // state is used to control the gate length relative to the rate of the module
  for (int i = 0; i < NUM_CHANNELS; i++) {
    channels[i].update();
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void readControls() {
  channelUpdated = 0;
  
  CLKState = digitalRead(CLK);
  if(CLKState == 0 && lastCLKState == 1) {
    Serial.print("Clicked: ");
    if(digitalRead(DT) == CLKState) {
      if(period > 1684) {
        period -= 64;
        Serial.println("Faster");
      }
    }
    else {
      if(period < 32703) {
        period += 64;
        Serial.println("Slower");
      }
    }
  }
  OCR1A = period;
  int checkSteps = map(analogRead(STEPS_IN) / 64, 0, 15, 1, 16);
  int checkHits = map(analogRead(HITS_IN) / 64, 0, 15, 0, checkSteps);
  int checkRot = map (analogRead(ROT_IN) / 64, 0, 15, 0, checkSteps - 1);



  lastCLKState = CLKState;
  //If the button is held for more than one loop,
  //do not execute until the button has been released
  if (!digitalRead(CHANNEL_PIN) && buttonReleased) {
    activeChannel++;
    activeChannel %= 4;

    lastSteps = checkSteps;
    lastHits = checkHits;
    lastRot = checkRot;

    buttonReleased = 0;
  }
  else if (digitalRead(CHANNEL_PIN)) {
    buttonReleased = 1;
  }

  if (checkSteps != lastSteps) {
    channels[activeChannel].setSteps(checkSteps);
    lastSteps = checkSteps;
    channelUpdated = 1;
  }

  if (checkHits != lastHits) {
    channels[activeChannel].setHits(checkHits);
    lastHits = checkHits;
    channelUpdated = 1;
  }

  if (checkRot != lastRot) {
    channels[activeChannel].setRot(checkRot);
    lastRot = checkRot;
    channelUpdated = 1;
  }
 
  if (channelUpdated) {
    channels[activeChannel].calcEuclid();
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Channel::Channel(int pin, int col[3]) {
  chSteps = MAX_LEDS;
  chHits = 0;
  posLED = 0;
  chGate = pin;
  pinMode(pin, OUTPUT);

  color[0] = col[0];
  color[1] = col[1];
  color[2] = col[2];
}

int Channel::getHits() {
  return chHits;
}

int Channel::getSteps() {
  return chSteps;
}

int Channel::getRot() {
  return chRot;
}

void Channel::setHits(int hits) {
  if (hits > chSteps) {
    chHits = chSteps;
  }
  else if (hits < 0) {
    chHits = 0;
  }
  else {
    chHits = hits;
  }
}

void Channel::setRot(int rot) {
  if (rot > chSteps) {
    chRot = chSteps;
  }
  else if (rot < 0) {
    chRot = 0;
  }
  else {
    chRot = rot;
  }
}

void Channel::setSteps(int steps) {
  if (steps > MAX_LEDS) {
    chSteps = MAX_LEDS;
  }
  else if (steps < 0) {
    chSteps = 0;
  }
  else {
    chSteps = steps;
  }
}

/**
 * Update the gate and/or active LED for a channel
 */
void Channel::update() {
  if (!state)
    posLED = (posLED + 1) % chSteps;
  if (steps & (1 << posLED))
    if (!state)
      digitalWrite(chGate, HIGH);
    else
      digitalWrite(chGate, LOW);
  else
    digitalWrite(chGate, LOW);
}

/**
 * Display the steps of the channel on the NeoPixel Ring
 */
void Channel::draw() {
  strip.clear();

  for (int i = chSteps - 1; i >= 0; i--) {
    if (steps & (1 << i)) {
      strip.setPixelColor(15 - i, strip.Color(color[0], color[1], color[2]));
    }
  }

  strip.setPixelColor(15 - posLED, strip.Color(255, 255, 255));

  strip.show();
}

/**
 * Calculate which steps are should be active in this channel
 */
void Channel::calcEuclid() {
  
  int vals[chSteps] = {0};
  steps = 0;                 // clear the steps

  
  float m = (float)chHits / chSteps;
  for (int x = 0; x < chSteps; x++) {
    vals[x] = floor(m * x);
    if (x != 0 && vals[x] != vals[x - 1]) {
      steps += 1 << (x + chRot) % chSteps;
    }
  }

  // The above code will never set the first step as active so we do it here
  if (chHits > 0) {
    steps += 1 << chRot;
  }
}
