/// Copyright 2014 Henry G. Weller
// -----------------------------------------------------------------------------
//  This file is part of
/// ---     TrackHand: DataHand with Laser TrackBall
// -----------------------------------------------------------------------------
//
//  TrackHand is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  TrackHand is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
//  for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with TrackHand.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------

#include "KeyMatrix.h"
#include "keyMaps.h"
#include "debug.h"

// -----------------------------------------------------------------------------

void KeyMatrix::set(const Mode& mode)
{
    if (currentMode_ != &mode)
    {
        if (mode.modal() || currentMode_->modal())
        {
            delay(200);
        }
        currentMode_ = mode.set(currentMode_);
    }
}


void KeyMatrix::debugKey(const char* hand, const uint8_t row, const uint8_t col)
{
    debug("Pressed ");
    debug(hand);
    debug(": ");
    debug(col);
    debug(" ");
    debug(row);
    debug(" ");
    debugln(pressedKeys_[nPressed_]);
}


void KeyMatrix::scan()
{
    nPressed_ = 0;

    for (uint8_t ri=0; ri<nRows_; ri++)
    {
        uint8_t rhRow = rhRows_[ri];
        uint8_t lhRow = lhRows_[ri];

        digitalWriteFast(rhRow, LOW);
        leftHand_.writeBit(lhRow, LOW);

        delayMicroseconds(columnStabTime_);

        // Columns are ONLY on port A
        uint8_t lhColumns = leftHand_.readA();

        for (uint8_t ci=0; ci<nColumns_; ci++)
        {
            uint8_t rhColumn = rhColumns_[ci];
            uint8_t lhColumn = lhColumns_[ci];

            // Check the right-hand column
            if (digitalRead(rhColumn) == LOW)
            {
                if (nPressed_ < maxPressed_)
                {
                    pressedKeys_[nPressed_] = rhKey(ri, ci);
                    debugKey("RH", ri, ci);
                    nPressed_++;
                }
            }

            // Check the left-hand column
            if (!bitRead(lhColumns, lhColumn))
            {
                if (nPressed_ < maxPressed_)
                {
                    pressedKeys_[nPressed_] = lhKey(ri, ci);
                    debugKey("LH", ri, ci);
                    nPressed_++;
                }
            }
        }

        digitalWriteFast(rhRow, HIGH);
        leftHand_.writeBit(lhRow, HIGH);
    }
}


bool KeyMatrix::send()
{
    const Mode* mode = NULL;

    // The set of modifiers pressed
    uint8_t modifiers = 0;

    // Mouse buttons
    uint8_t mouseButtons[3] = {0, 0, 0};

    // Scan for mode and modifiers
    for (uint8_t keyi=0; keyi<nPressed_; keyi++)
    {
        // Lookup key-code for current mode
        KEYCODE_TYPE keyCode = currentMode_->keyCode(pressedKeys_[keyi]);

        switch(keyCode)
        {
            case DH_KEY_NORM:
                mode = &normalMode_;
                break;
            case DH_KEY_SHIFT:
                mode = &shiftMode_;
                break;
            case DH_KEY_NAS:
                mode = &nasMode_;
                break;
            case DH_KEY_FN:
                mode = &fnMode_;
                break;
            case DH_KEY_MOUSE:
                mode = &mouseMode_;
                break;
            case DH_KEY_CTRL:
                modifiers |= MODIFIERKEY_CTRL;
                break;
            case DH_KEY_ALT:
                modifiers |= MODIFIERKEY_ALT;
                break;
            case DH_MOUSE_1:
                mouseButtons[0] = 1;
                break;
            case DH_MOUSE_1_1:
                mouseButtons[0] = 2;
                break;
            case DH_MOUSE_2:
                mouseButtons[1] = 1;
                break;
            case DH_MOUSE_3:
                mouseButtons[2] = 1;
                break;
        }
    }

    if (mode)
    {
        set(*mode);
    }
    // If the current mode is not modal reset to normal mode
    else if (!currentMode_->modal())
    {
        set(normalMode_);
    }

    uint8_t nSend = 0;
    KEYCODE_TYPE keyboardKeys[maxSend_] = {0, 0, 0, 0, 0, 0};
    bool unshiftedKeys = false;
    bool shiftedKeys = false;

    for (uint8_t keyi=0; keyi<nPressed_; keyi++)
    {
        // Lookup key-code for current mode
        KEYCODE_TYPE keyCode = currentMode_->keyCode(pressedKeys_[keyi]);

        // Special keys already handled
        if (keyCode >= 0xf0) continue;

        // High bit set for shifted keys
        if ((keyCode & (1<<7)))
        {
            shiftedKeys = true;
        }
        else
        {
            unshiftedKeys = true;
        }

        // Zero high bit
        keyCode &= 0x7f;

        // USB supports maxSend_ (6) keys per send, ignore any more
        if (nSend >= maxSend_) break;

        keyboardKeys[nSend++] = keyCode;
    }

    // Set shift modifier if keys are shifted
    if (shiftedKeys)
    {
        // Return without sending if both shifted and unshifted keys are pressed
        if (unshiftedKeys)
        {
            return false;
        }

        modifiers |= MODIFIERKEY_SHIFT;
    }

    // Check that some keys pressed have changed and transfer to send buffer
    bool keysChanged = false;
    for (uint8_t keyi=0; keyi<maxSend_; keyi++)
    {
        keyboard_keys[keyi] = keyboardKeys[keyi];

        if (keyboardKeysPrev_[keyi] != keyboardKeys[keyi])
        {
            keysChanged = true;
        }

        keyboardKeysPrev_[keyi] = keyboardKeys[keyi];
    }

    // Set modifiers
    Keyboard.set_modifier(modifiers);

    // Send if keys have changed
    if (keysChanged)
    {
        Keyboard.send_now();
    }

    bool mouseButtonsChanged = false;
    for (uint8_t buttoni=0; buttoni<3; buttoni++)
    {
        if (mouseButtonsPrev_[buttoni] != mouseButtons[buttoni])
        {
            mouseButtonsChanged = true;
        }

        mouseButtonsPrev_[buttoni] = mouseButtons[buttoni];
    }

    if (mouseButtonsChanged)
    {
        // Special handling for double-click of button 1
        if (mouseButtons[0] == 2)
        {
            Mouse.set_buttons(1, 0, 0);
            Mouse.set_buttons(0, 0, 0);
            Mouse.set_buttons(1, 0, 0);
        }
        else
        {
            Mouse.set_buttons
            (
                mouseButtons[0],
                mouseButtons[1],
                mouseButtons[2]
            );
        }
    }

    if (keysChanged || mouseButtonsChanged)
    {
        return true;
    }
    else
    {
        return false;
    }
}


