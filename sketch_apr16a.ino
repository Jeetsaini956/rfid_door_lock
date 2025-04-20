//RFID Based Solenoid Door lock using Arduino
//RFID RC522 Master Card, Add and Remove multiple Cards

#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define RST_PIN 9
#define SS_PIN  10
#define Relay 3
#define REDLED 4
#define GREENLED 5
#define BUZZER 6
const int Btn = 2;
int lastState = LOW;
int NewState;

#define STATE_STARTUP       0
#define STATE_STARTING      1
#define STATE_WAITING       2
#define STATE_SCAN_INVALID  3
#define STATE_SCAN_VALID    4
#define STATE_SCAN_MASTER   5
#define STATE_ADDED_CARD    6
#define STATE_REMOVED_CARD  7

const int cardArrSize = 10;
const int cardSize    = 4;
byte cardArr[cardArrSize][cardSize];
byte masterCard[cardSize] = {0x08, 0x96, 0xb8, 0x89}; //Change Master Card ID
byte readCard[cardSize];
byte cardsStored = 0;

// Create MFRC522 instance
MFRC522 mfrc522(SS_PIN, RST_PIN);

byte currentState = STATE_STARTUP;
unsigned long LastStateChangeTime;
unsigned long StateWaitTime;

//------------------------------------------------------------------------------------
int readCardState()
{
  int index;

  Serial.print("Card Data - ");
  for (index = 0; index < 4; index++)
  {
    readCard[index] = mfrc522.uid.uidByte[index];


    Serial.print(readCard[index]);
    if (index < 3)
    {
      Serial.print(",");
    }
  }
  Serial.println(" ");

  //Check Master Card
  if ((memcmp(readCard, masterCard, 4)) == 0)
  {
    return STATE_SCAN_MASTER;
  }

  if (cardsStored == 0)
  {
    return STATE_SCAN_INVALID;
  }

  for (index = 0; index < cardsStored; index++)
  {
    if ((memcmp(readCard, cardArr[index], 4)) == 0)
    {
      return STATE_SCAN_VALID;
    }
  }

  return STATE_SCAN_INVALID;
}

//------------------------------------------------------------------------------------
void addReadCard()
{
  int cardIndex;
  int index;

  if (cardsStored <= 20)
  {
    cardsStored++;
    cardIndex = cardsStored;
    cardIndex--;
  }

  for (index = 0; index < 4; index++)
  {
    cardArr[cardIndex][index] = readCard[index];
  }

  // Write the updated card list to EEPROM
  EEPROM.write(0, cardsStored);
  for (int i = 0; i < cardsStored; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      EEPROM.write((i * 4) + j + 1, cardArr[i][j]);
    }
  }
  //EEPROM.commit();
}

//------------------------------------------------------------------------------------
void removeReadCard()
{
  int cardIndex;
  int index;
  boolean found = false;

  if (cardsStored == 0)
  {
    return;
  }

  for (index = 0; index < cardsStored; index++)
  {
    if ((memcmp(readCard, cardArr[index], 4)) == 0)
    {
      found = true;
      cardIndex = index;
    }
  }

  if (found == true)
  {
    // Remove the card from the array
    for (index = cardIndex; index < (cardsStored - 1); index++)
    {
      for (int j = 0; j < 4; j++)
      {
        cardArr[index][j] = cardArr[index + 1][j];
      }
    }
    cardsStored--;
    // Write the updated card list to EEPROM
    EEPROM.write(0, cardsStored);
    for (int i = 0; i < cardsStored; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        EEPROM.write((i * 4) + j + 1, cardArr[i][j]);
      }
    }
    //EEPROM.commit();
  }
}

