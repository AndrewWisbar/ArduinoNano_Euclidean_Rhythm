
#include <Adafruit_NeoPixel.h>

const  uint16_t t_load = 0;

//General Symbolic COnstants
#define MAX_LEDS 16
#define NUM_CHANNELS 4
#define BRIGHT 10

//Digital Outputs
#define LED_PIN  2
#define GATE1_PIN 3
#define GATE2_PIN 4
#define GATE3_PIN 5
#define GATE4_PIN 6


//Analog Inputs
#define RATE_IN 0
#define STEPS_IN 1
#define HITS_IN 2
#define ROT_IN 3

//Digital Inputs
#define CHANNEL_PIN 7

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

int chColors[][3] = {{115, 171, 38}, {217, 28, 154}, {48, 138, 207}, {235, 167, 21}};

Channel channels[] = {Channel(GATE1_PIN, chColors[0]), Channel(GATE2_PIN, chColors[1]), Channel(GATE3_PIN, chColors[2]), Channel(GATE4_PIN, chColors[3])};

Adafruit_NeoPixel strip(MAX_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(CHANNEL_PIN, INPUT);

  strip.begin();
  strip.setBrightness(BRIGHT);
  strip.show();

  Serial.begin(9600);

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

ISR(TIMER1_COMPA_vect) {
  state = !state; // state is used to control the gate length
  for (int i = 0; i < NUM_CHANNELS; i++) {
    channels[i].update();
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void readControls() {
  channelUpdated = 0;
  period = map(analogRead(RATE_IN) / 4, 0, 255, 2500, 65535) / 2;
  OCR1A = period;
  int checkSteps = map(analogRead(STEPS_IN) / 64, 0, 15, 1, 16);
  int checkHits = map(analogRead(HITS_IN) / 64, 0, 15, 0, checkSteps);
  int checkRot = map (analogRead(ROT_IN) / 64, 0, 15, 0, checkSteps - 1);

  //If the button is held for more than one loop,
  //do not execute until the button has been released
  if (digitalRead(CHANNEL_PIN) && buttonReleased) {
    //Serial.print("Change Channel");
    //Serial.println();

    activeChannel++;
    activeChannel %= 4;

    lastSteps = checkSteps;
    lastHits = checkHits;
    lastRot = checkRot;

    buttonReleased = 0;

  }
  else if (!digitalRead(CHANNEL_PIN)) {
    buttonReleased = 1;
  }

  if (checkSteps != lastSteps) {
    //Serial.print("Changing Steps ");
    //Serial.print(checkSteps);
    //Serial.println();
    channels[activeChannel].setSteps(checkSteps);
    lastSteps = checkSteps;
    channelUpdated = 1;
  }

  if (checkHits != lastHits) {
    //Serial.print("Changing Hits ");
    //Serial.print(checkHits);
    //Serial.println();
    channels[activeChannel].setHits(checkHits);
    lastHits = checkHits;
    channelUpdated = 1;
  }

  if (checkRot != lastRot) {
    //Serial.print("Changing Rot ");
    //Serial.print(checkRot);
    //Serial.println();
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
