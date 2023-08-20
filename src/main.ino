#include <Arduino.h>
#include <iostream>
#include <Keypad.h>
#include <limits.h>
#include <SPI.h>
#include <SD.h>
#include <Fan_defines.h>

// put function declarations here:
// crc modbus site = https://www.tahapaksu.com/crc/
// modbus reg site = https://www.simplymodbus.ca/FC06.htm
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

File myFile;

void setup() {
  //===================   Setup Display =========================================
  display.begin();
  display.setRotation(3);
  display.setTextWrap(false);
  display.setCursor(0, 0);
  ShowSplash();
  delay(3000);


    //==========  setup ADC  ========================
      ///// ADC0 ADC1 ////
    adc->adc0->setAveraging(PS_Averaging); // set number of averages
    adc->adc0->setResolution(12); // set bits of resolution
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED); // change the conversion speed
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed
    adc->adc1->setAveraging(PS_Averaging); // set number of averages
    adc->adc1->setResolution(12); // set bits of resolution
    adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED); // change the conversion speed
    adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed
    PS_adcScale= adc->adc0->getMaxValue();

  //===================   Setup Serial Port =========================================
  //Serial port to the riden
  Serial1.setRX(Rx_Pin);
  Serial1.setTX(Tx_Pin);
  Serial1.begin(38400);
  //Serial port to the comp
  Serial8.setRX(34);
  Serial8.setTX(35);
  Serial8.begin(38400);
  //Send version to Serial Port
  delay(200);
  Serial8.println(Ver); // Send version statup to serial port 
  Serial8.println("~"); //  '~' is flag character for Marco Express

  // Initialize SD card and check if Fan file exists
  if (SD.begin(BUILTIN_SDCARD)) {
    Serial8.println("Initialized");
    if(SD.exists("test.csv") and SD.exists("favindex.csv")) {
      Serial8.println("Both Files have been found in the SD card");
    }
    else {
      Serial8.println("Missing some files...");
    }
  } else {
    Serial8.println("Did not initialize");
  }
  //==========  Setup FAN TACH   ========================
  if (FAN_TAC_enable) {for ( int i = 0; i < FAN_TAC_fancnt; i++)  FAN_TAC[i].begin(FAN_TAC_pin[i]);}

  //Set up Latch pin
  pinMode(latchPin, INPUT_PULLUP);

  //Set up pins to control fans
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  //Check the riden model
  CheckRiden();
  //Pull Favorites and Fans from the SD card
  UpdateTable();
}
//===========================Loop Process===============================//
/* 
Loop Process:
1. Setup Process
  1. Turn off the riden power if on
  2. Show the Setup screen with any updates based on flag
  3. Process the Setup Button Keypad
2. Main Loop
  1. Check if power connected in riden (NEEDS WORK)
  2. If latch is connected, continue else ONLY CheckAndRead
  3. If TenSecTest on, process the tensectest buttons, else process the general buttons
  4. Process the Fan Images
  5. Display the Changed Data
  6. Turn on Fans 
  7. Set the PWM
  8. Run TenSecTest if on
  9. Clear TensecTest if clear flag high
  10. Get the Fan Tac
  11. Get Voltage Divider info
  12. Control the State of the Riden
  13. Regardless of Latch CheckandRead Scenario
3. Always Running functions
  1. Check if Safety Latch is on
  2. Manage the Safety States of Riden Control
  3. Delay program by 1 ms
*/
void loop() {
  if (setupval) {
    if (!setupsafety) {VPState = off; setupsafety = true;} //1.1
    ShowSetup(); //1.2
    Setup_Buttons(); //1.3
  }
  else {
    //Process_Blink();
    PowerCheck(); //2.1
    if (latched) { //2.2
      if (tensectest) { //2.3
        TenSec_Buttons();
      } else {
        Main_Buttons();
      }
      Process_Fans(); //2.4
      DisplayData(); //2.5
      FanSet(); //2.6
      PWMSet(); //2.7
      TenSecTest(); //2.8
      TenSecClear(); //2.9
      Get_Fan_Tac(); //2.10
      Get_PS_Voltages(); //2.11
      StateControl(); //2.12
    }
    CheckandRead(); //2.13
  }
  SafetyCheck(); //3.1
  SafetyControl(); //3.2
  delay(1); //3.3
}


// put function definitions here:

//=========== Show Splash =============
// Shows the trenton logo and version
void ShowSplash() {
  display.setFontAdafruit();
  display.fillScreen(c_BLACK);
  display.setTextSize(3);
  display.setTextColor(c_ORANGE);
  display.setCursor(20, 100);
  display.println("Fan Controller");//  18+ for Hiletgo 320x240
  display.setTextColor(c_GREEN);
  display.setTextSize(2);
  display.setCursor(30, 140);display.print(Ver);
  display.setTextColor(c_WHITE);
  display.setTextSize(2);
}


//=========== Show Setup =============
// Shows the trenton logo and version
// Add FAV for the favorites add index add fansize.
void ShowSetup() {
  if (setupchange) {
    display.setFontAdafruit();
    display.fillScreen(c_BLACK);
    display.setFont(LiberationSansNarrow_14_Bold); display.setTextColor(c_WHITE);
    display.drawString("Fan Selection", 95, 10);
    display.setFont(LiberationSansNarrow_10_Bold);
    int h = (display.height()-60)/10;
    //This section adds the lines for the setup scren
    for (int i = 0; i < 11; i++) {
      display.drawLine(0, i*h + 30, display.width(), i*h + 30, c_WHITE);
    }
    //This part displays the fans in the setup screen with the green fan being the current index fan
    for (int i = 0; i < 10; i++) {
      display.setTextColor(c_WHITE);
      if (i == setupindx % 10) {display.setTextColor(c_GREEN);}
      display.drawString(fans[i + setupintval*(setupindx/10)].index, 0, i*h + 35); display.drawString(":",12,i*h + 35);
      display.drawString(fans[i + setupintval*(setupindx/10)].fannum, 20, i*h + 35); display.drawString(fans[i + setupintval*(setupindx/10)].size, 140, i*h + 35);
    }
    //Display the FAV text on the first five values
    if (setupindx < 10) {
      display.setTextColor(c_CYAN);
      for (int i = 0; i < 5; i++) {
        display.drawString("FAV",250,i*h+35);
      }
    }
    display.setTextColor(c_WHITE);
    //Little legend for users
    display.setTextColor(c_YELLOW);
    display.drawString("7 to go up", 0, 225);
    display.drawString("8 to go down", 60, 225);
    display.drawString("0 to select", 135, 225);
    setupchange = false;
  }
}

