/*
  Kitchen_Timer

  Display
  hh:mm for times >1h00
  mm:ss for times 1m00..59m59

  Configuration options see "Configuration.h"

  Make sure LOAD_GFXFF is defined in the used User_Setup file
  within the library folder.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  ######   TO SELECT YOUR DISPLAY TYPE, PINS USED AND ENABLE FONTS   ######
  #########################################################################
*/

// ########################
// ToDo:
// - Power-Off Hardware (reduce deep standby current from 300uA to 0uA)
// - long button press for power off
// - Code optimization
// - Info on Error
// - check pixel set outside narrow string and refresh back if needed
// - exchange button h/m <-> left/right
// ########################

#include "SPI.h"
#include "TFT_eSPI.h"
#include "Adafruit_GFX.h"
#include "esp_adc_cal.h"
#include <EEPROM.h>
#include <WiFi.h>
#include <RotaryEncoder.h>
#include "Configuration.h"
#include "Kitchen_Timer.h"
#include "sunset_240x135.h"

#include "Alarm_Clock.h"
#include "harp.h"
#include "XT_DAC_Audio.h"

// The custom font file attached to this sketch must be included
#include "DSEG7_Classic_Mini_Bold_72.h"
#define MYFONT &DSEG7_Classic_Mini_Bold_72

// clock pointer positions
uint8_t cp[9][4] {
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X-5, CLOCK_POS_Y-5},  // [0] 10h30
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X,   CLOCK_POS_Y-7},  // [1] 12h00
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X+5, CLOCK_POS_Y-5},  // [2] 01h30
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X+7, CLOCK_POS_Y},    // [3] 03h00
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X+5, CLOCK_POS_Y+5},  // [4] 04h30
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X,   CLOCK_POS_Y+7},  // [5] 06h00
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X-5, CLOCK_POS_Y+5},  // [6] 07h30
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X-7, CLOCK_POS_Y},    // [7] 09h00
    {CLOCK_POS_X, CLOCK_POS_Y, CLOCK_POS_X-5, CLOCK_POS_Y-5}   // [8] 10h30
};

#ifdef ROTARY_4STEP
// Setup a RotaryEncoder with 4 steps per latch for the 2 signal input pins:
RotaryEncoder encoder(ENCPIN1, ENCPIN2, RotaryEncoder::LatchMode::FOUR3);
#endif

#ifdef ROTARY_2STEP
// Setup a RotaryEncoder with 2 steps per latch for the 2 signal input pins:
RotaryEncoder encoder(ENCPIN1, ENCPIN2, RotaryEncoder::LatchMode::TWO03);
#endif

XT_Wav_Class Alarm(Alarm_Clock);      // create an object of type XT_Wav_Class that is used by 
                                      // the dac audio class (below), passing wav data as parameter.

XT_Wav_Class harpsound(harp);         // create an object of type XT_Wav_Class that is used by 
                                      // the dac audio class (below), passing wav data as parameter.
                                      
XT_DAC_Audio_Class DacAudio(25,0);    // Create the main player class object. 
                                      // Use GPIO 25, one of the 2 DAC pins and timer 0

uint32_t DemoCounter=0;               // Just a counter to use in the serial monitor
                                      // not essential to playing the sound


// global variables
uint8_t   buttons;          // binary button pressed info, each bit represents a button
uint32_t  lasttime_button;  // for debouncing
char      buff[256];        // generic text buffer for printf
char      errortext[64];    // error text to display
uint32_t  set_time_hm;      // set time hours / minutes
uint32_t  set_time_ms;      // set time minutes / seconds
uint32_t  start_time;       // millis() at start of measuring
uint32_t  et;               // current remaining time to display
uint32_t  et_last;          // millis() at start of pause
uint32_t  eh_last, em_last, es_last;        // last h, m, s for detecting nedd of display update
uint8_t   eh, em, es;       // display numbers h, m, s of et
uint8_t   esh, esm, ess;    // display numbers h, m, s of set_time
uint8_t   clock_pointer;    // position of clock pointer 0..7
boolean   mode_HM;          // true=HH:MM, false=MM:SS
uint8_t   eNextState;       // FSM state
uint32_t  timeout_counter;  // timeout for auto power off [ms]
float     battery_voltage;  // measured battery voltage
uint32_t  updatetime, updatecolon;

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

// ###############################################################################################

void drawClockPointer(void) {
  tft.drawLine(cp [clock_pointer]  [0], cp [clock_pointer]  [1], cp [clock_pointer]  [2], cp [clock_pointer]  [3], CLOCK_COL_FACE);    // erase old
  tft.drawLine(cp [clock_pointer+1][0], cp [clock_pointer+1][1], cp [clock_pointer+1][2], cp [clock_pointer+1][3], CLOCK_COL_POINTER); // draw new
}