KeyMatrix::KeyMatrix()
:
    leftHand_(Wire, 0),
    normalMode_(false, normalKeyMap, 31),
    shiftMode_(false, shiftKeyMap, 24),
    nasMode_(false, normalKeyMap, 30),
    fnMode_(false, normalKeyMap, 29),
    mouseMode_(true, mouseKeyMap, 28),
    currentMode_(normalMode_.set(NULL))
{}


void KeyMatrix::begin()
{
    // Setup serial port for debug messages
    Serial.begin(9600);

    // Setup the I2C connection to the left-hand unit
    Wire.begin();
    delay(100);

    // Initialize the right-hand row pins as output
    // Drives the IR leds
    for (uint8_t ri=0; ri<nRows_; ri++)
    {
        uint8_t rowPin = rhRows_[ri];
        pinMode(rowPin, OUTPUT);
        digitalWriteFast(rowPin, HIGH);
    }

    // Initialize the right-hand column pins as input
    // from the photo-transistors
    for (uint8_t ci=0; ci<nColumns_; ci++)
    {
        uint8_t columnPin = rhColumns_[ci];
        pinMode(columnPin, INPUT);
    }

    // Initialize the left-hand IO-expander
    uint8_t inputBits = 0;
    for (uint8_t ci=0; ci<nColumns_; ci++)
    {
        bitSet(inputBits, lhColumns_[ci]);
    }

    // Set the columns to input, rows to output and no pull-up resistors
    leftHand_.begin(inputBits, 0, 0, 0);

    // Set all rows to 1
    leftHand_.write(0xffff);
}


void KeyMatrix::sleep()
{
    currentMode_->sleep();
}


void KeyMatrix::wake()
{
    currentMode_->wake();

    // Wait for the keyboard to wake
    delay(1000);

    // Rested the USB buffer
    for (uint8_t keyi=0; keyi<maxSend_; keyi++)
    {
        keyboard_keys[keyi] = 0;
    }

    // Send a shift to wake-up the screen
    Keyboard.set_modifier(MODIFIERKEY_SHIFT);
    Keyboard.send_now();

    // Rest the modifiers
    Keyboard.set_modifier(0);
    Keyboard.send_now();
}


bool KeyMatrix::keysPressed()
{
    scan();
    return send();
}


void KeyMatrix::pause()
{
    delay(loopDelayTime_);
}


// -----------------------------------------------------------------------------