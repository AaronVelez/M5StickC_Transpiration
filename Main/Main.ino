/*
 Name:		Main.ino
 Created:	3/14/2022 10:00 AM
 Authors:	Emmanuel Reyes, Jessica Jacobo, Paola Carrera, Aarón Vélez
*/




//////////////////////////////////////////////////////////////////
////// Libraries and its associated constants and variables //////
//////////////////////////////////////////////////////////////////

////// Board library
#include <M5StickCPlus.h>




////// Credentials_Gas_Alarm_Photo_Lab.h is a user-created library containing paswords, IDs and credentials
#include "Credentials_Tor_Rey_IoT.h"
#ifdef WiFi_SSID_is_HEX
const bool ssidIsHex = true;
const char ssidHEX[] = WIFI_SSID_HEX;
char ssid[64];
#else
const bool ssidIsHex = false;
const char ssid[] = WIFI_SSID;
#endif
const char password[] = WIFI_PASSWD;
const char iot_server[] = IoT_SERVER;
const char iot_user[] = IoT_USER;
const char iot_device[] = IoT_DEVICE;
const char iot_credential[] = IoT_CREDENTIAL;
const char iot_data_bucket[] = IoT_DATA_BUCKET;


////// Comunication libraries
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;


////// Iot Thinger
#define THINGER_SERVER iot_server   // Delete this line if using a free thinger account 
#define _DEBUG_   // Uncomment for debugging connection to Thinger
#define _DISABLE_TLS_     // Uncoment if needed (port 25202 closed, for example)
#include <ThingerESP32.h>
ThingerESP32 thing(iot_user, iot_device, iot_credential);
/// <summary>
/// IMPORTANT
/// in file \thinger.io\src\ThingerClient.h at Line 355, the function handle_connection() was modified
/// to prevent the thinger handler to agresively try to reconnect to WiFi in case of a lost connection
/// This allows the alarn to keep monitoring gas levels even if there is no network connection
/// </summary>



////// Time libraries
// Integrated RTC definitions
RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;
#include <TimeLib.h>
#include <NTPClient.h>
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", 0, 300000); // For details, see https://github.com/arduino-libraries/NTPClient
// Time zone library
#include <Timezone.h>
//  Central Time Zone (Mexico City)
TimeChangeRule mxCDT = { "CDT", First, Sun, Apr, 2, -300 };
TimeChangeRule mxCST = { "CST", Last, Sun, Oct, 2, -360 };
Timezone mxCT(mxCDT, mxCST);





////// Enviroment III unit library
#include "UNIT_ENV.h"
SHT3X sht30;
QMP6988 qmp6988;



////// NCIR sensor library
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 NCIR = Adafruit_MLX90614();




//////////////////////////////////////////
////// User Constants and Variables //////
//////////////////////////////////////////

////// Station IDs & Constants
const int Station_Number = 9;
String Board = F("M5StickC Plus");
String Processor = F("ESP32-Pico");
String IoT_Hardware = F("ESP32-Pico WiFi");
String IoT_Software = F("Thinger ESP32");
String RTC_Hardware = F("BM8563");
String IoT_Asset_Type = F("");
String IoT_Group = F("");
String IoT_Station_Name = F("Tor Rey IoT");
String Firmware = F("v1.0.0");

bool debug = true;


////// PINs
const int Rx_PIN = 25;
const int Tx_PIN = 26;


////// Time variables
time_t LastNTP;     // Last UTP time that the RTC was updated form NTP
time_t UTC_t;       // UTC UNIX time stamp
time_t local_t;     // Local time with DST adjust in UNIX time stamp format
int s = -1;		    // Seconds
int m = -1;		    // Minutes
int h = -1;		    // Hours
int dy = -1;	    // Day
int mo = -1;	    // Month
int yr = -1;	    // Year


// State Variables
const int IoT_interval = 1;     // upload interval in minutes
const int Meassure_interval = 10;    // upload measure interval in seconds
int LastLcd = -1;           // Last time the screen was updated
int LastSum = -1;			// Last minute that variables were added to the sum for later averaging
int SumNum = 0;				// Number of times a variable value has beed added to the sum for later averaging
int LastAvg= -1;			// Last minute that variables were averaged

time_t t_WiFiCnxTry = 0;      // Last time a (re)connection to internet happened
const int WiFiCnx_frq = 30;  // (re)connection to internet frequency in seconds



// Variables
const int n = 500;       // measure n times the ADC input for averaging
 
float Tair = 0;         // Air temperature
float RHair = 0;        // Relative Humedity of air
float Patm = 0;         // Atmosferic Pressure
float Tleaf = 0;        // Leaf temperature
float AirWaterP = 0;    // Air water vapor pressure in kPa
float LeafWaterP = 0;   // Leaf Pressure in kPa
float VPD = 0;          // Vapor Pressure Deficit
float Weight = 0;       // Balance weight in grams