void drawClockFace(void) {
  tft.fillCircle(CLOCK_POS_X,    CLOCK_POS_Y-18,  4, CLOCK_COL_KNOB);  // 3 dots
  tft.fillCircle(CLOCK_POS_X-10, CLOCK_POS_Y-16,  3, CLOCK_COL_KNOB);
  tft.fillCircle(CLOCK_POS_X+10, CLOCK_POS_Y-16,  3, CLOCK_COL_KNOB);

  tft.fillCircle(CLOCK_POS_X,    CLOCK_POS_Y,   17, CLOCK_COL_CASE);
  tft.fillCircle(CLOCK_POS_X,    CLOCK_POS_Y,   14, CLOCK_COL_FACE);

  clock_pointer = 0;
  drawClockPointer();
}

void show_battery() {

// read battery voltage
  uint16_t v = analogRead(ADC_PIN);
  battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (VREF / 1000.0);
  String voltage = " " + String(battery_voltage) + "V";
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(BATCOL_VAL);
  tft.fillRect(BATVAL_POS_X-40, BATVAL_POS_Y, 40, 15, BATCOL_BACK);
  tft.drawString(voltage, BATVAL_POS_X, BATVAL_POS_Y, 2);

// battery symbol
  if(battery_voltage <= 2.99){
    tft.fillRect(BATSYM_POS_X,    BATSYM_POS_Y,    30,  14, BATCOL_FRAME_EMPTY); // body
    tft.fillRect(BATSYM_POS_X+30, BATSYM_POS_Y+4,   3,  6, BATCOL_FRAME_EMPTY); // right knob
    //tft.drawString(voltage, BATVAL_POS_X, BATVAL_POS_Y, 2);
  }
  else {
    tft.fillRect(BATSYM_POS_X,    BATSYM_POS_Y,    30,  14, BATCOL_FRAME); // body
    tft.fillRect(BATSYM_POS_X+30, BATSYM_POS_Y+4,   3,  6, BATCOL_FRAME); // right knob
  }
  tft.fillRect(BATSYM_POS_X+2, BATSYM_POS_Y+2, 26, 10, BATCOL_BACK); // clear level symbol


  if(battery_voltage >= 4.0){
    tft.fillRect(BATSYM_POS_X+2, BATSYM_POS_Y+2, 26, 10, BATCOL_FULL);
  }
  else if(battery_voltage >= 3.8){
    tft.fillRect(BATSYM_POS_X+2, BATSYM_POS_Y+2, 21, 10, BATCOL_FULL);
  }
  else if(battery_voltage >= 3.6){
    tft.fillRect(BATSYM_POS_X+2, BATSYM_POS_Y+2, 16, 10, BATCOL_MID);
  }
  else if(battery_voltage >= 3.4){
    tft.fillRect(BATSYM_POS_X+2, BATSYM_POS_Y+2, 11, 10, BATCOL_MID);
  }
  else if(battery_voltage >= 3.2){
    tft.fillRect(BATSYM_POS_X+2, BATSYM_POS_Y+2, 6, 10, BATCOL_LOW);
  }
  else if(battery_voltage >= 3.0){
    tft.fillRect(BATSYM_POS_X+2, BATSYM_POS_Y+2, 4, 10, BATCOL_LOW);
  }
  if(battery_voltage >= 4.60){
    tft.setTextColor(BATCOL_TXT);
    tft.drawString("CHG", BATSYM_POS_X+28, BATSYM_POS_Y-1, 2);
  }
  else if(battery_voltage >= 4.85){
    tft.setTextColor(BATCOL_TXT);
    tft.drawString("USB", BATSYM_POS_X+28, BATSYM_POS_Y-1, 2);
  }
}

void update_progressbar(uint8_t pmode, uint16_t pvalue,  uint16_t maxval) {
  uint16_t progbar_color_bar, progbar_color_frame;
  uint16_t  progbar;

  // select colors
  if (pmode & RUN) {
    progbar_color_bar = PROGBAR_COL_BAR;
    progbar_color_frame = PROGBAR_COL_FRAME;
  }
  if (pmode & END) {
    progbar_color_bar = PROGBAR_COL_BAR_OVL;
    progbar_color_frame = PROGBAR_COL_FRAME_OVL;
  }
  if (pmode & PAUSE) {
    progbar_color_bar = PROGBAR_COL_BAR_PAUSE;
    progbar_color_frame = PROGBAR_COL_FRAME_PAUSE;
  }

  // select style
  if (pmode & BACK) {
    tft.fillRect(0, PROGBAR_POS_Y-2, 240, PROGBAR_HEIGHT+4, TIME_COL_BACK); // background progress bar
  }
  if (pmode & FRAME) {
    tft.drawRect(0, PROGBAR_POS_Y, 240, PROGBAR_HEIGHT, progbar_color_frame); // border progress bar
  }
  if (pmode & FULL) {
    tft.fillRect(2, PROGBAR_POS_Y+2, 236, PROGBAR_HEIGHT-4, progbar_color_bar); // filled progress bar
  }
  if (pmode & PART) {
    progbar = 236 * ((float)pvalue / (float)maxval);
    tft.fillRect((uint32_t)(progbar+2), PROGBAR_POS_Y+2, (uint32_t)(236-progbar), PROGBAR_HEIGHT-4, PROGBAR_COL_BACK);  // decrease progress bar
  }
}

