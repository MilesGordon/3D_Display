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
//this sets the input voltage range (input sentitivity)
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
void initMicrophone();
void FFTJoy();
void setPixel(int x, int y, int z, color col);
color getPixel(int x, int y, int z);
color colorMap(float val, float min, float max);
color lerpColor(color a, color b, int val, int min, int max);
short FFT(short int dir,int m,float *x,float *y);

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

void setup() {
  strip.begin(); // this initializes the NeoPixel library.
  initCube();
  initMicrophone();
}

void loop() {
  FFTJoy();
  //this sends the updated pixel color to the hardware.
  strip.show();
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

void initMicrophone()
{
  pinMode(GAIN_CONTROL, OUTPUT);
  digitalWrite(GAIN_CONTROL, LOW);
  analogReference(INTERNAL); //sets the resolution (volts / unit)
                             //e.g., DEFAULT=5V (0.0049V / unit), INTERNAL=1.1V (0.0011V / unit)
}

/********************************************
 *   FFT JOY functions
 * *****************************************/
 void FFTJoy(){
    for(int i=0;i<pow(2,M);i++)
    {
        real[i]=analogRead(MICROPHONE)-INPUT_LEVEL;
        delayMicroseconds(SAMPLE_RATE);

      //  Serial.print(real[i]);
        imaginary[i]=0;
    }
    FFT(1, M, real, imaginary);
    for(int i=0;i<pow(2,M);i++)
    {
        imaginary[i]=sqrt(pow(imaginary[i],2)+pow(real[i],2));
//        Serial.print(imaginary[i]);
        if(imaginary[i]>maxVal)
            maxVal=imaginary[i];
    }
    if(maxVal>100)
        maxVal-=0.5;
//    Serial.println();
    for(int i=0;i<pow(2,M)/2;i++)
    {
        imaginary[i]=SIDE*imaginary[i]/maxVal;
        int y;
        for(y=0;y<=imaginary[i];y++)
            setPixel(i,y,SIDE-1,colorMap(y,0,SIDE));
        for(;y<SIDE;y++)
            setPixel(i,y,SIDE-1,black);
    }
    for(int z=0;z<SIDE-1;z++)
        for(int x=0;x<SIDE;x++)
            for(int y=0;y<SIDE;y++)
            {
                color col=getPixel(x,y,z+1);
                setPixel(x,y,z,col);
//                char output[50];
//                sprintf(output, "%d %d %d:  %d %d %d", x,y,z+1, col.red, col.green, col.blue);
//                Serial.println(output);
            }

    sample++;
    if(sample>=pow(2,M))
        sample-=pow(2,M);
}

short FFT(short int dir,int m,float *x,float *y)
{
   int n,i,i1,j,k,i2,l,l1,l2;
   float c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   n = 1;
   for (i=0;i<m;i++)
      n *= 2;

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0;
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<n;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1;
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1)
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }

   return(0);
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