#include <ThreeWire.h>
#include <Wire.h>
#include <RtcDS1302.h>
#include <LiquidCrystal_I2C.h>

ThreeWire myWire(4, 5, 2);  // IO/DAT, SCLK, CE/RST
RtcDS1302<ThreeWire> Rtc(myWire);

LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t month, day, hour, minute, second;
uint16_t year;

int mode_current = 0;
int mode_previous = -1;
const String mode_list[] = { "Time", "Stopwatch", "Timer" };

const byte pin_mode_button = 6;
const byte pin_start_button = 7;
const byte pin_reset_button = 8;
const byte pin_remove_button = 12;
const byte pin_add_button = 13;
const byte pin_buzzer = 3;
const byte pin_red = 9;
const byte pin_green = 10;
const byte pin_blue = 11;

bool is_started = false;
uint32_t start_time = 0;
String datetime = "00:00:00";
uint32_t elapsed_time = 0;
RtcDateTime dt = RtcDateTime(0, 0, 0, 0, 0, 0);
uint32_t stop_time = 0;

void setup() {
  Serial.begin(9600);

  // RTC Setup
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) Rtc.SetDateTime(compiled);

  delay(500);

  // setup LCD
  lcd.begin();
  lcd.backlight();
  lcd.noCursor();
  lcd.noBlink();

  // setup Digital Pin
  pinMode(pin_mode_button, INPUT);
  pinMode(pin_start_button, INPUT);
  pinMode(pin_reset_button, INPUT);
  pinMode(pin_add_button, INPUT);
  pinMode(pin_remove_button, INPUT);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pin_red, OUTPUT);
  pinMode(pin_green, OUTPUT);
  pinMode(pin_blue, OUTPUT);
}

void loop() {

  if (digitalRead(pin_mode_button) == HIGH) {
    lcd.clear();
    lcd.setCursor(0, 0);
    //lcd.print((String("Changing mode...")));

    // reset variable
    is_started = false;
    start_time = 0;
    if (mode_current == 0) {
      datetime = "00:00:00.000";
    } else if (mode_current == 1) {
      datetime = "00:00:00";
    }
    elapsed_time = 0;
    dt = RtcDateTime(0, 0, 0, 0, 0, 0);
    stop_time = 0;

    uint16_t rd = random(100, 1000);
    tone(pin_buzzer, rd, 1000);

    int size = sizeof(mode_list) / sizeof(String) - 1;
    if (mode_current >= size) {
      mode_current = 0;
    } else {
      mode_current++;
    }
    Serial.println("Changing mode : " + String(mode_current));
    delay(500);

  } else {
    //Serial.println("No");
    //noTone(pin_buzzer);
    // Get current DateTime
    RtcDateTime now = Rtc.GetDateTime();
    String current = getDateTime(now, false);
    if (mode_current == 0) {
      Serial.println(current);
    }

    if (mode_current != mode_previous) {
      mode_previous = mode_current;
      refreshDisplay();
    }

    switch (mode_current) {
      case 0:
        function_time(current);
        break;
      case 1:
        function_stopwatch();
        break;
      case 2:
        function_timer();
        break;
    }
  }
}

// Function : Time
String time_pre;
void function_time(String current) {
  if (time_pre != current) {
    rgb_color(102, 0, 204);
    time_pre = current;
    lcd.setCursor(0, 1);
    lcd.print((String(current)));
  }
}

// Function : stopwatch
void function_stopwatch() {
  //rgb_color(0, 0, 0);
  uint32_t current_time = 0;

  // Check if the start button is pressed
  if (digitalRead(pin_start_button) == HIGH) {
    if (is_started == true) {
      // Stop the stopwatch
      is_started = false;
      delay(100);
    } else {
      // Start the stopwatch
      is_started = true;
      start_time = millis() - elapsed_time;
      delay(100);
    }
    delay(50);
  }

  if (is_started == false) {
    // Display last time if the stopwatch is stopped
    lcd.setCursor(0, 1);
    lcd.print((String(datetime)));

    rgb_color(255, 0, 0);

    if (digitalRead(pin_reset_button) == HIGH) {
      refreshDisplay();
      start_time = millis();
      elapsed_time = 0;
      datetime = "00:00:00.000";
      dt = RtcDateTime(0, 0, 0, 0, 0, 0);
    }

  } else {
    // Calculate the elapsed time and display it
    current_time = millis();
    elapsed_time = current_time - start_time;
    uint32_t elapsed_seconds = elapsed_time / 1000;
    if (current_time % 10000 == 0) {
      refreshDisplay();
    }
    dt = getTime(elapsed_seconds);
    datetime = getDateTime(dt, true);
    lcd.setCursor(0, 1);
    lcd.print(datetime);
    delay(150);
    rgb_color(0, 128, 0);
  }
}

