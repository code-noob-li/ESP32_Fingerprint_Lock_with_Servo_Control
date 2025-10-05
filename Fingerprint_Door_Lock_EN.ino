// Pin configuration (see Serial Communication.txt)
#define FINGERPRINT_RX 16    // Fingerprint module RX (connected to GPIO16)
#define FINGERPRINT_TX 17    // Fingerprint module TX (connected to GPIO17)
#define ASRPRO_RX 3          // ASRPRO RX (connected to GPIO3)
#define ASRPRO_TX 1          // ASRPRO TX (connected to GPIO1)
#define OPEN_SERVO_PIN 13    // Door open servo signal line (connected to GPIO13)
#define CLOSE_SERVO_PIN 14   // Door close servo signal line (connected to GPIO14)
#define MATCH_BUTTON_PIN 18  // Fingerprint unlock recognition button (connected to GPIO18)
#define OPEN_BUTTON_PIN 19   // Door open servo button (connected to GPIO19)
#define CLOSE_BUTTON_PIN 21  // Door close servo button (connected to GPIO21)
#define ENROLL_BUTTON_PIN 22 // Fingerprint enrollment button (connected to GPIO22)

// Fingerprint module commands
const uint8_t CMD_WAKEUP[] = {0xF5, 0x2C, 0x00, 0x00, 0x01, 0x00, 0x2D, 0xF5};         // Wake up command
const uint8_t CMD_MATCH[] = {0xF5, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xF5};          // 1:N match command
const uint8_t CMD_CAPTURE1[] = {0xF5, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0xF5};       // First capture (3C3R)
const uint8_t CMD_CAPTURE2[] = {0xF5, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0xF5};       // Second capture (3C3R)
const uint8_t CMD_CAPTURE3[] = {0xF5, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF5};       // Third capture (3C3R)
const uint8_t CMD_GET_USER_COUNT[] = {0xF5, 0x09, 0x00, 0x00, 0x00, 0x00, 0x09, 0xF5}; // Get total user count
const uint8_t CMD_HANDSHAKE1[] = {0xF5, 0xFE, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xF5};     // Handshake command 1
const uint8_t CMD_HANDSHAKE2[] = {0xF5, 0xFD, 0x00, 0x00, 0x00, 0x00, 0xFD, 0xF5};     // Handshake command 2

// Servo objects
Servo myservo_open;  // Door open servo object
Servo myservo_close; // Door close servo object

// Button state variables
int matchButtonState = HIGH;
int lastMatchButtonState = HIGH;
int openButtonState = HIGH;
int lastOpenButtonState = HIGH;
int closeButtonState = HIGH;
int lastCloseButtonState = HIGH;
int enrollButtonState = HIGH;
int lastEnrollButtonState = HIGH;

// Button trigger flags
bool matchHasTriggered = false;
bool openHasTriggered = false;
bool closeHasTriggered = false;
bool enrollHasTriggered = false;

// Error count
int errorCount = 0;

// Serial objects
HardwareSerial fingerprintSerial(2); // Use Serial2 for fingerprint module
HardwareSerial asrproSerial(1);      // Use Serial1 for ASRPRO

void setup()
{
  // Allow servos to run on ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // Initialize serial debugging
  Serial.begin(115200);
  Serial.println("ESP32 Fingerprint Lock System Started");

  // Initialize fingerprint module serial
  fingerprintSerial.begin(115200, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);

  // Initialize ASRPRO serial
  asrproSerial.begin(115200, SERIAL_8N1, ASRPRO_RX, ASRPRO_TX);

  // Initialize button pins
  pinMode(MATCH_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OPEN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CLOSE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENROLL_BUTTON_PIN, INPUT_PULLUP);

  // Initialize servos
  myservo_open.attach(OPEN_SERVO_PIN, 544, 2400);
  myservo_close.attach(CLOSE_SERVO_PIN, 544, 2400);

  // Set initial position for servos
  myservo_open.write(0);    // Initial position for door open servo is 0 degrees
  myservo_close.write(0);   // Initial position for door close servo is 0 degrees

  Serial.println("System initialization complete");
}

