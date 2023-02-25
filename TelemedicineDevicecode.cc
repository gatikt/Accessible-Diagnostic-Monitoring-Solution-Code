#include "Wire.h"
#include "Arduino.h"
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "SPI.h"                //  spi
#include "Adafruit_GFX.h"       //  graphics
#include "Adafruit_ILI9341.h"   //  display
#include "XPT2046_Touchscreen.h"
#include "MAX30105.h"
#include "heartRate.h"
#include "Average.h"  

Average<float> averageValue(100);  
 

const byte MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640
const byte RATE_SIZE = 2; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
int sampleNumber = 0;      // variable to store the sample number   
int sensorPin = A6;       // select the input pin for the Pressure Sensor  
int sensorValue = 0;      // variable to store the Raw Data value coming from the sensor  
   
float averageInitialValue = 0; // variable to store the average inital value  
   
float diffPressure = 0;     // variable to store converted kPa value   
float volumetricFlow = 0;    // variable to store volumetric flow rate value   
float velocityFlow = 0;     // variable to store velocity of flow value   
float diffOffset = 0;        // variable to store offset differential pressure  
   
 //constants - these will not change  
const float tubeArea1 = 0.01592994; // area of venturi tube first section 0.003095 0.01592994  
const float tubeArea2 = 0.0042417; // area of venturi tube second section  
const float airDensity = 1.225;
 

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

static float mlx90640To[768];
paramsMLX90640 mlx90640;

#define TFT_DC          2
#define TFT_CS          15
#define TFT_MOSI        23
#define TFT_CLK         18
#define TFT_RST         4
#define TFT_MISO        19

#define TOUCH_CS          33
#define TOUCH_IRQ         35

#define _sclk             25
#define _mosi             32
#define _miso             39

#define TFT_LANDSCAPE             1
#define TFT_PORTRAIT_NORMAL       2
#define TFT_LANDSCAPE_INVERT      3
#define TFT_PORTRAIT_INVERT       4

const uint8_t TFT_ORIENTATION = TFT_LANDSCAPE_INVERT;


//  INITIALIZATION ******************************************************
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
XPT2046_Touchscreen touch( TOUCH_CS, TOUCH_IRQ );
TS_Point rawLocation;

MAX30105 particleSensor;

void setup()
{
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz
  
  SPI.begin(_sclk, _miso, _mosi);
  SPI.setClockDivider(1);//divide the clock by 1
  SPI.beginTransaction(SPISettings(60000000, MSBFIRST, SPI_MODE0));
  SPI.endTransaction();
  Serial.println("ILI9341 Touch & Go Display Test!");
  tft.begin(10000000);
  tft.setRotation(TFT_ORIENTATION);
  Serial.println("Menus test...");
  tft.fillScreen(ILI9341_BLACK);
  ShowMenu();
  tft.setTextColor( ILI9341_BLACK, ILI9341_WHITE );
  tft.setCursor(30, 15);
  tft.println( "Touch screen ready!" );
  touch.begin();
  delay(100);
 }
  

void ShowMenu()
{
  tft.fillRoundRect(240, 10, 80, 40, 10, ILI9341_CYAN);
  tft.setCursor(253, 27);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Spirometer");
  tft.fillRoundRect(240, 55, 80, 40, 10, ILI9341_CYAN);
  tft.setCursor(262, 72);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Oximeter");
  tft.fillRoundRect(240, 100, 80, 40, 10, ILI9341_CYAN);
  tft.setCursor(262, 116);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Thermal");
  tft.fillRoundRect(240, 145, 80, 40, 10, ILI9341_GREEN);
  tft.setCursor(253, 161);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("OVR HEALTH");
  tft.fillRoundRect(240, 190, 80, 40, 10, ILI9341_MAGENTA);
  tft.setCursor(263, 206);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("RESET");
  tft.fillRoundRect(5, 5, 230, 25, 10, ILI9341_WHITE);
  tft.fillRoundRect(5, 35, 230, 200, 10, ILI9341_WHITE);
 
}

int checkForButton(int x, int y) {
  if (x < 60) {
    return 0; //  this is the rectangle boundary check anything to the left of 60(less than) is ignored
  }
  // isolate the rows(buttons)

  if (y > 9 && y < 18) {
    return 1;
  }
  if (y > 20 && y < 32) {
    return 2;
  }
  if (y > 34 && y < 46) {
    return 3;
  }
  if (y > 48 && y < 60) {
    return 4;
  }
  if (y > 61 && y < 74) {
    return 5;
  }
}

