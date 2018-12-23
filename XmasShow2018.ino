#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN D6
#define LEDPIN 13

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
// Adafruit_NeoPixel strip = Adafruit_NeoPixel(300, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// Main pattern architecture based on this resource:
// https://learn.adafruit.com/multi-tasking-the-arduino-part-3/some-common-bits
enum pattern { NONE, TWINKLE_STAR, MORSE };
enum direction { FORWARD, REVERSE };
const unsigned int NUM_LIGHTS = 300;

struct LightData
{
  uint32_t currentColor;
  direction dir;
  bool enabled;
  short twinkleCount;
  uint8_t brightness;
};

class MorseHelper
{
  public: 
  
  MorseHelper()
  {
    
  }

  String toMorseString(char input)
  {
    switch (input)
    {
      case 'A': return "SL";
      case 'B': return "LSSS";
      case 'C': return "LSLS";
      case 'D': return "LSS";
      case 'E': return "S";
      case 'F': return "SSLS";
      case 'G': return "LLS";
      case 'H': return "SSSS";
      case 'I': return "SS";
      case 'J': return "SLLL";
      case 'K': return "LSL";
      case 'L': return "SLSS";
      case 'M': return "LL";
      case 'N': return "LS";
      case 'O': return "LLL";
      case 'P': return "SLLS";
      case 'Q': return "LLSL";
      case 'R': return "SLS";
      case 'S': return "SSS";
      case 'T': return "L";
      case 'U': return "SSL";
      case 'V': return "SSSL";
      case 'W': return "SLL";
      case 'X': return "LSSL";
      case 'Y': return "LSLL";
      case 'Z': return "LLSS";
      case ' ': return "X";
      default: return "";
    }
  }
};

class LightPatterns : public Adafruit_NeoPixel
{
  public:

    pattern ActivePattern = NONE;

    unsigned long Interval;
    unsigned long AppearInterval = 2000;
    unsigned long NewPatternStart = 0;
    unsigned long ChangeInterval = 60000;
    uint16_t Index;
    direction Direction;

    unsigned long lastUpdate;
    unsigned long lastTwinkle = 0;

    LightData stars[NUM_LIGHTS];
    const int NUM_TWINKLES = 3;

    void (*OnComplete)(); // callback

    LightPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
      : Adafruit_NeoPixel(pixels, pin, type)
    {
      OnComplete = callback;
    }

    void Update()
    {
      if (ActivePattern == TWINKLE_STAR && (lastTwinkle == 0 || (millis() - lastTwinkle) > AppearInterval))
      {
        lastTwinkle = millis();
        int newLightIndex = getRandomUnlitStarIndex();
        setStarOn(newLightIndex);
        AppearInterval = random(500,1000);
      }

      if ((millis() - lastUpdate) > Interval)
      {
        lastUpdate = millis();
        switch (ActivePattern)
        {
          case TWINKLE_STAR:
            TwinkleStarUpdate();
            break;
          case MORSE:
            MorseUpdate();
            break;
          default:
            break;
        }
      }

      if (millis() - NewPatternStart > ChangeInterval)
      {
        switch(ActivePattern)
        {
          case TWINKLE_STAR:
            Morse("MERRY CHRISTMAS");
            break;
          case MORSE:
            TwinkleStar(50);
            break;
        }
      }
    }

    int getRandomUnlitStarIndex()
    {
      int randomIndex = random(0, NUM_LIGHTS);
      while (stars[randomIndex].enabled == true)
      {
        randomIndex = random(0, NUM_LIGHTS);
      }
      return randomIndex;
    }

    void setStarOn(int index)
    {
      stars[index].enabled = true;
      stars[index].twinkleCount = 0;
      stars[index].dir = FORWARD;
      stars[index].currentColor = Color(0, 0, 0);
      stars[index].brightness = 0;
    }

    void setStarOff(int index)
    {
      stars[index].enabled = false;
      stars[index].twinkleCount = 0;
      stars[index].dir = FORWARD;
      stars[index].currentColor = Color(0, 0, 0);
      stars[index].brightness = 0;
    }

    void TwinkleStar(uint8_t interval, direction dir = FORWARD)
    {
      ClearStars();
      ActivePattern = TWINKLE_STAR;
      Direction = dir;
      Interval = interval;
      NewPatternStart = millis();
    }

