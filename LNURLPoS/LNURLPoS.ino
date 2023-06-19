
#include "SPI.h"
#include "TFT_eSPI.h"
#include <Keypad.h>
#include <string.h>
#include <Hash.h>
#include "esp_adc_cal.h"

//////////////KEYPAD///////////////////
bool isLilyGoKeyboard = false;

//////////////QR DISPLAY BRIGHTNESS///////////////////
int qrScreenBrightness = 180; // 0 = min, 255 = max

//////////////BATTERY///////////////////
const bool shouldDisplayBatteryLevel = true; // Display the battery level on the display?
const float batteryMaxVoltage = 4.2;          // The maximum battery voltage. Used for battery percentage calculation
const float batteryMinVoltage = 3.73;         // The minimum battery voltage that we tolerate before showing the warning

////////////////////////////////////////////////////////
////Note: See lines 75, 97, to adjust to keypad size////
////////////////////////////////////////////////////////

//////////////VARIABLES///////////////////
String inputs;
String virtkey;
RTC_DATA_ATTR int bootCount = 0;

#include "MyFont.h"

#define BIGFONT &FreeMonoBold24pt7b
#define MIDBIGFONT &FreeMonoBold18pt7b
#define MIDFONT &FreeMonoBold12pt7b
#define SMALLFONT &FreeMonoBold9pt7b
#define TINYFONT &TomThumb

TFT_eSPI tft = TFT_eSPI();
SHA256 h;

// QR screen colours
uint16_t qrScreenBgColour = tft.color565(qrScreenBrightness, qrScreenBrightness, qrScreenBrightness);

//////////////KEYPAD///////////////////
const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

//Big keypad setup
//byte rowPins[rows] = {12, 13, 15, 2}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {17, 22, 21}; //connect to the column pinouts of the keypad

//LilyGO T-Display-Keyboard
byte rowPins[rows] = {21, 27, 26, 22}; //connect to the row pinouts of the keypad
byte colPins[cols] = {33, 32, 25}; //connect to the column pinouts of the keypad

// 4 x 4 keypad setup
//byte rowPins[rows] = {21, 22, 17, 2}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {15, 13, 12}; //connect to the column pinouts of the keypad

//Small keypad setup
//byte rowPins[rows] = {21, 22, 17, 2}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {15, 13, 12};    //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);
int checker = 0;
char maxdig[20];

//////////////MAIN///////////////////
void logo()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // White characters on black background
  tft.setFreeFont(BIGFONT);
  tft.setCursor(7, 70);  // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("Base58⛓🔓"); // Using tft.print means text background is NEVER rendered

  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Green characters on black background
  tft.setFreeFont(MIDBIGFONT);
  tft.setCursor(42, 90);          // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("LARP Hash"); // Using tft.print means text background is NEVER rendered
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print(" Verifier");
}

void setup(void)
{
  Serial.begin(115200);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  btStop();
  h.begin();
  tft.begin();

  //Set to 3 for bigger keypad
  tft.setRotation(1);
  if (bootCount == 0)
  {
    logo();
    delay(1500);
  }

  ++bootCount;
  Serial.println("Boot count" + bootCount);
}


// inputs is always 6-chars or less
uint32_t convert_input(String inputs)
{
  uint32_t val = 0;
  for (size_t i = 0; i < inputs.length(); i++) {
    val = val * 10;
    val += get_val(inputs[i]);
  }
  return val;
}

uint8_t get_val(char a)
{
  switch (a) {
    case '1':
      return 1;
     case '2':
      return 2;
     case '3':
      return 3;
     case '4':
      return 4;
     case '5':
      return 5;
     case '6':
      return 6;
     case '7':
      return 7;
     case '8':
      return 8;
     case '9':
      return 9;
  }
  return 0;
}

void displayCalc(uint8_t stage, bool verify)
{
  uint8_t len_limit;
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // White characters on black background
  tft.setFreeFont(MIDBIGFONT);
  tft.setCursor(10, 35);
  switch (stage) {
    case 0:
      tft.print("Verify?");
      len_limit = 1;
      break;
    case 1:
      tft.print("Prev Block?");
      len_limit = 6;
      break;
    case 2:
      tft.print("Tx Commit?");
      len_limit = 4;
      break;
    case 3:
      tft.print("Height?");
      len_limit = 2;
      break;
    case 4:
      tft.print("Time?");
      len_limit = 4;
      break;
    case 5:
      if (verify) {
        tft.print("Nonce?");
      } else {
        tft.print("Difficulty?");
      }
      len_limit = 6;
      break;
    case 6:
      if (verify) {
        tft.print("Hash:");
      } else
        tft.print("Good Nonce:");
      break;
    default:
      abort();
  }

  if (inputs.length() < len_limit && stage != 6)
    inputs += virtkey;

  tft.setFreeFont(BIGFONT);
  tft.setCursor(50, 120);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  /* If it's stage 0, print Y/N */
  if (stage == 0 && inputs.length() > 0) {
    if (convert_input(inputs) == 0) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("N");
    } else
      tft.println("Y");
  } else
    tft.println(inputs);
}

uint32_t calculate_result(uint16_t prev_block, uint16_t tx_commit, uint16_t hght, uint16_t time_val, uint32_t nonce_val)
{
  uint8_t hashresult[20];
  uint32_t result;
  uint32_t accum;
  RMD160 h;
  
  h.begin();
  h.write((uint8_t *)"BTC-LARP:", 8);
  h.write(prev_block);
  h.write(tx_commit);
  h.write(hght);
  h.write(time_val);
  h.write(nonce_val);
  h.end(hashresult);

  accum = 1;
  for (int i = 0; i < 3; i++) {
    result += accum * hashresult[i];
    accum = accum * 255;
  }

  // there's only 10 ^ 7 - 1 options
  return result % 1000000;
}