void loop()
{
  while ( touch.touched() )
  {
    rawLocation = touch.getPoint();
    int bp = checkForButton(rawLocation.x / 50, rawLocation.y / 50);
    if (bp == 1)
    {
      getSpiro();
    }
    if (bp == 2)
    {
      getOxi();
    }
    if (bp == 3)
    {
      getThermal();
    }
    if (bp == 4)
    {
      getOvrh();
    }
    if (bp == 5)
    {
      initiateReset();
    }
  }

} //  end of loop


void getThermal(){
    if ( TFT_ORIENTATION == TFT_LANDSCAPE_INVERT )
  {
    tft.drawPixel( map( rawLocation.x, 340, 3900, 0, 320 ), map( rawLocation.y, 200, 3850, 0, 240 ), ILI9341_GREEN );
  }
  if ( TFT_ORIENTATION == TFT_LANDSCAPE )
  {
    tft.drawPixel( map( rawLocation.x, 340, 3900, 320, 0 ), map( rawLocation.y, 200, 3850, 240, 0 ), ILI9341_GREEN );
  }
  tft.fillRoundRect(5, 5, 230, 25, 10, ILI9341_WHITE);
  tft.fillRoundRect(5, 35, 230, 200, 10, ILI9341_WHITE);
  tft.setCursor(30, 15);
  tft.setTextSize(1);
  tft.println( "Touch screen ready!" );
  
 int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
    Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
    Serial.println("Parameter extraction failed");
    
    for (byte x = 0 ; x < 2 ; x++) //Read both subpages
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0)
    {
      Serial.print("GetFrame Error: ");
      Serial.println(status);
    }

    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
    float emissivity = 1;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
  }
  for (int x = 0 ; x < 1 ; x++)
  {
    delay(2000);
    tft.setCursor(25, 60);
    tft.setTextSize(2);
    tft.print("Body Temp");
    tft.print(":");
    tft.print(((mlx90640To[x]* 2.0)+ 32), 2);
    tft.print("F");
    tft.println();
    delay(1000);
    tft.setTextSize(1);
    tft.fillRoundRect(240, 100, 80, 40, 10, ILI9341_GREEN);
    tft.setCursor(262, 116);
    tft.setTextColor(ILI9341_BLACK);
    tft.print("Thermal");
  }
  }

void getOxi() {

tft.fillRoundRect(5, 5, 230, 25, 10, ILI9341_WHITE);
tft.fillRoundRect(5, 35, 230, 200, 10, ILI9341_WHITE);
tft.setCursor(30, 15);
tft.setTextSize(1);
tft.println( "Touch screen ready!" );

if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
{
tft.println("MAX30105 was not found ");
while (1);
}
tft.setCursor(40, 70);
tft.setTextSize(2);
tft.println("Place finger");
particleSensor.setup(); //Configure sensor with default settings
delay(3500);
long delta = millis() - lastBeat;
lastBeat = millis();
 
beatsPerMinute = 80 / (delta / 10000.0);
 
if (beatsPerMinute < 255 && beatsPerMinute > 20)
{
rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
rateSpot %= RATE_SIZE; //Wrap variable
 
//Take average of readings
beatAvg = 0;
for (byte x = 0 ; x < RATE_SIZE ; x++)
beatAvg += rates[x];
beatAvg /= RATE_SIZE;
}
tft.setCursor(40, 100);
tft.setTextSize(2);
tft.print(" SPO2 %=");
tft.print(floor(100 - ((beatsPerMinute-beatAvg)/beatAvg)));
tft.setCursor(40, 120); 
tft.setTextSize(2);
tft.print(" BPM=");
tft.print(beatsPerMinute);
tft.setCursor(40, 140); 
tft.setTextSize(2);
delay(1000);
tft.setTextSize(1);
tft.fillRoundRect(240, 55, 80, 40, 10, ILI9341_GREEN);
tft.setCursor(262, 72);
tft.setTextColor(ILI9341_BLACK);
tft.print("Oximeter");
}

