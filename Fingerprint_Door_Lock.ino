#include <Arduino.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>

// 引脚配置 (根据串口通信.txt)
#define FINGERPRINT_RX 16    // 指纹模块RX (连接到GPIO16)
#define FINGERPRINT_TX 17    // 指纹模块TX (连接到GPIO17)
#define ASRPRO_RX 3          // ASRPRO RX (连接到GPIO3)
#define ASRPRO_TX 1          // ASRPRO TX (连接到GPIO1)
#define OPEN_SERVO_PIN 13    // 开门舵机信号线 (连接到GPIO13)
#define CLOSE_SERVO_PIN 14   // 关门舵机信号线 (连接到GPIO14)
#define MATCH_BUTTON_PIN 18  // 指纹解锁识别按钮 (连接到GPIO18)
#define OPEN_BUTTON_PIN 19   // 开门舵机工作按钮 (连接到GPIO19)
#define CLOSE_BUTTON_PIN 21  // 关门舵机工作按钮 (连接到GPIO21)
#define ENROLL_BUTTON_PIN 22 // 指纹注册按钮 (连接到GPIO22)

// 指纹模块指令
const uint8_t CMD_WAKEUP[] = {0xF5, 0x2C, 0x00, 0x00, 0x01, 0x00, 0x2D, 0xF5};         // 唤醒指令
const uint8_t CMD_MATCH[] = {0xF5, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xF5};          // 1:N匹配指令
const uint8_t CMD_CAPTURE1[] = {0xF5, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0xF5};       // 第一次采集 (3C3R)
const uint8_t CMD_CAPTURE2[] = {0xF5, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0xF5};       // 第二次采集 (3C3R)
const uint8_t CMD_CAPTURE3[] = {0xF5, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF5};       // 第三次采集 (3C3R)
const uint8_t CMD_GET_USER_COUNT[] = {0xF5, 0x09, 0x00, 0x00, 0x00, 0x00, 0x09, 0xF5}; // 获取用户总数
const uint8_t CMD_HANDSHAKE1[] = {0xF5, 0xFE, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xF5};     // 握手命令1
const uint8_t CMD_HANDSHAKE2[] = {0xF5, 0xFD, 0x00, 0x00, 0x00, 0x00, 0xFD, 0xF5};     // 握手命令2

// 舵机对象
Servo myservo_open;  // 开门舵机对象
Servo myservo_close; // 关门舵机对象

// 按钮状态变量
int matchButtonState = HIGH;
int lastMatchButtonState = HIGH;
int openButtonState = HIGH;
int lastOpenButtonState = HIGH;
int closeButtonState = HIGH;
int lastCloseButtonState = HIGH;
int enrollButtonState = HIGH;
int lastEnrollButtonState = HIGH;

// 按钮触发标记
bool matchHasTriggered = false;
bool openHasTriggered = false;
bool closeHasTriggered = false;
bool enrollHasTriggered = false;

// 错误计数
int errorCount = 0;

// 串口对象
HardwareSerial fingerprintSerial(2); // 使用串口2连接指纹模块
HardwareSerial asrproSerial(1);      // 使用串口1连接ASRPRO

void setup()
{
  // 允许舵机在ESP32上运行
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // 初始化串口调试
  Serial.begin(115200);
  Serial.println("ESP32指纹锁系统启动");

  // 初始化指纹模块串口
  fingerprintSerial.begin(115200, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);

  // 初始化ASRPRO串口
  asrproSerial.begin(115200, SERIAL_8N1, ASRPRO_RX, ASRPRO_TX);

  // 初始化按钮引脚
  pinMode(MATCH_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OPEN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CLOSE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENROLL_BUTTON_PIN, INPUT_PULLUP);

  // 初始化舵机
  myservo_open.attach(OPEN_SERVO_PIN, 544, 2400);
  myservo_close.attach(CLOSE_SERVO_PIN, 544, 2400);

  // 设置舵机初始位置
  myservo_open.write(0);    // 开门舵机初始位置为0度
  myservo_close.write(0); // 关门舵机初始位置为0度

  Serial.println("系统初始化完成");
}

// 开门函数：将开门舵机从0度转到180度，延时后回到0度
void myservo_open_action()
{
  Serial.println("执行开门动作");
  // asrproSerial.print("open_door"); // 向ASRPRO发送开门语音指令
  myservo_open.write(180);
  delay(1500);
  myservo_open.write(0);
}

// 关门函数：将关门舵机从0度转到180度，延时后回到0度
void myservo_close_action()
{
  Serial.println("执行关门动作");
  // asrproSerial.print("close_door"); // 向ASRPRO发送关门语音指令
  myservo_close.write(180);
  delay(1500);
  myservo_close.write(0);
}

// 计算校验和
uint8_t calculateChecksum(const uint8_t *data)
{
  uint8_t checksum = 0;
  for (int i = 1; i < 6; i++)
  {
    checksum ^= data[i];
  }
  return checksum;
}

// 发送命令到指纹模块
void sendCommand(const uint8_t *cmd, int len)
{
  fingerprintSerial.write(cmd, len);
}

// 读取指纹模块响应
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