// Door open function: Rotate door open servo from 0 to 180 degrees, then return to 0 degrees after delay
void myservo_open_action()
{
  Serial.println("Executing door open action");
  // asrproSerial.print("open_door"); // Send open door voice command to ASRPRO
  myservo_open.write(180);
  delay(1500);
  myservo_open.write(0);
}

// Door close function: Rotate door close servo from 0 to 180 degrees, then return to 0 degrees after delay
void myservo_close_action()
{
  Serial.println("Executing door close action");
  // asrproSerial.print("close_door"); // Send close door voice command to ASRPRO
  myservo_close.write(180);
  delay(1500);
  myservo_close.write(0);
}

// Calculate checksum
uint8_t calculateChecksum(const uint8_t *data)
{
  uint8_t checksum = 0;
  for (int i = 1; i < 6; i++)
  {
    checksum ^= data[i];
  }
  return checksum;
}

// Send command to fingerprint module
void sendCommand(const uint8_t *cmd, int len)
{
  fingerprintSerial.write(cmd, len);
}

// Read fingerprint module response
int readResponse(uint8_t *response, int maxLen, unsigned long timeout = 2000)
{
  unsigned long startTime = millis();
  int index = 0;

  while (millis() - startTime < timeout)
  {
    while (fingerprintSerial.available())
    {
      response[index++] = fingerprintSerial.read();
      if (index >= maxLen)
      {
        return index;
      }
    }
    delay(10);
  }
  return index;
}

// Handshake with fingerprint module, ensure module is in correct command response state
bool moduleHandshake()
{
  Serial.println("Handshaking with fingerprint module...");

  // Clear receive buffer
  while (fingerprintSerial.available())
  {
    fingerprintSerial.read();
    delay(10);
  }

  // Send handshake command 1 (unsupported command 0xFE)
  sendCommand(CMD_HANDSHAKE1, 8);
  delay(10);

  // Loop sending handshake command 1 until response received
  unsigned long startTime = millis();
  uint8_t response[16];
  int responseLen = 0;

  while (millis() - startTime < 4000)
  { // Timeout 4 seconds
    sendCommand(CMD_HANDSHAKE1, 8);
    delay(200);

    if (fingerprintSerial.available())
    {
      responseLen = readResponse(response, sizeof(response));
      if (responseLen >= 8)
      {
        Serial.println("Received handshake command 1 response");
        break;
      }
    }
    delay(10);
  }

  if (responseLen < 8)
  {
    Serial.println("Handshake failed: no response to handshake command 1");
    return false;
  }

  // Clear buffer
  delay(10);
  while (fingerprintSerial.available())
  {
    fingerprintSerial.read();
    delay(10);
  }

  // Send handshake command 2 (0xFD)
  sendCommand(CMD_HANDSHAKE2, 8);
  delay(10);

  // Wait for handshake command 2 response
  startTime = millis();
  while (millis() - startTime < 100)
  { // Timeout 100ms
    if (fingerprintSerial.available())
    {
      responseLen = readResponse(response, sizeof(response));
      if (responseLen >= 8 && response[1] == 0xFD)
      {
        Serial.println("Handshake successful");
        return true;
      }
    }
    delay(1);
  }

  Serial.println("Handshake completed");
  return true;
}