void getSpiro() {

tft.fillRoundRect(5, 5, 230, 25, 10, ILI9341_WHITE);
tft.fillRoundRect(5, 35, 230, 200, 10, ILI9341_WHITE);
tft.setCursor(30, 15);
tft.setTextSize(1);
tft.println( "Touch screen ready!" );
pinMode(sensorPin, INPUT);  // Pressure sensor is on Analogue pin 0 
  
// get initial sensor value  
   for (int i = 0; i < 100; i++) {  
       
     // read the value from the sensor:   
     sensorValue = analogRead(sensorPin) * 1.51; 
              
     //push sensor values to averageValue object  
     averageValue.push(sensorValue);  
   }  
   
   for (int i = 0; i < 100; i++)   
   {  
    // get average Sensor values  
    averageValue.get(i);  
      
   }  
   
   //calculate mean average sensor and store it  
   averageInitialValue = averageValue.mean(); 

//read the value from the sensor:   
   sensorValue = analogRead(sensorPin);   
     
   // initial value   
   sensorValue = sensorValue - (int)averageInitialValue;  
    
   // increment sample counter   
   sampleNumber++;   
   
  // map the Raw data to kPa  
  diffPressure = map(sensorValue, 0, 1023, 0, 4000);   
   
  if (sensorValue >= 0)  
    {  
     //calculate volumetric flow rate for Exhalation  
     volumetricFlow = tubeArea1 * (sqrt((2/airDensity) * (diffPressure/(sq(tubeArea1/tubeArea2)-1))));  
   
     //calculate velocity of flow   
     velocityFlow = volumetricFlow / tubeArea1;  
    }  
  // convert reading to a positive value  
  else if (sensorValue <= 0) {  
   diffPressure = diffPressure *-1;  
      
    //calculate volumetric flow rate for Inhalation  
    volumetricFlow = tubeArea2 * (sqrt((2/airDensity) * (diffPressure/(1-sq(tubeArea2/tubeArea1)))));  
      
    //calculate velocity of flow   
    velocityFlow = volumetricFlow / tubeArea2;  
  }  
    
  // Print the results as comma separated values for easier processing  
  // in a spreadsheet program  
  
  tft.setCursor(20, 80);
  tft.setTextSize(2);
  tft.print("Exhale into Tube");
  delay(4000);
  tft.setCursor(20, 110); 
  tft.setTextSize(2);
  tft.print("diffPress");
  tft.print(diffPressure);  
  tft.setCursor(22, 130); 
  tft.setTextSize(2);
  tft.print("vol flow=");
  tft.print(volumetricFlow * 1000);
  tft.setCursor(25, 150); 
  tft.setTextSize(2);
  tft.print("vel flow=");
  tft.print(velocityFlow); 
  tft.setCursor(25, 170);
  tft.print("FVC% =");
  tft.print(((diffPressure-(volumetricFlow * 1000) )/diffPressure) * 100); 
  tft.println(); 
  delay(1000);
  tft.fillRoundRect(240, 10, 80, 40, 10, ILI9341_GREEN);
  tft.setCursor(253, 27);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Spirometer");
   
  // wait 100 milliseconds before the next loop  
  // for the analog-to-digital converter and  
  // pressure sensor to settle after the last reading:   
     
}

void initiateReset() { tft.fillRoundRect(240, 10, 80, 40, 10, ILI9341_CYAN);
  tft.setCursor(253, 27);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Spirometer");
  tft.fillRoundRect(240, 55, 80, 40, 10, ILI9341_CYAN);
  tft.setCursor(262, 72);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Oximeter");
  tft.fillRoundRect(240, 100, 80, 40, 10, ILI9341_CYAN);
  tft.setCursor(262, 116);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Thermal");
  tft.fillRoundRect(240, 145, 80, 40, 10, ILI9341_GREEN);
  tft.setCursor(253, 161);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("OVR HEALTH");
  tft.fillRoundRect(240, 190, 80, 40, 10, ILI9341_MAGENTA);
  tft.setCursor(263, 206);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("RESET");
  tft.fillRoundRect(5, 5, 230, 25, 10, ILI9341_WHITE);
  tft.fillRoundRect(5, 35, 230, 200, 10, ILI9341_WHITE);
  tft.setCursor(30, 15);
  tft.setTextSize(1);
  tft.println( "Touch screen ready!" );}
  
void getOvrh() {
  float health;
  tft.fillRoundRect(5, 5, 230, 25, 10, ILI9341_WHITE);
  tft.fillRoundRect(5, 35, 230, 200, 10, ILI9341_WHITE);
  tft.setCursor(30, 15);
  tft.setTextSize(1);
  tft.println( "Touch screen ready!" );
  tft.setCursor(30, 50);
  tft.setTextSize(2);
  health = (2 * (floor(100 - ((beatsPerMinute-beatAvg)/beatAvg))) + 2 * (((diffPressure-(volumetricFlow * 1000) )/diffPressure) * 100) + ((mlx90640To[x]* 2.0)+ 32))/ ((floor(100 - ((beatsPerMinute-beatAvg)/beatAvg))) + (((diffPressure-(volumetricFlow * 1000) )/diffPressure) * 100) + ((mlx90640To[x]* 2.0)+ 32));
  If ( health > 97 && health <=100 ){
  tft.print("Normal");}
  If ( health >=94 && health < 97 ){
  tft.print(" Slightly Abnormal");}
  If ( health >=90 && health <94 ){
  tft.print(" Abnormal");}
   If ( health <90 ){
  tft.print(" Severely Abnormal");}
   }