//=========== Safety Measures =============
// Checks if the latch is on or not.
void SafetyCheck() {
  if (Safetyclk <= 0) {
    //Depending on latch pin set a boolean value
    if (digitalRead(latchPin) == 0) 
      {if (!setupval) {
        //Reset screen and display "Ready"
        if(!latched) {Reset(); rconnchange = true;}
        display.setCursor(170, 225);
        display.setFont(LiberationSansNarrow_12_Bold);
        display.fillRect(170,223,50,18,c_BLACK);
        display.setTextColor(c_GREEN);
        display.print("Ready");
      } 
      latched = true;}
    else
      {if (!setupval) {
        //Display Disconnected
        display.setCursor(170, 225);
        display.setFont(LiberationSansNarrow_12_Bold);
        display.fillRect(170,223,50,18,c_BLACK);
        display.setTextColor(c_RED);
        display.print("Discon");
        VPState = off; statechange = true;
      }
      latched = false;}
    Safetyclk = 200;
  }
  else {Safetyclk--;}
}

//=========== Power Connection =============
// Checks if the power is connected
void PowerCheck() {
  if (PowerClk <= 0) {
    //If rconnchange goes active check and display
    if (rconnchange) {
      if (ridenconn) {
      //Display Ready if Riden is connected
        display.setCursor(270, 225);
        display.setFont(LiberationSansNarrow_12_Bold);
        display.fillRect(270,223,50,18,c_BLACK);
        display.setTextColor(c_GREEN);
        display.print("Ready");
      }
      else if (!ridenconn) {
      //Display Discon if Riden disconnected
        display.setCursor(270, 225);
        display.setFont(LiberationSansNarrow_12_Bold);
        display.fillRect(270,223,50,18,c_BLACK);
        display.setTextColor(c_RED);
        display.print("Discon");
      }
      rconnchange = false;
    }
    PowerClk = 200;
  }
  else {PowerClk--;}
}


//==========Reset display and info=========
void Reset() {
  //Set tensectest to off, and set all these flags to resetted values
  tensecclk = 0; tensectest = false; status = 3; half = false; 
  for (int i = 0; i < 4; i++) {
    fanstates[i] = 0;
    fanbools[i] = false;
    fanstop[i] = false;
    testsetup[i] = "Untested";
    test50[i] = "Untested";
    testfull[i] = "Untested";
    passfail[i] = true;
    if (fandis[i] == true) {
      fandis[i] = false;
    }
    updatedpsfl[i] = false;
  }
  VPState = off;
  DisplayScreen();
}




//===================== Update Internal Table ==========================
//RM 8/2/2023 - Optomize by using split by commas instead of using placeholder words.
void UpdateTable() {
  myFile = SD.open("test.csv", 'r');
  //Get Rid of the titles on the CSV
  String x = myFile.readStringUntil('\n',200);
  //Read in all of the important data from the excel sheet
  for (updateindex = 0; updateindex < sizeof(fans); updateindex++) {
    //Get entire line
    String x = myFile.readStringUntil('\n',200);
    if (x == "") {
      empty = true;
    }
    if (empty) {
      fansize = updateindex;
      updateindex = sizeof(fans);
    } else {
      //Get the partnum
      int partidx = x.indexOf("1535");
      String partnum = Substring(x, partidx, 0);
      //Get the wire amount
      int wireidx = x.indexOf("wire");
      int wires = Substring(x, wireidx, 5).toInt();
      //Get whether it is dual or not
      String duals = Substring(x, wireidx, 7);
      //Get the size
      String size = Substring(x, wireidx, 9);
      //Get current
      int curridx = x.indexOf("Curr");
      float curr = Substring(x, curridx, 5).toFloat();
      //Get the first RPM
      int rpm1idx = x.indexOf("RPM1");
      int rpm1 = Substring(x, rpm1idx, 5).toInt();
      // Get the second RPM
      int rpm2idx = x.indexOf("RPM2");
      int rpm2 = Substring(x, rpm2idx, 5).toInt();
      //Get the index
      int origidx = x.indexOf("12V,");
      int index = Substring(x, origidx, 4).toInt();
      //Create a fan and store it in the array
      Fan fan = {partnum, wires, duals, size, curr, rpm1, rpm2, index};
      fans[updateindex] = fan;
    }
  }
  
  myFile.close();
  //Read in the favorite fans
  myFile = SD.open("favindex.csv", 'r');
  fav1idx = (myFile.readStringUntil('\n', 10)).toInt();
  fav2idx = (myFile.readStringUntil('\n', 10)).toInt();
  fav3idx = (myFile.readStringUntil('\n', 10)).toInt();
  fav4idx = (myFile.readStringUntil('\n', 10)).toInt();
  fav5idx = (myFile.readStringUntil('\n', 10)).toInt();
  myFile.close();

  FanArrange();
}
//Helper method to parse strings
String Substring(String x, int r, int add) {
  String y = x.substring(r+add);
  int c = y.indexOf(",");
  return y.substring(0,c);
}
//=========================Fan Arrange============================
//Helper method to arrange the fans with favorites at the top
void FanArrange() {
  Fan temp[fansize]; int fandex = 0; int i = 0;
  while(fans[i].index != fav1idx) {
    i++;
  }
  temp[0] = fans[i]; i = 0;
  while(fans[i].index != fav2idx) {
    i++;
  }
  temp[1] = fans[i]; i = 0;
  while(fans[i].index != fav3idx) {
    i++;
  }
  temp[2] = fans[i]; i = 0;
  while(fans[i].index != fav4idx) {
    i++;
  }
  temp[3] = fans[i]; i = 0;
  while(fans[i].index != fav5idx) {
    i++;
  }
  temp[4] = fans[i];
  for (int i = 5; i < fansize; i++) {
    while (fans[fandex].index == fav1idx or fans[fandex].index == fav2idx or fans[fandex].index == fav3idx or fans[fandex].index == fav4idx or fans[fandex].index == fav5idx) {
      fandex++;
    }
    temp[i] = fans[fandex];
    fandex++;
  }
  for (int i = 0; i < fansize; i++) {
    fans[i] = temp[i];
  }
}

//====================== UpdateFile() ========================//
//Update the index of the Sd card and the array based on the setupindx (fan) the user chooses
void UpdateFile() {
  //Get the index of the fans currently
  myFile = SD.open("favindex.csv", 'r');
  fav1idx = (myFile.readStringUntil('\n', 10)).toInt(); int temp1 = fav1idx;
  fav2idx = (myFile.readStringUntil('\n', 10)).toInt(); int temp2 = fav2idx;
  fav3idx = (myFile.readStringUntil('\n', 10)).toInt(); int temp3 = fav3idx;
  fav4idx = (myFile.readStringUntil('\n', 10)).toInt(); int temp4 = fav4idx;
  fav5idx = (myFile.readStringUntil('\n', 10)).toInt(); int temp5 = fav5idx;
  myFile.close();
  //Based on which fans need to be switched update the index values
  if (!(fav1idx == fans[setupindx].index)) {
    if (!(fav2idx == fans[setupindx].index)) {
      if (!(fav3idx == fans[setupindx].index)) {
        if (!(fav4idx == fans[setupindx].index)) {
          temp5 = fav4idx;
        }
         temp4 = fav3idx;
      }
      temp3 = fav2idx;
    }
    temp2 = fav1idx;
    temp1 = fans[setupindx].index;
  }
  //send these new index values to helper function
  EditFile(temp1, temp2, temp3, temp4, temp5);
  //Arrange the fans in the array
  FanArrange();
}

