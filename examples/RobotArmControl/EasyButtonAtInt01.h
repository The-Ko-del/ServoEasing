/*
 * EasyButtonAtInt01.h
 *
 *  Arduino library for handling push buttons connected between ground and INT0 and / or INT1 pin (pin 2 / 3).
 *  The library is totally based on interrupt.
 *  Debouncing is implemented in a not blocking way! It is merely done by ignoring a button change within the debouncing time.
 *
 *  Copyright (C) 2018  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of EasyButtonAtInt01 https://github.com/ArminJo/EasyButtonAtInt01.
 *
 *  EasyButtonAtInt01 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef EASY_BUTTON_AT_INT01_H_
#define EASY_BUTTON_AT_INT01_H_

#include <Arduino.h>

/*
 * You can define your own value if you have buttons which are worse or better than the one I have.
 * Since debouncing is not done with blocking wait, reducing this value makes not much sense.
 * Test your value with just commenting out the line #define LED_FEEDBACK_FOR_DEBOUNCE_TEST below.
 * No other action other than defining the button is needed for this test. Define it e.g. with EasyButton Button0AtPin2(true);
 * Then press the button many times and see if the LED shows the state of the button for every press.
 * In case of the debouncing period too small, the led will sometimes stay active if the button was released.
 */
#ifndef BUTTON_DEBOUNCING_MILLIS
#define BUTTON_DEBOUNCING_MILLIS 50 // 35 millis measured for my button :-).
#endif

/*
 * Use the arduino function attachInterrupt().
 * Recommended if you get the error " multiple definition of `__vector_1'" (or `__vector_2')
 * because another library uses the attachInterrupt() function.
 * For one button it needs additional 160 bytes FLASH, for 2 buttons still 88 bytes.
 */
//#define USE_ATTACH_INTERRUPT
//
/*
 * Activate LED_BUILTIN as long as button is pressed
 * Serves as easy test to see if the debounce of 50 milliseconds is working with the button attached
 */
//#define LED_FEEDBACK_FOR_DEBOUNCE_TEST
#if defined (LED_FEEDBACK_FOR_DEBOUNCE_TEST)
#ifndef BUTTON_TEST_FEEDBACK_LED_PIN
#define BUTTON_TEST_FEEDBACK_LED_PIN LED_BUILTIN  // if not specified, use build in led - pin 13 on Uno board
#endif
#endif

//
//#define MEASURE_TIMING
#if defined(MEASURE_TIMING) || defined (LED_FEEDBACK_FOR_DEBOUNCE_TEST)
#include "digitalWriteFast.h"
#endif

#if defined(MEASURE_TIMING)
#ifndef BUTTON_TEST_TIMING_PIN
#define BUTTON_TEST_TIMING_PIN 12  // use pin 12
#endif
#endif

//#define TRACE

/*
 * These defines are here to enable saving of 150 bytes FLASH if only one button is needed
 */
//#define USE_BUTTON_0
//#define USE_BUTTON_1
#if not (defined(USE_BUTTON_0) || defined(USE_BUTTON_1))
#error USE_BUTTON_0 and USE_BUTTON_1 are not defined, please define them or remove the #include "EasyButtonAtInt01.h"
#endif

void handleINT0Interrupt();
void handleINT1Interrupt();

class EasyButton {

public:

#if defined(USE_BUTTON_0)
    static EasyButton * sPointerToButton0ForISR;
#endif
#if defined(USE_BUTTON_1)
    static EasyButton * sPointerToButton1ForISR;
#endif

    volatile bool ButtonStateIsActive;          // negative logic: true / active means button pin is LOW
    volatile bool ButtonToggleState;            // toggle is on press, not on release
    volatile bool ButtonStateHasJustChanged;    // Flag to enable action only once. Only set to true by library
                                                // Can be checked and set to false my main program to enable only one action per button press
    /*
     * Duration is set at button release or from an outside loop which polls the button state in order to check for button press timeouts,
     * since we get no interrupt until button is released.
     */
    volatile uint16_t ButtonPressDurationMillis; // Duration of active state.
    volatile long ButtonLastChangeMillis;       // for debouncing
    volatile long ButtonReleaseMillis;          // for double press recognition

    volatile bool isButton0AtPin2;
    void (*ButtonPressCallback)(bool aButtonToggleState) = NULL; // if not null, is called on every button press with ButtonToggleState as parameter

    EasyButton(bool aIsButton0AtPin2) {
        isButton0AtPin2 = aIsButton0AtPin2;
        init();
    }

    EasyButton(bool aIsButton0AtPin2, void (*aButtonPressCallback)(bool aButtonToggleState)) {
        isButton0AtPin2 = aIsButton0AtPin2;
        ButtonPressCallback = aButtonPressCallback;
        init();
    }

