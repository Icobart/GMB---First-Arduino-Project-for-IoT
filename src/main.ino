#include <Arduino.h>
#include <avr/sleep.h>/*
#include "func.h"*/
#include "TimerOne.h"

#define NUM_OF_BUTTON 4
#define NUM_OF_LED 4
#define LEDPULSE 10
#define POTENTIOMETER A0
#define LCD_SDA A4
#define LCD_SCL A5
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
unsigned long startTime = millis();

gameStatus state;

void wakeUp();

void setIdle();

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
    /*to do: potentiometer and lcd*/
    Serial.begin(9600);
}

void loop()
{
    readButtons(pinToRead, buttons, NUM_OF_BUTTON);
    switch (state)
    {
    case gameStatus::WAITING_START:
    // todo : lampeggio+testo (10 sec)
        if (buttons[0] == 1) //start
        {
            state = gameStatus::PREGAME;
        }
        fadingLeds();
        
        if (millis() - startTime >= 10000) {
            state = gameStatus::SLEEP;
        }
        break;
    case gameStatus::PREGAME:
        break;
    case gameStatus::GAME_LOOP:
        break;
    case gameStatus::GAME_OVER:
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
    // This function will be called when an interrupt occurs
    // It should be empty to just wake up the microcontroller
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
    sleep_disable();
    for (int i = 0; i < NUM_OF_BUTTON; i++)
    {
        detachInterrupt(digitalPinToInterrupt(pinToRead[i]));
    }
    startTime = millis();
    state = gameStatus::WAITING_START;
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
    //todo
}

void readButtons(int *pinToRead, int *buttons, int size)
{
    for (int i = 0; i < size; i++)
    {
        buttons[i] = digitalRead(pinToRead[i]);
    }
}

void writeDigitalLeds(int *pinToWrite, int *greenLeds, int size)
{
    for (int i = 0; i < size; i++)
    {
        digitalWrite(pinToWrite[i], greenLeds[i]);
    }
}
