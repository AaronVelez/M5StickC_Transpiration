////// Get time from NTP an dupdate RTC and local time variables
bool GetNTPTime() {
    if (timeClient.forceUpdate()) {
        UTC_t = timeClient.getEpochTime();
        // set system time to UTC unix timestamp
        setTime(UTC_t);
        // Set RTC time to UTC time from system time
        RTC_TimeStruct.Hours = hour();
        RTC_TimeStruct.Minutes = minute();
        RTC_TimeStruct.Seconds = second();
        RTC_DateStruct.WeekDay = weekday();
        RTC_DateStruct.Month = month();
        RTC_DateStruct.Date = day();
        RTC_DateStruct.Year = year();
                
        M5.Rtc.SetTime(&RTC_TimeStruct);  //writes the set time to the real time clock.
        M5.Rtc.SetData(&RTC_DateStruct);  //writes the set date to the real time clock. 
       
        // Convert to local time
        local_t = mxCT.toLocal(UTC_t);
        // Set system time lo local time
        setTime(local_t);
        LastNTP = UTC_t;
        if (debug) { Serial.println(F("NTP client update success!")); }
        return true;
    }
    else {
        if (debug) { Serial.println(F("NTP update not succesfull")); }
        return false;
    }
}


////// Get time from RTC and update UTC and local time variables
void GetRTCTime() {             
    M5.Rtc.GetTime(&RTC_TimeStruct);    // Get UTC time from RTC
    M5.Rtc.GetData(&RTC_DateStruct);
    setTime(RTC_TimeStruct.Hours,                   // Set system time to UTC time
        RTC_TimeStruct.Minutes,
        RTC_TimeStruct.Seconds,
        RTC_DateStruct.Date,
        RTC_DateStruct.Month,
        RTC_DateStruct.Year);
    UTC_t = now();                      // Get UTC time from system time in UNIX format
    local_t = mxCT.toLocal(UTC_t);      // Calculate local time in UNIX format
    setTime(local_t);                   // Set system time to local
    s = second();                       // Set time variables to local time from system time
    m = minute();
    h = hour();
    dy = day();
    mo = month();
    yr = year();
}