void print_main_time(uint8_t h_m, uint8_t m_s, uint32_t color_f, uint32_t color_b) {
  
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(color_f, color_b);
      
  if (h_m<10) {
    // check pixel set outside narrow string and refresh back if needed
    tft.fillRect(0, TIME_POS_Y, 240, 73, color_b);
    sprintf(buff, "%01d:%02d", h_m, m_s);
  }
  else {
    sprintf(buff, "%02d:%02d", h_m, m_s);
  }
  tft.drawString(buff, 117, TIME_POS_Y, GFXFF);
}

void flash_main_time(uint8_t hm, uint8_t ms, uint32_t color_fl, uint32_t color_fr, uint32_t color_b) {
  uint8_t pos_l, pos_r;
  
  if (hm<10) {
    pos_l = 49;
    pos_r = 154;  
    sprintf(buff, "%01d", hm);
    // check pixel set outside narrow string and refresh back if needed
  }
  else {
    pos_l = 49;
    pos_r = 184;  
    sprintf(buff, "%02d", hm);
  }

  tft.setTextDatum(TC_DATUM);
  if (color_fl == BLINKTIME_COL) {
    if (hm<10) {
      tft.fillRect(0, TIME_POS_Y, 40, 73, color_b);
    }
    tft.setTextColor(color_fl, color_b);
    tft.drawString(buff, pos_l, TIME_POS_Y, GFXFF);
  }
  if (color_fr == BLINKTIME_COL) {
    if (hm<10) {
      tft.fillRect(200, TIME_POS_Y, 40, 73, color_b);
    }
    sprintf(buff, "%02d", ms);
    tft.setTextColor(color_fr, color_b);
    tft.drawString(buff, pos_r, TIME_POS_Y, GFXFF);
  }
}

// @brief The interrupt service routine will be called on any change of one of the input signals.
ICACHE_RAM_ATTR void checkPosition()
{
  encoder.tick(); // just call tick() to check the state.
}

#ifdef BUTTONS
void int_button_TL(void) {
  if ((millis() - lasttime_button) > DEBOUNCETIME) {
    buttons |= TL;
    lasttime_button = millis();
  }
}
void int_button_TR(void) {
  if ((millis() - lasttime_button) > DEBOUNCETIME) {
    buttons |= TR;
    lasttime_button = millis();
  }
}
void int_button_BL(void) {
  if ((millis() - lasttime_button) > DEBOUNCETIME) {
    buttons |= BL;
    lasttime_button = millis();
  }
}
void int_button_BR(void) {
  if ((millis() - lasttime_button) > DEBOUNCETIME) {
    buttons |= BR;
    lasttime_button = millis();
  }
}
#else
void int_button_CLR(void) {
  if ((millis() - lasttime_button) > DEBOUNCETIME) {
    buttons |= CLR;
    lasttime_button = millis();
  }
}
#endif

void int_button_MODE(void) {
  if ((millis() - lasttime_button) > DEBOUNCETIME) {
    buttons |= MD;
    lasttime_button = millis();
/*
    while (digitalRead(BUTTON_MODE) == LOW) {
      // wait for release
      delay(10);
    }
    if ((millis() - lasttime_button) > LONGPRESS) {
      buttons |= PO;
    }
*/
  }
}
void int_button_RUNSTOP(void) {
  if ((millis() - lasttime_button) > DEBOUNCETIME) {
    buttons |= RS;
    lasttime_button = millis();
  }
}
 
// ###############################################################################################
// ***** eventhandlers *****