//==================== Edit File ====================
//Helper function for UpdateFile which writes over the current indexs in the sd card fav file
void EditFile(int idx1, int idx2, int idx3, int idx4, int idx5) {
  myFile = SD.open("favindex.csv", O_RDWR);
  myFile.println(idx1); 
  myFile.println(idx2); 
  myFile.println(idx3); 
  myFile.println(idx4); 
  myFile.println(idx5);
  myFile.close();
}

// Meant to edit the favorite value in the SD card NOT USED
void EditFile2(int idx, int val) {
  myFile = SD.open("test.csv", O_RDWR);
  String x = myFile.readStringUntil('\n',200);
  
  for (int i = 0; i < idx + 1; i++) {
    x = myFile.readStringUntil('\n',200);
  }
  int favidx = x.indexOf("12V,");
  int position = myFile.position() + favidx + 3 - x.length();
  myFile.close();

  myFile = SD.open("test.csv", O_RDWR);
  myFile.seek(position);
  myFile.print(val);
  myFile.close();
}


// ===================  Display Screen ==========================
// Displays the general background and info that doesnt need to be refreshed
void DisplayScreen() {
  //screen
  display.setFontAdafruit();
  display.fillScreen(c_BLACK);

  

  //Text for Header
  int x,y;
  x=2; y=188;
  display.setFont(LiberationSansNarrow_14_Bold); display.setTextColor(c_ORANGE); display.setCursor(160,0); display.print("PN-"); display.print(currfan.fannum);
  display.setFont(LiberationSansNarrow_16_Bold); display.setCursor(0,0); display.setTextColor(c_ORANGE); display.print("FanTester"); display.print(Ver);
  display.setFont(LiberationSansNarrow_12_Bold); display.setTextColor(c_WHITE); display.setCursor(15,18); display.print("Mode : "); display.drawString("Not Testing", 65,18);
  display.setFont(LiberationSansNarrow_11_Bold); display.setTextColor(c_YELLOW); display.drawString("4 for manual, 5 for auto",180, 18);
  display.setFont(LiberationSansNarrow_11_Bold);

  //Draw initial information and fan icon spaces
  poweredoff = true; fanpower = false;
  for(int i=0;i<4;i++) {
    display.drawBitmap(30+i*w, 40, fan1ico, 50, 50,c_GREY);
    display.drawRoundRect(27+i*w,37,56,56,8,c_WHITE);
    display.drawRoundRect(25+i*w,103,60,110,8, c_WHITE);
    display.setTextColor(c_MAGENTA);
    display.setTextColor(c_CYAN);
    display.drawString("TACH", 37 + w*i,135);
  }
  //Text for Footer
  display.setFont(LiberationSansNarrow_14_Bold); display.setCursor(0,225); display.setTextColor(c_CYAN); display.print("V : ");
  display.setFont(LiberationSansNarrow_14_Bold); display.setCursor(70,225); display.setTextColor(c_CYAN); display.print("I : ");
  display.setFont(LiberationSansNarrow_12_Bold); display.setCursor(120,225); display.print("Latch : ");
  display.setCursor(225,225); display.print("PWR : ");
  int tempcurr = currfan.current*100*4;
  if (currfan.dual == 'Y') {tempcurr = currfan.current*100*2;}
  ChangeCurrent(tempcurr); statechange = true;
}


// ===================  Display Data ==========================
// Takes in current fan data and displays data to the lcd
void DisplayData() {
  if (!tensectest) {
    GenModeText();
  }
  FanModeText();
  TachText();
  VoltText();
  if (tensectest) {
    TensectestDisplay();
  }
}

// Helper Function to Display the Mode of the Fan
void GenModeText() {
  if (statechange) {
      display.setFont(LiberationSansNarrow_12_Bold);
      if (!fanpower) {
        display.fillRect(65,17,110,17,c_BLACK);
        display.setTextColor(c_WHITE);
        display.drawString("Not Testing", 65, 18);
      }
      else {
        display.fillRect(65,17,110,17,c_BLACK);
        display.setTextColor(c_GREEN);
        display.drawString("Manual Testing", 65, 18);
      }
  }
}

// Helper Function to Display the Mode of the Fan
void FanModeText() {
  if (statechange) {
    display.setFont(LiberationSansNarrow_11_Bold);
    for (int i = 0; i < 4; i++) {
      if (!fanconnected[i]) {
        display.fillRect(27+i*w, 105, 56, 25, c_BLACK);
        display.setTextColor(c_CYAN);
        display.drawString("Disabled", 29 + w*i, 115);
      }
      else if (fanspassfail and passfail[i]) {
        display.fillRect(27+i*w, 105, 56, 25, c_BLACK);
        display.setTextColor(c_GREEN);
        display.drawString("Passed", 33 + w*i, 115);
      }
      else if (fanspassfail and !passfail[i]) {
        display.fillRect(27+i*w, 105, 56, 25, c_BLACK);
        display.setTextColor(c_RED);
        display.drawString("Failed", 37 + w*i, 115);
      }
      else if (!fanpower) {
        display.fillRect(27+i*w, 105, 56, 25, c_BLACK);
        display.setTextColor(c_MAGENTA);
        display.drawString("Off", 45 + w*i, 115);
      }
      else if (!fanbools[i]) {
        display.fillRect(27+i*w, 105, 56, 25, c_BLACK);
        display.setTextColor(c_YELLOW);
        display.drawString("Idle", 43 + w*i, 115);
      }
      else {
        display.fillRect(27+i*w, 105, 56, 25, c_BLACK);
        display.setTextColor(c_GREEN);
        display.drawString("Running", 29 + w*i, 115);
      }
    }
  }
}

//Helper function to display the TACHS of the fans
void TachText() {
  if (tacclk <= 0) {
    display.setTextColor(c_CYAN);
    display.setFont(LiberationSansNarrow_12_Bold);
    for (int i = 0; i < 4; i++) {
      display.fillRect(27+i*w, 150, 56, 15, c_BLACK);
      display.drawString(FAN_TAC_RPM[i], 37 + w*i,150);
    }
    tacclk = 200;
  }
  else {
    tacclk--;
  }
  
}

//Helper function to display the Voltage input
void VoltText() {
  if (VoltTimer <= 0) {
    display.setFont(LiberationSansNarrow_12_Bold); display.setCursor(25, 225); display.setTextColor(c_CYAN);
    display.fillRect(25,225,40,12,c_BLACK);
    display.print(PS_12V_v);
    display.setCursor(95, 225);
    display.fillRect(95,225,20,12,c_BLACK);
    display.print(ridencurr);
    VoltTimer = 100;
  }
  else {
    VoltTimer--;
  }
}

