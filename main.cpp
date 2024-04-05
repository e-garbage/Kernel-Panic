//Kernel Panic code
//This code is intended to be uploaded onto the arduino Nano of any Molto Brutto digital can
//Check Read-Me for detail about schematics, gerber and 3D printing files
//this code is a simplified version of an eurorack module
//code and design By e-garbage
//Inspired By Hagiwo's work

//licence : GPL v3.0

//Invoke vairous modules from the Mozzi Library 1.0.3
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h> 
#include <tables/cos2048_int8.h>
#include <Line.h>

//define global variables
#define CONTROL_RATE 128
#define POT_PIN_0 0 //not connected here
#define POT_PIN_3 3 //tone generator
#define CV_PIN 4 // gives trigger input
#define SENSOR_PIN_0 5 // gives random values from an antenna
#define SENSOR_PIN_1 6 // gives random values from an antenna

//define variables
int i,j,a , glitch_freq, glitch_out, tone_freq=3320, tone_out, tone_randomness, tone_randomness_2, tone_factor, main_out;
float lfo_freq=1.0 ;
byte gain = 255;
byte gain2 = 255;

//Define all the necessary Oscillators with their corresponding wavetable
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);
Oscil <COS2048_NUM_CELLS, AUDIO_RATE> aCos(COS2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil <COS2048_NUM_CELLS, AUDIO_RATE> aCos2(COS2048_DATA);
Oscil <COS2048_NUM_CELLS, AUDIO_RATE> aCosLFO(COS2048_DATA);

Line <unsigned int> aGain;


// define the input pin setup as well as initialise the oscillators
void setup(){
//PIN MODE
  pinMode(POT_PIN_0, INPUT_PULLUP);
  pinMode(POT_PIN_3, INPUT_PULLUP);
  pinMode(CV_PIN, INPUT_PULLUP);
  pinMode(SENSOR_PIN_0,INPUT_PULLUP);
  pinMode(SENSOR_PIN_1,INPUT_PULLUP);
//set OSC
  aSin.setFreq(3320); // for blip glitch generator
  aCos.setFreq(3320); // for blip glitch generator
  aSin2.setFreq(tone_freq); // for tone generator
  aCos2.setFreq(tone_freq); // for tone generator
  aCosLFO.setFreq(lfo_freq);
//start mozzi sound module
  startMozzi(); 
}


//infinit loop scanning and porcessing the input values (pot, sensor, CV)
void updateControl(){
//set up inputs read
  int pot_0 = mozziAnalogRead(POT_PIN_0); //no physical pot is connected to this pine in the Can version, but it gives a bit more randomness will picking up random values from the analog pin
  int pot_3 = mozziAnalogRead(POT_PIN_3); //tone generator
  int cv = mozziAnalogRead(CV_PIN); //the physical circuit is a CV input, but the code use it as Gate. Feel free to modify it to get actual CV
  int sensor_0 = mozziAnalogRead(SENSOR_PIN_0);
  int sensor_1 = mozziAnalogRead(SENSOR_PIN_1);

//BLIP GLITCH GENERATOR
//sound generator
 int glitch_factor = pot_0/10;
 int cycle_time = abs(glitch_factor-100);
  if (i <= cycle_time)
  {
    aSin.setFreq(glitch_freq);
    aCos.setFreq(glitch_freq);
    i++;
  }
  else {
    glitch_freq = (sensor_0 * 20); // remap 10 bits input value between 20Hz and 19'948Hz
    aSin.setFreq(glitch_freq);
    aCos.setFreq(glitch_freq);
    i=1;
  }

//pseudo enveloppe generator and CV control
  if (cv > 10)
  {
    if (j <= cycle_time)
      {
        gain = gain - (glitch_factor >> 2) +1;
        j++;
      }
    else {
        gain=cv;
        j=1;
      }
  }
  else
  {
    gain=2;
  }

//RANDOM TONE GENERATOR
//very harsh tone are generated
//set lfo gain modulation ranging from 0Hz to 68.20Hz
  lfo_freq = float(pot_3) / 15.0;
  aCosLFO.setFreq(lfo_freq);
  unsigned int gain3 = (128u+aCosLFO.next())<<8;
  aGain.set(gain3, AUDIO_RATE / CONTROL_RATE); 

//set the actual tones
  tone_factor = sensor_1; //pot_3
  if (tone_factor <= 100){
    tone_freq = 500;
    tone_randomness = abs(tone_freq * ((sensor_0/100+1)+(tone_factor+1)));
    tone_randomness_2 = abs((tone_freq + tone_factor+1) * ((sensor_1/80+1)+(tone_factor+1)));
  }
  else {
    tone_freq = 3320 * (tone_factor/100);
    tone_randomness = abs(tone_freq * ((sensor_0/100+1)*(tone_factor/10)));
    tone_randomness_2 = abs((tone_freq * tone_factor/100) * ((sensor_1/80+1)*(tone_factor/10)));
  }
  aSin2.setFreq(tone_randomness);
  aCos2.setFreq(tone_randomness);

//set Pot 0 to have a glitch only function (cut the tones when high enough)
  if (pot_0 > 1020){
    a=0;
  }
  else{
    a=1;
  }

}

//infinit loop that generate the actual audio signal
int updateAudio(){
  switch (a)
  {
  case 1:
    glitch_out= (gain * (aSin.next()+aCos.next()))>>8;
    main_out= (int)((long)((long) (aSin2.next()+aCos2.next()) * gain >>2 ) ) + glitch_out;
  break;
  case 0:
    glitch_out= (gain * (aSin.next()+aCos.next()))>>8;
    main_out = glitch_out;
  break;
  }
  return main_out;
}

void loop(){
  audioHook(); // required here
}