// 与指纹模块握手，确保模块处于正确的命令响应状态
bool moduleHandshake()
{
  Serial.println("与指纹模块握手...");

  // 清空接收缓冲区
  while (fingerprintSerial.available())
  {
    fingerprintSerial.read();
    delay(10);
  }

  // 发送握手命令1 (不支持的命令0xFE)
  sendCommand(CMD_HANDSHAKE1, 8);
  delay(10);

  // 循环发送握手命令1直到收到响应
  unsigned long startTime = millis();
  uint8_t response[16];
  int responseLen = 0;

  while (millis() - startTime < 4000)
  { // 超时4秒
    sendCommand(CMD_HANDSHAKE1, 8);
    delay(200);

    if (fingerprintSerial.available())
    {
      responseLen = readResponse(response, sizeof(response));
      if (responseLen >= 8)
      {
        Serial.println("收到握手命令1响应");
        break;
      }
    }
    delay(10);
  }

  if (responseLen < 8)
  {
    Serial.println("握手失败：未收到握手命令1响应");
    return false;
  }

  // 清空缓冲区
  delay(10);
  while (fingerprintSerial.available())
  {
    fingerprintSerial.read();
    delay(10);
  }

  // 发送握手命令2 (0xFD)
  sendCommand(CMD_HANDSHAKE2, 8);
  delay(10);

  // 等待握手命令2的响应
  startTime = millis();
  while (millis() - startTime < 100)
  { // 超时100ms
    if (fingerprintSerial.available())
    {
      responseLen = readResponse(response, sizeof(response));
      if (responseLen >= 8 && response[1] == 0xFD)
      {
        Serial.println("握手成功");
        return true;
      }
    }
    delay(1);
  }

  Serial.println("握手完成");
  return true;
}