    void TwinkleStarUpdate()
    {
      int fadeInterval = 5;

      for (int i = 0; i < NUM_LIGHTS; i++)
      {
        if (stars[i].enabled == true)
        {
          if (stars[i].dir == FORWARD)
          {
            fadeInterval = 5;
          }
          else
          {
            fadeInterval = -5;
          }

          int b = stars[i].brightness;
          int newBrightness = b + fadeInterval;

          if (newBrightness > 255)
          { 
            stars[i].dir = REVERSE;
            newBrightness = 255;
            stars[i].brightness = newBrightness;
          } 
          else if (newBrightness <= 0)
          {
            stars[i].dir = FORWARD;
            newBrightness = 0;
            stars[i].brightness = newBrightness;
            
            stars[i].twinkleCount ++;
            if (stars[i].twinkleCount >= NUM_TWINKLES)
            {
              setStarOff(i);
            }
          }
          else
          {
            stars[i].brightness = newBrightness;
            stars[i].currentColor = Color(newBrightness, newBrightness, newBrightness >> 2);
          }
          setPixelColor(i, stars[i].currentColor);
        }
        else
        {
          setPixelColor(i, Color(0, 0, 0));
        }
        
      }
      show();
    }

    void Morse(String text)
    {
      Interval = 80;
      Index = 0;
      ActivePattern = MORSE;
      MorseHelper helper;
      NewPatternStart = millis();
      
      int len = text.length();
      int lightIndex = 0;
      uint32_t redColor = Color(255, 0, 0);
      uint32_t greenColor = Color(0, 255, 0);
      uint32_t color = redColor;
      bool isRed = true;
      
      for(int i = 0; i < len; i ++)
      {
        char c = text[i];
        String letterCode = helper.toMorseString(c);
        int morseLetterCount = letterCode.length();
        color = isRed ? redColor : greenColor;
        
        for(int j = 0; j < morseLetterCount; j++)
        {
          if (lightIndex > NUM_LIGHTS - 1) 
          {
            break;
          }
          
          if (letterCode[j] == 'S')
          {
             setPixelColor(lightIndex, color);
          }
          else if (letterCode[j] == 'L')
          {
            setPixelColor(lightIndex, color);
            lightIndex++;
            setPixelColor(lightIndex, color);
            lightIndex++;
            setPixelColor(lightIndex, color);
            lightIndex++;
            setPixelColor(lightIndex, color);
            lightIndex++;
            setPixelColor(lightIndex, color);
            lightIndex++;
            setPixelColor(lightIndex, color);
            lightIndex++;
            setPixelColor(lightIndex, color);
          }
          else if (letterCode[j] == 'X')
          {
            setPixelColor(lightIndex, Color(0,0,0));
            lightIndex++;
            setPixelColor(lightIndex, Color(0,0,0));
            lightIndex++;
            setPixelColor(lightIndex, Color(0,0,0));
            lightIndex++;
            setPixelColor(lightIndex, Color(0,0,0));
            lightIndex++;
            setPixelColor(lightIndex, Color(0,0,0));
          }
          else
          {
            setPixelColor(lightIndex, Color(255, 255, 0)); // error parsing it
          }

          lightIndex ++;
          setPixelColor(lightIndex, Color(0,0,0));
          lightIndex ++;
          setPixelColor(lightIndex, Color(0,0,0));
          lightIndex ++;
          setPixelColor(lightIndex, Color(0,0,0));
          lightIndex ++;
          setPixelColor(lightIndex, Color(0,0,0));
          lightIndex ++;
        }
        
        isRed = !isRed;
      }

      for(int i = lightIndex; i < NUM_LIGHTS; i++)
      {
        setPixelColor(i, Color(0,0,0));
      }
      show();
    }

    void MorseUpdate()
    {
      uint32_t nextColor;
      
      for(int i = NUM_LIGHTS - 1; i > 0; i--)
      {
        uint32_t nextColor;

        if (i == NUM_LIGHTS - 1)
        {
          nextColor = getPixelColor(i);
          setPixelColor(0, nextColor);
        }
        setPixelColor(i, getPixelColor(i - 1));
      }
      show();
    }

    void ClearStars()
    {
      // clear stars
      for (int i = 0; i < NUM_LIGHTS; i++)
      {
        setStarOff(i);
        setPixelColor(i, stars[i].currentColor);
      }
      show();
    }
};

void MainStripComplete();
LightPatterns MainStrip(300, PIN, NEO_GRB + NEO_KHZ800, &MainStripComplete);

void setup() {
  randomSeed(analogRead(0));

  MainStrip.begin();
  //MainStrip.TwinkleStar(50);
  MainStrip.Morse("MERRY CHRISTMAS");
}

void loop() {
  MainStrip.Update();
}

void MainStripComplete() {

}
