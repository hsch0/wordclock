#include <Arduino.h>
#include <DS1307RTC.h>   //http://www.arduino.cc/playground/Code/Time
#include <TimeLib.h>
#include <Timezone.h>    //https://github.com/JChristensen/Timezone
#include <Wire.h>        //http://arduino.cc/en/Reference/Wire (supplied with the Arduino IDE)
#include <FastLED.h>

#define NUM_LEDS 110
#define MAX_BRIGHTNESS 255

//Central European Time (Berlin)
//EU rules: summer time starts last Sunday in March at 02:00 local (01:00 UTC)
//and ends last Sunday in October at 03:00 local (01:00 UTC)
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, +120};    //Daylight time = UTC +2 hours
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, +60};      //Standard time = UTC +1 hours
Timezone myTZ(myDST, mySTD);

//If TimeChangeRules are already stored in EEPROM, comment out the three
//lines above and uncomment the line below.
//Timezone myTZ(100);       //assumes rules stored at EEPROM address 100

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;

char inData[26]; // Allocate some space for the string
char inChar=-1; // Where to store the character read
byte index = 0; // Index into array; where to store the character

byte ledIntensity_r = 232;
byte ledIntensity_g = 200;
byte ledIntensity_b = 10;
CRGB leds[NUM_LEDS];
byte leds_old[NUM_LEDS][3];
byte leds_new[NUM_LEDS][3];

byte prevMinute = 0;
const byte byteSize = sizeof(byte);

void printTime(time_t t, const char *tz);
void sPrintI00(int val);
void sPrintDigits(int val);
char ChangeSettings();
void showTime();
void showMinuteWords();
void showHour();
void showLeds(const byte array[], byte arrayLength);
void rainbow(uint8_t wait);

// QClock words definitions
const byte UHR[] = {2, 1, 0};
const byte ES_IST[] = {99, 100, 102, 103, 104};

const byte EIN[]    = {52, 51, 50};
const byte EINS[]    = {52, 51, 50, 49};
const byte ZWEI[]   = {54, 53, 52, 51};
const byte DREI[]   = {34, 35, 36, 37};
const byte VIER[]   = {25, 24, 23, 22};
const byte FUENF[]   = {40, 41, 42, 43};
const byte SECHS[]    = {9, 8, 7, 6, 5};
const byte SIEBEN[]  = {49, 48, 47, 46, 45, 44};
const byte ACHT[]   = {12, 13, 14, 15};
const byte NEUN[]  = {29, 28, 27, 26};
const byte ZEHN[]   = {16, 17, 18, 19};
const byte ELF[]    = {32, 31, 30};
const byte ZWOELF[] = {60, 61, 62, 63, 64, 65};

const byte FUENF_NACH[]         = {106, 107, 108, 109, 74, 73, 72, 71};
const byte ZEHN_NACH[]          = {98, 97, 96, 95, 74, 73, 72, 71};
const byte VIERTEL_NACH[]       = {81, 82, 83, 84, 85, 86, 87, 74, 73, 72, 71};
const byte ZWANZIG_NACH[]       = {94, 93, 92, 91, 90, 89, 88, 74, 73, 72, 71};
const byte ZEHN_VOR_HALB[]      = {98, 97, 96, 95, 70, 69, 68, 55, 56, 57, 58};
const byte FUENF_VOR_HALB[]     = {106, 107, 108, 109, 70, 69, 68, 55, 56, 57, 58};
const byte HALB[]               = {55, 56, 57, 58};
const byte FUENF_NACH_HALB[]    = {106, 107, 108, 109, 74, 73, 72, 71, 55, 56, 57, 58};
const byte ZEHN_NACH_HALB[]     = {98, 97, 96, 95, 70, 69, 68, 55, 56, 57, 58};
const byte ZWANZIG_VOR[]        = {94, 93, 92, 91, 90, 89, 88, 70, 69, 68};
const byte VIERTEL_VOR[]        = {81, 82, 83, 84, 85, 86, 87, 70, 69, 68};
const byte ZEHN_VOR[]           = {98, 97, 96, 95, 70, 69, 68};
const byte FUENF_VOR[]          = {106, 107, 108, 109, 70, 69, 68};


void setup(void)
{
    Serial.begin(115200);
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    // The following lines can be uncommented to set the date and time
    //setTime(8,33,50,21,11,2018);
    //RTC.set(now());
  
    if(timeStatus()!= timeSet) 
        Serial.println(F("Unable to sync with the RTC"));
    else
        Serial.println(F("RTC has set the system time"));    

    FastLED.addLeds<WS2801, RGB>(leds, NUM_LEDS);
    //rainbow(3);
}

void loop(void)
{
    utc = now();
    printTime(utc, "UTC");
    local = myTZ.toLocal(utc, &tcr);
    printTime(local, tcr -> abbrev);

    showTime();
    delay(1000);
    
    ChangeSettings();
    
    // update system time from rtc every hour.
    if(minute(local)==0 && second(local)==0) {
        Serial.println(F("RTC Update"));
        setSyncProvider(RTC.get);   // the function to get the time from the RTC
        //rainbow(3);
        if(timeStatus()!= timeSet) 
            Serial.println(F("Unable to sync with the RTC"));
        else
            Serial.println(F("RTC has set the system time")); 
     }
    
}



int StartsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 0;
   return 1;
}

char ChangeSettings() {
    while (Serial.available() > 0) // Don't read unless
                                   // there you know there is data
    {
        if(index < 25) // One less than the size of the array
        {
            inChar = Serial.read(); // Read a character
            inData[index] = inChar; // Store it
            index++; // Increment where to write next
            inData[index] = '\0'; // Null terminate the string
        }
    }

    if (StartsWith(inData,"RGB")  == 0) {
        // RGB:230,200,010
        Serial.println(F("New RGB settings"));
        int a = (inData[4] - '0') * 100;
        int b = (inData[5] - '0') * 10;
        int c = (inData[6] - '0');
        ledIntensity_r = a + b + c;

        a = (inData[8] - '0') * 100;
        b = (inData[9] - '0') * 10;
        c = (inData[10] - '0');
        ledIntensity_g = a + b + c;

        a = (inData[12] - '0') * 100;
        b = (inData[13] - '0') * 10;
        c = (inData[14] - '0');
        ledIntensity_b = a + b +c ;
        
        for (int i=0;i<25;i++) {
            inData[i]=0;
        }
        index=0;
        return(0);
    }
    else if(StartsWith(inData,"TIME")  == 0) {
        // TIME:2016,05,27,19,23,31
        // TIME:2018,11,21,08,37,31
        Serial.println(F("New TIME settings"));
        int a = (inData[5] - '0') * 1000;
        int b = (inData[6] - '0') * 100;
        int c = (inData[7] - '0') * 10;
        int d = (inData[8] - '0') * 1;
        int year = a + b + c + d;

        a = (inData[10] - '0') * 10;
        b = (inData[11] - '0');
        int month = a + b;

        a = (inData[13] - '0') * 10;
        b = (inData[14] - '0');
        int day = a + b;

        a = (inData[16] - '0') * 10;
        b = (inData[17] - '0');
        int hour = a + b;

        a = (inData[19] - '0') * 10;
        b = (inData[20] - '0');
        int minute = a + b;

        a = (inData[22] - '0') * 10;
        b = (inData[23] - '0');
        int second = a + b;
        
        setTime(hour,minute,second,day,month,year);
        RTC.set(now());
        
        for (int i=0;i<25;i++) {
            inData[i]=0;
        }
        index=0;
        return(0);
    }
    else {
        return(1);
    }
}


//Function to print time with time zone
void printTime(time_t t, const char *tz)
{
    sPrintI00(hour(t));
    sPrintDigits(minute(t));
    sPrintDigits(second(t));
    Serial.print(' ');
    Serial.print(dayShortStr(weekday(t)));
    Serial.print(' ');
    sPrintI00(day(t));
    Serial.print(' ');
    Serial.print(monthShortStr(month(t)));
    Serial.print(' ');
    Serial.print(year(t));
    Serial.print(' ');
    Serial.print(tz);
    Serial.println();
}

//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
    if (val < 10) Serial.print('0');
    Serial.print(val, DEC);
    return;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
    Serial.print(':');
    if(val < 10) Serial.print('0');
    Serial.print(val, DEC);
}

//Cylon - LED sweeps back and forth, with the color, delay and number of cycles of your choice 
void cylon(CRGB color, uint16_t wait, uint8_t number_of_cycles)
{
  FastLED.setBrightness(255);
  for (uint8_t times = 0; times<=number_of_cycles; times++)
  {
    // Make it look like one LED is moving in one direction
    for(int led_number = 0; led_number < NUM_LEDS; led_number++)
    {
      //Apply the color that was passed into the function
      leds[led_number] = color;
      //Actually turn on the LED we just set
      FastLED.show();
      // Turn it back off
      leds[led_number] = CRGB::Black;
      // Pause before "going" to next LED
      delay(wait);
    }

    // Now "move" the LED the other direction
    for(int led_number = NUM_LEDS-1; led_number >= 0; led_number--)
    {
      //Apply the color that was passed into the function
      leds[led_number] = color;
      //Actually turn on the LED we just set
      FastLED.show();
      // Turn it back off
      leds[led_number] = CRGB::Black;
      // Pause before "going" to next LED
      delay(wait);
    }
  }
  return;
}


void rainbow(uint8_t wait) 
{

  uint16_t hue;
  FastLED.clear();

  for(hue=10; hue<255*3; hue++) 
  {

    fill_rainbow( &(leds[0]), NUM_LEDS /*led count*/, hue /*starting hue*/);    
    FastLED.show();
    delay(wait);
  }
  return;
}

