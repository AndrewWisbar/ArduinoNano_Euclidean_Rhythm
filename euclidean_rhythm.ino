
#include <Adafruit_NeoPixel.h>

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
  	int chSteps = 16;
  	int chHits = 0;
  	int chRot = 0;
    
    int color[3] = {0, 0, 0};
  
  	int posLED;
  
  int steps[MAX_LEDS] = { 0 };
  	int chGate;

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

unsigned long startMillis;
unsigned long currentMillis;
unsigned long period = 125;

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
  
  Serial.begin(16000);
  
  startMillis = millis();
}

void loop()
{ 
  readControls();
  channels[activeChannel].draw(); 
  
  
  currentMillis = millis();
 
  if(currentMillis - startMillis >= period) {
    for(int i = 0; i < NUM_CHANNELS; i++) {
      //Serial.println(i);
      channels[i].update();
    }
    startMillis = currentMillis;
  }
  

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void readControls() {
  channelUpdated = 0;
  period = map(analogRead(RATE_IN), 0, 1023, 50, 5000);

  int checkSteps = map(analogRead(STEPS_IN)/64, 0, 15, 1, 16);
  int checkHits = map(analogRead(HITS_IN)/64, 0, 15, 0, checkSteps);
  int checkRot = map (analogRead(ROT_IN)/64, 0, 15, 0, checkSteps-1);
  
  //If the button is held for more than one loop, 
  //do not execute until the button has been released
  if(digitalRead(CHANNEL_PIN) && buttonReleased) {
    Serial.print("Change Channel");
    Serial.println();
    
    activeChannel++;
    activeChannel %= 4;
    
    lastSteps = checkSteps;
    lastHits = checkHits;
    lastRot = checkRot;
    
    buttonReleased = 0;
    
  } else if(!digitalRead(CHANNEL_PIN)) {
    buttonReleased = 1;
  }  
  
  if(checkSteps != lastSteps) {
    Serial.print("Changing Steps ");
    Serial.print(checkSteps);
    Serial.println();
  	channels[activeChannel].setSteps(checkSteps);
    lastSteps = checkSteps;
  	channelUpdated = 1; 
  }
  
  if(checkHits != lastHits) {
    Serial.print("Changing Hits ");
    Serial.print(checkHits);
    Serial.println();
    channels[activeChannel].setHits(checkHits);
    lastHits = checkHits;
    channelUpdated = 1;
  }
  
  if(checkRot != lastRot) {
    Serial.print("Changing Rot ");
    Serial.print(checkRot);
    Serial.println();
    channels[activeChannel].setRot(checkRot);
    lastRot = checkRot;
    channelUpdated = 1;  
  }
	
  if(channelUpdated) {
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
  if(hits > chSteps) {
    chHits = chSteps;
  }else if(hits < 0) {
    chHits = 0;
  }else {
  	chHits = hits;
  }
}

void Channel::setRot(int rot) {
  if(rot > chSteps) {
    chRot = chSteps;
  }else if(rot < 0) {
    chRot = 0;
  }else {
  	chRot = rot;
  }
}

void Channel::setSteps(int steps) {
  if(steps > MAX_LEDS) {
    chSteps = MAX_LEDS;
  }else if(steps < 0) {
    chSteps = 0;
  }else {
  	chSteps = steps;
  }
}

void Channel::update() {
  digitalWrite(chGate, LOW);
  posLED = (posLED + 1)% chSteps;
  
  if(steps[posLED]) {
    digitalWrite(chGate, HIGH);
  }
}

void Channel::draw() {
  strip.clear();
  
  for(int i = chSteps - 1; i >= 0; i--) {
    if(steps[i]) {
      strip.setPixelColor(15 - i, strip.Color(color[0], color[1], color[2]));
    }
  }
  
  strip.setPixelColor(15 - posLED, strip.Color(255, 255, 255));
    
  strip.show();
}

void Channel::calcEuclid() {
  
  int vals[chSteps];
  for(int i = 0; i < chSteps; i++) {
  	vals[i] = 0;
    steps[i] = 0;
  }
  
  float m = (float)chHits / chSteps;
  for(int x = 0; x < chSteps; x++) {
    vals[x] = floor(m * x);
  	if(x != 0 && vals[x] != vals[x-1]) {
  		steps[(x + chRot) % chSteps] = 1; 	 
 	 }    
  }
	  
  if(chHits > 0) {
    steps[0 + chRot] = 1;
  }
  for(int i = 0; i < chSteps; i++) {
    Serial.print(steps[i]);
  }
  Serial.println();
}