//------------------------------------------------------------------------------------
void updateState(byte aState)
{
  if (aState == currentState)
  {
    return;
  }

  // do state change
  switch (aState)
  {
    case STATE_STARTING:
      StateWaitTime = 1000;
      digitalWrite(REDLED, HIGH);
      digitalWrite(GREENLED, LOW);
      break;
    case STATE_WAITING:
      StateWaitTime = 0;
      digitalWrite(REDLED, LOW);
      digitalWrite(GREENLED, LOW);
      break;
    case STATE_SCAN_INVALID:
      if (currentState == STATE_SCAN_MASTER)
      {
        addReadCard();
        aState = STATE_ADDED_CARD;
        StateWaitTime = 2000;
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, HIGH);
        Serial.println("Access Granted. Card added");
        success_buzzer();
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Access Granted ");
        lcd.setCursor(0, 1); // column, row
        lcd.print(" New Card Added ");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Scan Your RFID ");
        lcd.setCursor(0, 1); // column, row
        lcd.print(" Card/Key Chain ");
      }
      else if (currentState == STATE_REMOVED_CARD)
      {
        return;
      }
      else
      {
        StateWaitTime = 2000;
        digitalWrite(REDLED, HIGH);
        digitalWrite(GREENLED, LOW);
        Serial.println("Access Denied. Invalid Card detected");
        Failure_buzzer();
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Access Denied ");
        lcd.setCursor(0, 1); // column, row
        lcd.print("  Invalid Card  ");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Scan Your RFID ");
        lcd.setCursor(0, 1); // column, row
        lcd.print(" Card/Key Chain ");
      }
      break;
    case STATE_SCAN_VALID:
      if (currentState == STATE_SCAN_MASTER)
      {
        removeReadCard();
        aState = STATE_REMOVED_CARD;
        StateWaitTime = 2000;
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, HIGH);
        Serial.println("Access Granted. Card removed");
        success_buzzer();
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Access Granted ");
        lcd.setCursor(0, 1); // column, row
        lcd.print("  Card Removed  ");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Scan Your RFID ");
        lcd.setCursor(0, 1); // column, row
        lcd.print(" Card/Key Chain ");
      }
      else if (currentState == STATE_ADDED_CARD)
      {
        return;
      }
      else
      {
        StateWaitTime = 2000;
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, HIGH);
        Serial.println("Access Granted. Valid Card detected");
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Access Granted ");
        lcd.setCursor(0, 1); // column, row
        lcd.print(" Door Un-Locked ");
        beep();
        digitalWrite(Relay, LOW);
        delay(3000);
        digitalWrite(Relay, HIGH);
        lcd.clear();
        lcd.setCursor(0, 0); // column, row
        lcd.print(" Scan Your RFID ");
        lcd.setCursor(0, 1); // column, row
        lcd.print(" Card/Key Chain ");
      }
      break;
    case STATE_SCAN_MASTER:
      StateWaitTime = 5000;
      digitalWrite(REDLED, LOW);
      digitalWrite(GREENLED, HIGH);
      Serial.println("Access Granted. Master Card detected");
      beep();
      lcd.clear();
      lcd.setCursor(0, 0); // column, row
      lcd.print("   Master Card  ");
      lcd.setCursor(0, 1); // column, row
      lcd.print("    Detected    ");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0); // column, row
      lcd.print(" Scan Your RFID ");
      lcd.setCursor(0, 1); // column, row
      lcd.print(" Card/Key Chain ");
      break;
  }

  currentState = aState;
  LastStateChangeTime = millis();
}

void setup()
{
  // Start serial communication
  Serial.begin(115200);

  // Initialize the SPI bus
  SPI.begin();

  // Initialize the MFRC522 reader
  mfrc522.PCD_Init();

  // Initialize the EEPROM
  //EEPROM.begin(512);

  // Read the stored card list from EEPROM
  cardsStored = EEPROM.read(0);
  if (cardsStored > cardArrSize)
  {
    cardsStored = 0;
  }

  for (int i = 0; i < cardsStored; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      cardArr[i][j] = EEPROM.read((i * 4) + j + 1);
    }
  }

  LastStateChangeTime = millis();
  updateState(STATE_STARTING);

  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(Btn, INPUT_PULLUP);
  digitalWrite(Relay, HIGH);

  Serial.println("Put your card to the reader...");
  Serial.println();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); // column, row
  lcd.print(" Scan Your RFID ");
  lcd.setCursor(0, 1); // column, row
  lcd.print(" Card/Key Chain ");
}

void loop()
{
  NewState = digitalRead(Btn);
  if (lastState == HIGH && NewState == LOW) {

    Serial.println("Button Pressed. Access Granted");
    Serial.println();
    digitalWrite(REDLED, LOW);
    digitalWrite(GREENLED, HIGH);
    digitalWrite(Relay, LOW);
    lcd.clear();
    lcd.setCursor(0, 0); // column, row
    lcd.print(" Button Pressed ");
    lcd.setCursor(0, 1); // column, row
    lcd.print(" Door Un-Locked ");
    beep();
    delay(3000);
    digitalWrite(Relay, HIGH);
    digitalWrite(GREENLED, LOW);
    delay(50);
    lcd.clear();
    lcd.setCursor(0, 0); // column, row
    lcd.print(" Scan Your RFID ");
    lcd.setCursor(0, 1); // column, row
    lcd.print(" Card/Key Chain ");
  }


  byte cardState;

  if ((currentState != STATE_WAITING) &&
      (StateWaitTime > 0) &&
      (LastStateChangeTime + StateWaitTime < millis()))
  {
    updateState(STATE_WAITING);
  }

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  cardState = readCardState();
  updateState(cardState);
  lastState = NewState;
}

void success_buzzer()
{
  digitalWrite(BUZZER, HIGH);
  delay(2000);
  digitalWrite(BUZZER, LOW);
}

void Failure_buzzer()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    delay(50);
  }
}

void beep()
{
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}
