//arduino test sketch for TEF6621 (should be compatible to TEF6601 TEF6606 TEF6607 TEF6613 TEF6614 TEF6616 TEF6617 TEF6623 TEF6624)

#include <Wire.h> 

#define BUS_SLAVE_ADRESS B1100000 //I2C slave adress 

//band for subadress TUNER0 
#define BAND_LW_MW   B00000000 //AM: LW and MW
#define BAND_FM      B00100000 //FM: standard Europe, USA and Japan
#define BAND_SW      B01000000 //AM: SW
#define BAND_FM_OIRT B01100000 //FM: OIRT (eastern Europe)

//modes for write
#define MODE_STANDARD    B00000000  //write without tuning action
#define MODE_PRESET      B00100000  //tune to new station with short mute time
#define MODE_SEARCH      B01000000  //tune to new station and stay muted
#define MODE_AF_UPDATE   B01100000  //tune to AF station; store AF quality and tune back to main station
#define MODE_AF_JUMP     B10000000  //tune to AF station in minimum mute time
#define MODE_AF_CHECK    B10100000  //tune to AF station and stay muted
#define MODE_MIRROR_TEST B11000000  //check current image situation and select injection mode for best result
#define MODE_END         B11100000  //release mute from search mode or AF check mode

//subadresses for write
#define TUNER0       0x0   //default data byte: B00100110
#define TUNER1       0x1   //default data byte: B11111010
#define TUNER2       0x2   //default data byte: B00000000
#define RADIO        0x3   //default data byte: B10000000
#define SOFTMUTE0    0x4   //default data byte: B00000000
#define SOFTMUTE1    0x5   //default data byte: B00000000
#define SOFTMUTE2    0x6   //default data byte: B00000000
#define HIGHCUT0     0x7   //default data byte: B00000000
#define HIGHCUT1     0x8   //default data byte: B00000000
#define HIGHCUT2     0x9   //default data byte: B00000000
#define STEREO0      0xA   //default data byte: B00000000
#define STEREO1      0xB   //default data byte: B00000000
#define STEREO2      0xC   //default data byte: B00000000
#define CONTROL      0xD   //default data byte: B00010100
#define LEVEL_OFFSET 0xE   //default data byte: B01000000
#define AM_LNA       0xF   //default data byte: B00000100
#define RDS          0x10  //default data byte: B01000000
#define EXTRA        0x11  //default data byte: B00000000

//read register
#define STATUS     0x0
#define LEVEL      0x1
#define USN_WAM    0x2
#define IFCOUNTER  0x3
#define ID         0x4
#define RDS_STATUS 0x5
#define RDS_DAT3   0x6
#define RDS_DAT2   0x7
#define RDS_DAT1   0x8
#define RDS_DAT0   0x9
#define RDS_DATEE  0xA

//status bits
#define QRS  B11000000 //quality read status
                       //00 = no quality data available (tuning is in progress or quality data is settling)
                       //01 = quality data (LEVEL, USN and WAM) available;
                       //10 = AF update
#define POR  B00100000 //power-on reset indicator
                       //0 = normal operation
                       //1 = power on or power
#define STIN B00010000 //stereo indicator
                       //0 = no pilot detected
                       //1 = stereo pilot detected
#define RDAV B00000100 //RDS new data available
                       //0 = no data available
                       //1 = RDS new data available
#define TAS  B00000011 //tuning action state
                       //00 = tuning not active, not muted
                       //01 = muting in progress
                       //10 = tuning in progress
                       //11 = tuning ready and muted
                       
//local fm stations                      
unsigned long BFBS = 103000;
unsigned long WDR4 = 100500;
unsigned long EINSLIVE = 105500;
unsigned long WDR5 = 90600;
unsigned long WDR2B = 93200;

//level detector (RSSI) 0 to 255 = 0.25 V to 4.25 V
uint8_t level()
{
  return readDataByte(LEVEL);
}

//if signal good -> station found
bool stationFound()
{
  return (usn() < 2) && (level() > 200) && (wam() < 2 );
}

//compute frequency from kHz to needed format
unsigned int freq(unsigned long freq_rf,uint8_t band)
{
  switch(band)
  {
    case BAND_LW_MW: return freq_rf;
    break;
    case BAND_SW: return freq_rf * 5;
    break;
    case BAND_FM_OIRT: return freq_rf * 100 / 1000;
    break;
    default:
    case BAND_FM:  return freq_rf * 20 / 1000;
    break;
  }
}

//USN FM ultrasonic noise; 0 to 15 = 0 % to 100 % equivalent FM modulation at 100 kHz ultrasonic noise content (USN)
uint8_t usn()
{
  return readDataByte(USN_WAM) >> 4;
}

//WAM FM wideband AM (multipath); 0 to 15 = 0 % to 100 % AM modulation at 20 kHz wideband AM content
uint8_t wam()
{
  return readDataByte(USN_WAM) & B00001111;
}

//tune to new station with short mute time;
void preset(unsigned long freq_rf, uint8_t band)
{
  Wire.beginTransmission(BUS_SLAVE_ADRESS);
  Wire.write(MODE_PRESET | TUNER0);
  Wire.write(BAND_FM | freq12_8(freq(freq_rf, band)));
  Wire.write(freq7_0(freq(freq_rf, band)));
  Wire.endTransmission();  
}

//tune to new station and stay muted;
void search(unsigned long freq_rf, uint8_t band)
{
  Wire.beginTransmission(BUS_SLAVE_ADRESS);
  Wire.write(MODE_SEARCH | TUNER0);
  Wire.write(BAND_FM | freq12_8(freq(freq_rf, band)));
  Wire.write(freq7_0(freq(freq_rf, band)));
  Wire.endTransmission();  
}

//release mute from search mode or AF check mode
void endMute()
{
  Wire.beginTransmission(BUS_SLAVE_ADRESS);
  Wire.write(MODE_END | TUNER0);
  Wire.endTransmission();  
}

//read data byte from adress
uint8_t readDataByte(uint8_t adress)
{  
  uint8_t data_byte;
  Wire.beginTransmission(BUS_SLAVE_ADRESS);    
  Wire.requestFrom(BUS_SLAVE_ADRESS, adress + 1);
  for(uint8_t i = 0; i < adress + 1; i++)
    data_byte = Wire.read(); 
  Wire.endTransmission(); 
  return data_byte;   
}

//get 5 highest bits from freq
uint8_t freq12_8(unsigned int freq)
{
  return freq >> 8;
}

//get 8 lowest bits from freq
uint8_t freq7_0(unsigned int freq)
{
  return freq;
}

//search next station from freq
unsigned long nextStationFrom(unsigned long freq, uint8_t band)
{
  unsigned int i = 0;
  bool found = false;
  if (band == BAND_FM)
  {
    unsigned int steps = (108000 - freq) / 100;    
    do
    {
      i++;
      search(freq + 100 * i, band);
      delay(100);
      found = stationFound();
    }while((!found) && (!(i == steps))); 
  }
  if(found)
    endMute();
  return freq + 100 * i;    
}

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  
  preset(WDR5, BAND_FM); 
  delay(10000); 
    
  Serial.println(nextStationFrom(WDR5, BAND_FM));
}

void loop()
{
}