uint8_t Stopped_Handler(void) {
#ifndef BUTTONS
  static int  pos = 0;         // position of rotary encoder
  int         newPos;
  boolean     setleft = true;
  uint8_t     setcount;
#endif

  // entry
#if DBGSW
  Serial.printf("entered Stopped_Handler\n");
#endif

  tft.fillScreen(CANVAS_COL);      // Clear screen
  tft.setTextDatum(TC_DATUM);

  drawClockFace();
  drawClockPointer();

  updatetime=millis();

#ifndef BUTTONS
  setcount = 0;
#endif

  buttons=ENC; // start with invalid button to enter the loop


  // main

  while (1) {
    while (buttons==0) {
      // wait for button      
      if ((millis() - updatetime) > 500) {
        show_battery();
        if(battery_voltage <= 2.99){
          return FSM_LOW_BATT; // emergency break
        }
        if (setcount>0) {
          if (setcount %2) {
            // flash maintime normal #################################################
            if (mode_HM) {
              print_main_time(eh, em, TIME_COL, TIME_COL_BACK);
            }
            else {
              print_main_time(em, es, TIME_COL, TIME_COL_BACK);
            }
          }
          else {
            if (setleft) {
              // flash left part of maintime dimmed #################################################
              if (mode_HM) {
                flash_main_time(eh, em, BLINKTIME_COL, TIME_COL, TIME_COL_BACK);
              }
              else {
                flash_main_time(em, es, BLINKTIME_COL, TIME_COL, TIME_COL_BACK);
              }
            }
            else {
              // flash right part of maintime dimmed #################################################
              if (mode_HM) {
                flash_main_time(eh, em, TIME_COL, BLINKTIME_COL, TIME_COL_BACK);
              }
              else {
                flash_main_time(em, es, TIME_COL, BLINKTIME_COL, TIME_COL_BACK);
              }
            }
          }
          setcount--;
        }
        updatetime=millis();
      }
      if ((millis() - timeout_counter) > TIMEOUT) {
        return FSM_POWEROFF;
      }
      
      // Read the current position of the encoder and print out when changed.
      encoder.tick();
      newPos = encoder.getPosition();
      if (pos != newPos) {
        buttons |= ENC;
      }
    }

    timeout_counter = millis(); // reset timeout
#ifndef BUTTONS
  setcount = SETCOUNTNUMBER; // number of times to blink after rotaty action
#endif


#if DBGSW
      //Serial.printf("Stop->Button: %3d\n", buttons);
#endif
    // ***** Run *****
    if (buttons & RS) { // run
      buttons=0;
      start_time = millis();
      if (mode_HM) {
        et_last = set_time_hm;
      }
      else {
        et_last = set_time_ms;
      }
      return FSM_RUNNUNG;
    }

    // ***** Mode *****
    if (buttons & MD) { // change mode HH:MM <-> MM:SS
      //buttons=0;
      mode_HM = !mode_HM;
    }

#ifdef BUTTONS
    // ***** hh/mm ++ *****
    if (buttons & TL) { // increase left digits
      //buttons=0;
      if (mode_HM) {
        if (set_time_hm <= (MAX_H * 3600000)) {
          set_time_hm += 3600000; // +hh
        }
      }
      else {
        set_time_ms += 60000; // +mm
      }
    }
    
    // ***** hh/mm -- *****
    if (buttons & BL) { // decrease left digits
      //buttons=0;
      if (mode_HM) {
        if (set_time_hm >= 3600000) {
          set_time_hm -= 3600000; // -hh
        }
      }
      else {
        if (set_time_ms >= 60000) {
          set_time_ms -= 60000; // -mm
        }
      }
    }
    
    // ***** mm/ss ++ *****
    if (buttons & TR) { // increase left digits
      //buttons=0;
      if (mode_HM) {
        set_time_hm += 60000; // +mm
      }
      else {
        set_time_ms += 1000; // +ss
      }
    }
    
    // ***** mm/ss -- *****
    if (buttons & BR) { // decrease left digits
      //buttons=0;
      if (mode_HM) {
        if (set_time_hm >= 60000) {
          set_time_hm -= 60000; // -mm
        }
      }
      else {
        if (set_time_ms >= 1000) {
          set_time_ms -= 1000; // -ss
        }
      }
    }
#else
    if (mode_HM) {
      if (((int)encoder.getDirection())>0) {
        if (setleft) {
          set_time_hm += (newPos-pos) * 60*60000; // +hh
        }
        else {
          set_time_hm += (newPos-pos) * 60000; // +mm
        }
      }
      else {  // minus
        if (setleft) {
          if (set_time_hm >= 60*60000) {
            set_time_hm += (newPos-pos) * 60*60000; // +hh
          }
        }
        else {
          if (set_time_hm >= 1000) {
            set_time_hm += (newPos-pos) * 60000; // +mm
          }
        }
      }
    }
    else {
      Serial.printf("mode ms - ");
      if (((int)encoder.getDirection())>0) {  // plus
        if (setleft) {
          set_time_ms += (newPos-pos) * 60*1000; // +mm
        }
        else {
          set_time_ms += (newPos-pos) * 1000; // +ss
        }
      }
      else {  // minus
        if (setleft) {
          if (set_time_ms >= 60*1000) {
            set_time_ms += (newPos-pos) * 60*1000; // +mm
          }
        }
        else {
          if (set_time_ms >= 1000) {
            set_time_ms += (newPos-pos) * 1000; // +ss
          }
        }
      }
    }
    pos = newPos;

    if (buttons & (CLR)) { // change set left / right
      setleft = !setleft;
      setcount = SETCOUNTNUMBER; // number of times to blink after rotaty action
      // flash maintime normal #################################################
    }

#endif

    if (buttons & (PO)) { // power off
      buttons=0;
      return FSM_POWEROFF;
    }
    
    if (buttons) { // any set button pressed?
      if (mode_HM) {
#if EE_MODE_HM == 2
        EEPROM.write(EE_MODE, 1);
#endif
        EEPROM.write(EE_HM_H, ((set_time_hm - (set_time_hm % 3600000)) / 3600000));
        EEPROM.write(EE_HM_M, (((set_time_hm % 3600000) - (set_time_hm % 60000)) / 60000));
      }
      else {
#if EE_MODE_HM == 2
        EEPROM.write(EE_MODE, 0);
#endif
        EEPROM.write(EE_MS_M, ((set_time_ms - (set_time_ms % 60000)) / 60000));
        EEPROM.write(EE_MS_S, (((set_time_ms % 60000) - (set_time_ms % 1000)) / 1000));
      }
      delay(20);
#if DBGSW
      EEPROM.commit();
      Serial.printf("write EEPROM as:\n");
      Serial.printf("EE_INIT=%d, EE_MODE=%d, EE_HM_H=%d, EE_HM_M=%d, EE_MS_M=%d, EE_MS_S=%d\n", 
      EEPROM.read(EE_INIT), EEPROM.read(EE_MODE), EEPROM.read(EE_HM_H), EEPROM.read(EE_HM_M), EEPROM.read(EE_MS_M), EEPROM.read(EE_MS_S));
#endif
    }
    buttons=0;

    // update display
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(STARTTIME_COL, STARTTIME_COL_BACK);
    tft.fillRect(70, 10, 100, 22, STARTTIME_COL_BACK);
    if (mode_HM) {
      tft.drawString(STR_HM, 120, 10, 4);
      et = set_time_hm;
    }
    else {
      tft.drawString(STR_MS, 120, 10, 4);
      et = set_time_ms;
    }

    es = (et / 1000) % 60;
    em = (et / 60000) % 60;
    eh = (et / 3600000);
    
    // print main time
    if (mode_HM) {
      print_main_time(eh, em, TIME_COL, TIME_COL_BACK);
    }
    else {
      print_main_time(em, es, TIME_COL, TIME_COL_BACK);
    }
    buttons=0; // clear invalid buttons
  }
}