    /*
     * Sets pin 2 mode to INPUT_PULLUP and enables INT0 Interrupt on any logical change.
     */
    void init() {
#if defined(MEASURE_TIMING)
        pinModeFast(BUTTON_TEST_TIMING_PIN, OUTPUT);
#endif

#if defined(LED_FEEDBACK_FOR_DEBOUNCE_TEST)
        pinModeFast(BUTTON_TEST_FEEDBACK_LED_PIN, OUTPUT);
#endif
#if defined(USE_BUTTON_0) && not defined(USE_BUTTON_1)
        DDRD &= ~(_BV(2)); // pinModeFast(2, INPUT_PULLUP);
        PORTD |= _BV(2);
        sPointerToButton0ForISR = this;
#if defined(USE_ATTACH_INTERRUPT)
        attachInterrupt(digitalPinToInterrupt(2), &handleINT0Interrupt, CHANGE);
#else
        // interrupt on any logical change
        EICRA |= (1 << ISC00);
        // clear interrupt bit
        EIFR |= 1 << INTF0;
        // enable interrupt on next change
        EIMSK |= 1 << INT0;
#endif //USE_ATTACH_INTERRUPT
#endif
#if defined(USE_BUTTON_1) && not defined(USE_BUTTON_0)
        DDRD &= ~(_BV(3));
        PORTD |= _BV(3);
        sPointerToButton1ForISR = this;
#if defined(USE_ATTACH_INTERRUPT)
        attachInterrupt(digitalPinToInterrupt(3), &handleINT1Interrupt, CHANGE);
#else
        // interrupt on any logical change
        EICRA |= (1 << ISC10);
        // clear interrupt bit
        EIFR |= 1 << INTF1;
        // enable interrupt on next change
        EIMSK |= 1 << INT1;
#endif //USE_ATTACH_INTERRUPT
#endif

#if defined(USE_BUTTON_0) && defined(USE_BUTTON_1)
        if (isButton0AtPin2) {
            DDRD &= ~(_BV(2)); // pinModeFast(2, INPUT_PULLUP);
            PORTD |= _BV(2);
            sPointerToButton0ForISR = this;
#if defined(USE_ATTACH_INTERRUPT)
            attachInterrupt(digitalPinToInterrupt(2), &handleINT0Interrupt, CHANGE);
#else
            // interrupt on any logical change
            EICRA |= (1 << ISC00);
            // clear interrupt bit
            EIFR |= 1 << INTF0;
            // enable interrupt on next change
            EIMSK |= 1 << INT0;
#endif //USE_ATTACH_INTERRUPT
        } else {
            DDRD &= ~(_BV(3));
            PORTD |= _BV(3);
            sPointerToButton1ForISR = this;
#if defined(USE_ATTACH_INTERRUPT)
            attachInterrupt(digitalPinToInterrupt(3), &handleINT1Interrupt, CHANGE);
#else
            // interrupt on any logical change
            EICRA |= (1 << ISC10);
            // clear interrupt bit
            EIFR |= 1 << INTF1;
            // enable interrupt on next change
            EIMSK |= 1 << INT1;
#endif //USE_ATTACH_INTERRUPT
        }
#endif
        ButtonStateIsActive = false; // negative logic for ButtonStateIsActive! true means button pin is LOW
        ButtonToggleState = false;
    }

    /*
     * Used for long button press recognition.
     * Updates the ButtonPressDurationMillis by polling, since this cannot be done by interrupt.
     */
    uint16_t updateButtonPressDuration() {
        uint8_t tActualButtonStateIsActive;
#if defined(USE_BUTTON_0) && not defined(USE_BUTTON_1)
        tActualButtonStateIsActive = PIND & _BV(2);  //  = digitalReadFast(2);
#endif

#if defined(USE_BUTTON_1) && not defined(USE_BUTTON_0)
        tActualButtonStateIsActive = PIND & _BV(3);  //  = digitalReadFast(3);
#endif

#if defined(USE_BUTTON_0) && defined(USE_BUTTON_1)
        if (isButton0AtPin2) {
            tActualButtonStateIsActive = PIND & _BV(2);  //  = digitalReadFast(2);
        } else {
            tActualButtonStateIsActive = PIND & _BV(3);  //  = digitalReadFast(3);
        }
#endif
        if (tActualButtonStateIsActive) {
            // Button still active -> update ButtonPressDurationMillis
            ButtonPressDurationMillis = millis() - ButtonLastChangeMillis;
        }
        return ButtonPressDurationMillis;
    }
};

#if defined(USE_BUTTON_0)
EasyButton * EasyButton::sPointerToButton0ForISR;
#endif
#if defined(USE_BUTTON_1)
EasyButton * EasyButton::sPointerToButton1ForISR;
#endif