// Wake up fingerprint module
bool wakeupModule()
{
  Serial.println("Attempting to wake up fingerprint module...");

  moduleHandshake();

  sendCommand(CMD_WAKEUP, 8);

  uint8_t response[16];
  int len = readResponse(response, sizeof(response));
  if (len > 0)
  {
    Serial.print("Received response: ");
    for (int i = 0; i < len; i++)
    {
      if (response[i] < 0x10)
        Serial.print("0");
      Serial.print(response[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    Serial.println("Fingerprint module wakeup successful");
    return true;
  }
  else
  {
    Serial.println("Fingerprint module wakeup failed");
    return false;
  }
}

// 1:N fingerprint matching
void matchFingerprint()
{
  Serial.println("Executing fingerprint match");
  
  // Wake up fingerprint module first
  if (!wakeupModule())
  {
    Serial.println("Fingerprint module wakeup failed, unable to match");
    asrproSerial.print("error"); // Send error voice command to ASRPRO
    return;
  }
  
  // Notify ASRPRO to start matching process after wakeup
  asrproSerial.print("match"); // Send match voice command to ASRPRO

  sendCommand(CMD_MATCH, 8);
  delay(100);

  uint8_t response[16];
  int len = readResponse(response, sizeof(response));

  if (len >= 8 && response[0] == 0xF5 && response[1] == 0x0C)
  {
    // According to tm_code_hex.txt:
    // User 1 matched response: F5 0C 00 01 01 00 0C F5 (Q1=0x00, Q2=0x01, Q3=0x01)
    // User 2 matched response: F5 0C 00 02 02 00 0C F5 (Q1=0x00, Q2=0x02, Q3=0x02)
    // Condition: Q1 and Q2 not both 0 and Q3 ≠ 0x08 means match success
    if ((response[2] != 0x00 || response[3] != 0x00) && response[4] != 0x08)
    { // Match success
      Serial.println("Fingerprint match successful");
      asrproSerial.print("success"); // Send success voice command to ASRPRO
      errorCount = 0;                // Reset error count
      myservo_open_action();         // Execute door open action after match success
    }
    else
    { // Match failed
      if (errorCount >= 3)
      {
        Serial.println("Error count reached 3");
        asrproSerial.print("final_error"); // Send final error voice command to ASRPRO
      }
      else
      {
        Serial.println("Fingerprint match failed, error count less than 3");
        errorCount++;
        asrproSerial.print("error"); // Send error voice command to ASRPRO
      }
    }
  }
  else
  {
    Serial.println("No response from fingerprint module or response error");
    errorCount++;
    asrproSerial.print("error"); // Send error voice command to ASRPRO
  }
}

// 3R3C fingerprint enrollment
void enrollFingerprint()
{
  Serial.println("Executing fingerprint enrollment");
  asrproSerial.print("register"); // Send register voice command to ASRPRO

  // Wake up fingerprint module first
  if (!wakeupModule())
  {
    Serial.println("Fingerprint module wakeup failed, unable to enroll");
    asrproSerial.print("error"); // Send error voice command to ASRPRO
    return;
  }

  // First capture
  sendCommand(CMD_CAPTURE1, 8);
  delay(100);

  uint8_t response[16];
  int len = readResponse(response, sizeof(response));

  if (len >= 8 && response[0] == 0xF5 && response[1] == 0x01 && response[4] == 0x00)
  {
    // Second capture
    sendCommand(CMD_CAPTURE2, 8);
    delay(100);

    len = readResponse(response, sizeof(response));

    if (len >= 8 && response[0] == 0xF5 && response[1] == 0x02 && response[4] == 0x00)
    {
      // Third capture
      sendCommand(CMD_CAPTURE3, 8);
      delay(100);

      len = readResponse(response, sizeof(response));

      if (len >= 8 && response[0] == 0xF5 && response[1] == 0x03 && response[4] == 0x00)
      {
        Serial.println("Fingerprint enrollment successful");
        asrproSerial.print("success_tm"); // Send enrollment success voice command to ASRPRO
        errorCount = 0;                   // Reset error count
        return;
      }
    }
  }

  // Enrollment failed
  Serial.println("Fingerprint enrollment failed");
  errorCount++;
  asrproSerial.print("error"); // Send error voice command to ASRPRO

  if (errorCount >= 3)
  {
    Serial.println("Error count reached 3");
    asrproSerial.print("final_error"); // Send final error voice command to ASRPRO
    errorCount = 0;                    // Reset error count
  }
}

void loop()
{
  // Check if ASRPRO sent command
  if (asrproSerial.available())
  {
    String command = asrproSerial.readStringUntil('\n');
    command.trim(); // Remove spaces and newlines from start/end

    if (command == "open_door")
    {
      Serial.println("Received open door command from ASRPRO, triggering fingerprint match");
      matchFingerprint(); // Trigger fingerprint match instead of direct door open
    }
    else if (command == "close_door")
    {
      Serial.println("Received close door command from ASRPRO");
      myservo_close_action(); // Execute door close action
    }
  }

  // Read button states
  matchButtonState = digitalRead(MATCH_BUTTON_PIN);
  openButtonState = digitalRead(OPEN_BUTTON_PIN);
  closeButtonState = digitalRead(CLOSE_BUTTON_PIN);
  enrollButtonState = digitalRead(ENROLL_BUTTON_PIN);

  // Detect falling edge of fingerprint match button (pressed)
  if (matchButtonState == LOW && lastMatchButtonState == HIGH)
  {
    delay(50); // Software debounce
    if (digitalRead(MATCH_BUTTON_PIN) == LOW)
    {
      if (!matchHasTriggered)
      {
        matchFingerprint();
        matchHasTriggered = true;
      }
    }
  }

  // Reset trigger flag after fingerprint match button released
  if (matchButtonState == HIGH && lastMatchButtonState == LOW)
  {
    delay(50); // Software debounce
    if (digitalRead(MATCH_BUTTON_PIN) == HIGH)
    {
      matchHasTriggered = false;
    }
  }

  // Detect falling edge of open door button (pressed)
  if (openButtonState == LOW && lastOpenButtonState == HIGH)
  {
    delay(50); // Software debounce
    if (digitalRead(OPEN_BUTTON_PIN) == LOW)
    {
      if (!openHasTriggered)
      {
        myservo_open_action();
        openHasTriggered = true;
      }
    }
  }

  // Reset trigger flag after open door button released
  if (openButtonState == HIGH && lastOpenButtonState == LOW)
  {
    delay(50); // Software debounce
    if (digitalRead(OPEN_BUTTON_PIN) == HIGH)
    {
      openHasTriggered = false;
    }
  }

  // Detect falling edge of close door button (pressed)
  if (closeButtonState == LOW && lastCloseButtonState == HIGH)
  {
    delay(50); // Software debounce
    if (digitalRead(CLOSE_BUTTON_PIN) == LOW)
    {
      if (!closeHasTriggered)
      {
        myservo_close_action();
        closeHasTriggered = true;
      }
    }
  }

  // Reset trigger flag after close door button released
  if (closeButtonState == HIGH && lastCloseButtonState == LOW)
  {
    delay(50); // Software debounce
    if (digitalRead(CLOSE_BUTTON_PIN) == HIGH)
    {
      closeHasTriggered = false;
    }
  }

  // Detect falling edge of fingerprint enrollment button (pressed)
  if (enrollButtonState == LOW && lastEnrollButtonState == HIGH)
  {
    delay(50); // Software debounce
    if (digitalRead(ENROLL_BUTTON_PIN) == LOW)
    {
      if (!enrollHasTriggered)
      {
        enrollFingerprint();
        enrollHasTriggered = true;
      }
    }
  }

  // Reset trigger flag after fingerprint enrollment button released
  if (enrollButtonState == HIGH && lastEnrollButtonState == LOW)
  {
    delay(50); // Software debounce
    if (digitalRead(ENROLL_BUTTON_PIN) == HIGH)
    {
      enrollHasTriggered = false;
    }
  }

  // Save current button states for next comparison
  lastMatchButtonState = matchButtonState;
  lastOpenButtonState = openButtonState;
  lastCloseButtonState = closeButtonState;
  lastEnrollButtonState = enrollButtonState;

  delay(10); // Short delay in main loop
}
