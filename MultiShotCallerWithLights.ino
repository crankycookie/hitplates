#include <FastLED.h>

// How many samples to take to verify a hit
#define HIT_SAMPLES 512

// How high the sample avg must be to verify a hit (0-1024)
#define HIT_THRESHOLD 550

// How many quiet samples to take before counting the next hit
#define COOLDOWN_SAMPLES 200

// How low the sample avg must be before counting the next hit (0-1024)
#define COOLDOWN_THRESHOLD 50

// How loud the initial sound must be before verifying a hit (0-1024)
#define ACTIVATION_THRESHOLD 500

// How long the hit LED stays lit in ms
#define HIT_ALERT_TIME_MS 200

// How many leds are in the strip total? (We assume front has 10 leds and back has 10 leds)
#define NUM_LEDS 20

// LED PIN
#define LEDS_PIN 11

// Enable Serial Debugging
#define DEBUG false

// Global Variables
int STARTING_HIT_POINTS = 10;
int analogIn = A0; // Pin 14
int analogIn2 = A1; // Pin 15
volatile int hitCount = 0;
volatile int hitCount2 = 0;
volatile uint32_t hitTime = 0;
volatile int currentHitPoints = 0;
volatile int prevHitPoints = 0;
CRGB leds[NUM_LEDS];

void setup() {
  if(DEBUG) {
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
  }

  pinMode(analogIn, INPUT);
  pinMode(analogIn2, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Flash 3 times to indicate it has been reset
  for (int i = 0; i < 3; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(HIT_ALERT_TIME_MS);
    digitalWrite(LED_BUILTIN, LOW);
    delay(HIT_ALERT_TIME_MS);
  }

  FastLED.addLeds<WS2812, LEDS_PIN, GRB>(leds, NUM_LEDS);

  currentHitPoints = STARTING_HIT_POINTS;
}

void loop() {
  // When dead don't leave this conditional
  if (currentHitPoints <= 0) {
    // flash and beep
    digitalWrite(LED_BUILTIN, HIGH);
    setHitFlash();
    delay(HIT_ALERT_TIME_MS);
    delay(HIT_ALERT_TIME_MS);
    
    // clear and silence for 300ms
    digitalWrite(LED_BUILTIN, LOW);
    clearLeds();
    FastLED.show();
    delay(HIT_ALERT_TIME_MS);
    delay(HIT_ALERT_TIME_MS);

    // Don't read any other values since we are dead
    return;
  }

  // Show hit indicator if within 200ms of last hit
  if((millis() - hitTime) < HIT_ALERT_TIME_MS){
    digitalWrite(LED_BUILTIN, HIGH);

    setHitFlash();
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    setHpIndicator();
  }

  // Start reading the first hit plate if any noise is heard
  if (analogRead(analogIn) > ACTIVATION_THRESHOLD){
    if(sampleAndCheckIfAboveThreshold(analogIn, HIT_THRESHOLD, HIT_SAMPLES)){
      hitCount++;
      currentHitPoints -= 1;
      hitTime = millis();

      if(DEBUG){
        Serial.print("Front Hit Recorded - Current HP:");
        Serial.print(currentHitPoints);
        Serial.print(" Front Hit Count:");
        Serial.println(hitCount);
      }
      
      while(sampleAndCheckIfAboveThreshold(analogIn, COOLDOWN_THRESHOLD, COOLDOWN_SAMPLES)){
        //noop - stay in loop until we have cooled down
      }
    }
  }

  // Start reading the second hit plate if any noise is heard
  if (analogRead(analogIn2) > ACTIVATION_THRESHOLD){
    if(sampleAndCheckIfAboveThreshold(analogIn2, HIT_THRESHOLD, HIT_SAMPLES)){
      hitCount2++;
      currentHitPoints -= 2;
      hitTime = millis();

      if(DEBUG) {
        Serial.print("Back Hit Recorded - Current HP:");
        Serial.print(currentHitPoints);
        Serial.print(" Back Hit Count:");
        Serial.println(hitCount2);
      }

      while(sampleAndCheckIfAboveThreshold(analogIn2, COOLDOWN_THRESHOLD, COOLDOWN_SAMPLES)){
        //noop - stay in loop until we have cooled down
      }
    }
  }
}

bool sampleAndCheckIfAboveThreshold(int inputPin, int threshold, int sampleCount) {
  volatile double hitSampleSum = 0;

  // Sum up the next set of SAMPLES
  for (int i = 0; i < sampleCount; i++){
    hitSampleSum = hitSampleSum + analogRead(inputPin);
  }

  // Check if average is above threshold
  if ( (hitSampleSum/sampleCount) > threshold) {
    return true;
  }

  return false;
}

void setHitFlash(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Red;
  }
  FastLED.show();
  prevHitPoints = 0;
}

void setHpIndicator(){
  if(prevHitPoints != currentHitPoints){
    double health_percent = double(currentHitPoints)/double(STARTING_HIT_POINTS);
    double led_count = double(NUM_LEDS/2);
    int greenLedCount = floor(health_percent * led_count);
    if(DEBUG) {
        Serial.print("health_precent:");
        Serial.print(health_percent);
        Serial.print(" lit led count: ");
        Serial.println(greenLedCount);
    }
    clearLeds();
    for(int i = 0; i < greenLedCount; i++){

      // Set front LEDs
      leds[i] = CRGB::Green;

      // Set back LEDs
      leds[i+NUM_LEDS/2] = CRGB::Green;
    }
    prevHitPoints = currentHitPoints;
    FastLED.show();
  }
}

void clearLeds(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }
}