uint8_t Running_Handler(void) {
  boolean colon;
  // entry
#if DBGSW
  Serial.printf("entered Running_Handler\n");
#endif

  if (mode_HM) {
    esm = (set_time_hm / 60000) % 60;
    esh = (set_time_hm / 3600000);
  }
  else {
    ess = (set_time_ms / 1000) % 60;
    esm = (set_time_ms / 60000) % 60;
  }

  tft.fillRect(65, 10, 110, 22, STARTTIME_COL_BACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(STARTTIME_COL, STARTTIME_COL_BACK);
  if (mode_HM) {
    sprintf(buff, "[%02d:%02d]", esh, esm);
  }
  else {
    sprintf(buff, "[%02d:%02d]", esm, ess);
  }
  tft.drawString(buff, 120, 10, 4);

  if (mode_HM) {
    print_main_time(eh, em, TIME_COL, TIME_COL_BACK);
    update_progressbar((RUN|BACK|FRAME|FULL|PART), eh*60+em, esh*60+esm);
  }
  else {
    print_main_time(em, es, TIME_COL, TIME_COL_BACK);
    update_progressbar((RUN|BACK|FRAME|FULL|PART), em*60+es, esm*60+ess);
  }

  updatetime=millis();
  updatecolon = updatetime;
  colon = true;

  
  // main

  while (1){
    if (buttons==0) {
      eh_last = eh;
      em_last = em;
      es_last = es;

      et = et_last - (millis() - start_time); // count down
      if (mode_HM) {
        if (et>set_time_hm) et=0; // overflow?
      }
      else {
        if (et>set_time_ms) et=0; // overflow?
      }
    
      // update display
      es = (et / 1000) % 60;
      em = (et / 60000) % 60;
      eh = (et / 3600000);

      // print main time
      if (mode_HM) {
        if (em_last != em) {
          print_main_time(eh, em, TIME_COL, TIME_COL_BACK);
          update_progressbar((RUN|PART), eh*60+em, esh*60+esm);
        }
      }
      else {
        if (es_last != es) {
          print_main_time(em, es, TIME_COL, TIME_COL_BACK);
          update_progressbar((RUN|PART), em*60+es, esm*60+ess);
          show_battery();
          if(battery_voltage <= 2.99){
            return FSM_LOW_BATT; // emergency break
          }
        }
      }

      // flash colon in mode HH:MM
      if (((millis() - updatecolon) > COLONSPEED) && (mode_HM)) {
        updatecolon=millis();
        tft.setTextDatum(TC_DATUM);
        if (colon) {
          tft.setTextColor(PAUSETIME_COL, TIME_COL_BACK);
        }
        else {
          tft.setTextColor(TIME_COL, TIME_COL_BACK);
        }
        colon = !colon;

        if (eh<10) {
          tft.drawString(":", 90, TIME_POS_Y, GFXFF);
        }
        else {
          tft.drawString(":", 119, TIME_POS_Y, GFXFF);
        }
        show_battery();
        if(battery_voltage <= 2.99){
          return FSM_LOW_BATT; // emergency break
        }
      }

      if ((millis() - updatetime) > CLOCKSPEED) {
        updatetime=millis();
        clock_pointer++;
        if (clock_pointer>=8) clock_pointer=0;
        drawClockPointer();
      }

      if (et==0) { // time is up
        buttons=0;
        timeout_counter = millis(); // reset timeout
        return FSM_END;
      }
#if DBGSW
      //Serial.printf("set_time:%d | et_last:%d | start_time:%d | et:%d |\n", set_time, et_last, start_time, et);
#endif
      //delay (500);    
    }
    else {
#if DBGSW
      //Serial.printf("Run>Button: %3d\n", buttons);
#endif
      if (buttons & RS) { // pause
        buttons=0;
        et_last = et;
        timeout_counter = millis(); // reset timeout
        return FSM_PAUSED;
      }
      buttons=0; // invalid button
    }
  }
}

uint8_t Paused_Handler(void) {
  boolean change_col = true;
  // entry
#if DBGSW
  Serial.printf("entered Paused_Handler\n");
#endif
  tft.fillRect(65, 10, 110, 22, STARTTIME_COL_BACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(STARTTIME_COL, STARTTIME_COL_BACK);
  tft.drawString(STR_Pause, 120, 10, 4);

  if (mode_HM) {
    update_progressbar((PAUSE|BACK|FRAME|FULL|PART), eh*60+em, esh*60+esm);
  }
  else {
    update_progressbar((PAUSE|BACK|FRAME|FULL|PART), em*60+es, esm*60+ess);
  }
  updatetime=millis();

  // main
  while (buttons==0) {
    // wait for button
    if ((millis() - updatetime) > 1000) {
      show_battery();

      // flash main time
      if (change_col) {
        if (mode_HM) {
          print_main_time(eh, em, PAUSETIME_COL, PAUSETIME_COL_BACK);
          update_progressbar((RUN|PART), eh*60+em, esh*60+esm);
        }
        else {
          print_main_time(em, es, PAUSETIME_COL, PAUSETIME_COL_BACK);
          update_progressbar((RUN|PART), em*60+es, esm*60+ess);
        }
      }
      else {
        if (mode_HM) {
          print_main_time(eh, em, TIME_COL, PAUSETIME_COL_BACK);
          update_progressbar((RUN|PART), eh*60+em, esh*60+esm);
        }
        else {
          print_main_time(em, es, TIME_COL, PAUSETIME_COL_BACK);
          update_progressbar((RUN|PART), em*60+es, esm*60+ess);
        }
      }
      change_col = !change_col;

      if(battery_voltage <= 2.99){
        return FSM_LOW_BATT; // emergency break
      }
      updatetime=millis();
    }
    if ((millis() - timeout_counter) > TIMEOUT) {
      // safe current time
      if (mode_HM) {
        EEPROM.write(EE_MODE, 1);
        EEPROM.write(EE_HM_H, (eh));
        EEPROM.write(EE_HM_M, (em));
      }
      else {
        EEPROM.write(EE_MODE, 0);
        EEPROM.write(EE_MS_M, (em));
        EEPROM.write(EE_MS_S, (es));
      }
      delay(20);
      return FSM_POWEROFF;
    }
  }
  timeout_counter = millis(); // reset timeout

#if DBGSW
      //Serial.printf("Pause->Button: %3d\n", buttons);
#endif
  
  if (buttons & RS) { // run
    buttons=0;
    start_time = millis();
    return FSM_RUNNUNG;
  }

  // any other button
  buttons=0;
  return FSM_STOPPED;
}

uint8_t End_Handler(void) {
  boolean change_col = true;
  uint32_t sound_timeout;
    
  // entry
#if DBGSW
  Serial.printf("entered End_Handler\n");
#endif

  update_progressbar((END|BACK|FRAME), eh*60+em, esh*60+esm);

  drawClockFace();
  drawClockPointer();

  // play sound
  sound_timeout = millis(); // reset sound timeout
#if DBGSW
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(STARTTIME_COL, STARTTIME_COL_BACK);
  //tft.drawString("...play sound1...", 120, 80, 4);
  Serial.printf("...play sound1...\n");
#endif

  DacAudio.Play(&Alarm);
  
  // main

  while (buttons==0) {
    // wait for button      
    if ((millis() - updatetime) > 1000) {
      show_battery();
      if(battery_voltage <= 2.99){
        return FSM_LOW_BATT; // emergency break
      }

      // flash main time
      if (change_col) {
        if (mode_HM) {
          print_main_time(eh, em, PAUSETIME_COL, STARTTIME_COL_BACK);
          update_progressbar((RUN|PART), eh*60+em, esh*60+esm);
        }
        else {
          print_main_time(em, es, PAUSETIME_COL, STARTTIME_COL_BACK);
          update_progressbar((RUN|PART), em*60+es, esm*60+ess);
        }
        tft.setTextColor(PAUSETIME_COL, STARTTIME_COL_BACK);
      }
      else {
        if (mode_HM) {
          print_main_time(eh, em, STARTTIME_COL, STARTTIME_COL_BACK);
          update_progressbar((RUN|PART), eh*60+em, esh*60+esm);
        }
        else {
          print_main_time(em, es, STARTTIME_COL, STARTTIME_COL_BACK);
          update_progressbar((RUN|PART), em*60+es, esm*60+ess);
        }
        tft.setTextColor(STARTTIME_COL, STARTTIME_COL_BACK);
      }
      tft.drawString(STR_TIMEOVER, 120, 10, 4);
      change_col = !change_col;
      
      updatetime=millis();
    }
    if ((millis() - timeout_counter) > TIMEOUT_END) {
      return FSM_POWEROFF;
    }
    if (millis() - sound_timeout >= SOUND_TIMEOUT) {
      // play sound
#if DBGSW
      tft.setTextColor(STARTTIME_COL, STARTTIME_COL_BACK);
      Serial.printf("...play sound2...\n");
#endif

      DacAudio.Play(&Alarm);
      sound_timeout = millis(); // reset sound timeout
    }
    DacAudio.FillBuffer(); 
  }

  timeout_counter = millis(); // reset timeout
  
  if (buttons & (MD | RS | CLR)) { // clear
    buttons=0;
    return FSM_STOPPED;
  }
}

uint8_t Poweroff_Handler(void) {
  // entry
#if DBGSW
  Serial.printf("entered Poweroff_Handler\n");
#endif
  tft.fillScreen(TFT_BLUE);      // Clear screen
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(CC_DATUM);
#if DBGSW
  Serial.printf("Poweroff_Handler filled screen\n");
#endif

  // main
  tft.drawString(STR_POWEROFF, 120, 68, 4);
#if DBGSW
  Serial.printf("Poweroff_Handler drawn string\n");
#endif

  EEPROM.commit();

  int r = digitalRead(TFT_BL);
  delay(2000);
  digitalWrite(TFT_BL, !r);

  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
  delay(200);
  esp_deep_sleep_start();
  delay(200);
  
  strcpy( errortext, "shutdown not possible" );
  return FSM_ERROR;
}

uint8_t Lowbatt_Handler(void) {
  uint8_t i=10;
  uint16_t v;
  float battery_voltage;
  String voltage;
  boolean showsybol=true;
  
  // entry
#if DBGSW
  Serial.printf("entered Lowbatt_Handler\n");
#endif
  tft.fillScreen(TFT_MAROON);      // Clear screen
  tft.setTextDatum(CC_DATUM);

  // main

  while (i>0) {
    i--;
    if (showsybol) {
      tft.fillRect(10, 20, 200, 95, TFT_RED); // body
      tft.fillRect(210, 38, 20, 60, TFT_RED); // right knob
      tft.fillRect(15, 25, 190, 85, TFT_MAROON); // inner body
      tft.setTextColor(TFT_BLACK);
    }
    else {
      tft.fillRect(10, 20, 200, 95, TFT_MAROON); // body
      tft.fillRect(210, 38, 20, 60, TFT_MAROON); // right knob
      tft.setTextColor(TFT_YELLOW);
    }
    showsybol = !showsybol;

    // read battery voltage
    v = analogRead(ADC_PIN);
    battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (VREF / 1000.0);
    voltage = " " + String(battery_voltage) + "V";
    tft.drawString("Low Battery!", 110, 48, 4);
    tft.drawString(voltage, 110, 88, 4);
    delay(500);
  }
  
  return FSM_POWEROFF;
}

uint8_t Error_Handler(const char* errstring) {
  // entry
#if DBGSW
  Serial.printf("entered Error_Handler\n");
#endif
  // main
  tft.fillScreen(TFT_RED);      // Clear screen
  tft.setTextColor(TFT_YELLOW);
  tft.setTextDatum(CC_DATUM);

  // main
  tft.drawString("ERROR !", 120, 48, 4);
  tft.drawString(errstring, 120, 88, 4);
  delay(5000);

  while (buttons==0) {
  // wait for button      
  }

  return FSM_POWEROFF;
}

// ###############################################################################################

void setup(void) {

//  Serial.begin(250000);
  Serial.begin(115200);
#if DBGSW
  Serial.printf("entered setup\n");
#endif

  WiFi.mode(WIFI_OFF); // save power
  //WiFi.forceSleepBegin(); //This also works

  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  // check for initalisation and plausibility:
  if  (
        (EEPROM.read(EE_INIT) == 'T')
        &&
        (EEPROM.read(EE_HM_H) < MAX_H+1)
        &&
        (EEPROM.read(EE_HM_M) < 60)
        && 
        (EEPROM.read(EE_MS_M) < 60)
        && 
        (EEPROM.read(EE_MS_S) < 60)
      ) 
  {
#if DBGSW
    Serial.printf("EEPROM set correct - reading vaules...\n");
#endif
    set_time_hm = (EEPROM.read(EE_HM_H) * 60*60*1000) + (EEPROM.read(EE_HM_M) * 60*1000);
    set_time_ms = (EEPROM.read(EE_MS_M) * 60*1000   ) + (EEPROM.read(EE_MS_S) *1000);

#if EE_MODE_HM == 2
    if (EEPROM.read(EE_MODE) == 0) {
      mode_HM = false;
    }
    else {
      mode_HM = true;
    }
#endif

#if DBGSW
    Serial.printf("set_time_hm = %d, set_time_ms = %d, mode_HM = %b\n", set_time_hm, set_time_ms, mode_HM);
#endif
  }
  else {
#if DBGSW
    Serial.printf("EEPROM not set correct - writing default vaules...\n");
    Serial.printf("EE_INIT=%d, EE_MODE=%d, EE_HM_H=%d, EE_HM_M=%d, EE_MS_M=%d, EE_MS_S=%d\n", 
        EEPROM.read(EE_INIT), EEPROM.read(EE_MODE), EEPROM.read(EE_HM_H), EEPROM.read(EE_HM_M), EEPROM.read(EE_MS_M), EEPROM.read(EE_MS_S));
#endif
    EEPROM.write(EE_INIT, 'T');
    EEPROM.write(EE_HM_H, 1);
    EEPROM.write(EE_HM_M, 30);
    EEPROM.write(EE_MS_M, 6);
    EEPROM.write(EE_MS_S, 0);
#if EE_MODE_HM == 2
    EEPROM.write(EE_MODE, 0);
#endif
    EEPROM.commit();
  }

// select if mode HM/MS shall be fix or last state from EEPROM
#if EE_MODE_HM == 0
  mode_HM = true;
#endif
#if EE_MODE_HM == 1
  mode_HM = false;
#endif
#if EE_MODE_HM == 2
  mode_HM = EEPROM.read(EE_MODE);
#endif

  tft.begin();
  tft.setRotation(1);
  tft.setTextSize(1);

#ifdef BUTTONS
  pinMode(BUTTON_TL, INPUT_PULLUP);
  pinMode(BUTTON_TR, INPUT_PULLUP);
  pinMode(BUTTON_BL, INPUT_PULLUP);
  pinMode(BUTTON_BR, INPUT_PULLUP);
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(BUTTON_RUNSTOP, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON_TL),int_button_TL, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_TR),int_button_TR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_BL),int_button_BL, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_BR),int_button_BR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_MODE),int_button_MODE, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RUNSTOP),int_button_RUNSTOP, FALLING);
#else //ROTARY
  pinMode(ENCPIN1, INPUT_PULLUP);
  pinMode(ENCPIN2, INPUT_PULLUP);
  pinMode(BUTTON_CLEAR, INPUT_PULLUP);
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(BUTTON_RUNSTOP, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON_CLEAR),int_button_CLR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_MODE),int_button_MODE, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RUNSTOP),int_button_RUNSTOP, FALLING);
  attachInterrupt(ENCPIN1, checkPosition, CHANGE);
  attachInterrupt(ENCPIN2, checkPosition, CHANGE);
