/**
 * @file main.cpp
 * @author Joe
 * @brief Source code for Tidal Force's "Days Worked Without Incident" counter.
 */

// Libs
#include <Arduino.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <RTClib.h>
#include <Bounce2.h>

// Headers
#include "boardPins.h"

// Libs
#include "display.h"

// Objects
Display display(3, LATCH_PIN, CLOCK_PIN, DATA_PIN);
RTC_DS1307 rtc;

// Buttons
const uint8_t buttonInterval = 8;
Bounce2::Button advButton = Bounce2::Button();
Bounce2::Button subButton = Bounce2::Button();
Bounce2::Button rstButton = Bounce2::Button();

void setup()
{
    // Setup serial
    delay(200);
    Serial.begin(115200);
    Serial.println(MOTD);

    // Configure Watchdog
    wdt_enable(WDTO_4S);

    // Pins
    pinMode(STAT_LED, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);

    // Setup RTC
    if (!rtc.begin())
    {
        Serial.println("\x1B[31mCouldn't find RTC!\x1B[0m");
        Serial.flush();
        while (1)
        {
            display.setErr(); // Set error on display
            delay(100);
        }
    }

    if (!rtc.isrunning()) // If the rtc is NOT running
    {
        rtc.adjust(DateTime(SECONDS_PER_DAY * 0)); // Set the time back to 0
    }

    // Attach buttons
    advButton.attach(ADV_PIN, INPUT);
    subButton.attach(SUB_PIN, INPUT);
    rstButton.attach(RST_PIN, INPUT);

    // Configure buttons
    advButton.interval(buttonInterval);
    advButton.setPressedState(LOW);
    subButton.interval(buttonInterval);
    subButton.setPressedState(LOW);
    rstButton.interval(buttonInterval);
    rstButton.setPressedState(LOW);
}

void loop()
{
    // Reset the watchdog counter
    wdt_reset();

    // Update buttons
    advButton.update();
    subButton.update();
    rstButton.update();

    // Get time
    DateTime now = rtc.now();
    uint16_t currentDay = now.unixtime() / SECONDS_PER_DAY;

    // Flash LED
    digitalWrite(STAT_LED, now.unixtime() % 2 == 0);

    // Set time
    display.setNum(currentDay);

    // Loop pause
    delay(10);

    if (advButton.pressed())
    {
        rtc.adjust(DateTime(SECONDS_PER_DAY * (currentDay + 1))); // One extra day
    }

    if (subButton.pressed())
    {
        if (currentDay > 0)
        {
            rtc.adjust(DateTime(SECONDS_PER_DAY * (currentDay - 1))); // One less day}
        }
        else
        {
            // You cant go less than 0
            display.setErr();
            delay(800);
        }
    }

    if (rstButton.pressed())
    {
        // Cool animation
        for (uint16_t i = 0; i < currentDay; i++)
        {
            display.setNum(currentDay - i);
            wdt_reset();
            delay((2000 / (currentDay - i)) + 100); // Speeds up as we approach
        }

        rtc.adjust(DateTime(SECONDS_PER_DAY * 0)); // One extra day
    }
}

void serialEvent()
{
    while (Serial.available())
    {
        int16_t newDay = Serial.parseInt();
        if (newDay != 0)
        {
            rtc.adjust(DateTime(SECONDS_PER_DAY * newDay));
        }
    }
}
