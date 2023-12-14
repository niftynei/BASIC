
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

#define BIGFONT &AllertaStencil_Regular24pt7b
#define MIDBIGFONT &AllertaStencil_Regular18pt7b
#define MIDFONT &AllertaStencil_Regular12pt7b
#define SMALLFONT &AllertaStencil_Regular9pt7b
#define BASE58FONT &arial24pt7b
#define TINYFONT &TomThumb

TFT_eSPI tft = TFT_eSPI();
SHA256 h;

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

#define B58_PURPLE 0x2043 /* 36, 9, 28 */

//////////////MAIN///////////////////
void logo()
{
  tft.fillScreen(B58_PURPLE);
  tft.setTextColor(TFT_WHITE, B58_PURPLE); // white characters on purple background
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(10, 30);  // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("Base58's LARP"); // Using tft.print means text background is NEVER rendered

  tft.setTextColor(TFT_YELLOW, B58_PURPLE);
  tft.setFreeFont(MIDBIGFONT);
  tft.setCursor(10, 70);
  tft.print("BASIC");
  tft.setCursor(10, 100);
  tft.setFreeFont(SMALLFONT);
  tft.setTextColor(TFT_WHITE, B58_PURPLE);
  tft.print("machine"); // Using tft.print means text background is NEVER rendered
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
    delay(5000);
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
  
  tft.fillScreen(B58_PURPLE);
  tft.setTextColor(TFT_WHITE, B58_PURPLE); // White characters on black background
  tft.setFreeFont(MIDBIGFONT);
  tft.setCursor(10, 35);
  switch (stage) {
    case 0:
      tft.print("Prev Block?");
      len_limit = 6;
      break;
    case 1:
      tft.print("Tx Commit?");
      len_limit = 4;
      break;
    case 2:
      tft.print("Time?");
      len_limit = 4;
      break;
    case 3:
      tft.print("Target?");
      len_limit = 6;
      break;
    case 4:
      tft.print("Nonce?");
      len_limit = 6;
      break;
    case 5:
      tft.print("Nonce:");
      len_limit = 6;
      break;
    case 6:
      tft.print("Blockhash:");
      len_limit = 6;
      break;
    default:
      abort();
  }

  if (inputs.length() < len_limit && stage != 5 && stage != 6)
    inputs += virtkey;

  tft.setFreeFont(BIGFONT);
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_YELLOW, B58_PURPLE);

  tft.println(inputs);
}

// 103554 (12:34)
// ...... (12:01)
uint32_t calculate_result(uint16_t prev_block, uint16_t tx_commit, uint16_t target, uint16_t time_val, uint32_t nonce_val)
{
  uint8_t hashresult[20], hr, nonce[4];
  uint32_t result;
  uint32_t accum;
  RMD160 h;

  for (int i = 0; i < 4; i++) {
    nonce[i] = ((nonce_val >> (i * 8)) & 0xff);
  }
  h.begin();
  h.write((uint8_t *)"BTC-LARP:", 8);
  h.write(highByte(prev_block));
  h.write(lowByte(prev_block));
  h.write(highByte(tx_commit));
  h.write(lowByte(tx_commit));
  h.write(highByte(time_val));
  h.write(lowByte(time_val));
  h.write(highByte(target));
  h.write(lowByte(target));
  h.write(nonce, 4);
  h.end(hashresult);

  accum = 1;
  for (int i = 0; i < 5; i++) {
    hr = hashresult[i];
    result += accum * hr;
    accum = accum * 255;
  }
  
  return result;
}

void displayNonceTry(uint32_t block_hash, uint32_t low_nonce, uint32_t nonce_val)
{

  tft.fillScreen(B58_PURPLE);
  tft.setTextColor(TFT_WHITE, B58_PURPLE);
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(10, 30);
  tft.printf("Lowest Hash: %d", block_hash); 
  
  tft.setTextColor(TFT_YELLOW, B58_PURPLE);
  tft.setFreeFont(MIDFONT);
  tft.setCursor(10, 70);
  tft.printf("Tries: %d", nonce_val);
  tft.setCursor(10, 100);
  tft.setFreeFont(SMALLFONT);
  tft.setTextColor(TFT_WHITE, B58_PURPLE);
  tft.printf("Best Nonce: %d", low_nonce);
}

void displayTryCanceled()
{

  tft.fillScreen(B58_PURPLE);
  tft.setFreeFont(BIGFONT);
  tft.setCursor(50, 120);
  tft.setTextColor(TFT_WHITE, B58_PURPLE);
  tft.println("Exit...");
  delay(1500);
}

void loop()
{
  uint32_t prev_block;
  uint32_t nonce, low_nonce;
  uint16_t time_val;
  uint16_t tx_commit;
  uint16_t target;
  uint8_t stage = 0;
  bool verify;
  uint32_t block_hash, lowest;
  uint32_t nonce_try;

  digitalWrite(4, HIGH);
  inputs = "";
  prev_block = nonce = target = time_val = tx_commit = 0;
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
              prev_block = convert_input(inputs);
              inputs = "";
              break;
            case 1:
              tx_commit = convert_input(inputs);
              inputs = "";
              break;
            case 2:
              time_val = convert_input(inputs);
              inputs = "";
              break;
            case 3:
              target = convert_input(inputs);
              inputs = "";
              break;
            case 4:
              nonce = convert_input(inputs);
              
              if (nonce > 0) {
                block_hash = calculate_result(prev_block, tx_commit, target, time_val, nonce);
                /* Verifying... skip stage 6! */
                stage++;
                inputs = String(block_hash);
              } else {
                nonce_try = 0;
                block_hash = lowest = 999999;
                while (block_hash > target && nonce_try < 1000000) {
                  nonce_try++;

                  if (block_hash < lowest){
                    lowest = block_hash;
                    low_nonce = nonce_try;
                  }
                  if (nonce_try % 999 == 0 || nonce_try == 1)
                    displayNonceTry(lowest, low_nonce, nonce_try);
                  block_hash = calculate_result(prev_block, tx_commit, target, time_val, nonce_try);

                  delayMicroseconds(1500);
                  key = keypad.getKey();
                  if (key != NO_KEY) {
                    key = NO_KEY;
                    inputs = "";
                    stage = 7;
                    displayTryCanceled();

                    break;
                  }    
                  if (block_hash > target) {
                    inputs = String(0);
                  } else {
                    // display the winning nonce!
                    inputs = String(nonce_try);
                  }
                }
              }
              break;
            case 5:
              inputs = String(block_hash);
              break;
            case 6:
              inputs = "";
              break;
          }
          
          stage++;
          virtkey = "";
          
          if (stage > 6) {
            // we go back to the beginning!
            stage = 0;
          }
      }
      else if (key == '*')
      {
        if (inputs.length() > 0 && stage != 5 && stage != 6) {
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

    int textColour = TFT_YELLOW;
    if (batteryPercentage > 70)
    {
      textColour = TFT_YELLOW;
    }
    else if (batteryPercentage > 30)
    {
      textColour = TFT_YELLOW;
    }
    else
    {
      textColour = TFT_RED;
    }

    tft.setTextColor(textColour, B58_PURPLE);
    tft.setFreeFont(SMALLFONT);

    int textXPos = 195;
    if (batteryPercentage < 100)
    {
      textXPos = 200;
    }

    // Clear the area of the display where the battery level is shown
    tft.fillRect(textXPos - 2, 0, 50, 20, B58_PURPLE);
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
