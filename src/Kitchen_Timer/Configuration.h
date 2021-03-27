/*
  Kitchen_Timer Configuration

  Display Settings

  You can select between 6 Button Interface

  +---------------------------------------------+
  |  [set + h/m]   [set + m/s]                  |
  |                               [Mode/Clear]  |
  |     8    8   :   8    8                     |
  |                                [Run/Stop]   |
  |  [set - h/m]   [set - m/s]                  |
  +---------------------------------------------+

  or Rotary Encoder plus 3 Buttons Interface

                    [Run/Stop]
         (    <--  Rotary Encoder  -->   )
  +---------------------------------------------+
  |                                             |
  |                                 [Mode]      |
  |     8    8   :   8    8                     |
  |                                 [Clear]     |
  |                                             |
  +---------------------------------------------+


*/
#ifndef KITCHENTIMER_CONFIGURATION_H
#define KITCHENTIMER_CONFIGURATION_H

// ***** Symbol / Graphics *****

// Canvas
#define CANVAS_COL              TFT_BLACK

// Battery Symbol
#define BATSYM_POS_X            202
#define BATSYM_POS_Y            4
#define BATCOL_FRAME            TFT_BROWN
#define BATCOL_FRAME_EMPTY      TFT_RED
#define BATCOL_BACK             TFT_BLACK
#define BATCOL_FULL             TFT_DARKGREEN
#define BATCOL_MID              TFT_ORANGE
#define BATCOL_LOW              TFT_RED
#define BATCOL_TXT              TFT_BLACK
#define BATVAL_POS_X            240
#define BATVAL_POS_Y            22
#define BATCOL_VAL              TFT_DARKGREEN

// Progress Bar
#define PROGBAR_POS_Y           123
#define PROGBAR_HEIGHT          12
#define PROGBAR_COL_FRAME       TFT_DARKGREEN
#define PROGBAR_COL_BAR         TFT_DARKGREEN
#define PROGBAR_COL_FRAME_OVL   TFT_RED
#define PROGBAR_COL_BAR_OVL     TFT_RED
#define PROGBAR_COL_FRAME_PAUSE TFT_DARKGREY
#define PROGBAR_COL_BAR_PAUSE   TFT_DARKGREY
#define PROGBAR_COL_BACK        TFT_BLACK

// Main Time
#define TIME_POS_Y              47
#define TIME_COL                TFT_GREEN
#define TIME_COL_HM             TFT_CYAN
#define TIME_COL_BACK           TFT_BLACK
#define STARTTIME_COL           TFT_RED
#define STARTTIME_COL_BACK      TFT_BLACK
#define PAUSETIME_COL           TFT_DARKGREY
#define PAUSETIME_COL_BACK      TFT_BLACK
#define BLINKTIME_COL           TFT_DARKGREEN
#define BLINKTIME_COL_HM        TFT_DARKCYAN

// Charge
#define CHARGE_COL_BACK         TFT_BLACK
#define CHARGE_COL_TEXT         TFT_RED


// Clock Icon (Position = Center of the clock)
#define CLOCK_POS_X             19
#define CLOCK_POS_Y             21
#define CLOCK_COL_CASE          TFT_DARKGREY
#define CLOCK_COL_KNOB          TFT_DARKGREY
#define CLOCK_COL_FACE          TFT_NAVY
#define CLOCK_COL_POINTER       TFT_RED

#define CLOCKSPEED              150           // Milliseconds Interval clock pointer
#define COLONSPEED              1000          // Milliseconds Interval Colon ar run

#define STARTSCREENDELAY        2000          // Time to show the startup screen
#define TIMEOUT                 30*1000       // timeout for auto power off [ms]
#define TIMEOUT_END             10*60*1000    // timeout for auto power off during END state (waiting for button) [ms]
#define SOUND_TIMEOUT           60*1000       // timeout for repeating sound in end state [ms]
#define MAX_H                   24            // max set hours
#define MAX_M                   99            // max set minutes

#define EE_MODE_HM              2             // 0=always start in Mode HH:MM, 1=always start in Mode MM:SS, 2=remember last mode from EEPROM

// ***** Language *****
// select only one !
//#define LANG_EN     // english (default)
#define LANG_DE     // german
//#define LANG_HU     // hungarian
//#define LANG_IT     // italian
//#define LANG_ES     // spanish

// ***** Hardware *****
#define ADC_EN                  14
#define ADC_PIN                 34

// uncommend one of the following lines to select between rotary encoder or simple buttons
//#define BUTTONS
//#define ROTARY_2STEP
#define ROTARY_4STEP

// port pins for buttons
#ifdef BUTTONS
#define BUTTON_TL               33 // Button top left
#define BUTTON_TR               32 // Button top right
#define BUTTON_BL               17 // Button bottom left
#define BUTTON_BR               12 // Button bottom right
#define BUTTON_MODE              0 // Button onboard GPIO35
#define BUTTON_RUNSTOP          35 // Button onboard GPIO0
#define HOLD                    27 // keep power on

#else //ROTARY
// rotary encoder
#define ENCPIN1 32
#define ENCPIN2 33
#define BUTTON_RUNSTOP          17 // Button Rotary Encoder
#define BUTTON_MODE              0 // Button onboard GPIO35
#define BUTTON_CLEAR            35 // Button onboard GPIO0
#define HOLD                    27 // keep power on
#endif

#define DEBOUNCETIME           200 // Milliseconds
#define LONGPRESS             1000 // time for detecting long pressed button


#endif // KITCHENTIMER_CONFIGURATION_H
