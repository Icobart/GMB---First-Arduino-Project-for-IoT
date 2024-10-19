#include <Arduino.h>
#include <avr/sleep.h>/*
#include "func.h"*/
#include "TimerOne.h"
#include <EnableInterrupt.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define NUM_OF_BUTTON 4
#define NUM_OF_LED 4
#define LEDPULSE 10
#define POTENTIOMETER A0
#define LCD_SDA A4
#define LCD_SCL A5
#define SCORE_INCREMENT 100
#define MIN_T1 1000
#define FACTOR 0.9
#define DEBOUNCE_DELAY 50

typedef enum {
    SLEEP,
    WAITING_START,
    PREGAME,
    GAME_LOOP,
    GAME_OVER
} gameStatus;

int pinToRead[NUM_OF_BUTTON] = {2, 3, 4, 5};
int pinToWrite[NUM_OF_BUTTON] = {6, 7, 8, 9};

int buttons[NUM_OF_BUTTON];
int greenLeds[NUM_OF_LED];

int fadeAmount;
int currIntensity;
int difficultyLevel;
int score;
bool correct=false;
int targetNumber;
int composedNumber;
int previousButtonStates[NUM_OF_BUTTON] = {0};
unsigned long roundStartTime;
unsigned long T1 = 10000;
unsigned long startTime = millis();

gameStatus state;

LiquidCrystal_I2C lcd(0x27, 20, 4);

// Function prototypes
void waitingStart(); // Function to handle the waiting start state
void preGame(); // Function to handle the pre-game state
void gameLoop(); // Function to handle the game loop state
void gameOver(); // Function to handle the game over state
void wakeUp(); // Function to wake up the microcontroller from sleep mode
void setIdle(); // Function to put the microcontroller to sleep
void selectDifficultyLevel(); // Function to select the difficulty level
void fadingLeds(); // Function to fade the LED Ls
void lcdInitialPrint(); // Function to print the initial message on the LCD
void readButtons(int *pinToRead, int *buttons, int size); // Function to read the buttons
void writeDigitalLeds(int *pinToWrite, int *greenLeds, int size); // Function to write to the digital green LEDs
void resetGameState(); // Function to reset the game state
void displayRandomNumber(); // Function to display a random number on the LCD
void updateLedsBasedOnButtons(); // Function to update the LEDs based on the button presses
void checkCorrectNumber(); // Function to check if the player has composed the correct number
void handleCorrectNumber(); // Function to handle the case when the player has composed the correct number
void handleGameOver(); // Function to handle the game over state

void setup()
{   
    state = gameStatus::WAITING_START;
    for (int i = 0; i < NUM_OF_BUTTON; i++)
    {
        pinMode(pinToRead[i], INPUT);
    }
    for (int i = 0; i < NUM_OF_LED; i++)
    {
        pinMode(pinToWrite[i], OUTPUT);
    }
    currIntensity = 0;
    fadeAmount = 5;
    pinMode(LEDPULSE, OUTPUT);
    lcd.init();
    lcd.backlight();
    lcd.display();
    Serial.begin(9600);
}

void loop()
{
    readButtons(pinToRead, buttons, NUM_OF_BUTTON);
    switch (state)
    {
    case gameStatus::WAITING_START:
        waitingStart();
        break;
    case gameStatus::PREGAME:
        preGame();
        break;
    case gameStatus::GAME_LOOP:
        gameLoop();
        break;
    case gameStatus::GAME_OVER:
        gameOver();
        break;
    case gameStatus::SLEEP:
        setIdle();
        break;
    default:
        break;
    }
}

void waitingStart()
{
    lcdInitialPrint();
    if (buttons[0] == 1)
    {
        state = gameStatus::PREGAME;
    }
    selectDifficultyLevel();
    fadingLeds();
    if (millis() - startTime >= 10000) {
        state = gameStatus::SLEEP;
    }
}