// ==========================  Process Fans ==========================
// Takes in current fan data and displays the visual states to the lcd 

void Process_Fans() {
  if (FanClk <= 0 || fanoffstate) {
        for (int i = 0; i < 4; i++) {
        if (fanbools[i] == true and fanstates[i] == 0 and fanconnected[i] and fanpower and !fanspassfail) {
          display.fillRect(30+w*i ,40,50,50,c_BLACK);
          display.drawBitmap(30+w*i, 40, fan1ico, 50, 50,c_GREEN);
          fanstates[i] = 1;
          fanstop[i] = false;
        }
        else if (fanbools[i] == true and fanstates[i] == 1 and fanconnected[i] and fanpower and !fanspassfail) {
          display.fillRect(30+w*i ,40,50,50,c_BLACK);
          display.drawBitmap(30+w*i , 40, fan2ico, 50, 50,c_GREEN);
          fanstates[i] = 0;
        }
        else if (!fanstop[i] and fanconnected[i] and fanpower and !fanspassfail) {
            display.fillRect(30+w*i ,40,50,50,c_BLACK);
            display.drawBitmap(30+w*i, 40, fan1ico, 50, 50,c_YELLOW);
            fanstop[i] = true; fandis[i] = false;
        }
        else if (!fandis[i] and !fanconnected[i]) {
            display.fillRect(30+w*i ,40,50,50,c_BLACK);
            display.drawBitmap(30+w*i, 40, fan1ico, 50, 50,c_GREY);
            display.drawLine(30+w*i+1,41,30+w*i+49,89, c_RED);
            display.drawLine(30+w*i+49,41,30+w*i+1,89,c_RED);
            fandis[i] = true; fanstop[i] = false;
        }
        else if (!poweredoff and !fanpower and fanconnected[i] and !fanspassfail) {
          display.fillRect(30+w*i,40,50,50,c_BLACK);
          display.drawBitmap(30+w*i, 40, fan1ico, 50, 50,c_GREY);
          fanbools[i] = false;
          fanstop[i] = false;
          fandis[i] = false;
          if (i == 3) {
            poweredoff = true;
          }
        }
        else if (fanspassfail and !passfail[i] and !updatedpsfl[i] and fanconnected[i]) {
          display.fillRect(30+w*i,40,50,50,c_BLACK);
          display.drawBitmap(30+w*i, 40, fan1ico, 50, 50,c_RED);
          updatedpsfl[i] = true;
        }
        else if (fanspassfail and passfail[i] and updatedpsfl[i] and fanconnected[i]) {
          display.fillRect(30+w*i,40,50,50,c_BLACK);
          display.drawBitmap(30+w*i, 40, fan1ico, 50, 50,c_GREEN);
          updatedpsfl[i] = true;
        }
      }
    FanClk = 100;
  }
  else {FanClk--;}
}



 // ===================  Blink Routine - NOT USED ==========================
 // heart beat - Blinks a box or character in top right of display - NOT USED
 // lets you know is alive 
bool Process_Blink(){
  if (blnkcntr <= 0) {
      if (!blnkstate) {
        blnkcntr=blnkon;
        //display.fillRect(304,0,15,15,c_BLACK); // enable for blinking box
        display.setTextColor(c_GREEN,c_BLACK);
        b_color=c_GREY;b_color=RGB(15,31,15);
   }else{
      blnkcntr=blnkoff;
      //  display.fillRect(10,10,64,72,c_GREEN); // enable for blinking box
      display.setTextColor(c_BLACK,c_GREEN);
      b_color=c_GREEN;f_color=c_BLACK;
        }
     digitalWrite(blnkLED,blnkstate);// enable for blinking LED on Teensy
     blnkstate = !blnkstate;
      display.setFontAdafruit();display.setTextSize(0);
     display.setCursor(315,0 );display.print(" ");
   
  return true;
  }
   blnkcntr -=1;
   return false;  
}

//Turns on Fans when running state
void FanSet() {
  if (statechange) {
    for (int i = 0; i < 4; i++) {
      if (!fanbools[i] or !fanconnected[i]) {
        digitalWrite(Fan_Array[i],0);
      }
      else {
        digitalWrite(Fan_Array[i], 1);
      }
    }
  }
}

//Turns on Fans when running state - Needs Verification
void PWMSet() {
  if (statechange) {
      if (fanpower and !rpmhalf) {
        analogWrite(PWM_Array[0],256);
      }
      else if (fanpower and rpmhalf) {
        analogWrite(PWM_Array[0], 127);
      }
      else {
        analogWrite(PWM_Array[0], 0);
      }
    }
    statechange = false;
}


// ==========  Process KeyPad ===================
//  Calls Keypad routine and takes action on keys pressed
void Main_Buttons(){
   keycntr -=1;kpntr=0;
   if (keycntr > 0) return;
   keycntr = keywait;
   
   lcd_key = keypad.getKey(); // read the buttons
   if (lcd_key == 0) lcd_key =' ';

   pKey =0;btnMsg[0]=0;  //  like btnMsg='';
   btnDisplay=true;//
      // depending on which button was pushed, we perform an action     
   switch (lcd_key) { 
       case 0:   break;  
       case '1': if(!poweredoff and !splashed and fanconnected[0]) {fanbools[0] = !fanbools[0]; if(currfan.dual == 'Y') {fanbools[1] = !fanbools[1];} statechange = true;} if (fanspassfail) {clearclk = 0;} break;
       case '2': if(!poweredoff and !splashed and fanconnected[1]) {fanbools[1] = !fanbools[1]; if(currfan.dual == 'Y') {fanbools[0] = !fanbools[0];} statechange = true;} if (fanspassfail) {clearclk = 0;}  break;
       case '3': if(!poweredoff and !splashed and fanconnected[2]) {fanbools[2] = !fanbools[2]; if(currfan.dual == 'Y') {fanbools[3] = !fanbools[3];} statechange = true; checkread = true;} if (fanspassfail) {clearclk = 0;} break;
       case 'A': if(!poweredoff and !splashed and fanconnected[3]) {fanbools[3] = !fanbools[3]; if(currfan.dual == 'Y') {fanbools[2] = !fanbools[2];} statechange = true;} if (fanspassfail) {clearclk = 0;} break;
       case '4': fanconnected[0] = !fanconnected[0]; if(currfan.dual == 'Y') {fanconnected[1] = !fanconnected[1];} poweredoff = false; statechange = true; if (fanspassfail) {clearclk = 0;} break;
       case '5': fanconnected[1] = !fanconnected[1]; if(currfan.dual == 'Y') {fanconnected[0] = !fanconnected[0];} poweredoff = false; statechange = true; if (fanspassfail) {clearclk = 0;} break;
       case '6': fanconnected[2] = !fanconnected[2]; if(currfan.dual == 'Y') {fanconnected[3] = !fanconnected[3];} poweredoff = false; statechange = true; if (fanspassfail) {clearclk = 0;} break;
       case 'B': fanconnected[3] = !fanconnected[3]; if(currfan.dual == 'Y') {fanconnected[2] = !fanconnected[2];} poweredoff = false; statechange = true; if (fanspassfail) {clearclk = 0;} break;
       case '*': if (!splashed) {fanpower = !fanpower; poweredoff = false; statechange = true; if (fanpower) {VPState = on;} else {VPState = off;}} if (fanspassfail) {clearclk = 0;} break;
       case 'D': if (!splashed) {status = 0; tensectest = true;} if (fanspassfail) {clearclk = 0;} break;
       case '0': setupchange = true; setupsafety = false; setupval = !setupval; UpdateFile(); setupindx = 0; if (fanspassfail) {clearclk = 0;} break;
       case 'C': ShowSplash(); splashed = true; if (fanspassfail) {clearclk = 0;} break;
       case '9': checkread = true; if (fanspassfail) {clearclk = 0;} break;
       case '#': Reset(); splashed = false;if (fanspassfail) {clearclk = 0;}  break;
       case '8': if (currfan.wires == 4) {rpmhalf = !rpmhalf;} statechange = true; break;
     return ;
   }
}


