/*
 * arduino_clock.ino
 * 
 * A small clock made from a 2.8" TFT Touch screen and a ENC28J60 Ethernet Module. 
 * 
 * @author thatging3rkid
 * 
 * Sources:
 * http://forum.arduino.cc/index.php?topic=171941.0
 * http://www.wennekes.info/scripts.php?chapter=Arduino&article=NTP%20Client%20-%20ENC28J60
 */

#include <Time.h>           // http://www.arduino.cc/playground/Code/Time
#include <TimeLib.h>        // http://www.arduino.cc/playground/Code/Time
#include <EtherCard.h>      // https://github.com/jcw/ethercard
#include <Timezone.h>       // https://github.com/JChristensen/Timezone

#include <gfxfont.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0x9CD3

// Make a TFT object
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// EtherCard setup
static byte mac[] = { 0x74,0x1F,0x72,0x2D,0xFF,0x02 };
byte Ethernet::buffer[700];

// Timezone setup
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   //UTC - 5 hours
Timezone usEastern(usEDT, usEST);

byte s_minute = 0;
byte r_minute = -1;
bool error = false;
byte error_code = 0b000000;
const unsigned long seventy_years = 2208988800UL;

void setup () {
  Serial.begin(19200);

  // TFT initalization
  tft.reset();
  uint16_t identifier = tft.readID();
  if(identifier==0x0101){
    identifier=0x9341;
  } else if(identifier==0x1111) {     
    identifier=0x9328;
  }
  tft.begin(identifier);

  //reset_eth();
  
  // Ethernet initalization
  if (ether.begin(sizeof Ethernet::buffer, mac) == 0) {
    Serial.println( "Failed to access Ethernet controller");
  }
  if (!ether.dhcpSetup()) {
    Serial.println("DHCP failed");
  }

  // Print out DHCP info
  ether.printIp("Allocated IP: ", ether.myip);
  ether.printIp("Gateway IP  : ", ether.gwip);  
  ether.printIp("DNS IP      : ", ether.dnsip);  

  // Get the time from NTP and set the Time lib Time
  setTime(getNTPTime());
  
  // Set the random seed 
  randomSeed(analogRead(A15)); // set to an unconnected analog pin
    
  // Print the time to the screen
  tft_time();
  
  // Print the time for debugging
  printTime();

  // Print out that initalization is complete
  Serial.println("Initalization is complete!");
}

void tft_time(void) {
  tft.fillScreen(BLACK);
  tft.setRotation(3);
  tft.setTextColor(GREY);
  tft.setTextSize(8);
  
  tft.setCursor(20, 50);

  if (hourFormat12() < 10) {
    tft.print("0");
  }
  tft.print(hourFormat12());
  tft.print(":");
  if (minute() < 10) {
    tft.print("0");
  }
  tft.print(minute());

  if (isPM()) {
    tft.print("p");
  } else if (isAM()) {
    tft.print("a");
  }

  tft.setCursor(20, 115);
  tft.setTextSize(3);
  
  if (weekday() == 1) {
    tft.print("Sunday");
  } else if (weekday() == 2) {
    tft.print("Monday");
  } else if (weekday() == 3) {
    tft.print("Tuesday");
  } else if (weekday() == 4) {
    tft.print("Wednesday");
  } else if (weekday() == 5) {
    tft.print("Thursday");
  } else if (weekday() == 6) {
    tft.print("Friday");
  } else if (weekday() == 7) {
    tft.print("Saturday");
  }

  tft.setCursor(20, 143);

  if (month() == 1) {
    tft.print("January");
  } else if (month() == 2) {
    tft.print("February");
  } else if (month() == 3) {
    tft.print("March");
  } else if (month() == 4) {
    tft.print("April");
  } else if (month() == 5) {
    tft.print("May");
  } else if (month() == 6) {
    tft.print("June");
  } else if (month() == 7) {
    tft.print("July");
  } else if (month() == 8) {
    tft.print("August");
  } else if (month() == 9) {
    tft.print("September");
  } else if (month() == 10) {
    tft.print("October");
  } else if (month() == 11) {
    tft.print("November");
  } else if (month() == 12) {
    tft.print("December");
  } 

  tft.print(" ");
  tft.print(day());

  if (day() == 1) {
    tft.print("st");
  } else if (day() == 2) {
    tft.print("nd");
  } else if (day() == 3) {
    tft.print("rd");
  } else if (day() == 21) {
    tft.print("st");
  } else if (day() == 22) {
    tft.print("nd");
  } else if (day() == 23) {
    tft.print("rd");
  } else if (day() == 31) {
    tft.print("st");
  } else {
    tft.print("th");
  }
  
  tft.setCursor(20, 173);
  tft.print(year());

  if (error) {
    tft.setTextColor(RED);
    tft.setCursor(225, 215);
    tft.print("ERROR");
  }

  s_minute = minute();
}


void loop () {
  ether.packetLoop(ether.packetReceive());

  if ((hour() == 1) && isAM() && (r_minute != -1)) {
    r_minute = -1;
  }
  
  if ((hour() == 12) && isAM()) {
    /*
     * r_minute and random time selection
     * 
     * In order to be a good NTP dev, you should not set a program to get an NTP update at a specified time (like 12:00am).
     * Instead, the program should randomly select a time to update from the NTP servers. 
     * 
     * This program randomly select a time to update between 12:00am and 12:59am
     * 
     * r_minute stores data about the time that has been selected
     * r_minute = -2       -> time update has been completed
     * r_minute = -1       -> a random time must be selected
     * r_minute = 0...59   -> a random time has been selected and is waiting for that minute
     */
    if (r_minute == -1) {
      r_minute = random(60);
    }
    if (minute() == r_minute) {
      r_minute = -2;
      setTime(getNTPTime());
      printTime();
    }
  }

  if (minute() != s_minute) {
    tft_time();
  } else {
    delay(1000);
  }

}

void printTime() {
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year()); 
  Serial.print(" ");
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');   
  Serial.print(digits);
}

uint8_t ntpServer[] = {216, 239, 35, 0}; // time.google.com

unsigned long getNTPTime() {
  unsigned long timeFromNTP;
  int i = 40; // Number of max attempts
  Serial.println("NTP request sent");
  while(i > 0) {
    ether.ntpRequest(ntpServer, 123);
    Serial.print("."); //Each dot is a NTP request
    word length = ether.packetReceive();
    ether.packetLoop(length);
    if(length > 0 && ether.ntpProcessAnswer(&timeFromNTP, 123)) {
      Serial.println();
      Serial.println("NTP reply received");
      return usEastern.toLocal(timeFromNTP - seventy_years);      
    }
    delay(500);
    i--;
  }
  Serial.println();
  Serial.println("NTP reply failed");
  return 0;
}

void reset_eth() {
  pinMode(ETH_RS_PIN, OUTPUT); // this lets you pull the pin low.
  digitalWrite(ETH_RS_PIN, LOW); // this resets the ENC28J60 hardware
  delay(100); // this makes sure the ENC28j60 resets OK.
  digitalWrite(ETH_RS_PIN, HIGH); // this makes for a fast rise pulse;
  pinMode(ETH_RS_PIN, INPUT); // this releases the pin,(puts it in high impedance); lets the pullup in the board do its job.
}
