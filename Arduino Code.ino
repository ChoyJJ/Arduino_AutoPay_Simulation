/*
This model uses a Arduino Uno R3 to design an AutoPay Machine.
*/
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
//To simulate the data stored when the ticket is taken
//when the user enters the carpark
String Database[5] = {"0123456", "1234567", "2345678", "3456789", "4567890"};
//The time the card is take out of the system
//All time are calculated in seconds
//For 12pm, 10am, 8.30am, 2pm,6.30pm
long Database_Time[5] = {43200, 36000, 30600, 50400, 66600};
//To Record the time using millis function
long InitialTime, CurrentTime;
const long TimeOutDuration = 30000;

//Set the time of the system when the system is reset
//Sets the Initial Time
long setTime() {
  Serial.println("Insert the current time");
  lcd.print("Insert Time");
  delay(500);
  Serial.println("Hour: ");
  while (!Serial.available()) {}
  int hour = Serial.parseInt();
  Serial.println(hour);
  delay(500);
  Serial.println("Minute: ");
  while (!Serial.available()) {}
  int minute = Serial.parseInt();
  Serial.println(minute);
  //the decimals have a L added at the back to prevent error
  //when multiplying for a long variable
  //long variable is used because the value is more than 32k
  long time = hour * 60 * 60L + minute * 60L;
  lcd.clear();
  return time;
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  Serial.begin(9600);
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(11, INPUT);
  //Call the function to set InitalTime
  InitialTime = setTime();
}

void loop() {
  //setting the variable for ticket input detection
  int TktInput = 0;
  //Setting the variable to check for ticket availability
  int Error;
  //Variable for showing the time
  int hour, minute;
  //Sets the payment price according to the duration
  int Price;
  //Sets the variable for Receiving Notes and Payment
  int RM1 = 0, RM5 = 0, RM10 = 0, Pay = 0, Remain;
  //Records the String value in the Barcode
  String Barcode;
  //Sets the variable for Returning changes
  int Return_RM10 = 0, Return_RM5 = 0, Return_RM1 = 0, Changes;
  //Sets the variable for time out during rreceiving notes
  long TimeOut = 0;
  //Record the current millis to update the time
  long Time = millis() / 1000L;
  //Records the current time
  CurrentTime = InitialTime + Time;
  lcd.clear();
  lcd.print("Start");
  //Call a function to write the time in lcd and serial monitor
  //in the for mat hh:mm
  WriteTime(CurrentTime);
  delay(1000);
  //Wait for user to input a ticket
  //When no ticket, show welcome message
  while (!TktInput) {
    Welcome_msg(TktInput);
    lcd.clear();
  }
  //Simulates the action of reading the Barcode input with Serial Monitor
  ReadTicket(Barcode);
  //Check for ticket availability, if not available will show error
  TicketCheck(Barcode, Error);
  //If error in Barcode, returns the ticket and restart the loop
  if (Error == 1) {
    Return_Ticket();
    return;
  }
  delay(500);
  //Record the start time of the ticket and compare it with current time
  //Calculate the duration of the ticket
  long Duration = duration(CurrentTime, Barcode);
  //sets the maximum price at RM10 for 5 hours
  if (Duration > 18000) {
    Price = 10;
  }
  else if (Duration < 0) {
    lcd.clear();
    lcd.print("Invalid Ticket");
    lcd.setCursor(0, 1);
    lcd.print("Duration");
    delay(2000);
    Return_Ticket();
    return;
  }
  else {
    //if not exceed 5 hours
    Fee_calculation(Price, Duration);
  }
  delay(500);
  if (Price > 0) {
    long StartTimer = millis();
    Remain = Price;
    ShowPrice(Price);
    while (Remain > 0 && TimeOut < TimeOutDuration) {
      ReadNotes(RM1, RM5, RM10, Pay, StartTimer);
      Remain = Price - Pay;
      if (Remain < 0)
        Remain = 0;
      ShowPrice(Remain);
      long CurrentTimer = millis();
      TimeOut = CurrentTimer - StartTimer;
      delay(100);
    }
  }
  if (TimeOut > TimeOutDuration) {
    Return_Note(RM1, RM5, RM10);
  }
  else if (Pay > Price) {
    Return_Changes (Price, Pay, Return_RM10, Return_RM5, Return_RM1);
    Return_Note (Return_RM1, Return_RM5, Return_RM10);
  }
  Return_Ticket();
  delay(3000);
}

void WriteTime(long int CurrentTime) {
  int hour = CurrentTime / 3600;
  int minutes = CurrentTime - hour * 3600;
  int minute = minutes / 60;
  if (hour < 10)
    Serial.print("0");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10)
    Serial.print("0");
  Serial.print(minute);
  Serial.println();
  lcd.setCursor(0, 1);
  if (hour < 10)
    lcd.print("0");
  lcd.print(hour);
  lcd.print(":");
  if (minute < 10)
    lcd.print("0");
  lcd.print(minute);
  lcd.println();
}