// ==========  Process KeyPad2 ===================
//  Calls Keypad routine and takes action on keys pressed
// For the setup routine only
void Setup_Buttons(){
   keycntr -=1;kpntr=0;
   if (keycntr > 0) return;
   keycntr = keywait;
   
   lcd_key = keypad.getKey(); // read the buttons
   if (lcd_key == 0) lcd_key =' ';

   pKey =0;btnMsg[0]=0;  //  like btnMsg='';
   btnDisplay=true;//
      // depending on which button was pushed, we perform an action     
   switch (lcd_key) { 
       case '0':  VPState = off; setupchange = true; setupval = !setupval; if (!setupval) {currfan = fans[setupindx]; DisplayData(); UpdateFile(); Reset(); rconnchange = true;} break;
       case '7': setupchange = true; if (setupindx > 0) {setupindx--;} break;
       case '8': setupchange = true; if (setupindx < (fansize-1)) {setupindx++;}break;
     return ;
   }
}

// ==========  Process KeyPad3 ===================
//  Calls Keypad routine and takes action on keys pressed
// For the setup routine only
void TenSec_Buttons(){
   keycntr -=1;kpntr=0;
   if (keycntr > 0) return;
   keycntr = keywait;
   
   lcd_key = keypad.getKey(); // read the buttons
   if (lcd_key == 0) lcd_key =' ';

   pKey =0;btnMsg[0]=0;  //  like btnMsg='';
   btnDisplay=true;//
      // depending on which button was pushed, we perform an action     
   switch (lcd_key) { 
       case 'D': tensectest = false; status = 2; Reset(); break; 
     return;
   }
}


// ================================================================
//                 Read FAN Tachs
//
// Method: FAN_TAC[x].begin(FAN_TAC[x]);  Start Frequency measurement
//         Read # of measurements taken (up to 24)
//         If measurements > 15 then get all stored readings to array
//         Process array,  discard first and last few, or any too high or <1
//         average measurements and scale to RPM
//
//  Notes: freqmeasuremulti.()lib keeps getting reading while stored data is read
//     Workaround is to discard first 3 and last 3 readings
//  There is also a FreqCounter library but only one channel and better suited for high freq (>10kHz)
// =================================================================


void Get_Fan_Tac(){
    int fcnt,favailable;  float FreqRead; 
    FAN_TAC_RPMspec = currfan.rpm1;
    FAN_TAC_RPMspec2 = currfan.rpm2;
    if (rpmhalf) {FAN_TAC_RPMspec = currfan.rpm1/1.3; FAN_TAC_RPMspec2 = currfan.rpm2;}
    FAN_TAC_Freqspec=FAN_TAC_RPMspec/FAN_TAC_FreqToRPM;
    FAN_TAC_Freqspec2=FAN_TAC_RPMspec2/FAN_TAC_FreqToRPM;
    if ( !FAN_TAC_enable || (FAN_TAC_Timems >0 && millis() < FAN_TAC_Timems )) return;
    FAN_TAC_Timems=millis()+FAN_TAC_Intervalms;
  //   ft[0]=ft[1]; ft[1] = millis(); // enable this line for development - to check cycle times
    
    //===== Read FAN Tach 0,1,2,3 ============
    if (currfan.dual == 'N') {
     for (int i = 0; i < FAN_TAC_fancnt; i++) {
       FAN_TAC_FREQ[i] = 0.0;  FAN_TAC_RPM[i] = 0; //clear Fan Tach results
       fcnt = 0;
       favailable = FAN_TAC[i].available(); //will save up to 23 readings, need at least 10 for good average
       
       if (favailable > 15)  {  
               // Get all the stored data and put in array, get as fast as possible or subtle errors occur
         for (int r = 0; r < favailable; r++)  FAN_TAC_f[r]=FAN_TAC[i].read();
              // Process stored data from array, throwing out first and last few readiings
              // and any invalid data (too big or small)
         for (int r = 3; r < favailable-3; r++)   {
            FreqRead = FAN_TAC[i].countToFrequency(FAN_TAC_f[r]);
              if ((FreqRead > 0) && (FreqRead < (FAN_TAC_Freqspec * 1.5)))  {
              //if (i == 0 && FAN_TAC_Debg2) a[fcnt] = (int)FreqRead;  // enable this line for debg2 - save raw data for debug
              //  add up valid readings
             FAN_TAC_FREQ[i] = FAN_TAC_FREQ[i] + FreqRead; //  (FreqRead );
             fcnt += 1;
           } }
         FAN_TAC_FREQ[i] = FAN_TAC_FREQ[i] / fcnt; // calculate average rpm based on # of valid readings
         FAN_TAC_RPM[i] = (int)(FAN_TAC_FREQ[i] * 30);
       // if (i == 0 )  fcnt_a0=fcnt; // enable this line for debg2
        }
      }
    }
    else {
     for (int i = 0; i < FAN_TAC_fancnt; i++) {
        if (i % 2 == 0) {
          FAN_TAC_FREQ[i] = 0.0;  FAN_TAC_RPM[i] = 0; //clear Fan Tach results
          fcnt = 0;
          favailable = FAN_TAC[i].available(); //will save up to 23 readings, need at least 10 for good average
          
          if (favailable > 15)  {  
                  // Get all the stored data and put in array, get as fast as possible or subtle errors occur
            for (int r = 0; r < favailable; r++)  FAN_TAC_f[r]=FAN_TAC[i].read();
                  // Process stored data from array, throwing out first and last few readiings
                  // and any invalid data (too big or small)
            for (int r = 3; r < favailable-3; r++)   {
                FreqRead = FAN_TAC[i].countToFrequency(FAN_TAC_f[r]);
                  if ((FreqRead > 0) && (FreqRead < (FAN_TAC_Freqspec * 1.5)))  {
                  //if (i == 0 && FAN_TAC_Debg2) a[fcnt] = (int)FreqRead;  // enable this line for debg2 - save raw data for debug
                  //  add up valid readings
                FAN_TAC_FREQ[i] = FAN_TAC_FREQ[i] + FreqRead; //  (FreqRead );
                fcnt += 1;
              } }
            FAN_TAC_FREQ[i] = FAN_TAC_FREQ[i] / fcnt; // calculate average rpm based on # of valid readings
            FAN_TAC_RPM[i] = (int)(FAN_TAC_FREQ[i] * 30);
          // if (i == 0 )  fcnt_a0=fcnt; // enable this line for debg2
          }
        }
        else {
          FAN_TAC_FREQ[i] = 0.0;  FAN_TAC_RPM[i] = 0; //clear Fan Tach results
          fcnt = 0;
          favailable = FAN_TAC[i].available(); //will save up to 23 readings, need at least 10 for good average
          
          if (favailable > 15)  {  
                  // Get all the stored data and put in array, get as fast as possible or subtle errors occur
            for (int r = 0; r < favailable; r++)  FAN_TAC_f[r]=FAN_TAC[i].read();
                  // Process stored data from array, throwing out first and last few readiings
                  // and any invalid data (too big or small)
            for (int r = 3; r < favailable-3; r++)   {
                FreqRead = FAN_TAC[i].countToFrequency(FAN_TAC_f[r]);
                  if ((FreqRead > 0) && (FreqRead < (FAN_TAC_Freqspec2 * 1.5)))  {
                  //if (i == 0 && FAN_TAC_Debg2) a[fcnt] = (int)FreqRead;  // enable this line for debg2 - save raw data for debug
                  //  add up valid readings
                FAN_TAC_FREQ[i] = FAN_TAC_FREQ[i] + FreqRead; //  (FreqRead );
                fcnt += 1;
              } }
            FAN_TAC_FREQ[i] = FAN_TAC_FREQ[i] / fcnt; // calculate average rpm based on # of valid readings
            FAN_TAC_RPM[i] = (int)(FAN_TAC_FREQ[i] * 30);
          // if (i == 0 )  fcnt_a0=fcnt; // enable this line for debg2
          }
        }
      }
    }
}