char ScaleMessage[50];
String str;
String ScaleStatus;

float TairSum = 0;     
float RHairSum = 0;    
float PatmSum = 0;     
float TleafSum = 0;   
float AirWaterPSum = 0;    
float LeafWaterPSum = 0;   
float VPDSum = 0;  
float WeightSum = 0;

float TairAvg = 0;
float RHairAvg = 0;
float PatmAvg = 0;
float TleafAvg = 0;
float AirWaterPAvg = 0;
float LeafWaterPAvg = 0;
float VPDAvg = 0;
float WeightAvg = 0;






//////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board //
//////////////////////////////////////////////////////////////////////////
void setup() {
    ////// Initialize and setup M5Stack
    M5.begin(true, true, true);
    M5.Lcd.println(F("M5 started"));
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, Rx_PIN, Tx_PIN);
    // Serial2.begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert)
    // Disable G36 to use G25
    gpio_pulldown_dis(GPIO_NUM_36);
    gpio_pullup_dis(GPIO_NUM_36);

#ifdef WiFi_SSID_is_HEX
    String ssidStr = HexString2ASCIIString(ssidHEX);
    ssidStr.toCharArray(ssid, sizeof(ssid) + 1);
#endif

    M5.Lcd.print(F("SSID name: "));
    M5.Lcd.println(ssid);

    if (password == "") { WiFi.begin(ssid); }
    else { WiFi.begin(ssid, password); }

    WiFi.setAutoReconnect(false);
    M5.Lcd.print(F("Connecting to internet..."));
    // Test for 10 seconds if there is WiFi connection;
    // if not, continue to loop in order to monitor gas levels
    for (int i = 0; i <= 10; i++) {
        if (WiFi.status() != WL_CONNECTED) {
            M5.Lcd.print(".");
            delay(1000);
        }
        else {
            M5.Lcd.println(F("\nConnected to internet!"));
            break;
        }
        if (i == 10) {
            M5.Lcd.println(F("\nNo internet connection"));
            WiFi.disconnect();  // if no internet, disconnect. This prevents the board to be busy only trying to connect.
        }
    }





    ////// Configure IoT
    M5.Lcd.println(F("Configuring IoT..."));
    if (password == "") { thing.add_wifi(ssid); }
    else { thing.add_wifi(ssid, password); }
   
    // Define output resources
    thing["RT_Tair"] >> [](pson& out) { out = Tair; };
    thing["RT_RHair"] >> [](pson& out) { out = RHair; };
    thing["RT_Atmmosferic_Pressure"] >> [](pson& out) { out = Patm; };
    thing["RT_VPD"] >> [](pson& out) { out = VPD; };
    thing["RT_Weight"] >> [](pson& out) { out = Weight; };
    thing["RT_Scale_Status"] >> [](pson& out) { out = ScaleStatus; };

    thing["Avg_Data"] >> [](pson& out) {
        out["Avg_Tair"] = TairAvg;
        out["Avg_RHair"] = RHairAvg;
        out["Avg_Atmmosferic_Pressure"] = PatmAvg;
        out["Avg_VPD"] = VPDAvg;
        out["Avg_Weight"] = WeightAvg;
        out["UNIX_local"] = local_t;   
    };





    ////// Start RTC
    // Started with M5StickC plus board


    //////// If internet, start NTP client engine
    //////// and update RTC and system time
    //////// If no internet, get time form RTC
    M5.Lcd.println(F("Starting NTP client engine..."));
    timeClient.begin();
    M5.Lcd.print(F("Trying to update NTP time..."));
    // Try to update NTP time.
    // If not succesful , get RTc time and continue to loop in order to monitor gas levels
    if (!GetNTPTime()) {
        GetRTCTime();
        M5.Lcd.println(F("\nTime updated from RTC"));
    }
    else { M5.Lcd.println(F("\nTime updated from NTP")); }
    
    
    


    ////// Start Environmental III Unit
    qmp6988.init();     // Initialize pressure sensor





    ////// Start NCIR
    NCIR.begin();
    Serial.print(F("Emmisivity: "));
    Serial.println(NCIR.readEmissivity());





    ////// Setup finish message
    M5.Lcd.println(F("Setup done!"));
    M5.Lcd.println(F("\nStarting in "));
    for (int i = 0; i <= 5; i++) {
        M5.Lcd.println(5 - i);
        if (i != 5) { delay(1000); }
        else { delay(250); }
    }
    M5.Lcd.fillScreen(BLACK);
}



