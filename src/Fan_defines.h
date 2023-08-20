//----------------------------------------------------------------------
//NOVA_Defines.h  Include file for NOVA Control
//
//  Placed the defines (no code) here to keep the main .ino cleaner
//----------------------------------------------------------------------

#include <ADC.h>
#include <FreqMeasureMulti.h>

// ========= Serial Port Defines =================
//  Using USB to Serial-TTL for connection to PC
#define Rx_Pin 0
#define Tx_Pin 1

// ========= Fonts for Display =================
// Special for ili9341_t3n display library
// had to change LiberationSansNarrowxxx file to work with ili9341_t3n (not needed forili9341_t3 )
// why special ili9341_t3n display library? One special feature: no need to run a fill rect cmd to clear text under new text
// ili9341_t3n has this feature when using custom fonts
// Why custom fonts? The LiberationSansNarrow fonts allows more text to be displayed and looks great
#include <ili9341_t3n_font_ArialBold.h>
#include <ili9341_t3n_font_Arial.h>
#include <ili9341_t3n_font_LiberationSansNarrow.h>
#include <ili9341_t3n_font_LiberationSansNarrowBold.h>

#define start = 0
//#define Protoboard1
/* ======  ILI9341_t3n  Highly Optimized Display driver for ili9314 =====================
- Download the 'ILI9341_t3n' ZIP file from https://github.com  ILI9341_t3 already good ILI9341_t3n a few more features
Do not use the Arduino IDE "Add ZIP library" feature, instead we are going to manually install it into your Arduino libraries folder
Uncompress the ZIP file (ILI9341_t3n-master.zip)
Rename the uncompressed folder from ILI9341_t3-master to ILI9341_t3
Delete any existing copies of ILI9341_t3 from your Arduino/libraries folder
Copy the new ILI9341_t3 folder into your Arduino/libraries folder
Close and reopen the Arduino IDE 
In Platform IO, just search in Libraies and add, then change the two references from ILI9341_t3 to ILI9341_t3n */
#include <ILI9341_t3n.h>
 // ----------  LCD TFT Assignment ILI9341 320x240 (NON Adafruit Screen (RED))------------

//FOR TEENSY 3.5
// #define TFT_SCLK 14 // defaults to 13, alternate is 14,27
// #define TFT_CS 15 // DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23 
// #define TFT_RST  255 //  not needed, fake out =255 , tie to 3.3V
// #define TFT_DC  10 //
// #define TFT_MOSI 7  //[SOI/MOSI]defaults to pin 11, alt pin 7,28,  
// #define TFT_MISO  8  //[SDO/MISO] defaults to pin 12,alt pin 8,39 
//FOR TEENSY 4.1
#define TFT_SCLK 13 // defaults to 13, alternate is 14,27
#define TFT_CS 10 // DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23 
#define TFT_RST  255 //  not needed, fake out =255 , tie to 3.3V
#define TFT_DC  37 //
#define TFT_MOSI 11  //[SOI/MOSI]defaults to pin 11, alt pin 7,28,  
#define TFT_MISO  12  //[SDO/MISO] defaults to pin 12,alt pin 8,39 


//#define TFT_LED  19  // tie to 3.5 
//   Vcc     // tie to 3.3V, NOT 5V,
ILI9341_t3n display = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO) ;
#define RGB(r,g,b) (r<<11|g<<5|b)  //  R = 5 bits 0 to 31, G = 6 bits 0 to 63, B = 5 bits 0 to 31

#ifdef TouchEnabled 
    char Ver[]="FAN_CTRL 0.1a";  //Touch
  #include <XPT2046_Touchscreen.h>
  #define TCH_IRQ   9   // unique for touch
  #define TCH_CS    6   // same as TFT display
  XPT2046_Touchscreen ts(TCH_CS, TCH_IRQ);  // Param 2 - Touch IRQ Pin - interrupt enabled polling
  bool Touch_Enabled=false;
#else
  char Ver[]="V1.0";  //No Touch

#endif

#ifdef ProtoboardFinal 
  int PWM_Array[] = {2,3,4,5};
  int FAN_TAC_pin[4]={28,29,22,23}; //{5,6,10}; //pin #s-5, 6, 9, 10, 20, 21, 22, 23 for Teensy 3.x, 6, 9, 10, 20, 22, 23 for LC
  int pwminvert = 0;
#else
  int Fan_Array[] = {2,3,4,5};
  int PWM_Array[] = {23};
  int FAN_TAC_pin[4]={6,7,8,9}; //{5,6,10}; //pin #s-5, 6, 9, 10, 20, 21, 22, 23 for Teensy 3.x, 6, 9, 10, 20, 22, 23 for LC
  int pwminvert =  0;
#endif

