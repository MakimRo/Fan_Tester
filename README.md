# Fan-Tester
Git Repository for the Fan Automated Testing Project.

Check Src file for main.ino (main code). Headers written in FAN_Defines.h file.

Revision control check

Install Notes:

Sd Card needed with a file which contains all of the fans, and file which contains five numbers representing the index values of the favorite fans relative to the fan file. Sdcard file with fans must be named test.csv and favorite file must be named favindex.csv.

Use a non adafruit TFT size 3.5, ILI9341.

If exceeds 60 fans, go into Fan_Defines.h and change variable fans[array] to an increment of 10 above fansize.

Code Setup:
1. First download VSCode IDE and install Platformio extension as well as the C/C++ Extension Pack extension.
There are also some helpful extensions like Git History, Git Project Manager, that could help. Link for VSCode = https://code.visualstudio.com/download
2. Next get the Github for Desktop app. Then log in with your github account and you should be able to access whatever repositories you have access to. Link = https://desktop.github.com/
3. If your computer already doesn't have the teensy loader make sure to download that from the pjrc teensy site. Link = https://www.pjrc.com/teensy/loader.html
4. If you wish to use the Serial Port Interface download Bray Terminal Link = https://sites.google.com/site/terminalbpp/
5. Then clone the Github repository into your local by clicking Clone Repository from Internet. From there you can also create a new repository or even follow their tutorial repository.
6. From there you should be able to change branches, add, commit, and push code to the main github repository using git commands.
7. If the computer doesn't have git installed already install git using this link: https://git-scm.com/download/win



Todo now:
Riden connection check
Figure out why riden not accurately changing to 6 volt before tensectest



Todo later: 
Add CRC checker, resistor to input, on serial port
Add a bypass safety switch for engineer debug
Have multiple modes on the tensectest button depending on ready, running, off states for the LED.
Make the fan info csv into a EEPROM