//=========== Get_PS_Voltages =====================================
void Get_PS_Voltages(){
  if (VoltTimer <= 0) {
    int vread = adc->adc0->analogRead(PS_12V);
    if (PS_indx > 20) {
      PS_12V_v = PS_avgtotal/PS_indx;
      PS_indx = 0;
      PS_avgtotal = 0;
    }
    else {
      PS_avgtotal = PS_avgtotal + (vread  * 3.3 * PS_12Vscale)  / PS_adcScale;
      PS_indx++;
    }
    VoltTimer = 20;
  }
  else {
    VoltTimer--;
  }
  }

unsigned int CRC16_2(unsigned char *buf, int len)
{  
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
  crc ^= (unsigned int)buf[pos];    // XOR byte into least sig. byte of crc

  for (int i = 8; i != 0; i--) {    // Loop over each bit
    if ((crc & 0x0001) != 0) {      // If the LSB is set
      crc >>= 1;                    // Shift right and XOR 0xA001
      crc ^= 0xA001;
    }
    else                            // Else LSB is not set
      crc >>= 1;                    // Just shift right
    }
  }

  return crc;
}


void ChangeVoltage(int vol) {
  int y = vol & 0xFF00;
  int xy = y >> 8;
  int xz = vol & 0x00FF;
  byte num[] = {0x01,0x06,0x00,0x08,xy,xz};
  int x = CRC16_2(num,sizeof(num));
  int sy = (x & 0x00FF);
  int sz = (x & 0xFF00) >> 8;
  byte num1[] = {0x01,0x06,0x00,0x08,xy,xz, sy, sz};
  Serial1.write(num1,sizeof(num1));
}


void ChangeCurrent(int vol) {
  vol = vol * currmult;
  int y = vol & 0xFF00;
  int xy = y >> 8;
  int xz = vol & 0x00FF;
  byte num[] = {0x01,0x06,0x00,0x09,xy,xz};
  int x = CRC16_2(num,sizeof(num));
  int sy = (x & 0x00FF);
  int sz = (x & 0xFF00) >> 8;
  byte num1[] = {0x01,0x06,0x00,0x09,xy,xz, sy, sz};
  Serial1.write(num1,sizeof(num1));
}

void TurnOff() {
  byte num1[] = {0x01,0x06,0x00,0x12,0x00,0x00,0x29,0xCF};
  Serial1.write(num1,sizeof(num1));
}

void TurnOn() {
  byte num1[] = {0x01,0x06,0x00,0x12,0x00,0x01,0xE8,0x0F};
  Serial1.write(num1,sizeof(num1));
}
// Register 8-12
void CheckValues() {
  Serial1.clear();
  byte num1[] = {0x01,0x03,0x00,0x08,0x00,0x04,0xC5,0xCB};
  Serial1.write(num1,sizeof(num1));
}

void ReadValues() {
  byte buff[80];
  Serial1.readBytes(buff, 13);
  int rv1 = (buff[3] << 8) | buff[4];
  setvolt = rv1; 
  int cr1 = (buff[5] << 8) | buff[6];
  setcurr = cr1; 
  rv1 = (buff[7] << 8) | buff[8]; 
  ridenvolt = rv1/100;
  cr1 = (buff[9] << 8) | buff[10];
  if (ridenvolt*.97 <= float(PS_12V_v) and float(PS_12V_v) <= ridenvolt *1.03) {
    ridencurr = cr1;
    if (!ridenconn) {
      rconnchange = true;
      //CheckRiden();
    }
    ridenconn = true;
  }
  else {
    ridencurr = 0; if (ridenconn == true) {rconnchange = true;}
    ridenconn = false;
  }
}
// NOT USED - Riden autolocks when sending serial data
void LockValues() {
  byte num1[] = {0x01,0x06,0x00,0x15,0x00,0x01,0x59,0xCE};
  Serial1.write(num1,sizeof(num1));
}

void CheckRiden() {
  Serial1.clear();
  byte num1[] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};
  Serial1.write(num1,sizeof(num1));
  byte buff[80];
  Serial1.readBytes(buff, 13);
  ridenvs = (buff[3] << 8) | buff[4];
  ridenvs = ridenvs/10;
  if (ridenvs == 6006) {
    currmult = 10;
  } else {
    currmult = 1;
  }
}