//========================== Setup Screen info===================================
bool setupval = true;
int setupindx = 0;
int setupintval = 10;
bool setupchange = true;
bool setupsafety = false;
typedef struct { 
  String fannum;
  int wires;
  String dual;
  String size;
  float current;
  int rpm1;
  int rpm2;
  int index;
} Fan;

//fakeout fan
Fan fanfake = {"1535COFANFake",3,"N","120 x 40mm",.50,4200,0,1};

int fav1idx = 0;
int fav2idx = 0;
int fav3idx = 0;
int fav4idx = 0;
int fav5idx = 0;
//change based on how many fans
int updateindex = 0;
int fansize;
bool empty = false;

//keep size in increments of 10 larger than fan size
Fan fans[60] = {};
Fan currfan;

//===========================Set up Safety========================================
int latchPin = A14;
bool latched;
int Safetyclk = 200;
bool dislatched = false;
int PowerClk = 200;
bool ridenconn = true;
int discontimer = 400;
int ridenvs;
bool rconnchange = true;

//========Set-up Fan visual controls====================================================
int FanClk = 100;
int fanstates[4] = {0,0,0,0};
bool fanpower = true; bool poweredoff = false;
bool fanbools[4] = {false, false, false, false};
bool fanstop[4] = {false,false,false,false};
bool fandis[4] = {false, false, false, false};
bool fanconnected[4] = {true, true, true, true};
int x =20,y=30; int w = 284/4,h = 184;

// ----------  NOVA FAN Bar Tach - Define Var and Teensy Pin#  ------------
//    Connected to 12V Fan Power Supply, with resistor divider
//    Nominal raw Fan Freq 160Hz -> need to /30 to achieve typical 4800rpm spec

bool FAN_TAC_enable=true;
FreqMeasureMulti FAN_TAC[4];  // array based measurement, max instances limited to physical Teensy (ex. Teensy 3.5 supports 8)
long FAN_TAC_Intervalms = 500; // will read down to 30Hz , increase interval or tweak fan rountine if lower values needed
long FAN_TAC_Timems=millis();
 
int FAN_TAC_fancnt=4;
int FAN_TAC_indx=0;
int FAN_TAC_RPMspec;  // Typical FAN rpm (from spec sheet)
int FAN_TAC_RPMspec2;
int FAN_TAC_limitLow=4500;  // min FAN rpm warning
int FAN_TAC_FreqToRPM=30;  // Freq to RPM (varies by Fan)
int FAN_TAC_Freqspec;  //FAN_TAC_RPMspec/FAN_TAC_FreqToRPM 4800/30=160
int FAN_TAC_Freqspec2;  //FAN_TAC_RPMspec/FAN_TAC_FreqToRPM 4800/30=160
int FAN_TAC_RPM[4]={0,0,0,0}; 
float FAN_TAC_FREQ[4]={0,0,0,0};   

bool FAN_TAC_DevDisplay=!true; // Send All Fan Updates directly to Serial Port & TFT display
bool FAN_TAC_Debg2=!true; // Enable to capture FAN Tach Data for fan 0 and send to serial port
 
 // Following only needed for development or alternate FAN TACH Routines
int a[30];
int FAN_TAC_a[4]={0,0,0,0}; 
float FAN_TAC_f[30]={0,0,0,0}; // only needed in alternate FAN TACH routine

long ft0,ft[4]; // need to keep between routine executions
bool FAN_TAC_tick=false;
int tacclk = 200;

//=========Set-up Visual Data ================================
int DataClk = 100;
bool statechange = false;
bool splashed = false;

//Buffer to read csv
char buff[80];

//======Keypad + Pwr PB assigment =======================================================
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
byte rowPins[ROWS] =  {A0,A1,A2,A3}; //  connect to the row pinouts of the keypad
byte colPins[COLS] = {A4,A5,A6,A7}; // //connect to the column pinouts of the keypad
char keys[ROWS][COLS] =   {'1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D'};
int keycntr=0, key=0, pKey=0, keywait=100, kpntr=0;  //50;


//=====10-sec Test===========
#define voltstate 19900
#define voltcheck 17050
#define fan1run   17000
#define fan1check 15550
#define fan2run   15500
#define fan2check 14050
#define fan3run   14000
#define fan3check 12550
#define fan4run   12500
#define fan4check 11050
#define fullvolt  10000
#define fullcheck 6000
#define halfvolt  5000
#define halfcheck 1000
int tensecclk;
bool tensectest = false;
int status = 3;
bool fanoffstate = false;
bool tenstatechange = false;
bool tensectestflag = false;
bool fanspassfail = false;
int clearclk = -1;
String testsetup[] = {"Untested", "Untested", "Untested", "Untested"};
String testfull[] = {"Untested", "Untested", "Untested", "Untested"};
String test50[] = {"Untested", "Untested", "Untested", "Untested"};
bool passfail[] = {true, true, true, true};
bool updatedpsfl[] = {false, false, false, false};