void handleINT01Interrupts(EasyButton * aButtonControlPtr) {
    // Read button value
    bool tActualButtonStateIsActive;

#if defined(USE_BUTTON_0) && not defined(USE_BUTTON_1)
    tActualButtonStateIsActive = PIND & _BV(2);  //  = digitalReadFast(2);
#endif

#if defined(USE_BUTTON_1) && not defined(USE_BUTTON_0)
    tActualButtonStateIsActive = PIND & _BV(3);  //  = digitalReadFast(3);
#endif

#if defined(USE_BUTTON_0) && defined(USE_BUTTON_1)
    if (aButtonControlPtr->isButton0AtPin2) {
        tActualButtonStateIsActive = PIND & _BV(2);  //  = digitalReadFast(2);
    } else {
        tActualButtonStateIsActive = PIND & _BV(3);  //  = digitalReadFast(3);
    }
#endif

    tActualButtonStateIsActive = !tActualButtonStateIsActive; // negative logic for ButtonStateIsActive! true means button pin is LOW

    long tMillis = millis();
    long tDeltaMillis = tMillis - aButtonControlPtr->ButtonLastChangeMillis;
    // Check for bouncing
    if (tDeltaMillis <= BUTTON_DEBOUNCING_MILLIS) {
#ifdef TRACE
        Serial.println("Button bouncing");
//        Serial.print("ms=");
//        Serial.print(tMillis);
//        Serial.print(" D=");
//        Serial.println(tDeltaMillis);
#endif
        /*
         * Button signal is ringing - do nothing, ignore and wait for next interrupt
         */
    } else {
        if (tActualButtonStateIsActive == aButtonControlPtr->ButtonStateIsActive) {
#ifdef TRACE
            Serial.println("Spike detected");
#endif
            /*
             * ActualButtonStateIsActive == OldButtonStateIsActive. We had an interrupt, but nothing seems to have changed -> spike
             * Do nothing, ignore and wait for next interrupt
             */
        } else {
            /*
             * Valid change detected
             */
            aButtonControlPtr->ButtonLastChangeMillis = tMillis;
            aButtonControlPtr->ButtonPressDurationMillis = tDeltaMillis;
#ifdef TRACE
            Serial.print("Change detected. State=");
            Serial.println(tActualButtonStateIsActive);
#endif
            aButtonControlPtr->ButtonStateIsActive = tActualButtonStateIsActive;
            aButtonControlPtr->ButtonStateHasJustChanged = true;
            if (tActualButtonStateIsActive) {
                /*
                 * Action on button press, no action on release
                 */
#ifdef LED_FEEDBACK_FOR_DEBOUNCE_TEST
                digitalWriteFast(BUTTON_TEST_FEEDBACK_LED_PIN, HIGH);
#endif
                aButtonControlPtr->ButtonToggleState = !aButtonControlPtr->ButtonToggleState;
                if (aButtonControlPtr->ButtonPressCallback != NULL) {
                    // call callback function
                    aButtonControlPtr->ButtonPressCallback(aButtonControlPtr->ButtonToggleState);
                    /*
                     * Check button again since it may changed back while processing callback function
                     */
                    tActualButtonStateIsActive = PIND & _BV(2);
                    tActualButtonStateIsActive = !tActualButtonStateIsActive;
                    if (aButtonControlPtr->ButtonStateIsActive != tActualButtonStateIsActive) {
                        // button released now, so maintain status
#ifdef TRACE
                        Serial.println("Button release during callback processing detected.");
#endif
                        aButtonControlPtr->ButtonStateIsActive = tActualButtonStateIsActive;
                        aButtonControlPtr->ButtonStateHasJustChanged = true;
                    }
                }
            } else {
                /*
                 * Button release
                 */
                aButtonControlPtr->ButtonReleaseMillis = tMillis;
#ifdef LED_FEEDBACK_FOR_DEBOUNCE_TEST
                digitalWriteFast(BUTTON_TEST_FEEDBACK_LED_PIN, LOW);
#endif
            }
        }
    }

}

/*
 * This functions are weak and can be replaced by your own code
 */
#if defined(USE_BUTTON_0)
void __attribute__ ((weak)) handleINT0Interrupt() {
    handleINT01Interrupts(EasyButton::sPointerToButton0ForISR);
}
#endif

#if defined(USE_BUTTON_1)
void __attribute__ ((weak)) handleINT1Interrupt() {
    handleINT01Interrupts(EasyButton::sPointerToButton1ForISR);
}
#endif

#if not defined(USE_ATTACH_INTERRUPT)
// ISR for PIN PD2
// Cannot make the vector itself weak, since the vector table is already filled by weak vectors resulting in ignoring my weak one:-(
//ISR(INT0_vect, __attribute__ ((weak))) {
#if defined(USE_BUTTON_0)
ISR(INT0_vect) {
#ifdef MEASURE_TIMING
    digitalWriteFast(BUTTON_TEST_TIMING_PIN, HIGH);
#endif
    handleINT0Interrupt();
#ifdef MEASURE_TIMING
    digitalWriteFast(BUTTON_TEST_TIMING_PIN, LOW);
#endif
}
#endif

#if defined(USE_BUTTON_1)
ISR(INT1_vect) {
#ifdef MEASURE_TIMING
    digitalWriteFast(BUTTON_TEST_TIMING_PIN, HIGH);
#endif
    handleINT1Interrupt();
#ifdef MEASURE_TIMING
    digitalWriteFast(BUTTON_TEST_TIMING_PIN, LOW);
#endif
}
#endif
#endif // not defined(USE_ATTACH_INTERRUPT)

#endif /* EASY_BUTTON_AT_INT01_H_ */