//lock front panel get model and part num
void StateControl() {
  Serial1.flush();
  if (statetimer <= 0) {
    switch (VPState) {
      case off: TurnOff(); VPState = volt0; break;
      case volt0: ChangeVoltage(0); VPState = checkandread; break;
      case on: TurnOn(); if (half) {VPState = halfpow;} else {VPState = volt1200;} break;
      case halfpow: ChangeVoltage(600); VPState = checkandread; break;
      case volt1200: ChangeVoltage(1200); VPState = checkandread; break;
      case checkandread: VPState = idle; checkread = true; break;
      case idle: VPState = checkidle; break;
      case checkidle: checkread = true; VPState = idle; break;
      return;
    }
    statetimer = 200;
  }
  else {
    statetimer--;
  }
}

//lock front panel get model and part num
void SafetyControl() {
  Serial1.flush();
  if (statetimer <= 0) {
    switch (VPState) {
      case off: TurnOff(); VPState = volt0; break;
      case volt0: ChangeVoltage(0); VPState = checkandread; break;
    }
    statetimer = 200;
  }
  else {
    statetimer--;
  }
}


//lock front panel get model and part num - NOT NEEDED
void DisconControl() {
  if (discontimer == 0) {
    checkread = true;
    discontimer = 400;
  } else {
    discontimer--;
  }
}

void CheckandRead() {
  if (checkread) {
    checkreadclk = 350; 
    checkread = false;
  }
  if (checkreadclk == 0) {
    CheckValues();
    if (fakeout > 0) {fakeout--;}
    else {ReadValues();}
    checkreadclk = -1;
  } else if (checkreadclk > 0) {
    checkreadclk--;
  }
}

// not used
void CheckandRead2() {
    CheckValues();
    if (fakeout > 0) {fakeout--;}
    else {ReadValues();}
}



// ================================================================
//                 TEN SECOND TEST
// Method meant to run fans for 10 seconds and then turn
// back to manual testing mode. Will not activate if latch
// is not closed, and also will reset when latch opened during test.
// Resets all other states of manual testing when run.
// Within ten second test, have a safety startup for each fan at 6 volts, check current and tach, then go full and check tach , then go 50 and check tach.
// =================================================================
void TenSecTest() {
  if (status == 0) {
    fanpower = false; poweredoff = false; statechange = true; rpmhalf = false;
    tensecclk = 20000; tenstatechange = true;
    Process_Fans();
    DisplayData();
    display.fillRect(65,17,110,17,c_BLACK);
    display.setFont(LiberationSansNarrow_12_Bold); display.setTextColor(c_GREEN); display.drawString("20-sec Test", 65, 18);
    fanpower = true; statechange = true; half = true; VPState = on;
    Process_Fans();
    DisplayData();
    for (int i = 0; i < 4; i++) {
      testsetup[i] = "Untested"; testfull[i] = "Untested"; test50[i] = "Untested";
    }
    status = 1;
  }
  else if (status == 1) {
    if (tensecclk <= 0) {
      status = 2;
    }
    if (currfan.dual == 'N') {
      if (tensecclk == voltstate) { 
      VPState = on;
      checkread = true; 
      for (int i = 0; i < 4; i++) {testsetup[i] = "Testing";}
      tenstatechange = true;
      }
      if (tensecclk == voltcheck) {
        if (setvolt*.90 <= ridenvolt  and ridenvolt <= setvolt*1.10) {
        }
        else {
          for (int i = 0; i < 4; i++) {testsetup[i] = "Failed"; passfail[i] = true;}
          tenstatechange = true;
        }
      }
      
      if (tensecclk == fan1run) {
        fanbools[0] = true; statechange = true; 
        checkread = true; 
      }
      if (tensecclk ==  fan1check) {
        if (setcurr > ridencurr and ridencurr != 0 and FAN_TAC_RPM[0] > 0) {
          testsetup[0] = "Passed";
          tenstatechange = true;
          fanbools[0] = false; statechange = true;
        }
        else {
          testsetup[0] = "Failed";
          tenstatechange = true;
          fanbools[0] = false; statechange = true; passfail[0] = false;
        }
      }
      
      if (tensecclk == fan2run) {
        fanbools[1] = true; statechange = true;
        checkread = true; 
      }
      if (tensecclk ==  fan2check) {
        if (setcurr > ridencurr and ridencurr != 0 and FAN_TAC_RPM[1] > 0) {
          testsetup[1] = "Passed";
          tenstatechange = true;
          fanbools[1] = false; statechange = true;
        }
        else {
          testsetup[1] = "Failed";
          tenstatechange = true;
          fanbools[1] = false; statechange = true; passfail[1] = false;
        }
      }

      if (tensecclk == fan3run) {
        fanbools[2] = true; statechange = true;
        checkread = true; 
      }
      if (tensecclk ==  fan3check) {
        
        if (setcurr > ridencurr and ridencurr != 0 and FAN_TAC_RPM[2] > 0) {
          testsetup[2] = "Passed";
          tenstatechange = true;
          fanbools[2] = false; statechange = true;
        }
        else {
          testsetup[2] = "Failed";
          tenstatechange = true;
          fanbools[2] = false; statechange = true; passfail[2] = false;
        }
      }

      if (tensecclk ==  fan4run) {
        fanbools[3] = true; statechange = true;
        checkread = true; 
      }
      if (tensecclk ==  fan4check) {
        if (setcurr > ridencurr and ridencurr != 0 and FAN_TAC_RPM[3] > 0) {
          testsetup[3] = "Passed";
          tenstatechange = true;
          fanbools[3] = false; statechange = true;
        }
        else {
          testsetup[3] = "Failed";
          tenstatechange = true;
          fanbools[3] = false; statechange = true; passfail[3] = false;
        }
      }

      if (tensecclk == fullvolt) {
        half = false; VPState = on;
        for (int i = 0; i < 4; i++) {
          if (passfail[i]) {
            fanbools[i] = true;
          }
          testfull[i] = "Testing";
        }
        statechange = true;
        tenstatechange = true;
      }

      if (tensecclk == fullcheck) {
        for (int i = 0; i < 4; i++) {
          if (currfan.rpm1*.90 <= FAN_TAC_RPM[i] and FAN_TAC_RPM[i] <= currfan.rpm1*1.10) {
            testfull[i] = "Passed";
            tenstatechange = true;
          }
          else {
            testfull[i] = "Failed";  passfail[i] = false;
            tenstatechange = true;
          }
        }
      }
      if (currfan.wires == 4) {
        if (tensecclk == halfvolt) {
          rpmhalf = true; statechange = true;
          for (int i = 0; i < 4; i++) {
            test50[i] = "Testing";
            tenstatechange = true;
          }
        }

        if (tensecclk == halfcheck) {
          for (int i = 0; i < 4; i++) {
            if ((currfan.rpm1/2)*.50 <= FAN_TAC_RPM[i] and FAN_TAC_RPM[i] <= (currfan.rpm1/2)*1.50) {
              test50[i] = "Passed";
              tenstatechange = true;
            }
            else {
              test50[i] = "Failed";  passfail[i] = false;
              tenstatechange = true;
            }
          }
        }
      }
      else if (tensecclk == halfvolt){
        tensecclk = 1;
      }
    }
    else  {
      if (tensecclk == voltstate) { 
        VPState = on;
        checkread = true; 
        for (int i = 0; i < 4; i++) {testsetup[i] = "Testing";}
        tenstatechange = true;
      }
      if (tensecclk == voltcheck) {
        if (setvolt*.90 <= ridenvolt  and ridenvolt <= setvolt*1.10) {
        }
        else {
          for (int i = 0; i < 4; i++) {testsetup[i] = "Failed"; passfail[i] = true;}
          tenstatechange = true;
        }
      }
      if (tensecclk == fan1run) {
        fanbools[0] = true; fanbools[1] = true; statechange = true; 
        checkread = true; 
      }
      if (tensecclk ==  fan1check) {
        if (setcurr > ridencurr and ridencurr != 0 and FAN_TAC_RPM[0] > 0 and FAN_TAC_RPM[1] > 0) {
          testsetup[0] = "Passed"; testsetup[1] = "Passed";
          tenstatechange = true;
          fanbools[0] = false; fanbools[1] = false; statechange = true;
        }
        else {
          testsetup[0] = "Failed"; testsetup[1] = "Failed";
          tenstatechange = true;
          fanbools[0] = false; fanbools[1] = false; statechange = true; passfail[0] = false; passfail[1] = false;
        }
      }

      if (tensecclk == fan2run) {
        fanbools[2] = true; fanbools[3] = true; statechange = true; 
        checkread = true; 
      }
      if (tensecclk ==  fan2check) {
        if (setcurr > ridencurr and ridencurr != 0 and FAN_TAC_RPM[2] > 0 and FAN_TAC_RPM[3] > 0) {
          testsetup[2] = "Passed"; testsetup[3] = "Passed";
          tenstatechange = true;
          fanbools[2] = false; fanbools[3] = false; statechange = true;
        }
        else {
          testsetup[2] = "Failed"; testsetup[3] = "Failed";
          tenstatechange = true;
          fanbools[2] = false; fanbools[3] = false; statechange = true; passfail[2] = false; passfail[3] = false;
        }
      }
      if (tensecclk == fullvolt) {
        half = false; VPState = on;
        for (int i = 0; i < 4; i++) {
          if (passfail[i]) {
            fanbools[i] = true;
          }
          testfull[i] = "Testing";
        }
        statechange = true;
        tenstatechange = true;
      }

      if (tensecclk == fullcheck) {
        for (int i = 0; i < 4; i++) {
          if (i % 2 == 0) {
            if (currfan.rpm1*.90 <= FAN_TAC_RPM[i] and FAN_TAC_RPM[i] <= currfan.rpm1*1.10) {
              testfull[i] = "Passed";
              tenstatechange = true;
            }
            else {
              testfull[i] = "Failed";  passfail[i] = false;
              tenstatechange = true;
            }
          } 
          else {
              if (currfan.rpm2*.90 <= FAN_TAC_RPM[i] and FAN_TAC_RPM[i] <= currfan.rpm2*1.10) {
                testfull[i] = "Passed";
                tenstatechange = true;
              }
              else {
                testfull[i] = "Failed";  passfail[i] = false;
                tenstatechange = true;
              }
          }
        }
      }

      if (currfan.wires == 4) {
        if (tensecclk == halfvolt) {
          rpmhalf = true; statechange = true;
          for (int i = 0; i < 4; i++) {
            test50[i] = "Testing";
            tenstatechange = true;
          }
        }

        if (tensecclk == halfcheck) {
          for (int i = 0; i < 4; i++) {
            if (i % 2 == 0) {
              if ((currfan.rpm1/2)*.50 <= FAN_TAC_RPM[i] and FAN_TAC_RPM[i] <= (currfan.rpm1/2)*1.50) {
                test50[i] = "Passed";
                tenstatechange = true;
              }
              else {
                test50[i] = "Failed";  passfail[i] = false;
                tenstatechange = true;
              }
            } 
            else {
              if ((currfan.rpm2/2)*.50 <= FAN_TAC_RPM[i] and FAN_TAC_RPM[i] <= (currfan.rpm2/2)*1.50) {
                test50[i] = "Passed";
                tenstatechange = true;
              }
              else {
                test50[i] = "Failed";  passfail[i] = false;
                tenstatechange = true;
              }
          }
          }
        }
      }
      else if (tensecclk == halfvolt){
        tensecclk = 1;
      }
      

    }
    


    tensecclk--;
  }
  else if (status == 2){
    display.fillRect(65,17,110,17,c_BLACK);
    display.setFont(LiberationSansNarrow_12_Bold); display.setTextColor(c_WHITE); display.drawString("Manual Testing", 65,18);
    fanpower = false; poweredoff = false; statechange = true; tensectest = false; fanbools[0] = false; fanbools[1] = false, fanbools[2] = false, fanbools[3] = false;
    status = 3; rpmhalf = false; VPState = off; tensectestflag = true; fanspassfail = true;
  }
}