//    Connected to 12V Fan Power Supply, with resistor divider
float PS_12Vscale = 5.00/1.047; //(8.2k + 2.2k)/2.2k  8.2 in series   scale voltage read thru divider
float PS_12V_v= 0;
 int PS_12V = A17; //  Analog pin for 12V voltages
 bool PS_12V_adc= 0;// need to indicate which adc (teensy 3.5 has 2) is used per pin , see 'Teensy3.5_AnalogCard'
int PS_Averaging = 16;
int PS_indx = 0;
float PS_avgtotal = 0;
int PS_adcScale = 4096; // gets set after ADC Setup in void Setup 
int VoltTimer = 100;
ADC *adc = new ADC();; // adc object
int statetimer = 200;
int VPState = 1;
int fakeout = 0;
int ridenvolt = 0;
int ridencurr = 0;
int setcurr = 0;
int currmult = 1;
int setvolt = 0;
bool checkread = false;
int checkreadclk = -1;
bool half = false;
bool rpmhalf = false;
int autoclk = 350;
#define off 1
#define volt0 2
#define on 3
#define volt1200 4
#define delaystate 5
#define checkandread 6
#define idle 7
#define halfpow 8
#define checkidle 9
#define preon 10
//===========================================================================================

  bool DispTFT=true;   
  bool dispSet = false;  // end all display sequences with  //if (dispSet)  display.display(); if true

// =================  Display variables  ===============================
#define inv_false not inv_true
//            RGB FORMAT RRRR RGGG GGGB BBBB  - Note 5 bits Red & Blue, 6 bits for Green
#define c_BLACK   0x0000  // can also not define here and use ST77XX_BLACK for color defines
#define c_BLUE    0x001F  //0000 0000 0001 1111
#define c_ORANGE  0xFC00
#define c_RED     0xF800  //1111 1000 0000 0000
#define c_GREEN   0x07E0  //0000 0111 1110 0000
#define c_CYAN    0x07FF
#define c_MAGENTA 0xF81F
#define c_YELLOW  0xFFE0  //1111 1111 1110 0000 
#define c_WHITE   0xFFFF
#define co_WHITE   1       // for Oled 1306 library - expose or error when PlatformC compile
#define c_GREY    0x31E7  // 0011 0001 1110 0111 //B 0000 1000 0010 0001   //B 0011 1000 1110 0111
int b_color,f_color;
 int dispRefresh = 400;  //time between display refresh
 int dispCntr =0;
 int dispOn = 1400;
 int dispOff =700;
bool dispState = false;



//LED Blink defines & Variables Left
 int blnkon = 1000;
 int blnkoff =500;
 int blnkcntr =0;
 bool blnkstate = false;
 int  blnkLED = LED_BUILTIN;  //13;




// =================  Keypad Display variables  ===============================
// define some values used by the panel and buttons
char lcd_key   = 0;
char lastkey=0;
bool btnDisplay=false; 
//String btnMsg=""; 
char btnMsg[80];
char bMsg[16];
long btnDisplayTimeout=0;
int indx;

// ========= Serial & LCD Format , control and defines =================
char buf[120];
String serMsg="";