void showTime() {
  // reset all leds

  memset(leds, 0, NUM_LEDS * 3);
  memset(leds_new, 0, NUM_LEDS * 3);

  showMinuteWords();
  showHour();

  for(byte i=0; i<=NUM_LEDS-1; i++) {
     leds[i][0] = leds_old[i][0];   
     leds[i][1] = leds_old[i][1];
     leds[i][2] = leds_old[i][2]; 
  }
  
  //fading to new value...
  for(byte i=0; i<=NUM_LEDS-1; i++) {
    leds[i].r = leds_new[i][0];
    leds[i].g = leds_new[i][1];
    leds[i].b = leds_new[i][2];
    FastLED.show();
  }
    /*
    while(leds_new[i][0] != leds[i].r || leds_new[i][1] != leds[i].g || leds_new[i][2] != leds[i].b){
      Serial.print(leds_new[i][0]);
      Serial.print('R');
      Serial.print(leds[i].r);
      Serial.println();
      Serial.print(leds_new[i][1]);
      Serial.print('G');
      Serial.print(leds[i].g);
      Serial.println();
      Serial.print(leds_new[i][2]);
      Serial.print('B');
      Serial.print(leds[i].b);
      Serial.println();
      //delay(10);
      if(leds_new[i][0] > leds[i].r){
        leds[i].r++;
        leds[i].r++;
        leds[i].r++;
        leds[i].r++;
      }
      else if(leds_new[i][0] < leds[i].r){
        leds[i].r--;
        leds[i].r--;
        leds[i].r--;
        leds[i].r--;
      }
  
      if(leds_new[i][1] > leds[i].g){
        leds[i].g++;
      }
      else if(leds_new[i][1] < leds[i].g){
        leds[i].g--;
      }
  
      if(leds_new[i][2] > leds[i].b){
        leds[i].b++;
        leds[i].b++;
        leds[i].b++;
        leds[i].b++;
      }
      else if(leds_new[i][2] < leds[i].b){
        leds[i].b--;
        leds[i].b--;
        leds[i].b--;
        leds[i].b--;
      }
      FastLED.show();
    }
  }
  */


  
  

  for(byte i=0; i<=NUM_LEDS-1; i++) {
     leds_old[i][0] = leds[i].r;   
     leds_old[i][1] = leds[i].g;
     leds_old[i][2] = leds[i].b; 
  }
}

void showMinuteWords() {
  showLeds(ES_IST, 5);

  if(minute(local) < 5) {
    showLeds(UHR, 3);
  } else if(minute(local) < 10) {
    showLeds(FUENF_NACH, 8);
  } else if(minute(local) < 15) {
    showLeds(ZEHN_NACH, 8);
  } else if(minute(local) < 20) {
    showLeds(VIERTEL_NACH, 11);
  } else if(minute(local) < 25) {
    showLeds(ZWANZIG_NACH, 11);
  } else if(minute(local) < 30) {
    showLeds(FUENF_VOR_HALB, 11);
  } else if(minute(local) < 35) {
    showLeds(HALB, 4);
  } else if(minute(local) < 40) {
    showLeds(FUENF_NACH_HALB, 12);
  } else if(minute(local) < 45) {
    showLeds(ZWANZIG_VOR, 10);
  } else if(minute(local) < 50) {
    showLeds(VIERTEL_VOR, 10);
  } else if(minute(local) < 55) {
    showLeds(ZEHN_VOR, 7);
  } else {
    showLeds(FUENF_VOR, 7);
  }
}

void showHour() {
  byte stunde = hour(local);

  if(minute(local) > 24) {
    stunde++;
  }

  if((stunde == 1 || stunde == 13) && minute() < 5) {
    showLeds(EIN, 3);
  } else if(stunde == 1 || stunde == 13) {
    showLeds(EINS, 4);
  } else if(stunde == 2 || stunde == 14) {
    showLeds(ZWEI, 4);
  } else if(stunde == 3 || stunde == 15) {
    showLeds(DREI, 4);
  } else if(stunde == 4 || stunde == 16) {
    showLeds(VIER, 4);
  } else if(stunde == 5 || stunde == 17) {
    showLeds(FUENF, 4);
  } else if(stunde == 6 || stunde == 18) {
    showLeds(SECHS, 5);
  } else if(stunde == 7 || stunde == 19) {
    showLeds(SIEBEN, 6);
  } else if(stunde == 8 || stunde == 20) {
    showLeds(ACHT, 4);
  } else if(stunde == 9 || stunde == 21) {
    showLeds(NEUN, 4);
  } else if(stunde == 10 || stunde == 22) {
    showLeds(ZEHN, 4);
  } else if(stunde == 11 || stunde == 23) {
    showLeds(ELF, 3);
  } else if(stunde == 12 || stunde == 24  || stunde == 0) {
    showLeds(ZWOELF, 5);
  }
}

void showLeds(const byte array[], byte arrayLength) {
  for(byte i=0; i<=arrayLength-1; i++) {
    leds_new[array[i]][0] = ledIntensity_r; 
    leds_new[array[i]][1] = ledIntensity_b; 
    leds_new[array[i]][2] = ledIntensity_g;
  }
}
