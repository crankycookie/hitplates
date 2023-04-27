#include <FastLED.h>

// How many samples to take to verify a hit
// Original value = 512
#define HIT_SAMPLES 512

// How high the sample avg must be to verify a hit (0-1024)
// Original value = 550
#define HIT_THRESHOLD 600

// How many quiet samples to take before counting the next hit
#define COOLDOWN_SAMPLES 200

// How low the sample avg must be before counting the next hit (0-1024)
#define COOLDOWN_THRESHOLD 50

// How loud the initial sound must be before verifying a hit (0-1024)
// Original value = 500
#define ACTIVATION_THRESHOLD 500

// How long the hit LED stays lit in ms
#define HIT_ALERT_TIME_MS 200

// How many leds are in the strip total? (We assume front has 10 leds and back has 10 leds)
#define NUM_LEDS 20

// How bright the LEDs shine (0-255)
#define LED_BRIGHTNESS 80

// LED PIN
#define LEDS_PIN 11

// Enable Serial Debugging
#define DEBUG false

// Amount of starting hit points
#define STARTING_HIT_POINTS 100

// Amount of damage each hit to the front plate does
#define FRONT_PLATE_DAMAGE 10

// Amount of damage each hit to the back plate does
#define BACK_PLATE_DAMAGE 15

// Mic input on pin 14
#define FRONT_PLATE_PIN A0

// Mic input on pin 15
#define BACK_PLATE_PIN A1

// Global Variables
volatile uint32_t hitTime = 0;
volatile int currentHitPoints = 0;
volatile int prevHitPoints = 0;
CRGB leds[NUM_LEDS];

void setup() {
  if(DEBUG) {
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
  }

  pinMode(FRONT_PLATE_PIN, INPUT);
  pinMode(BACK_PLATE_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  FastLED.addLeds<WS2812, LEDS_PIN, GRB>(leds, NUM_LEDS);

  currentHitPoints = STARTING_HIT_POINTS;

  startupAnimation();
}

void loop() {
  deadAnimation();
  hitAnimation();
  checkForHit(FRONT_PLATE_PIN, FRONT_PLATE_DAMAGE);
  checkForHit(BACK_PLATE_PIN, BACK_PLATE_DAMAGE);
}

bool sampleAndCheckIfAboveThreshold(int inputPin, int threshold, int sampleCount) {
  volatile double hitSampleSum = 0;

  // Sum up the next set of SAMPLES
  for (int i = 0; i < sampleCount; i++){
    hitSampleSum = hitSampleSum + analogRead(inputPin);
  }
  
  if(DEBUG){
    Serial.print("Hit reading Average: ");
    Serial.println((hitSampleSum/sampleCount));
  }

  // Check if average is above threshold
  if ( (hitSampleSum/sampleCount) > threshold) {    
    return true;
  }

  return false;
}

void clearLeds(){
  setLEDs("black", 0, NUM_LEDS, NULL);
}

void setLEDs(String color, int startLED, int endLED, int brightLevel){
  int hue = 0;
  int saturation = 255;
  int brightness = LED_BRIGHTNESS;
  if(brightLevel){
    brightness = brightLevel;
  }

  if(color.equals("red")){
    hue = 0;
  } else if(color.equals("orange")){
    hue = 22;
  } else if(color.equals("green")){
    hue = 85;
  } else if(color.equals("black")){
    brightness = 0;
  }
  
  for(int i = startLED; i < endLED; i++){
    leds[i].setHSV( hue, saturation, brightness);
  }
  FastLED.show();
}

void showHpIndicator(){
  if(prevHitPoints != currentHitPoints){
    double health_percent = double(currentHitPoints)/double(STARTING_HIT_POINTS);
    double led_count = double(NUM_LEDS/2);
    int greenLedCount = floor(health_percent * led_count);
    
    //clearLeds();
    setLEDs("green", 0, NUM_LEDS, 1);
    setLEDs("green", 0, greenLedCount, NULL);
    setLEDs("green", NUM_LEDS/2, NUM_LEDS/2+greenLedCount, NULL);
    prevHitPoints = currentHitPoints;
  }
}

void beep(int duration){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(duration);
  digitalWrite(LED_BUILTIN, LOW);
}

void startupAnimation(){
  // Flash 3 times to indicate it has been reset
  for (int i = 0; i < 3; i++){
    setLEDs("red", 0, NUM_LEDS, NULL);
    beep(HIT_ALERT_TIME_MS);
    clearLeds();
    delay(HIT_ALERT_TIME_MS);
  }
}

void deadAnimation(){
  // When dead don't leave this conditional
  if (currentHitPoints <= 0) {
    while(true){
      // flash and beep
      setLEDs("red", 0, NUM_LEDS, NULL);
      beep(HIT_ALERT_TIME_MS*2);
      
      // clear and silence for 300ms
      clearLeds();
      delay(HIT_ALERT_TIME_MS*2);
    }
  }  
}

void hitAnimation(){
  // Show hit indicator if within 200ms of last hit
  if((millis() - hitTime) < HIT_ALERT_TIME_MS){
    digitalWrite(LED_BUILTIN, HIGH);
    setLEDs("orange", 0, NUM_LEDS, NULL);
    prevHitPoints = 0;
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    showHpIndicator();
  }
}

void checkForHit(uint8_t platePin, int damage){
  // Start reading the hit plate if any noise is heard
  if (analogRead(platePin) > ACTIVATION_THRESHOLD){
    if(sampleAndCheckIfAboveThreshold(platePin, HIT_THRESHOLD, HIT_SAMPLES)){
      currentHitPoints -= damage;
      hitTime = millis();
      
      while(sampleAndCheckIfAboveThreshold(platePin, COOLDOWN_THRESHOLD, COOLDOWN_SAMPLES)){
        //noop - stay in loop until we have cooled down
      }
    }
  }
}
