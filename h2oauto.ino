#include <math.h>
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
// Global Values
// Rele for electrovalve
int rele_electrovalve = 10;
// Moisture sensors
int moisture_pins[] = {A0,A1};
double moisture_min = 30;
double moisture_max = 55;
//double moisture_dif = 30;
double moisture_avg = 0;
double previous_avg = -1;
// Time
long watering_default = 300000; // TODO: change to 300000 (5 min) ~100ml in our conditions
long watering_time = 0;
long watering_final = 0;
long measuring_time = 15000; // 15000 (15 seg)
long waiting_time = 120000; // 120000 (2 min)
long hour = 21600000;
int cycle_time = 24;
int cycle_default = 24;
long total_time = 0;
// Watering boolean
boolean watering = true;
// LCD geometry
const int LCD_COLS = 16;
const int LCD_ROWS = 2;
// LCD
hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip
// Visualization
int doubledigits = 2;
int interval = 4000; // 4000 (4 seg) of visualization

void setup() {
  Serial.begin(9600);
  pinMode(rele_electrovalve, OUTPUT);
  digitalWrite(rele_electrovalve, LOW);
  // Initialize LCD 
  int status;
  status = lcd.begin(LCD_COLS, LCD_ROWS);
  if(status) // non zero status means it was unsuccesful
  {
    status = -status; // convert negative status value to positive number
    // begin() failed so blink error code using the onboard LED if possible
    hd44780::fatalError(status); // does not return
  }
  // LCD configuration
  lcd.lineWrap();
  // Print messages
  lcd.print("  Hydro-System    ORNI Project");
  delay(interval);
  lcd.clear();
  String message = " Starting codes ";
  for(int i = 0; i < message.length(); i++)
  {
    if (i % 31 == 0) {
      lcd.clear();
    }
    lcd.print(message[i]);
    delay(70); // slow things down to watch the printing & wrapping
  }
  delay(interval);
  lcd.clear();
}