void displayNonceTry(uint32_t nonce_val)
{

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // White characters on black background
  tft.setFreeFont(MIDBIGFONT);
  tft.setCursor(10, 45);
  tft.print("Trying...");
  tft.setFreeFont(BIGFONT);
  tft.setCursor(50, 120);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(nonce_val);
}

void loop()
{
  uint32_t prev_block;
  uint32_t nonce_or_hash_val;
  uint16_t time_val;
  uint16_t height_val;
  uint16_t tx_commit;
  uint8_t stage = 0;
  bool verify;

  digitalWrite(4, HIGH);
  inputs = "";
  prev_block = nonce_or_hash_val = time_val = height_val = tx_commit = 0;
  displayCalc(stage, false);

  for (;;)
  {
    char key = keypad.getKey();
    if (key != NO_KEY)
    {
      if (key == '#')
      {
          switch (stage) {
            case 0:
              verify = (convert_input(inputs) != 0);
              break;
            case 1:
              prev_block = convert_input(inputs);
              break;
            case 2:
              tx_commit = convert_input(inputs);
              break;
            case 3:
              height_val = convert_input(inputs);
              break;
            case 4:
              time_val = convert_input(inputs);
              break;
            case 5:
              nonce_or_hash_val = convert_input(inputs);
              break;
            case 6:
              // We do nothing;
              break;
          }
          
          stage++;
          inputs = "";
          virtkey = "";
          
          if (stage == 6) {
            uint32_t nonce_val;
            uint32_t block_hash;

            block_hash = 0;
            if (verify) {
              nonce_val = nonce_or_hash_val;
            } else {
              nonce_val = 0;
            }

            block_hash = calculate_result(prev_block, tx_commit, height_val, time_val, nonce_val);

            for (; !verify && block_hash > nonce_or_hash_val && nonce_val < 1000000;) {
              nonce_val++;
              displayNonceTry(nonce_val );
              block_hash = calculate_result(prev_block, tx_commit, height_val, time_val, nonce_val);
              
              Serial.print("found hash:");
              Serial.print(block_hash);
              Serial.print(" .. looking for under");
              Serial.println(nonce_or_hash_val);
              delay(50);
            }
            
            if (verify) {
              // set the inputs to the computed hash!
              inputs = String(block_hash);
            } else
              // set the inputs to the winning nonce!
              inputs = String(nonce_val);
          }
          if (stage == 7) {
            // we go back to the beginning!
            stage = 0;
          }
      }
      else if (key == '*')
      {
        if (inputs.length() > 0) {
          String tmp = "";
          for (size_t i = 0; i < inputs.length() - 1; i++)
            tmp += inputs[i];

          inputs = tmp;
        }
        virtkey = "";
      }
      else
        virtkey = String(key);

      displayCalc(stage, verify);
    }
    delay(5);
  }
}

long lastBatteryUpdate = millis();
int batteryLevelUpdatePeriod = 10; // update every X seconds
/**
 * Display the battery voltage
 */
void displayBatteryVoltage(bool forceUpdate)
{
  long currentTime = millis();
  if (
      (shouldDisplayBatteryLevel &&
       (currentTime > (lastBatteryUpdate + batteryLevelUpdatePeriod * 1000)) &&
       !isPoweredExternally()) ||
      (shouldDisplayBatteryLevel && forceUpdate && !isPoweredExternally()))
  {
    lastBatteryUpdate = currentTime;
    bool showBatteryVoltage = false;
    float batteryCurV = getInputVoltage();
    float batteryAllowedRange = batteryMaxVoltage - batteryMinVoltage;
    float batteryCurVAboveMin = batteryCurV - batteryMinVoltage;

    int batteryPercentage = (int)(batteryCurVAboveMin / batteryAllowedRange * 100);

    if (batteryPercentage > 100)
    {
      batteryPercentage = 100;
    }

    int textColour = TFT_GREEN;
    if (batteryPercentage > 70)
    {
      textColour = TFT_GREEN;
    }
    else if (batteryPercentage > 30)
    {
      textColour = TFT_YELLOW;
    }
    else
    {
      textColour = TFT_RED;
    }

    tft.setTextColor(textColour, TFT_BLACK);
    tft.setFreeFont(SMALLFONT);

    int textXPos = 195;
    if (batteryPercentage < 100)
    {
      textXPos = 200;
    }

    // Clear the area of the display where the battery level is shown
    tft.fillRect(textXPos - 2, 0, 50, 20, TFT_BLACK);
    tft.setCursor(textXPos, 16);

    // Is the device charging?
    if (isPoweredExternally())
    {
      tft.print("CHRG");
    }
    // Show the current voltage
    if (batteryPercentage > 10)
    {
      tft.print(String(batteryPercentage) + "%");
    }
    else
    {
      tft.print("LO!");
    }

    if (showBatteryVoltage)
    {
      tft.setFreeFont(SMALLFONT);
      tft.setCursor(155, 36);
      tft.print(String(batteryCurV) + "v");
    }
  }
}

/**
 * Get the voltage going to the device
 */
float getInputVoltage()
{
  delay(100);
  uint16_t v1 = analogRead(34);
  return ((float)v1 / 4095.0f) * 2.0f * 3.3f * (1100.0f / 1000.0f);
}

/**
 * Does the device have external or internal power?
 */
bool isPoweredExternally()
{
  float inputVoltage = getInputVoltage();
  if (inputVoltage > 4.5)
  {
    return true;
  }
  else
  {
    return false;
  }
}
