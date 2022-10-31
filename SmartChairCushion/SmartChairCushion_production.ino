#include <toneAC.h>
#include <LowPower.h>
//#include <CuteBuzzerSounds.h>
//#include <Sounds.h>
//#include "pitches.h"

//set this flag for testing mode
int testing = 1;

//status enum
#define MEASURE 0
#define CHECKING 1
#define SITTING 0
#define STANDUP 1

//threshold to tell if sensor is pressed
#define THRESHOLD 600 //the pull-up resistor is 1M, sensor untouched is >1M, so untouched must be > 50% of 1024. this is true for 2 sensors since they are connected together.

//Digital Pins 2-13 (green) PWM capable: 3, 5, 6, 9 10, 11
//#define BEEPER 8 // seems louder than pin 10 which is used for SPI
// use pin 9 and pin 10 for AcTone
#define SENSOR A0
#define SENSOR_DRIVE 4

int SITTING_LIMIT = 3600;  //3600s
int STANDUP_LIMIT = 300; //300s
int mode = MEASURE;
int lastStatus = SITTING;
int currentStatus = STANDUP;
int sittingCount = 0;
int standupCount = 0;

// the setup function runs once when you press reset or power the board
void setup() {

    testing = 0;
    
    // test mode/demmo mode uses short period
    if (testing){
      
      SITTING_LIMIT = 30; //30s
      STANDUP_LIMIT = 5;  //5s
    }
     
    // initialize output pin
    //pinMode(BEEPER, OUTPUT);
    pinMode(SENSOR_DRIVE, OUTPUT);

    // confirm current status
    while(lastStatus!=currentStatus){

      delay(100);
      lastStatus = measure();
      delay(100);
      currentStatus = measure();
      
    }//end of while

    // beep on startup: >90dB
    beepAC(100);

}

// confirm "new" sit
void beepSit(){

   // beep 0.1s in 4khz
   beepAC(100);
}

void beepAC(int period){

  toneAC(4000); 
  delay(period); 
  toneAC(0);      
}

// this will take <2s in active
// the peizo passive beeper rated as 0.1mA @3V per datasheet 
void beepStandup(){

    for(int i=0; i<3; i++){

        int v1 = measure();
        delay(100);
        int v2 = measure();

        if (v1 == v2){

            if(v1 == SITTING){
              
                 beepAC(100);
                 delay(100);
                 beepAC(100);
                 delay(100);
                 beepAC(100); 
            }// end of if sitting

            return; 
        }//end of v1==v2
        
    }//end of for 3 retries
    
}

// read ADC
int measure(){

  digitalWrite(SENSOR_DRIVE, HIGH);

  // this is a "must"; otherwise the reading will be always 0
  delay(1);
  
  int value = analogRead(SENSOR);
  digitalWrite(SENSOR_DRIVE, LOW);

  if (value > THRESHOLD){

    return STANDUP;
  }
  
  return SITTING;

}

// the loop function runs over and over again forever
void loop() {
    
    LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF); 

    // the following will take 0.6ms @ 3mA
    // when in power down mode, the power is 1uA per data sheet
    // a cell battery @ 200mAh will last 6 years if it beeps 8 times a day (3s per beep)
    if (mode == MEASURE){
      
        //read ADC
        int temp = measure();

        //confirm the change
        if (temp != lastStatus){

          lastStatus = temp; 
        }else{

          currentStatus = temp;
        }

        mode = CHECKING;
      
    }else{

        if (currentStatus == STANDUP){

            standupCount++;
  
            if (standupCount >= STANDUP_LIMIT){
  
                sittingCount = 0;
                standupCount = 0;
            }
            
        }else{

            // a "new" sitting, beep
            if (sittingCount == 0){

                //confirm sitting detected
                beepSit();
            }
            
            standupCount = 0;
            sittingCount++;

            if (sittingCount >= SITTING_LIMIT){

                //beep to notify 
                beepStandup();

                // doesn't beep since it is not a "new" sit
                // add beep after a half of notification time
                sittingCount = sittingCount/2;
            }
        }

        mode = MEASURE;
    }
  
}