const unsigned char fan1ico [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0x80, 0x00, 0x00, 
0x00, 0x01, 0xff, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x7f, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x1f, 0xfe, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x0f, 0xfe, 0x00, 0xf8, 0x00, 
0x00, 0x00, 0x07, 0xfe, 0x01, 0xfc, 0x00, 
0x00, 0x00, 0x07, 0xfc, 0x03, 0xfe, 0x00, 
0x03, 0xfc, 0x07, 0xf8, 0x07, 0xfe, 0x00, 
0x0f, 0xff, 0x07, 0x78, 0x0f, 0xff, 0x00, 
0x0f, 0xff, 0xcc, 0x0c, 0x1f, 0xff, 0x00, 
0x1f, 0xff, 0xf8, 0x06, 0x7f, 0xff, 0x00, 
0x3f, 0xff, 0xf1, 0xc3, 0xff, 0xff, 0x00, 
0x3f, 0xff, 0xf3, 0xf3, 0xff, 0xff, 0x80, 
0x3f, 0xff, 0xe3, 0xf3, 0xff, 0xff, 0x00, 
0x7f, 0xff, 0xe3, 0xf1, 0xff, 0xff, 0x00, 
0x3f, 0xff, 0xf3, 0xf3, 0xff, 0xff, 0x00, 
0x7f, 0xff, 0xf1, 0xe3, 0xff, 0xff, 0x00, 
0x3f, 0xff, 0xf0, 0x07, 0xff, 0xfe, 0x00, 
0x3f, 0xff, 0x18, 0x06, 0xff, 0xfe, 0x00, 
0x3f, 0xfc, 0x0e, 0x18, 0x7f, 0xfc, 0x00, 
0x3f, 0xf8, 0x07, 0xf8, 0x1f, 0xf0, 0x00, 
0x1f, 0xf0, 0x0f, 0xf0, 0x01, 0xc0, 0x00, 
0x1f, 0xf0, 0x1f, 0xf8, 0x00, 0x00, 0x00, 
0x0f, 0xc0, 0x1f, 0xf8, 0x00, 0x00, 0x00, 
0x01, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x3f, 0xff, 0x80, 0x00, 0x00, 
0x00, 0x00, 0x7f, 0xff, 0xc0, 0x00, 0x00, 
0x00, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x00, 
0x00, 0x00, 0x7f, 0xff, 0xe0, 0x00, 0x00, 
0x00, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x00, 
0x00, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x00, 
0x00, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x00, 
0x00, 0x00, 0x1f, 0xff, 0xc0, 0x00, 0x00, 
0x00, 0x00, 0x0f, 0xff, 0xc0, 0x00, 0x00, 
0x00, 0x00, 0x07, 0xff, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char fan2ico [] PROGMEM {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0xb0, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 
0x00, 0x3f, 0x80, 0x07, 0xff, 0x80, 0x00, 
0x00, 0x7f, 0xf0, 0x07, 0xff, 0x80, 0x00, 
0x00, 0xff, 0xe0, 0x07, 0xff, 0xc0, 0x00, 
0x01, 0xf7, 0xf8, 0x07, 0xff, 0xe0, 0x00, 
0x03, 0xff, 0xf8, 0x07, 0xff, 0xe0, 0x00, 
0x07, 0xff, 0xfc, 0x07, 0xff, 0xf0, 0x00, 
0x07, 0xff, 0xfc, 0x07, 0xff, 0xe0, 0x00, 
0x07, 0xff, 0xfe, 0x07, 0xff, 0xf0, 0x00, 
0x0f, 0xff, 0xfe, 0x0f, 0xff, 0xd0, 0x00, 
0x07, 0xff, 0xfe, 0x07, 0xff, 0xe0, 0x00, 
0x0f, 0xff, 0x7e, 0x0f, 0xff, 0xc0, 0x00, 
0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 
0x07, 0xff, 0xff, 0x5f, 0xff, 0x80, 0x00, 
0x03, 0xff, 0xfc, 0x1f, 0xfe, 0x80, 0x00, 
0x00, 0xf5, 0xf0, 0x03, 0xfc, 0x00, 0x00, 
0x00, 0x00, 0x78, 0xe3, 0xf4, 0x00, 0x00, 
0x00, 0x00, 0x33, 0xf3, 0x40, 0x00, 0x00, 
0x00, 0x00, 0x13, 0xf1, 0x80, 0x00, 0x00, 
0x00, 0x00, 0x33, 0xf1, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x93, 0xf3, 0x00, 0x00, 0x00, 
0x00, 0x02, 0xf0, 0xe1, 0x80, 0x00, 0x00, 
0x00, 0x1b, 0xf8, 0x83, 0xa0, 0x00, 0x00, 
0x00, 0x1f, 0xf8, 0x07, 0xff, 0xf0, 0x00, 
0x00, 0x3f, 0xff, 0x1f, 0xff, 0xf8, 0x00, 
0x00, 0x7f, 0xff, 0x7f, 0xff, 0xf8, 0x00, 
0x00, 0xff, 0xff, 0x7f, 0xff, 0xfc, 0x00, 
0x01, 0xff, 0xfc, 0x1f, 0xff, 0xfe, 0x00, 
0x01, 0xff, 0xfc, 0x1f, 0xff, 0xfc, 0x00, 
0x03, 0xff, 0xf8, 0x1f, 0xff, 0xfc, 0x00, 
0x01, 0xff, 0xf8, 0x0f, 0xff, 0xfc, 0x00, 
0x01, 0xff, 0xf8, 0x1f, 0xff, 0xf8, 0x00, 
0x01, 0xff, 0xf8, 0x07, 0xff, 0xf0, 0x00, 
0x00, 0xff, 0xf8, 0x07, 0xff, 0xf8, 0x00, 
0x00, 0xff, 0xfc, 0x03, 0xff, 0xe0, 0x00, 
0x00, 0x7f, 0xf8, 0x01, 0xff, 0x40, 0x00, 
0x00, 0xbf, 0xf8, 0x01, 0x7f, 0x80, 0x00, 
0x00, 0x3f, 0xf8, 0x00, 0x36, 0x00, 0x00, 
0x00, 0x0f, 0xe8, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};