void preGame()
{
    resetGameState();
    analogWrite(LEDPULSE, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Go!");
    delay(2000); // Display the message for 2 seconds
    lcd.clear();
    score = 0;
    state = gameStatus::GAME_LOOP;
    correct = false;
    roundStartTime = millis();
    displayRandomNumber();
}

void gameLoop()
{
    updateLedsBasedOnButtons();
    checkCorrectNumber();
    if (correct && millis() - roundStartTime < T1) {
        handleCorrectNumber();
    } else if (!correct && millis() - roundStartTime >= T1) {
        state = gameStatus::GAME_OVER;
    }
}

void gameOver()
{
    handleGameOver();
    resetGameState();
    startTime = millis();
    state = gameStatus::WAITING_START;
}

void wakeUp()
{
    for (int i = 0; i < NUM_OF_BUTTON; i++)
    {
        disableInterrupt(pinToRead[i]);
    }
    startTime = millis();
    state = gameStatus::WAITING_START;
    lcd.display();
    lcd.backlight();
}

void setIdle()
{   
    analogWrite(LEDPULSE, 0); // Turn off the LED pulse

    lcd.noDisplay(); // Turn off the LCD display
    lcd.noBacklight(); // Turn off the LCD backlight

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    for (int i = 0; i < NUM_OF_BUTTON; i++)
    {
        enableInterrupt(pinToRead[i], wakeUp, RISING);
    }
    sleep_mode();

    sleep_disable();
}

void selectDifficultyLevel()
{
    int potValue = analogRead(POTENTIOMETER);
    difficultyLevel = map(potValue, 0, 1023, 1, 4);
    T1 = 10000 / difficultyLevel;
}

void fadingLeds()
{
    analogWrite(LEDPULSE, currIntensity);   
    currIntensity = currIntensity + fadeAmount;
    if (currIntensity == 0 || currIntensity == 255)
    {
        fadeAmount = -fadeAmount;
    }
}

void lcdInitialPrint()
{
    lcd.setCursor(0, 0);
    lcd.print("Welcome to GMB!");
    lcd.setCursor(0, 1);
    lcd.print("Press B1 to Start");
}

void readButtons(int *pinToRead, int *buttons, int size)
{
    static int lastButtonState[NUM_OF_BUTTON] = {LOW};
    static unsigned long lastDebounceTime[NUM_OF_BUTTON] = {0};
    for (int i = 0; i < size; i++)
    {
        int reading = digitalRead(pinToRead[i]);
        if (reading != lastButtonState[i])
        {
            lastDebounceTime[i] = millis();
        }
        if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY)
        {
            buttons[i] = reading;
        }
        lastButtonState[i] = reading;
    }
}

void writeDigitalLeds(int *pinToWrite, int *greenLeds, int size)
{
    for (int i = 0; i < size; i++)
    {
        digitalWrite(pinToWrite[i], greenLeds[i]);
    }
}

void resetGameState()
{
    // Reset every state to 0
    for (int i = 0; i < NUM_OF_BUTTON; i++) {
        previousButtonStates[i] = 0;
        buttons[i] = 0;
    }
    // Turn off all LEDs
    for (int i = 0; i < NUM_OF_LED; i++) {
        greenLeds[i] = 0;
        digitalWrite(pinToWrite[i], LOW);
    }
    writeDigitalLeds(pinToWrite, greenLeds, NUM_OF_LED);
}

void displayRandomNumber()
{
    // Generate a random number between 0 and 15
    randomSeed(analogRead(12)); // Seed the random number generator with a noise value
    targetNumber = random(0, 16);

    // Display the random number on the LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Number: ");
    lcd.print(targetNumber);
}

void updateLedsBasedOnButtons()
{
    // Update LEDs based on button presses
    for (int i = 0; i < NUM_OF_BUTTON; i++) {
        if (buttons[i] == HIGH && previousButtonStates[i] == LOW) {
            greenLeds[i] = !greenLeds[i]; // Toggle the LED state
        }
        previousButtonStates[i] = buttons[i]; // Update the previous state
    }
    writeDigitalLeds(pinToWrite, greenLeds, NUM_OF_LED);
}

void checkCorrectNumber()
{
    // Check if the player has composed the correct number
    composedNumber = 0;
    for (int i = 0; i < NUM_OF_BUTTON; i++) {
        composedNumber |= (greenLeds[i] << i); // Compose the number based on the LED states
    }

    if (composedNumber == targetNumber) {
        correct = true;
    }
}

void handleCorrectNumber()
{
    // Handle the case when the player has composed the correct number
    score += SCORE_INCREMENT; // Increment the score
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("GOOD! Score: ");
    lcd.print(score);
    delay(2000); // Display the message for 2 seconds

    // Reduce the time T1 by some factor FACTOR
    T1 = max(T1 * FACTOR, MIN_T1); // Ensure T1 does not go below a minimum value
    correct = false;
    roundStartTime = millis();
    resetGameState();
    displayRandomNumber();
}

void handleGameOver()
{
    // Handle the game over state
    digitalWrite(LEDPULSE, HIGH); // Turn on the LED pulse
    delay(1000); // Delay for 1 second
    digitalWrite(LEDPULSE, LOW); // Turn off the LED pulse

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game Over");
    lcd.setCursor(0, 1);
    lcd.print("Final Score: ");
    lcd.print(score);
    delay(10000); // Display the message for 10 seconds
    resetGameState();
    startTime = millis();
    state = gameStatus::WAITING_START;
}