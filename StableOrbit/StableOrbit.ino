#include <Adafruit_NeoPixel.h>
#include <math.h>

//set up the pin that controls the LEDs, the type of LEDs (WS2812B) and the number of LEDs in the cube (8*8*8=512)
#define PIXEL_PIN 2
// How many NeoPixels are attached to the Arduino?
#define PIXEL_COUNT 512
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
#define PIXEL_TYPE NEO_GRB + NEO_KHZ800
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

#define SIDE 8

#define MICROPHONE 14
#define GAIN_CONTROL 3

//adjust this value for higher (< 1024) or lower (= 1024)
#define INPUT_LEVEL 550
//this sets our 'sample rate'.  I went through a bunch of trial and error to
//find a good sample rate to put a soprano's vocal range in the spectrum of the cube
#define SAMPLE_RATE 200  //98 hits just the right spot to put a 1KHz tone in the center spectrum of the cube
                         //however, the original author was right in choosing a higher value, as trial and
                         //error proved that most vocal and instrument frequencies are put in the center with 200

/*  datatype definitions
*/

typedef struct{
  unsigned char red, green, blue;
} color;

typedef struct{
  float x;
  float y;
  float z;
} point;

/******************************
 * function definitions
 * ***************************/
void initCube();
void setPixel(int x, int y, int z, color col);
void orbital(color col);
color getPixel(int x, int y, int z);
color colorMap(float val, float min, float max);
color lerpColor(color a, color b, int val, int min, int max);

/*********************************
 * FFTJoy variables *
 * *******************************/
//M adjusts the span (or spread) of the fft's frequency spectrum.
//Higher values widen the spectrum, lower values narrow. Through
//trial and error the optimal value for M was determined to be 4
//(making 2^4=16 samples), which proves to be the best setup for
//audio analysis in the cube's narrow span of led strips.
#define M 4
#define ARRAY_SIZE 16 //(int)pow(2,M)
float real[ARRAY_SIZE];
float imaginary[ARRAY_SIZE];
float maxVal=0;
float sample;

//maxBrightness is the brightness limit for each pixel.  All color data will be scaled down
//so that the largest value is maxBrightness
int maxBrightness=50;
color black;
color sun;
color earth;

int earthX = 0;
int earthY = 0;
int earthZ = 0;

int serialIn = 0;
int globalDelay = 1;

void setup() {
  strip.begin(); // this initializes the NeoPixel library.
  initCube();
  Serial.begin(115200);

  sun.red = 100;
  sun.blue = 6;
  sun.green = 26;

  earth.red = 0;
  earth.blue = 65;
  earth.green = 100;
}

void loop() {
// x z y
  int coords[3]; 

  setPixel(3, 3, 3, sun);
  setPixel(3, 3, 4, sun);
  setPixel(3, 2, 3, sun);
  setPixel(3, 2, 4, sun);
  setPixel(4, 3, 3, sun);
  setPixel(4, 3, 4, sun);
  setPixel(4, 2, 3, sun);
  setPixel(4, 2, 4, sun);
  
  if(Serial.available()){
//    setPixel(earthX, earthZ, earthY, black);

    for(int inx = 0; inx < 3; inx++){
      coords[inx] = (Serial.read() - 48);
      delay(globalDelay);
    }

    earthX = coords[0];
    Serial.println(earthX);
    earthY = coords[1];
    Serial.println(earthY);
    earthZ = coords[2];
    Serial.println(earthZ);
    setPixel(earthX, earthZ, earthY, earth);
    Serial.println(".");
  }
  else{
    delay(globalDelay);
  }
//  for(int inc = 0; inc < 3; inc++){
  fadePixel(5);
//  }
  //this sends the updated pixel color to the hardware.
  strip.show();
//  Serial.println("\n");
  delay(globalDelay); //50 x 0.2 works
}

/********************************************
 *   Initialization functions
 * *****************************************/
void initCube()
{
    black.red=0;
    black.green=0;
    black.blue=0;
}



/********************************************
 *   Pixel control functions
 * *****************************************/



//sets a pixel at position (x,y,z) to the col parameter's color
void setPixel(int x, int y, int z, color col)
{
    int index = (z*SIDE*SIDE) + (x*SIDE) + y;
        strip.setPixelColor(index,strip.Color(col.red,  col.green, col.blue));
   
}

//returns the color value currently displayed at the x,y,z location
color getPixel(int x, int y, int z)
{
    int index = (z*SIDE*SIDE) + (x*SIDE) + y;
    uint32_t col=strip.getPixelColor(index);
    color pixelColor;
    pixelColor.red=(col>>16)&255;
    pixelColor.green=(col>>8)&255;
    pixelColor.blue=col&255;
    return pixelColor;
}

