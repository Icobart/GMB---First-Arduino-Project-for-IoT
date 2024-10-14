#include <Arduino.h>
#include <avr/sleep.h>/*
#include "func.h"*/
#include "TimerOne.h"
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

int brightness;
int fadeAmount;
int currIntensity;
int difficultyLevel;
int score;
bool correct=false;
int targetNumber;
unsigned long roundStartTime;
unsigned long T1 = 10000;
unsigned long startTime = millis();

gameStatus state;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void wakeUp();

void setIdle();

void selectDifficultyLevel();

void fadingLeds();

void lcdInitialPrint();

void readButtons(int *pinToRead, int *buttons, int size);

void writeDigitalLeds(int *pinToWrite, int *greenLeds, int size);

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
    /*to do: lcd*/
    lcd.init();
    lcd.backlight();
    Serial.begin(9600);
}

void loop()
{
    readButtons(pinToRead, buttons, NUM_OF_BUTTON);
    switch (state)
    {
    case gameStatus::WAITING_START:
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
        break;
    case gameStatus::PREGAME:
        for (int i = 0; i < NUM_OF_LED; i++) {
            greenLeds[i] = 0;
            digitalWrite(pinToWrite[i], LOW);
        }
        analogWrite(LEDPULSE, 0);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Go!");
        delay(2000); // Display the message for 2 seconds
        lcd.clear();
        score = 0;
        state = gameStatus::GAME_LOOP;
        //setup for the first round
        correct = false;
        roundStartTime = millis();
        // Turn off all LEDs
        for (int i = 0; i < NUM_OF_LED; i++) {
            greenLeds[i] = 0;
            digitalWrite(pinToWrite[i], LOW);
        }

        // Generate a random number between 0 and 15
        targetNumber = random(0, 16);

        // Display the random number on the LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Number: ");
        lcd.print(targetNumber);
        break;
    case gameStatus::GAME_LOOP:

        // Start timing for the round

        readButtons(pinToRead, buttons, NUM_OF_BUTTON);

        // Update LEDs based on button presses
        for (int i = 0; i < NUM_OF_BUTTON; i++) {
            greenLeds[i] = buttons[i];
        }

        // Check if the player has composed the correct number
        int composedNumber = 0;
        for (int i = 0; i < NUM_OF_BUTTON; i++) {
            composedNumber |= (buttons[i] << i);
        }

        if (composedNumber == targetNumber) {
            correct = true;
        }

        if (correct && millis() - roundStartTime < T1) {
            score += SCORE_INCREMENT;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("GOOD! Score: ");
            lcd.print(score);
            delay(2000); // Display the message for 2 seconds

            // Reduce the time T1 by some factor FACTOR
            T1 = max(T1 * FACTOR, MIN_T1); // Ensure T1 does not go below a minimum value
            correct = false;
            roundStartTime = millis();
            // Turn off all LEDs
            for (int i = 0; i < NUM_OF_LED; i++) {
                greenLeds[i] = 0;
                digitalWrite(pinToWrite[i], LOW);
            }

            // Generate a random number between 0 and 15
            targetNumber = random(0, 16);

            // Display the random number on the LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Number: ");
            lcd.print(targetNumber);
        } else if (!correct && millis() - roundStartTime >= T1) {
            state = gameStatus::GAME_OVER;
        }
        break;
    case gameStatus::GAME_OVER:
        digitalWrite(LEDPULSE, HIGH);
        delay(1000);
        digitalWrite(LEDPULSE, LOW);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Game Over");
        lcd.setCursor(0, 1);
        lcd.print("Final Score: ");
        lcd.print(score);
        delay(10000);
        startTime = millis();
        state = gameStatus::WAITING_START;
        break;
    case gameStatus::SLEEP:
        setIdle();
        break;
    default:
        break;
    }

    writeDigitalLeds(pinToWrite, greenLeds, NUM_OF_LED);
}

void wakeUp()
{
    sleep_disable();
    for (int i = 0; i < NUM_OF_BUTTON; i++)
    {
        detachInterrupt(digitalPinToInterrupt(pinToRead[i]));
    }
    startTime = millis();
    state = gameStatus::WAITING_START;
}

void setIdle()
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    for (int i = 0; i < NUM_OF_BUTTON; i++)
    {
        attachInterrupt(digitalPinToInterrupt(pinToRead[i]), wakeUp, RISING);
    }
    sleep_mode();
}

void selectDifficultyLevel()
{
    int potValue = analogRead(POTENTIOMETER);
    difficultyLevel = map(potValue, 0, 1023, 1, 4);
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
    lcd.print("System Ready");
    lcd.setCursor(0, 1);
    lcd.print("Press Button");
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