// 唤醒指纹模块
bool wakeupModule()
{
  Serial.println("尝试唤醒指纹模块...");

  moduleHandshake();

  sendCommand(CMD_WAKEUP, 8);

  uint8_t response[16];
  int len = readResponse(response, sizeof(response));
  if (len > 0)
  {
    Serial.print("收到响应: ");
    for (int i = 0; i < len; i++)
    {
      if (response[i] < 0x10)
        Serial.print("0");
      Serial.print(response[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    Serial.println("指纹模块唤醒成功");
    return true;
  }
  else
  {
    Serial.println("指纹模块唤醒失败");
    return false;
  }
}

// 1:N匹配指纹
void matchFingerprint()
{
  Serial.println("执行指纹匹配");
  
  // 先唤醒指纹模块
  if (!wakeupModule())
  {
    Serial.println("指纹模块唤醒失败，无法进行匹配");
    asrproSerial.print("error"); // 向ASRPRO发送错误语音指令
    return;
  }
  
  // 唤醒成功后通知ASRPRO开始匹配过程
  asrproSerial.print("match"); // 向ASRPRO发送匹配语音指令

  sendCommand(CMD_MATCH, 8);
  delay(100);

  uint8_t response[16];
  int len = readResponse(response, sizeof(response));

  if (len >= 8 && response[0] == 0xF5 && response[1] == 0x0C)
  {
    // 根据tm_code_hex.txt文档:
    // 成功匹配用户1的响应: F5 0C 00 01 01 00 0C F5 (Q1=0x00, Q2=0x01, Q3=0x01)
    // 成功匹配用户2的响应: F5 0C 00 02 02 00 0C F5 (Q1=0x00, Q2=0x02, Q3=0x02)
    // 判断条件: Q1和Q2不全为0 且 Q3 ≠ 0x08 表示匹配成功
    if ((response[2] != 0x00 || response[3] != 0x00) && response[4] != 0x08)
    { // 匹配成功
      Serial.println("指纹匹配成功");
      asrproSerial.print("success"); // 向ASRPRO发送成功语音指令
      errorCount = 0;                  // 重置错误计数
      myservo_open_action();           // 匹配成功后执行开门动作
    }
    else
    { // 匹配失败
      if (errorCount >= 3)
      {
        Serial.println("错误次数达到3次");
        asrproSerial.print("final_error"); // 向ASRPRO发送最终错误语音指令
      }
      else
      {
        Serial.println("指纹匹配失败,次数未达到3次");
        errorCount++;
        asrproSerial.print("error"); // 向ASRPRO发送错误语音指令
      }
    }
  }
  else
  {
    Serial.println("指纹模块无响应或响应错误");
    errorCount++;
    asrproSerial.print("error"); // 向ASRPRO发送错误语音指令
  }
}

// 3R3C指纹注册
void enrollFingerprint()
{
  Serial.println("执行指纹注册");
  asrproSerial.print("register"); // 向ASRPRO发送注册语音指令

  // 先唤醒指纹模块
  if (!wakeupModule())
  {
    Serial.println("指纹模块唤醒失败，无法进行注册");
    asrproSerial.print("error"); // 向ASRPRO发送错误语音指令
    return;
  }

  // 第一次采集
  sendCommand(CMD_CAPTURE1, 8);
  delay(100);

  uint8_t response[16];
  int len = readResponse(response, sizeof(response));

  if (len >= 8 && response[0] == 0xF5 && response[1] == 0x01 && response[4] == 0x00)
  {
    // 第二次采集
    sendCommand(CMD_CAPTURE2, 8);
    delay(100);

    len = readResponse(response, sizeof(response));

    if (len >= 8 && response[0] == 0xF5 && response[1] == 0x02 && response[4] == 0x00)
    {
      // 第三次采集
      sendCommand(CMD_CAPTURE3, 8);
      delay(100);

      len = readResponse(response, sizeof(response));

      if (len >= 8 && response[0] == 0xF5 && response[1] == 0x03 && response[4] == 0x00)
      {
        Serial.println("指纹注册成功");
        asrproSerial.print("success_tm"); // 向ASRPRO发送成功语音指令
        errorCount = 0;                     // 重置错误计数
        return;
      }
    }
  }

  // 注册失败
  Serial.println("指纹注册失败");
  errorCount++;
  asrproSerial.print("error"); // 向ASRPRO发送错误语音指令

  if (errorCount >= 3)
  {
    Serial.println("错误次数达到3次");
    asrproSerial.print("final_error"); // 向ASRPRO发送最终错误语音指令
    errorCount = 0;                      // 重置错误计数
  }
}

void loop()
{
  // 检查ASRPRO是否有发送命令
  if (asrproSerial.available())
  {
    String command = asrproSerial.readStringUntil('\n');
    command.trim(); // 去除首尾空格和换行符

    if (command == "open_door")
    {
      Serial.println("收到来自ASRPRO的开门命令，触发指纹匹配");
      matchFingerprint(); // 触发指纹匹配而不是直接执行开门动作
    }
    else if (command == "close_door")
    {
      Serial.println("收到来自ASRPRO的关门命令");
      myservo_close_action(); // 执行关门动作
    }
  }

  // 读取按钮状态
  matchButtonState = digitalRead(MATCH_BUTTON_PIN);
  openButtonState = digitalRead(OPEN_BUTTON_PIN);
  closeButtonState = digitalRead(CLOSE_BUTTON_PIN);
  enrollButtonState = digitalRead(ENROLL_BUTTON_PIN);

  // 检测指纹匹配按钮下降沿（按下）
  if (matchButtonState == LOW && lastMatchButtonState == HIGH)
  {
    delay(50); // 软件消抖
    if (digitalRead(MATCH_BUTTON_PIN) == LOW)
    {
      if (!matchHasTriggered)
      {
        matchFingerprint();
        matchHasTriggered = true;
      }
    }
  }

  // 指纹匹配按钮释放后重置触发标记
  if (matchButtonState == HIGH && lastMatchButtonState == LOW)
  {
    delay(50); // 软件消抖
    if (digitalRead(MATCH_BUTTON_PIN) == HIGH)
    {
      matchHasTriggered = false;
    }
  }

  // 检测开门按钮下降沿（按下）
  if (openButtonState == LOW && lastOpenButtonState == HIGH)
  {
    delay(50); // 软件消抖
    if (digitalRead(OPEN_BUTTON_PIN) == LOW)
    {
      if (!openHasTriggered)
      {
        myservo_open_action();
        openHasTriggered = true;
      }
    }
  }

  // 开门按钮释放后重置触发标记
  if (openButtonState == HIGH && lastOpenButtonState == LOW)
  {
    delay(50); // 软件消抖
    if (digitalRead(OPEN_BUTTON_PIN) == HIGH)
    {
      openHasTriggered = false;
    }
  }

  // 检测关门按钮下降沿（按下）
  if (closeButtonState == LOW && lastCloseButtonState == HIGH)
  {
    delay(50); // 软件消抖
    if (digitalRead(CLOSE_BUTTON_PIN) == LOW)
    {
      if (!closeHasTriggered)
      {
        myservo_close_action();
        closeHasTriggered = true;
      }
    }
  }

  // 关门按钮释放后重置触发标记
  if (closeButtonState == HIGH && lastCloseButtonState == LOW)
  {
    delay(50); // 软件消抖
    if (digitalRead(CLOSE_BUTTON_PIN) == HIGH)
    {
      closeHasTriggered = false;
    }
  }

  // 检测指纹注册按钮下降沿（按下）
  if (enrollButtonState == LOW && lastEnrollButtonState == HIGH)
  {
    delay(50); // 软件消抖
    if (digitalRead(ENROLL_BUTTON_PIN) == LOW)
    {
      if (!enrollHasTriggered)
      {
        enrollFingerprint();
        enrollHasTriggered = true;
      }
    }
  }

  // 指纹注册按钮释放后重置触发标记
  if (enrollButtonState == HIGH && lastEnrollButtonState == LOW)
  {
    delay(50); // 软件消抖
    if (digitalRead(ENROLL_BUTTON_PIN) == HIGH)
    {
      enrollHasTriggered = false;
    }
  }

  // 保存当前按钮状态供下次比较
  lastMatchButtonState = matchButtonState;
  lastOpenButtonState = openButtonState;
  lastCloseButtonState = closeButtonState;
  lastEnrollButtonState = enrollButtonState;

  delay(10); // 主循环短延时
}