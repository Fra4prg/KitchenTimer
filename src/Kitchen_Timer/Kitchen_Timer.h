/*
  Kitchen_Timer internal defines
*/

#ifndef KITCHEN_TIMER_H
#define KITCHEN_TIMER_H

#define STR_VERSION "(c) Frank Scholl 27.03.2021"

#define DBGSW 0

#define EEPROM_SIZE 6
// addresses for EEPROM
#define EE_INIT 0
#define EE_MODE 1
#define EE_HM_H 2
#define EE_HM_M 3
#define EE_MS_M 4
#define EE_MS_S 5

// buttons pressed
#define TL            1    // Button top left
#define TR            2    // Button top right
#define BL            4    // Button bottom left
#define BR            8    // Button bottom right in case of 6 buttons
#define CLR           8    // Button bottom right in case or rotaty encoder
#define MD            16   // Button Mode/CLear
#define RS            32   // Button Run/Stop
#define PO            64   // long press for power off 
#define ENC           128  // Rotary Encoder 
#define SETCOUNTNUMBER  6  // number of times to blink after rotaty action (even number !)

// progress bar states
#define BACK          1
#define FRAME         2
#define FULL          4
#define PART          8
#define PARTP         16
#define RUN           32
#define PAUSE         64
#define END           128


// states of FSM
#define FSM_STOPPED   1
#define FSM_RUNNUNG   2
#define FSM_PAUSED    3
#define FSM_END       4
#define FSM_LOW_BATT  5
#define FSM_POWEROFF  6
#define FSM_ERROR     7

#define VREF 1100

// Language Strings
#ifdef LANG_EN
#define STR_HM        "Hrs:Min"
#define STR_MS        "Min:Sec"
#define STR_Pause     "[Pause]"
#define STR_TIMEOVER  "Time over!"
#define STR_POWEROFF  "Power off..."
#define STR_CHARGING  "Charging..."
#define STR_BATSTATE  "Battery Voltage"
#endif

#ifdef LANG_DE
#define STR_HM        "Std:Min"
#define STR_MS        "Min:Sek"
#define STR_Pause     "[Pause]"
#define STR_TIMEOVER  "Zeit abgelaufen!"
#define STR_POWEROFF  "Ausschalten..."
#define STR_CHARGING  "Laden..."
#define STR_BATSTATE  "Batterie Spannung"
#endif

#ifdef LANG_HU
#define STR_HM        "Ora:Perc"
#define STR_MS        "Perc:Mprc"
#define STR_Pause     "[Szünet]"
#define STR_TIMEOVER  "Lejárt Idő!"
#define STR_POWEROFF  "Kikapcsolás..."
#define STR_CHARGING  "tölt..."
#define STR_BATSTATE  "Telep feszültség"
#endif

#ifdef LANG_IT
#define STR_HM        "Ora:Min"
#define STR_MS        "Min:Sec"
#define STR_Pause     "[Pausa]"
#define STR_TIMEOVER  "Tempo scaduto!"
#define STR_POWEROFF  "Fuori..."
#define STR_CHARGING  "Caricare..."
#define STR_BATSTATE  "Batteria Tensione"
#endif

#ifdef LANG_ES
#define STR_HM        "Hora:Min"
#define STR_MS        "Min:Seg"
#define STR_Pause     "[Pausa]"
#define STR_TIMEOVER  "Tiempo expirado!"
#define STR_POWEROFF  "Apagado..."
#define STR_CHARGING  "Carga..."
#define STR_BATSTATE  "Bateria Voltaje"
#endif


// Stock font and GFXFF reference handle
#define GFXFF 1

#ifndef TFT_DISPOFF
#define TFT_DISPOFF   0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN     0x10
#endif

#endif // KITCHEN_TIMER_H