//////////////////////////////////////////////////////////////////////////
// The loop function runs over and over again until power down or reset //
//////////////////////////////////////////////////////////////////////////
void loop() {
    ////// State 0. Test internet connection; if not, try to connect.
    if (WiFi.status() != WL_CONNECTED) { WiFi.disconnect(); }
    if (WiFi.status() != WL_CONNECTED &&
        UTC_t - t_WiFiCnxTry > WiFiCnx_frq) {
        Serial.println(F("Loop reconect try"));
        WiFi.begin(ssid, password);

        t_WiFiCnxTry = UTC_t;
    }
    
    
    
    ////// State 1. Keep the Iot engine runing
    thing.handle();





    ////// State 2. Get time from RTC time
    GetRTCTime();
    if (debug) { Serial.println((String)"Time: " + h + ":" + m + ":" + s); }





    ////// State 3. Update RTC time from NTP server at midnight
    if (h == 0 &&
        m == 0 &&
        s == 0 &&
        UTC_t != LastNTP) {
        GetNTPTime();
    }
    
    



    ////// State 4. Check if it is time to read sensors nad updtae screen
    if ((s % Meassure_interval == 0) && (s != LastSum)) {
        // Read scale
        memset(ScaleMessage, 0, sizeof(ScaleMessage));  // Delete old messagee
        Serial2.print(F("P"));                          // Send Print reqest
        while(Serial2.available() == 0) {}              // Wait for Scale response
        int i = 0;
        while (Serial2.available() > 0) {
            int IncomingByte;
            IncomingByte = Serial2.read();
            ScaleMessage[i] = IncomingByte;
            i++;
        }
        str = String(ScaleMessage);
        if (debug) {
            Serial.print(F("Scale full response message: "));
            Serial.println(str);
            Serial.print(F("Extracted value: "));
            Serial.println(str.substring(str.indexOf("\r") + 1, str.indexOf("\r") + 8));
        }
        ScaleStatus = str.substring(0, str.indexOf("\r"));
        str = str.substring(str.indexOf("\r") + 1, str.indexOf("\r") + 8);
        Weight = str.toFloat();
        
        
        // Read environmental sensors 
        Tair = sht30.cTemp;  //Store the temperature obtained from shT30. 
        RHair = sht30.humidity; //Store the humidity obtained from the SHT30.  
        Patm = qmp6988.calcPressure();
        Tleaf = 0;


        // Caulculate VPD
        VPD = CalculateVPD(Tair, RHair, Tleaf);


        // Update Screen
    // Reset screen
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setCursor(0, 0);
        // CO2 mVolt
        M5.Lcd.setTextSize(2.5);
        M5.Lcd.println();
        M5.Lcd.println(" CO2 mV:");
        M5.Lcd.println();
        M5.Lcd.setTextSize(3.5);
        M5.Lcd.print(" ");
        M5.Lcd.println(0000000);
        M5.Lcd.println();
        // CO2 ppm
        M5.Lcd.setTextSize(2.5);
        M5.Lcd.println(" CO2 ppm:");
        M5.Lcd.println();
        M5.Lcd.setTextSize(3.5);
        M5.Lcd.print(" ");
        M5.Lcd.println(000000);
        M5.Lcd.println();
        // Calibration status
        /*
        M5.Lcd.setTextSize(2.5);
        M5.Lcd.println(" Is Cal?");
        M5.Lcd.println();
        M5.Lcd.setTextSize(3.5);
        M5.Lcd.print(" ");
        M5.Lcd.println(IsCal);
        */

        // Sum values
        TairSum += Tair;
        RHairSum += RHair;
        PatmSum += Patm;
        TleafSum += Tleaf;
        VPDSum += VPD;
        WeightSum += Weight;

        SumNum += 1;
        LastSum = s;

    }





    ////// State 5. Check if it is time to average and send values to IoT
    if ((m % IoT_interval == 0) && (m != LastAvg)) {

    }




    
    delay(1000);
}






// Function for WiFi SSID with non-ASCII characters
String HexString2ASCIIString(String hexstring) {
    String temp = "", sub = "", result;
    char buf[3];
    for (int i = 0; i < hexstring.length(); i += 2) {
        sub = hexstring.substring(i, i + 2);
        sub.toCharArray(buf, 3);
        char b = (char)strtol(buf, 0, 16);
        if (b == '\0')
            break;
        temp += b;
    }
    return temp;
}

/*
unsigned int hexToDec(String hexString) {

    unsigned int decValue = 0;
    int nextInt;

    for (int i = 0; i < hexString.length(); i++) {

        nextInt = int(hexString.charAt(i));
        if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
        if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
        if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
        nextInt = constrain(nextInt, 0, 15);

        decValue = (decValue * 16) + nextInt;
    }

    return decValue;
}
*/

float CalculateVPD(float AirTemp, float AirRH, float LeafTemp) {
    float result = 0;
    AirWaterPSum = 0/*Tarea*/;
    LeafWaterPSum = 0/*Tarea*/ ;
    result = AirWaterPSum - LeafWaterPSum;

    return result;

}