void Welcome_msg(int& Tkt) {
  String msg = "Welcome. Please Insert your Parking Ticket.";
  for (int i = 0; i < msg.length() - 15; i++) {
    //Simulate the receiving of ticket
    //When ticket receive = HIGH
    Tkt = digitalRead(8);
    if (Tkt)
      return;
    lcd.clear();
    lcd.print(msg.substring(i, i + 16));
    delay(200);
    if (i == 0 || i == msg.length() - 16)
      delay(800);
  }
  return;
}

//Simulating the action of sending the read barcode to Arduino
void ReadTicket(String& Barcode) {
  Serial.println("Please input the barcode number");
  delay(500);
  lcd.clear();
  lcd.print("Reading.....");
  while (!Serial.available()) {}
  if (Serial.available()) {
    Barcode = Serial.readString();
    Serial.read();
    delay(500);
  }
  lcd.clear();
  return;
}

//Check ticket for availability and return error
void TicketCheck(String Barcode, int& error) {
  //An array of barcodes will be recorded in a database
  //The database will include the time
  int Correct = 0;
  for (int i = 0; i < 5; i++) {
    if (Barcode == Database[i]) {
      Correct++;
    }
  }
  if (Correct == 0) {
    Serial.println("Error");
    Serial.println("Ticket Out");
    lcd.clear();
    lcd.print("Error");
    delay(1000);
    error = 1;
  }
  else {
    Serial.print("Ticket Received: ");
    Serial.println(Barcode);
    lcd.clear();
    lcd.print("Ticket Received");
    delay(1000);
    error = 0;
  }
  return;
}

long duration(long Time, String Barcode) {
  long in_time;
  for (int i = 0; i < 5; i++) {
    if (Barcode == Database[i]) {
      in_time = Database_Time[i];
    }
  }
  long Duration = Time - in_time;
  return Duration;
}

void Fee_calculation(int& Price, long Duration) {
  int hours = Duration / 3600;
  int minutes = (Duration - hours * 3600) / 60;
  Price = hours * 2;
  if (minutes > 15) {
    Price = Price + 1;
  }
  return;
}

void ShowPrice(int Price) {
  lcd.clear();
  lcd.print("Remain: RM");
  lcd.print(Price);
  return;
}

//Simulating the action of paying notes using buttons
void ReadNotes(int& RM1, int& RM5, int& RM10, int& Pay, long& TimerStart) {
  int RM1_in = digitalRead(9);
  int RM5_in = digitalRead(10);
  int RM10_in = digitalRead(11);
  if (RM1_in) {
    RM1 = RM1 + 1;
    Pay = Pay + 1;
  }
  else if (RM5_in) {
    RM5 = RM5 + 1;
    Pay = Pay + 5;
  }
  else if (RM10_in) {
    RM10 = RM10 + 1;
    Pay = Pay + 10;
  }
  while (RM1_in == 1 || RM5_in == 1 || RM10_in == 1) {
    int RM1_in = digitalRead(9);
    int RM5_in = digitalRead(10);
    int RM10_in = digitalRead(11);
    if (!RM1_in && !RM5_in && !RM10_in) {
      TimerStart = millis();
      delay(1000);
      return;
    }
  }
  return;
}

void Return_Changes (int Price, int Pay, int& Return_RM10, int& Return_RM5, int& Return_RM1) {
  int Changes = Pay - Price;
  Return_RM10 = 0;
  if (Changes >= 5) {
    Return_RM5 = Return_RM5 + 1;
    Changes = Changes - 5;
  }

  while (Changes >= 1 && Changes < 5) {
    Return_RM1 = Return_RM1 + 1;
    Changes = Changes - 1;
  }
  return;
}

void Return_Note(int RM1, int RM5, int RM10) {
  Serial.println("Change");
  Serial.print("RM1: ");
  Serial.println(RM1);
  Serial.print("RM5: ");
  Serial.println(RM5);
  Serial.print("RM10: ");
  Serial.println(RM10);
  lcd.clear();
  lcd.print("Change");
  lcd.setCursor(0, 1);
  lcd.print("RM1: ");
  lcd.print(RM1);
  delay(3000);
  lcd.setCursor(0, 1);
  lcd.print("RM5: ");
  lcd.print(RM5);
  delay(3000);
  lcd.setCursor(0, 1);
  lcd.print("RM10: ");
  lcd.print(RM10);
  delay(3000);
  return;
}

void Return_Ticket () {
  String msg_1 = "Please take your parking ticket";
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < msg_1.length() - 15; i++) {
      // display the returning of ticket
      lcd.clear();
      lcd.print(msg_1.substring(i, i + 16));
      delay(200);
      if (i == 0 || i == msg_1.length() - 16)
        delay(800);
    }
  }
  Serial.println ("Ticket Returned");
  delay(1000);
  return;
}