#endif

  buttons = 0; // clear buttons
  lasttime_button = millis();     // reset debouncing
  
  tft.fillScreen(CANVAS_COL);     // Clear screen
  tft.setFreeFont(MYFONT);        // Select the font

  // show nice startup picture
  tft.setSwapBytes(true);
  tft.pushImage(0, 0, 240, 135, sunset);

  // show startup text
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(" Kitchen Timer ", 122, 92, 4); // shadow
  tft.setTextColor(TFT_MAGENTA);
  tft.drawString(" Kitchen Timer ", 120, 90, 4); // foreground

  // display version
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_CYAN);
  tft.drawString(STR_VERSION, 120, 135, 2);

  // play startup sound
  DacAudio.Play(&harpsound);
  while (harpsound.Playing==true) {
    DacAudio.FillBuffer(); 
    delay (1);
  }
  delay (STARTSCREENDELAY);

  timeout_counter = millis();

  // enter FSM
  eNextState = FSM_STOPPED;
}

// ###############################################################################################

void loop() {
// the state machine
#if DBGSW
  Serial.printf("State change eNextState = %d\n", eNextState);
//      Serial.printf("set_time:%d | et_last:%d | start_time:%d | et:%d |\n", set_time, et_last, start_time, et);
#endif

  switch(eNextState) {
    case FSM_STOPPED:
      eNextState = Stopped_Handler();
      break;
    case FSM_RUNNUNG:
      eNextState = Running_Handler();
      break;
    case FSM_PAUSED:
      eNextState = Paused_Handler();
      break;
    case FSM_END:
      eNextState = End_Handler();
      break;
    case FSM_POWEROFF:
      eNextState = Poweroff_Handler();
      break;
    case FSM_LOW_BATT:
      eNextState = Lowbatt_Handler();
      break;
    case FSM_ERROR:
      eNextState = Error_Handler(errortext);
      break;
    default:
      eNextState = Error_Handler("Wrong FSM State");
      break;
  }
}

// ###############################################################################################