void loop() {  
  if (watering) {
    total_time = 0;
    watering_time = watering_default;
    double moisture_avg1 = readMoisture(moisture_pins[0], 1);
    double moisture_avg2 = readMoisture(moisture_pins[1], 2);
    lcd.print("Moisture in     sensor 1: " + String(moisture_avg1,doubledigits) + "%");
    delay(interval);
    total_time += interval;
    lcd.clear();
    lcd.print("Moisture in     sensor 2: " + String(moisture_avg2,doubledigits) + "%");
    delay(interval);
    total_time += interval;
    lcd.clear();
    double moisture_avg = (moisture_avg1 + moisture_avg2)/2;
    String moisture_avgmsg = String(moisture_avg,doubledigits);
    String message = "Pre-watering    moisture: " + moisture_avgmsg + "%";
    lcd.print(message);
    delay(interval);
    total_time += interval;
    lcd.clear();
    // If moisture percentage is lower than minimum
    if (moisture_avg <= moisture_min) {
      // While moisture percentage is lower than maximum
      while (moisture_avg < moisture_max) {
        // Initial moisture
        moisture_avgmsg = String(moisture_avg,doubledigits);
        message = "Initial moistureis " + moisture_avgmsg + "%";
        lcd.print(message);
        delay(interval);
        total_time += interval;
        lcd.clear();
        // Watering
        wateringFor(watering_time);
        // New moisture
        double newmoisture_avg1 = readMoisture(moisture_pins[0], 1);
        double newmoisture_avg2 = readMoisture(moisture_pins[1], 2);
        lcd.print("New moisture in sensor 1: " + String(moisture_avg1,doubledigits) + "%");
        delay(interval);
        total_time += interval;
        lcd.clear();
        lcd.print("New moisture in sensor 2: " + String(moisture_avg2,doubledigits) + "%");
        delay(interval);
        total_time += interval;
        lcd.clear();
        double newmoisture_avg = (newmoisture_avg1 + newmoisture_avg2)/2;
        double difference = newmoisture_avg - moisture_avg;
        String new_moisturemsg = String(newmoisture_avg,doubledigits);
        //String differencemsg = String(difference,doubledigits);
        //minutemsg = String(round(watering_time/60000));
        //secondmsg = String(round((watering_time % 60000)/1000));
        message = "New moisture is " + new_moisturemsg + "%";
        lcd.print(message);
        delay(interval);
        total_time += interval;
        lcd.clear();
        // Error, no changes or impossible changes in moisture
        if (difference <= 0) {
          while (true) {
            message = "Error!: Check   moisture sensors";
            lcd.print(message);
            delay(interval);
            total_time += interval;
            lcd.clear();
            message = "or water level  (Restart after)";
            lcd.print(message);
            delay(interval);
            total_time += interval;
            lcd.clear();
            //break; 
          }
        }
        else {
          watering_time = watering_time*(moisture_max - difference)/difference;
          previous_avg = moisture_avg;
          moisture_avg = newmoisture_avg;
        }
        /*String minutemsg = String(round(waiting_time/60000));
        String secondmsg = String(round((waiting_time % 60000)/1000));
        message = "Final watering  for " + minutemsg + ":" + secondmsg + " [min]";
        lcd.print(message);
        delay(waiting_time);
        lcd.clear();*/
      }
      // Succesful watering
      lcd.print("Successful      watering");
      delay(interval);
      total_time += interval;
      lcd.clear();
      String moisture_avgmsg = String(moisture_avg,doubledigits);
      message = "Final moisture is " + moisture_avgmsg + "%";
      lcd.print(message);
      delay(interval);
      total_time += interval;
      lcd.clear();
      watering = false;
    }
    else {
      watering = false;
    }
  }
  else {
    // First watering
    if (previous_avg == -1) {
      lcd.print("First watering  not needed");
      delay(interval);
      total_time += interval;
      lcd.clear();
      cycle_time = cycle_default;
      for (int i = cycle_default; i >= 0; i--) {
        lcd.print("Next watering in");
        lcd.print(String(i) +" hrs");
        delay(hour - (total_time/cycle_default)/hour); //TODO: change to hour
        lcd.clear();
      }
      //previous_avg = moisture_avg;
    }
    /*if (previous_avg==moisture_avg) {
      lcd.print("Connect sensors or fill water");
      delay(interval);
      lcd.clear();
    }*/
    else {
      double difference_avg = moisture_avg - previous_avg;
      lcd.print("No more watering needed for now");
      delay(interval);
      total_time += interval;
      lcd.clear();
      cycle_time = cycle_default - round(total_time/hour);
      for (int i = cycle_time; i >= 0; i--){
        lcd.print("Next watering in");
        lcd.print(String(i) +" hrs");
        delay(hour - (total_time/cycle_default)/hour); //TODO: change to hour
        lcd.clear();
      }
    }
    watering = true;
    //lcd.print("Connect sensors or fill water");
    /*
    cycle_time = round(cycle_time + cycle_time * (float) difference_avg/100);
    lcd.print("Adjusting waiting time");
    delay(interval);
    lcd.clear();
    lcd.print("Changing        " + String(round(cycle_time * (float) difference_avg/100)) +" hrs");
    delay(interval);
    lcd.clear();
    }*/
  }
}
double readMoisture(int moisture_pin, int number) {
  double moisture_read;
  double moisture_value;
  double moisture_avg;
  double moisture_tot = 0;
  int cont = 0;
  String message = "Reading moisturein sensor "+ String(number)+"...";
  lcd.print(message);
  while (cont < measuring_time/1000) {
    moisture_read = analogRead(moisture_pin);
    moisture_value = map(moisture_read,670,100,0,100);
    moisture_tot += moisture_value;
    /*String moisture_readmsg = String(moisture_read,doubledigits);
    String moisture_valuemsg = String(moisture_read,doubledigits);
    String message = "Raw moisture: " + moisture_readmsg + "  Moisture: " + moisture_valuemsg + "%" + " - at " + String(cont) + " seconds";
    lcd.print(message);
    */
    delay(1000);
    cont += 1;
  }
  moisture_avg = moisture_tot / (measuring_time/1000);
  if (moisture_avg < 0)
    moisture_avg = 0.0;
  lcd.clear();    
  lcd.print("Finished reading");
  delay(2000);
  lcd.clear();
  return moisture_avg;
}
void wateringFor(long miliseconds) {
  String init_msg = "Watering for    " + String(round(miliseconds/60000)) + ":" + String(round((miliseconds % 60000)/1000)) + " [min]";
  lcd.print(init_msg);
  delay(interval);
  total_time += interval;
  lcd.clear();
  digitalWrite(rele_electrovalve, HIGH);
  /*for (int i = 0; i < miliseconds / 100; i= i + miliseconds/100) {
    delay(i);
  }*/
  String water_msg = "Watering your   beautiful plants";
  lcd.print(water_msg);
  delay(miliseconds);
  total_time += miliseconds;
  lcd.clear();
  digitalWrite(rele_electrovalve, LOW);
  String final_msg = "Finished        watering";
  lcd.print(final_msg);
  delay(interval);
  lcd.clear();
}