//handles display for tensectest
void TensectestDisplay() {
  if (tenstatechange) {
    display.setFont(LiberationSansNarrow_10_Bold);
    for (int i = 0; i < 4; i++) { 
      if (testsetup[i] == "Untested") {
        display.setTextColor(c_WHITE);
      }
      else if (testsetup[i] == "Testing") {
        display.setTextColor(c_YELLOW);
      }
      else if (testsetup[i] == "Passed"){
        display.setTextColor(c_GREEN);
      }
      else if (testsetup[i] == "Failed"){
        display.setTextColor(c_RED);
      }
      if (fanconnected[i]) {
        display.drawString("Setup", 37 + w*i,165);
      }

      if (testfull[i] == "Untested") {
        display.setTextColor(c_WHITE);
      }
      else if (testfull[i] == "Testing") {
        display.setTextColor(c_YELLOW);
      }
      else if (testfull[i] == "Passed"){
        display.setTextColor(c_GREEN);
      }
      else if (testfull[i] == "Failed"){
        display.setTextColor(c_RED);
      }
      if (fanconnected[i]) {
        display.drawString("100%", 37 + w*i,180);
      }

    if (test50[i] == "Untested") {
        display.setTextColor(c_WHITE);
      }
      else if (test50[i] == "Testing") {
        display.setTextColor(c_YELLOW);
      }
      else if (test50[i] == "Passed"){
        display.setTextColor(c_GREEN);
      }
      else if (test50[i] == "Failed"){
        display.setTextColor(c_RED);
      }
      if (fanconnected[i] and currfan.wires == 4) {
        display.drawString("50%", 37 + w*i,195);
      }
    }
    tenstatechange = false;
  }
}

void TenSecClear() {
  if (tensectestflag) {
    clearclk = 20000;
    tensectestflag = false;
  }
  else if (clearclk == 0) {
    fanspassfail = false;
    Reset();
    clearclk = -1;
  }
  else {
    clearclk--;
  }
}