#include <Arduino.h>
#include <RPi_Pico_TimerInterrupt.h>
#include <SPI.h>
#include <TMCStepper.h>

#define EN_PIN 6    // Enable
#define DIR_PIN 9   // Direction
#define STEP_PIN 8  // Step
#define CS_PIN 5    // Chip select
#define SW_MOSI 3   // Software Master Out Slave In (MOSI)(SDI)
#define SW_MISO 4   // Software Master In Slave Out (MISO) (SDO)
#define SW_SCK 2    // Software Slave Clock (SCK)
#define R_SENSE 0.075f

// Setup interval between switching the LED on or off
#define FLASH_INTERVAL_US_SLOW 500000
#define FLASH_INTERVAL_US_FAST 100000
#define STEP_INTERVAL_US 300
#define OTHER_TIMER_MS 5000

// TMC5160Stepper driver(CS_PIN, R_SENSE, SW_MOSI, SW_MISO, SW_SCK);
TMC5160Stepper driver(CS_PIN, R_SENSE);

// Initialise Timers
RPI_PICO_Timer slowFlashTimer(0);
RPI_PICO_Timer fastFlashTimer(1);
RPI_PICO_Timer stepperTimer(2);
RPI_PICO_Timer otherTimer(3);

// Global Variables
bool fastFlash = true;
bool fastFlash2 = true;
bool LED_state = false;
bool direction = true;
bool lastStepState = 0;

// Timer interrupt service routines
// Flashing callbacks
bool flash_LED_callback_SLOW(struct repeating_timer *t) {
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
    return true;
}
bool flash_LED_callback_FAST(struct repeating_timer *t) {
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
    return true;
}
// Continuous rotation stepper callback
bool stepper_movement_callback(struct repeating_timer *t) {
    lastStepState = !lastStepState;
    digitalWrite(STEP_PIN, lastStepState);
    return true;
}
// Other timer callback (currently used for switching blink speed and stepper direction)
bool other_timer_callback(struct repeating_timer *t) {
    fastFlash2 = !fastFlash2;
    return true;
}

// Serial and stepper initialisation on core0
void setup() {
    // Initialise serial
    Serial.begin(9600);

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // initialise motor pins
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, LOW);  // Enable driver in hardware

    // Setup hardware SPI
    SPI.setRX(SW_MISO);
    SPI.setCS(CS_PIN);
    SPI.setSCK(SW_SCK);
    SPI.setTX(SW_MOSI);
    SPI.begin();

    // Initialise driver
    driver.begin();           //  SPI: Init CS pins and possible SW SPI pins
    driver.toff(5);           // Enables driver in software
    driver.rms_current(500);  // Set motor RMS current
    driver.microsteps(4);     // Set microsteps to 1/16th

    // Setup LED flashing interrupts
    slowFlashTimer.attachInterruptInterval(FLASH_INTERVAL_US_SLOW, flash_LED_callback_SLOW);
    fastFlashTimer.attachInterruptInterval(FLASH_INTERVAL_US_FAST, flash_LED_callback_FAST);
    fastFlashTimer.stopTimer();

    // Setup stepper interrupt
    stepperTimer.attachInterruptInterval(STEP_INTERVAL_US, stepper_movement_callback);

    // Setup other timer
    otherTimer.attachInterruptInterval(OTHER_TIMER_MS * 1000, other_timer_callback);
}
void loop() {
    delay(5000);  // Wait 5 seconds before switching
    if (fastFlash) {
        // Stop slow flash and start fast flash
        driver.shaft(true);
        slowFlashTimer.stopTimer();
        fastFlashTimer.restartTimer();
        Serial.println("Fast flash. Timer0 disabled Timer1 enabled");
    } else {
        // Stop fast flash and start slow flash
        driver.shaft(false);
        fastFlashTimer.stopTimer();
        slowFlashTimer.restartTimer();
        Serial.println("Slow flash. Timer1 disabled Timer0 enabled");
    }
    // Hello there
    fastFlash = !fastFlash;
}

// Handling the timers happens on core1
void setup1() {
}

// Loop for alternating between the timers
void loop1() {
}
