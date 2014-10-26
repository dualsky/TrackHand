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
/// Title: Main function
///  Description:
//    Constructs the KeyMatrix, TrackBall and PowerSave classes
//    and enters the matrix scan loop.
//    The trackball operates on interrupt.
// -----------------------------------------------------------------------------

#include "KeyMatrix.h"
#include "TrackBall.h"
#include "PowerSave.h"
#include "MCP23018.h"

// -----------------------------------------------------------------------------

// Construct the keyboard matrix
KeyMatrix keyMatrix;

// Construct the trackball
TrackBall trackBall;

// Construct the power-saving mode for the keyboard and trackball
PowerSave powerSave(keyMatrix, trackBall);


int main(void)
{
    keyMatrix.begin();
    trackBall.begin();
    powerSave.begin();

    while (1)
    {
        powerSave(keyMatrix.keysPressed() || trackBall.move());
        keyMatrix.pause();
    }

    return 0;
}


// -----------------------------------------------------------------------------