// Function: Timer
void function_timer() {
  uint32_t current_time = 0;
  if (digitalRead(pin_start_button) == HIGH) {
    if (is_started == true) {
      // Stop the timer
      is_started = false;
      Serial.println("TIMER STOPPED : BUTTON START : STOP");
      delay(100);
    } else {

      // Start the timer
      is_started = true;
      start_time = millis() - elapsed_time;
      refreshDisplay();
      Serial.println("TIMER STARTED : BUTTON START : START");
      delay(100);
    }
    delay(50);
  }


  if (is_started == true) {

    if (stop_time <= 0) {
      is_started = false;
      stop_time = 0;
      dt = RtcDateTime(0, 0, 0, 0, 0, 0);
      datetime = getDateTime(dt, false);
      return;
    }

    current_time = millis();

    elapsed_time = current_time - start_time;
    uint32_t stop_time_ms = stop_time * 1000;
    uint32_t remaining_time = stop_time_ms - elapsed_time;
    uint32_t time_sec = remaining_time / 1000;

    float progress = ((float)time_sec / stop_time);
    int color = progress * 255;
    rgb_color(255 - color, color, 0);

    dt = getTime(time_sec);
    datetime = getDateTime(dt, false);

    lcd.setCursor(0, 1);
    lcd.print(datetime);
    if (time_sec <= 3 && time_sec > 0) {
      tone(pin_buzzer, (200 * time_sec), 1000);
    }

    if (time_sec <= 0) {
      tone(pin_buzzer, 1000, 5000);
      is_started = false;
      Serial.println("TIMER STOPPED : Time's Up!");
      refreshDisplay();
    }

  } else {
    rgb_color(0, 0, 255);
    lcd.setCursor(0, 1);
    lcd.print(datetime);
    //noTone(pin_buzzer);

    // RESET time
    if (digitalRead(pin_reset_button) == HIGH) {
      refreshDisplay();
      stop_time = 0;
      dt = RtcDateTime(0, 0, 0, 0, 0, 0);
      datetime = getDateTime(dt, false);
    }
    // Add & Substract time in second(s)
    if (digitalRead(pin_add_button) == HIGH) {
      stop_time += 10;
      dt = getTime(stop_time);
      datetime = getDateTime(dt, false);
      lcd.setCursor(0, 1);
      lcd.print(datetime);
      Serial.print("Stop Time : +10s = ");
      Serial.println(stop_time);
      rgb_color(67, 255, 60);
      delay(500);
    }
    if (digitalRead(pin_remove_button) == HIGH) {
      if (stop_time <= 0) {
        stop_time = 0;
        return;
      }
      stop_time -= 10;
      dt = getTime(stop_time);
      datetime = getDateTime(dt, false);
      lcd.setCursor(0, 1);
      lcd.print(datetime);
      Serial.print("Stop Time : -10s = ");
      Serial.println(stop_time);
      rgb_color(128, 0, 0);
      delay(500);
    }
  }
}

// Set Mode Titles:
void refreshDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print((String(mode_list[mode_current]) + String(":")));
}

RtcDateTime getTime(uint32_t time_s) {
  uint32_t hours = time_s / 3600;
  uint32_t minutes = (time_s % 3600) / 60;
  uint32_t seconds = time_s % 60;
  RtcDateTime t = RtcDateTime(0, 0, 0, hours, minutes, seconds);
  return t;
}

void rgb_color(int red, int green, int blue) {
  analogWrite(pin_red, red);
  analogWrite(pin_green, green);
  analogWrite(pin_blue, blue);
}

// Get DateTime Function
String getDateTime(const RtcDateTime& dt, bool ms) {
  uint8_t hour = dt.Hour();
  uint8_t minute = dt.Minute();
  uint8_t second = dt.Second();
  uint16_t millisecond = 0;

  if (ms) {
    millisecond = (elapsed_time % 1000);
  }


  char buffer[20];
  if (ms == true) {
    sprintf(buffer, "%02d:%02d:%02d.%03d", hour, minute, second, millisecond);
  } else {
    sprintf(buffer, "%02d:%02d:%02d", hour, minute, second);
  }

  return String(buffer);
}