void fadePixel(int fadespeed){
  for (int x = 0; x < 8; x++){
    for (int y = 0; y < 8; y++){
      for (int z = 0; z < 8; z++){
        color pixel = getPixel(x, y, z);
          if(pixel.red > 0){
            pixel.red -= fadespeed;
          }
          if(pixel.green > 0){
            pixel.green -= fadespeed;
          }
          if(pixel.blue > 0){
            pixel.blue -= fadespeed;
          }
          setPixel(x, y, z, pixel);
      }
    }
  }
}

//int breaThe = 0;
//uint32_t Red = pixels.Color(255, 0, 0);
//
//for (breaThe; breaThe < 5; breaThe++) {
//   for (brightIncrement = 0; brightIncrement < 240; brightIncrement = brightIncrement + 2) {
//     for (i = 0; i < ledNum; i++) {
//       pixels.setPixelColor(i, Green, brightIncrement);
//       pixels.show();
//     }
//   }
//   for (brightIncrement = 240; brightIncrement > 0; brightIncrement = brightIncrement - 2) {
//     for (i = 0; i < ledNum; i++) {
//       pixels.setPixelColor(i, Green, brightIncrement);
//       pixels.show();
//     }
//   }
//   for (i = 0; i < ledNum; i++) {
//     pixels.setPixelColor(i, 0, 0, 0);
//     pixels.show();
//   }
//}  

//returns a color from a set of colors fading from blue to green to red and back again
//the color is returned based on where the parameter *val* falls between the parameters
//*min* and *max*.  If *val* is min, the function returns a blue color.  If *val* is halfway
//between *min* and *max*, the function returns a yellow color.  
color colorMap(float val, float min, float max)
{
  float range=1024;
  val=range*(val-min)/(max-min);
  color colors[6];
  colors[0].red=0;
  colors[0].green=0;
  colors[0].blue=maxBrightness;
 
  colors[1].red=0;
  colors[1].green=maxBrightness;
  colors[1].blue=maxBrightness;
 
  colors[2].red=0;
  colors[2].green=maxBrightness;
  colors[2].blue=0;
 
  colors[3].red=maxBrightness;
  colors[3].green=maxBrightness;
  colors[3].blue=0;
 
  colors[4].red=maxBrightness;
  colors[4].green=0;
  colors[4].blue=0;

  colors[5].red=maxBrightness;
  colors[5].green=0;
  colors[5].blue=maxBrightness;
 
  if (val<=range/6)
    return(lerpColor(colors[0], colors[1], val, 0, range/6));
  else if (val<=2*range/6)
    return(lerpColor(colors[1], colors[2], val, range/6, 2*range/6));
  else if (val<=3*range/6)
    return(lerpColor(colors[2], colors[3], val, 2*range/6, 3*range/6));
  else if (val<=4*range/6)
    return(lerpColor(colors[3], colors[4], val, 3*range/6, 4*range/6));
  else if (val<=5*range/6)
    return(lerpColor(colors[4], colors[5], val, 4*range/6, 5*range/6));
  else
    return(lerpColor(colors[5], colors[0], val, 5*range/6, range));
}

//returns a color that's an interpolation between colors a and b.  The color
//is controlled by the position of val relative to min and max -- if val is equal to min,
//the resulting color is identical to color a.  If it's equal to max, the resulting color
//is identical to color b.  If val is (max-min)/2, the resulting color is the average of
//color a and color b
color lerpColor(color a, color b, int val, int min, int max)
{
    color lerped;
    lerped.red=a.red+(b.red-a.red)*(val-min)/(max-min);
    lerped.green=a.green+(b.green-a.green)*(val-min)/(max-min);
    lerped.blue=a.blue+(b.blue-a.blue)*(val-min)/(max-min);
    return lerped;
}

//void orbital(color col){
//    if(Serial.available()){
//    setPixel(earthX, earthZ, earthY, black);
//
//    for(int inx = 0; inx < 3; inx++){
//      coords[inx] = (Serial.read() - 48);
//      delay(1);
//    }
//
//    earthX = coords[0];
//    Serial.println(earthX);
//    earthY = coords[1];
//    Serial.println(earthY);
//    earthZ = coords[2];
//    Serial.println(earthZ);
//    setPixel(earthX, 2, earthY, earth);
//    Serial.println(".");
//  }
//  else{
//    delay(1);
//  }
//}


