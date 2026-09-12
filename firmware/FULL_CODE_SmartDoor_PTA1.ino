// PUBLIC REPOSITORY VERSION
// Secrets are replaced with placeholders. Configure your local private copy before flashing.

// =====================================================
// PTA SMART DOOR
// ESP32 + BLYNK + TELEGRAM
// FINAL POLISH / EXHIBITION BUILD
// =====================================================
//
// BASE:
//   HIGH PERFORMANCE / LOW LATENCY V2.2
//
// FIXED:
//   RC522 startup / polling / recovery
//   AS608 delayed UART startup
//   AS608 UART re-sync recovery
//   AS608 controlled handshake
//   No infinite initialization loop
//   No sensor failure ESP32 restart
//   Reduced RFID polling noise
//   Reduced fingerprint polling noise
//   Hardware-first main loop
//   Cooperative timing / reduced background polling overhead
//   Reduced network task wakeups
//   Controlled sensor health checks
//
// PRESERVED:
//   RFID authentication
//   Fingerprint authentication
//   RFID enrollment
//   Fingerprint enrollment 
//   Blynk
//   Telegram
//   Preferences user database
//   Local authentication
//   Touch exit
//   Relay / solenoid logic
//   LCD
//   LEDs
//   Buzzer
//   Access logs
//   5-second auto-lock
// =====================================================


// =====================================================
// BLYNK CONFIG
// =====================================================

#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "PTA Smart Door"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"


// =====================================================
// TELEGRAM CONFIG
// =====================================================

#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define TELEGRAM_CHAT_ID "YOUR_TELEGRAM_CHAT_ID"


// =====================================================
// LIBRARIES
// =====================================================

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"


// =====================================================
// WIFI
// =====================================================

// =====================================================
// MULTIPLE WIFI CONFIG
// =====================================================
// Add up to 5 WiFi networks here.
// Leave unused entries as empty strings.
// The ESP32 will try them in order and automatically
// move to the next network if the current one fails.
// =====================================================

const uint8_t WIFI_NETWORK_COUNT = 5;

const char* wifiSSIDs[WIFI_NETWORK_COUNT] =
{
  "YOUR_WIFI_SSID_1",
  "",
  "",
  "",
  ""
};

const char* wifiPasswords[WIFI_NETWORK_COUNT] =
{
  "YOUR_WIFI_PASSWORD_1",
  "",
  "",
  "",
  ""
};

uint8_t currentWiFiIndex = 0;
unsigned long wifiAttemptStarted = 0;
const unsigned long WIFI_NETWORK_TIMEOUT = 10000;


// =====================================================
// TIME
// =====================================================

const char* ntpServer = "pool.ntp.org";

const long gmtOffset_sec = 8 * 3600;

const int daylightOffset_sec = 0;


// =====================================================
// LCD
// =====================================================

LiquidCrystal_I2C lcd(
  0x27,
  16,
  2
);


// =====================================================
// RFID
// =====================================================

#define SS_PIN 5
#define RST_PIN 2

#define RFID_SCK_PIN  18
#define RFID_MISO_PIN 19
#define RFID_MOSI_PIN 23

MFRC522 rfid(
  SS_PIN,
  RST_PIN
);


// =====================================================
// FINGERPRINT
// =====================================================

#define FP_RX 16
#define FP_TX 17

HardwareSerial mySerial(2);

Adafruit_Fingerprint finger =
  Adafruit_Fingerprint(&mySerial);

bool fingerprintSensorOK = false;


// =====================================================
// OUTPUT
// =====================================================

#define GREEN_LED 25
#define RED_LED 33
#define BUZZER 32
#define RELAY_PIN 26
#define TOUCH_PIN 27


// =====================================================
// TELEGRAM
// =====================================================

WiFiClientSecure telegramClient;

UniversalTelegramBot bot(
  BOT_TOKEN,
  telegramClient
);


// =====================================================
// USER DATABASE
// =====================================================

Preferences preferences;

#define MAX_USERS 50
#define MAX_NAME_LEN 32
#define MAX_RFID_LEN 32

struct User
{
  String name;
  String rfidUID;
  int fingerprintID;
  bool active;
  bool exists;
};

User users[MAX_USERS];


// =====================================================
// USER SNAPSHOT
// =====================================================

struct UserSnapshot
{
  bool exists;
  bool active;
  int fingerprintID;

  char name[MAX_NAME_LEN + 1];

  char rfidUID[MAX_RFID_LEN + 1];
};


// =====================================================
// SELECTED USER
// =====================================================

String selectedName = "";

volatile int selectedUserID = 1;


// =====================================================
// ENROLLMENT
// =====================================================

volatile bool rfidEnrollmentMode = false;

volatile bool fingerprintEnrollmentMode = false;

volatile int enrollingFingerprintID = -1;


// =====================================================
// ACCESS COUNT
// =====================================================

int accessCount = 0;


// =====================================================
// ACCESS LOG
// =====================================================

#define MAX_LOGS 10

String accessLogs[MAX_LOGS];

int accessLogCount = 0;


// =====================================================
// DOOR STATE
// =====================================================

volatile bool doorUnlocked = false;

unsigned long doorUnlockStart = 0;

const unsigned long DOOR_UNLOCK_TIME = 5000;


// =====================================================
// LCD STATE
// =====================================================

char lcdLine1[17] = "";

char lcdLine2[17] = "";


// =====================================================
// TEMPORARY LCD
// =====================================================

unsigned long messageTimer = 0;

bool temporaryLCDMessage = false;

// Short confirmation so the LCD returns to READY without feeling stuck.
const unsigned long LCD_MESSAGE_TIME = 2500;


// =====================================================
// BUZZER STATE
// =====================================================

bool buzzerActive = false;

unsigned long buzzerStart = 0;

const unsigned long BUZZER_TIME = 200;


// =====================================================
// DENIED STATE
// =====================================================

bool deniedActive = false;

int deniedStep = 0;

unsigned long deniedTimer = 0;


// =====================================================
// TOUCH DEBOUNCE
// =====================================================

bool previousTouchState = LOW;

unsigned long lastTouchTime = 0;

const unsigned long TOUCH_DEBOUNCE = 300;


// =====================================================
// FINGERPRINT SCAN
// =====================================================

unsigned long fingerprintLastScan = 0;

const unsigned long FINGERPRINT_SCAN_INTERVAL = 65;


// =====================================================
// FINGERPRINT STATE
// =====================================================

enum FingerprintState
{
  FP_READY,
  FP_DETECTING,
  FP_PROCESSING,
  FP_WAIT_REMOVE
};

FingerprintState fingerprintState =
  FP_READY;


// =====================================================
// FINGERPRINT PROCESSING
// =====================================================

unsigned long fingerprintProcessingStart = 0;

const unsigned long FINGERPRINT_PROCESS_TIME = 1500;


// =====================================================
// FINGERPRINT ERROR
// =====================================================

unsigned long fingerprintLastError = 0;

const unsigned long FINGERPRINT_ERROR_PRINT_INTERVAL = 1500;


// =====================================================
// FINGERPRINT STARTUP / RECOVERY
// =====================================================

enum FingerprintInitState
{
  FP_INIT_WAIT,
  FP_INIT_READY,
  FP_INIT_RETRY
};

FingerprintInitState fingerprintInitState =
  FP_INIT_WAIT;

unsigned long fingerprintInitTimer = 0;

unsigned long fingerprintLastInitAttempt = 0;

const unsigned long FINGERPRINT_STARTUP_DELAY = 3500;

const unsigned long FINGERPRINT_RETRY_INTERVAL = 4000;

const unsigned long FINGERPRINT_LONG_RECOVERY_INTERVAL = 12000;

const uint8_t FINGERPRINT_MAX_STARTUP_RETRIES = 8;

uint8_t fingerprintStartupRetries = 0;

bool fingerprintInitMessageShown = false;


// =====================================================
// FINGERPRINT UART RECOVERY
// =====================================================

unsigned long fingerprintLastUARTRecovery = 0;

const unsigned long FINGERPRINT_UART_RECOVERY_INTERVAL = 15000;


// =====================================================
// RFID DUPLICATE PROTECTION
// =====================================================

char lastRFIDUID[MAX_RFID_LEN + 1] = "";

unsigned long lastRFIDProcessed = 0;

const unsigned long RFID_REPEAT_BLOCK_TIME = 1200;


// =====================================================
// RFID STATE
// =====================================================

enum RFIDState
{
  RFID_READY,
  RFID_CARD_PRESENT,
  RFID_PROCESSING,
  RFID_WAIT_REMOVE
};

RFIDState rfidState =
  RFID_READY;


// =====================================================
// RFID POLLING
// =====================================================

unsigned long rfidLastScan = 0;

const unsigned long RFID_SCAN_INTERVAL = 35;


// =====================================================
// RFID RECOVERY
// =====================================================

unsigned long rfidLastRecovery = 0;

const unsigned long RFID_RECOVERY_INTERVAL = 6000;


// =====================================================
// RFID HEALTH
// =====================================================

unsigned long rfidLastHealthCheck = 0;

const unsigned long RFID_HEALTH_INTERVAL = 7000;


// =====================================================
// RFID STATUS
// =====================================================

bool rfidReaderOK = false;

char currentRFIDUID[MAX_RFID_LEN + 1] = "";


// =====================================================
// RFID RETRY
// =====================================================

const uint8_t RFID_READ_RETRIES = 2;


// =====================================================
// RFID FAILURE CONTROL
// =====================================================

uint8_t rfidRecoveryFailures = 0;

const uint8_t RFID_MAX_SILENT_FAILURES = 3;

bool rfidFailureMessageShown = false;


// =====================================================
// FINGERPRINT ENROLLMENT STATE
// =====================================================

enum FingerEnrollState
{
  FP_IDLE,
  FP_WAIT_FIRST,
  FP_REMOVE_FIRST,
  FP_WAIT_SECOND
};

volatile FingerEnrollState fpEnrollState =
  FP_IDLE;

unsigned long fpEnrollTimer = 0;

// Total enrollment timeout shared by the mutually-exclusive
// fingerprint/RFID enrollment modes.
const unsigned long ENROLLMENT_TIMEOUT = 30000UL;

unsigned long enrollmentStartTime = 0;

bool enrollmentTimerActive = false;


// =====================================================
// RFID ENROLLMENT MESSAGE
// =====================================================

bool rfidEnrollMessageShown = false;


// =====================================================
// DATA MUTEX
// =====================================================

SemaphoreHandle_t dataMutex = nullptr;


// =====================================================
// ACCESS EVENT
// =====================================================

#define EVENT_QUEUE_LENGTH 12
#define METHOD_LEN 16
#define RESULT_LEN 8
#define REASON_LEN 32
#define TIME_LEN 24
#define CREDENTIAL_LEN 40

struct AccessEvent
{
  char user[MAX_NAME_LEN + 1];

  char method[METHOD_LEN + 1];

  char result[RESULT_LEN + 1];

  char reason[REASON_LEN + 1];

  char timestamp[TIME_LEN + 1];

  char credential[CREDENTIAL_LEN + 1];
};


// =====================================================
// BLYNK EVENT
// =====================================================

enum BlynkEventType
{
  BLYNK_EVENT_ACCESS = 1,
  BLYNK_EVENT_DOOR = 2,
  BLYNK_EVENT_STATUS = 3,
  BLYNK_EVENT_SHOW_USER = 4
};

struct BlynkEvent
{
  uint8_t type;

  bool doorState;

  int userIndex;

  AccessEvent access;

  char text[64];
};


// =====================================================
// TELEGRAM EVENT
// =====================================================

enum TelegramEventType
{
  TELEGRAM_EVENT_ACCESS = 1,
  TELEGRAM_EVENT_USER_ADDED = 2,
  TELEGRAM_EVENT_USER_DELETED = 3,
  TELEGRAM_EVENT_USER_BLOCKED = 4,
  TELEGRAM_EVENT_USER_UNBLOCKED = 5
};

struct UserActionEvent
{
  char name[MAX_NAME_LEN + 1];

  int userID;

  char timestamp[TIME_LEN + 1];
};

struct TelegramEvent
{
  uint8_t type;

  AccessEvent access;

  UserActionEvent userAction;
};


// =====================================================
// HARDWARE COMMAND
// =====================================================

enum HardwareCommandType
{
  HW_COMMAND_UNLOCK = 1,
  HW_COMMAND_LOCK = 2,
  HW_COMMAND_LCD = 3
};

struct HardwareCommand
{
  uint8_t type;

  char user[MAX_NAME_LEN + 1];

  char method[METHOD_LEN + 1];

  char lcdLine1[17];

  char lcdLine2[17];

  unsigned long lcdDuration;
};


// =====================================================
// STATIC QUEUE MEMORY
// =====================================================

QueueHandle_t blynkEventQueue = nullptr;

QueueHandle_t telegramEventQueue = nullptr;

QueueHandle_t hardwareCommandQueue = nullptr;

StaticQueue_t blynkQueueStruct;

StaticQueue_t telegramQueueStruct;

StaticQueue_t hardwareQueueStruct;

uint8_t blynkQueueStorage[
  EVENT_QUEUE_LENGTH * sizeof(BlynkEvent)
];

uint8_t telegramQueueStorage[
  EVENT_QUEUE_LENGTH * sizeof(TelegramEvent)
];

uint8_t hardwareQueueStorage[
  12 * sizeof(HardwareCommand)
];


// =====================================================
// TASK HANDLES
// =====================================================

TaskHandle_t blynkTaskHandle = nullptr;

TaskHandle_t telegramTaskHandle = nullptr;


// =====================================================
// NETWORK TIMERS
// =====================================================

unsigned long lastBlynkConnectAttempt = 0;

unsigned long lastWiFiReconnectAttempt = 0;

unsigned long lastTelegramCheck = 0;

const unsigned long BLYNK_RECONNECT_INTERVAL = 12000;

const unsigned long WIFI_RECONNECT_INTERVAL = 12000;

// Poll Telegram frequently so commands do not wait up to 10 seconds.
const unsigned long TELEGRAM_CHECK_INTERVAL = 1000;


// =====================================================
// NETWORK STATE
// =====================================================

volatile bool telegramOnlineSent = false;

// Manual WiFi selection from Blynk V9.
// 0 = automatic, 1..5 = configured WiFi slot.
volatile bool wifiManualSelection = false;
volatile uint8_t wifiManualIndex = 0;


// =====================================================
// FUNCTION PROTOTYPES
// =====================================================

void copyText(
  char* destination,
  size_t destinationSize,
  const char* source
);

bool lockData();

void unlockData();

void normalizeLCDLine(
  char* destination,
  const char* source
);

void lcdShow(
  const String& line1,
  const String& line2 = ""
);

void showReadyScreen();

void queueLCDMessage(
  const char* line1,
  const char* line2,
  unsigned long duration
);

String getCurrentTime();

void saveUserLocked(int index);

void loadUser(int index);

void loadAllUsers();

int getRegisteredUserCountLocked();

int getRegisteredUserCount();

bool getUserSnapshot(
  int index,
  UserSnapshot& snapshot
);

void copyAllUserSnapshots(
  UserSnapshot* snapshots
);

void updateRegisteredUserCount();

void showUser(int index);

void getRFIDUID(
  char* output,
  size_t outputSize
);

const char* getRFIDPICCTypeName();

void finishRFID();

bool readRFIDCardRobust();

bool initializeRFID();

void recoverRFID();

void updateRFIDHealth();

void checkRFID();

void checkRFIDEnrollment();

void cancelEnrollment(
  const char* reason,
  bool showStatus = true
);

void updateEnrollmentTimeout();

int findRFIDOwner(
  const char* uid
);

int findFingerprintOwner(
  int fingerprintID
);

bool getUserAccessInfo(
  int index,
  char* name,
  size_t nameSize,
  bool& active
);

void addAccessLogLocal(
  const AccessEvent& event
);

String buildAccessLogDisplay();

void prepareAccessEvent(
  AccessEvent& event,
  const char* user,
  const char* method,
  const char* result,
  const char* reason,
  const char* credential = ""
);

void queueAccessEvent(
  const AccessEvent& event
);

void queueBlynkStatus(
  const char* text
);

void queueBlynkShowUser(
  int index
);

void queueDoorState(
  bool unlocked
);

void queueTelegramUserEvent(
  uint8_t eventType,
  const char* name,
  int userID
);

void startBuzzer();

void updateBuzzer();

void grantAccess(
  const char* userName,
  const char* method,
  const char* credential = ""
);

void denyAccess(
  const char* userName,
  const char* method,
  const char* reason,
  const char* credential = ""
);

void updateDenied();

void updateLCD();

bool fingerprintIDUsed(
  int id
);

int findAvailableFingerprintID();

void startFingerprintEnrollment();

void checkFingerprintEnrollment();

void checkFingerprint();

void checkTouch();

void processHardwareCommands();

void updateDoor();

String htmlEscape(
  const char* text
);

String getTelegramUsers();

String getTelegramLogs();

String getTelegramStatus();

void sendTelegram(
  const String& message
);

void telegramAccessNotification(
  const AccessEvent& event
);

void telegramUserNotification(
  const TelegramEvent& event
);

void handleTelegramMessages(
  int numNewMessages
);

void processBlynkEvent(
  const BlynkEvent& event
);

void blynkTask(
  void* parameter
);

void telegramTask(
  void* parameter
);


// =====================================================
// STARTUP / RECOVERY FUNCTIONS
// =====================================================

void startFingerprintInitialization();

void updateFingerprintInitialization();

bool performFingerprintHandshake();

bool recoverFingerprintUART();

void resetRFIDHardware();

void startWiFiConnection(uint8_t index);

void tryNextWiFiNetwork();

bool hasConfiguredWiFi(uint8_t index);

bool readRFIDVersion(
  byte& version
);


// =====================================================
// TEXT HELPER
// =====================================================

void copyText(
  char* destination,
  size_t destinationSize,
  const char* source
)
{
  if (
    destination == nullptr ||
    destinationSize == 0
  )
  {
    return;
  }

  if (
    source == nullptr
  )
  {
    destination[0] = '\0';
    return;
  }

  strncpy(
    destination,
    source,
    destinationSize - 1
  );

  destination[destinationSize - 1] =
    '\0';
}


// =====================================================
// MUTEX
// =====================================================

bool lockData()
{
  if (
    dataMutex == nullptr
  )
  {
    return false;
  }

  return (
    xSemaphoreTake(
      dataMutex,
      pdMS_TO_TICKS(50)
    ) == pdTRUE
  );
}

void unlockData()
{
  if (
    dataMutex != nullptr
  )
  {
    xSemaphoreGive(
      dataMutex
    );
  }
}


// =====================================================
// LCD NORMALIZE
// =====================================================

void normalizeLCDLine(
  char* destination,
  const char* source
)
{
  for (
    int i = 0;
    i < 16;
    i++
  )
  {
    destination[i] = ' ';
  }

  destination[16] = '\0';

  if (
    source == nullptr
  )
  {
    return;
  }

  for (
    int i = 0;
    i < 16 &&
    source[i] != '\0';
    i++
  )
  {
    destination[i] = source[i];
  }
}


// =====================================================
// LCD SHOW
// =====================================================

void lcdShow(
  const String& line1,
  const String& line2
)
{
  char newLine1[17];

  char newLine2[17];

  normalizeLCDLine(
    newLine1,
    line1.c_str()
  );

  normalizeLCDLine(
    newLine2,
    line2.c_str()
  );

  if (
    strcmp(
      lcdLine1,
      newLine1
    ) == 0 &&
    strcmp(
      lcdLine2,
      newLine2
    ) == 0
  )
  {
    return;
  }

  strcpy(
    lcdLine1,
    newLine1
  );

  strcpy(
    lcdLine2,
    newLine2
  );

  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    lcdLine1
  );

  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    lcdLine2
  );
}


// =====================================================
// READY SCREEN
// =====================================================

void showReadyScreen()
{
  lcdShow(
    "SYSTEM READY",
    "SCAN RFID/FINGER"
  );
}


// =====================================================
// LCD QUEUE
// =====================================================

void queueLCDMessage(
  const char* line1,
  const char* line2,
  unsigned long duration
)
{
  if (
    hardwareCommandQueue == nullptr
  )
  {
    return;
  }

  HardwareCommand command;

  memset(
    &command,
    0,
    sizeof(command)
  );

  command.type =
    HW_COMMAND_LCD;

  copyText(
    command.lcdLine1,
    sizeof(command.lcdLine1),
    line1
  );

  copyText(
    command.lcdLine2,
    sizeof(command.lcdLine2),
    line2
  );

  command.lcdDuration =
    duration;

  xQueueSend(
    hardwareCommandQueue,
    &command,
    0
  );
}


// =====================================================
// CURRENT TIME
// =====================================================

String getCurrentTime()
{
  time_t now =
    time(nullptr);

  if (
    now < 1700000000
  )
  {
    return "Time unavailable";
  }

  struct tm timeinfo;

  localtime_r(
    &now,
    &timeinfo
  );

  char buffer[
    TIME_LEN + 1
  ];

  strftime(
    buffer,
    sizeof(buffer),
    "%d/%m/%Y %H:%M:%S",
    &timeinfo
  );

  return String(
    buffer
  );
}


// =====================================================
// SAVE USER
// =====================================================

void saveUserLocked(
  int index
)
{
  if (
    index < 0 ||
    index >= MAX_USERS
  )
  {
    return;
  }

  char prefix[8];

  snprintf(
    prefix,
    sizeof(prefix),
    "u%d",
    index
  );

  char key[24];

  // A deleted slot must not retain any old persistent data.
  if (
    !users[index].exists
  )
  {
    snprintf(
      key,
      sizeof(key),
      "%s_name",
      prefix
    );
    preferences.remove(key);

    snprintf(
      key,
      sizeof(key),
      "%s_rfid",
      prefix
    );
    preferences.remove(key);

    snprintf(
      key,
      sizeof(key),
      "%s_fp",
      prefix
    );
    preferences.remove(key);

    snprintf(
      key,
      sizeof(key),
      "%s_active",
      prefix
    );
    preferences.remove(key);

    snprintf(
      key,
      sizeof(key),
      "%s_exists",
      prefix
    );
    preferences.putBool(
      key,
      false
    );

    if (
      preferences.getBool(
        key,
        true
      )
    )
    {
      Serial.print(
        "[DATABASE] ERROR: failed to persist deleted slot "
      );
      Serial.println(index);
    }

    return;
  }

  snprintf(
    key,
    sizeof(key),
    "%s_exists",
    prefix
  );

  preferences.putBool(
    key,
    users[index].exists
  );

  snprintf(
    key,
    sizeof(key),
    "%s_name",
    prefix
  );

  preferences.putString(
    key,
    users[index].name
  );

  snprintf(
    key,
    sizeof(key),
    "%s_rfid",
    prefix
  );

  preferences.putString(
    key,
    users[index].rfidUID
  );

  snprintf(
    key,
    sizeof(key),
    "%s_fp",
    prefix
  );

  preferences.putInt(
    key,
    users[index].fingerprintID
  );

  snprintf(
    key,
    sizeof(key),
    "%s_active",
    prefix
  );

  preferences.putBool(
    key,
    users[index].active
  );
}


// =====================================================
// LOAD USER
// =====================================================

void loadUser(
  int index
)
{
  if (
    index < 0 ||
    index >= MAX_USERS
  )
  {
    return;
  }

  char prefix[8];

  snprintf(
    prefix,
    sizeof(prefix),
    "u%d",
    index
  );

  char key[24];

  snprintf(
    key,
    sizeof(key),
    "%s_exists",
    prefix
  );

  users[index].exists =
    preferences.getBool(
      key,
      false
    );

  if (
    !users[index].exists
  )
  {
    users[index].name = "";
    users[index].rfidUID = "";
    users[index].fingerprintID = -1;
    users[index].active = false;

    users[index].name.reserve(
      MAX_NAME_LEN
    );

    users[index].rfidUID.reserve(
      MAX_RFID_LEN
    );

    return;
  }

  snprintf(
    key,
    sizeof(key),
    "%s_name",
    prefix
  );

  users[index].name =
    preferences.getString(
      key,
      ""
    );

  snprintf(
    key,
    sizeof(key),
    "%s_rfid",
    prefix
  );

  users[index].rfidUID =
    preferences.getString(
      key,
      ""
    );

  snprintf(
    key,
    sizeof(key),
    "%s_fp",
    prefix
  );

  users[index].fingerprintID =
    preferences.getInt(
      key,
      -1
    );

  snprintf(
    key,
    sizeof(key),
    "%s_active",
    prefix
  );

  users[index].active =
    preferences.getBool(
      key,
      false
    );

  users[index].name.reserve(
    MAX_NAME_LEN
  );

  users[index].rfidUID.reserve(
    MAX_RFID_LEN
  );
}


// =====================================================
// USER COUNT
// =====================================================

int getRegisteredUserCountLocked()
{
  int count = 0;

  for (
    int i = 0;
    i < MAX_USERS;
    i++
  )
  {
    if (
      users[i].exists
    )
    {
      count++;
    }
  }

  return count;
}

int getRegisteredUserCount()
{
  int count = 0;

  if (
    lockData()
  )
  {
    count =
      getRegisteredUserCountLocked();

    unlockData();
  }

  return count;
}


// =====================================================
// LOAD ALL USERS
// =====================================================

void loadAllUsers()
{
  for (
    int i = 0;
    i < MAX_USERS;
    i++
  )
  {
    loadUser(i);
  }

  Serial.print(
    "[DATABASE] Users loaded: "
  );

  Serial.print(
    getRegisteredUserCount()
  );

  Serial.print(
    "/"
  );

  Serial.println(
    MAX_USERS
  );
}


// =====================================================
// USER SNAPSHOT
// =====================================================

bool getUserSnapshot(
  int index,
  UserSnapshot& snapshot
)
{
  memset(
    &snapshot,
    0,
    sizeof(snapshot)
  );

  snapshot.fingerprintID =
    -1;

  if (
    index < 0 ||
    index >= MAX_USERS
  )
  {
    return false;
  }

  if (
    !lockData()
  )
  {
    return false;
  }

  snapshot.exists =
    users[index].exists;

  snapshot.active =
    users[index].active;

  snapshot.fingerprintID =
    users[index].fingerprintID;

  copyText(
    snapshot.name,
    sizeof(snapshot.name),
    users[index].name.c_str()
  );

  copyText(
    snapshot.rfidUID,
    sizeof(snapshot.rfidUID),
    users[index].rfidUID.c_str()
  );

  unlockData();

  return true;
}


// =====================================================
// COPY ALL USERS
// =====================================================

void copyAllUserSnapshots(
  UserSnapshot* snapshots
)
{
  if (
    snapshots == nullptr
  )
  {
    return;
  }

  if (
    !lockData()
  )
  {
    return;
  }

  for (
    int i = 0;
    i < MAX_USERS;
    i++
  )
  {
    snapshots[i].exists =
      users[i].exists;

    snapshots[i].active =
      users[i].active;

    snapshots[i].fingerprintID =
      users[i].fingerprintID;

    copyText(
      snapshots[i].name,
      sizeof(snapshots[i].name),
      users[i].name.c_str()
    );

    copyText(
      snapshots[i].rfidUID,
      sizeof(snapshots[i].rfidUID),
      users[i].rfidUID.c_str()
    );
  }

  unlockData();
}


// =====================================================
// BLYNK USER COUNT
// =====================================================

void updateRegisteredUserCount()
{
  int count =
    getRegisteredUserCount();

  if (
    Blynk.connected()
  )
  {
    Blynk.virtualWrite(
      V14,
      count
    );
  }

  Serial.print(
    "[DATABASE] Registered users: "
  );

  Serial.print(
    count
  );

  Serial.print(
    " / "
  );

  Serial.println(
    MAX_USERS
  );
}


// =====================================================
// SHOW USER
// =====================================================

void showUser(
  int index
)
{
  UserSnapshot snapshot;

  if (
    !getUserSnapshot(
      index,
      snapshot
    )
  )
  {
    return;
  }

  if (
    !Blynk.connected()
  )
  {
    return;
  }

  if (
    !snapshot.exists
  )
  {
    Blynk.virtualWrite(
      V10,
      "EMPTY"
    );

    String emptyInfo;

    emptyInfo.reserve(
      48
    );

    emptyInfo +=
      "User ";

    emptyInfo +=
      String(
        index + 1
      );

    emptyInfo +=
      " is empty";

    Blynk.virtualWrite(
      V11,
      emptyInfo
    );

    return;
  }

  const char* status =
    snapshot.active
      ? "ACTIVE"
      : "BLOCKED";

  Blynk.virtualWrite(
    V10,
    status
  );

  String info;

  info.reserve(
    180
  );

  info +=
    "User ";

  info +=
    String(
      index + 1
    );

  info +=
    "\nName: ";

  info +=
    snapshot.name;

  info +=
    "\nRFID: ";

  if (
    snapshot.rfidUID[0] == '\0'
  )
  {
    info +=
      "-";
  }
  else
  {
    info +=
      snapshot.rfidUID;
  }

  info +=
    "\nFingerprint: ";

  if (
    snapshot.fingerprintID == -1
  )
  {
    info +=
      "-";
  }
  else
  {
    info +=
      "ID ";

    info +=
      String(
        snapshot.fingerprintID
      );
  }

  info +=
    "\nStatus: ";

  info +=
    status;

  Blynk.virtualWrite(
    V11,
    info
  );

  Serial.print(
    "[USER] User "
  );

  Serial.print(
    index + 1
  );

  Serial.print(
    ": "
  );

  Serial.print(
    snapshot.name
  );

  Serial.print(
    " | "
  );

  Serial.println(
    status
  );
}


// =====================================================
// RFID UID
// =====================================================

void getRFIDUID(
  char* output,
  size_t outputSize
)
{
  if (
    output == nullptr ||
    outputSize == 0
  )
  {
    return;
  }

  output[0] =
    '\0';

  const char hex[] =
    "0123456789ABCDEF";

  size_t pos = 0;

  for (
    byte i = 0;
    i < rfid.uid.size;
    i++
  )
  {
    if (
      i > 0
    )
    {
      if (
        pos + 1 >= outputSize
      )
      {
        break;
      }

      output[pos++] =
        ' ';
    }

    if (
      pos + 2 >= outputSize
    )
    {
      break;
    }

    byte value =
      rfid.uid.uidByte[i];

    output[pos++] =
      hex[
        (value >> 4) & 0x0F
      ];

    output[pos++] =
      hex[
        value & 0x0F
      ];
  }

  output[pos] =
    '\0';
}


// =====================================================
// RFID PICC TYPE
// =====================================================

const char* getRFIDPICCTypeName()
{
  MFRC522::PICC_Type piccType =
    rfid.PICC_GetType(
      rfid.uid.sak
    );

  switch (
    piccType
  )
  {
    case MFRC522::PICC_TYPE_MIFARE_MINI:
      return "MIFARE Mini";

    case MFRC522::PICC_TYPE_MIFARE_1K:
      return "MIFARE 1K";

    case MFRC522::PICC_TYPE_MIFARE_4K:
      return "MIFARE 4K";

    case MFRC522::PICC_TYPE_MIFARE_UL:
      return "MIFARE Ultralight";

    case MFRC522::PICC_TYPE_MIFARE_PLUS:
      return "MIFARE Plus";

    case MFRC522::PICC_TYPE_MIFARE_DESFIRE:
      return "MIFARE DESFire";

    case MFRC522::PICC_TYPE_TNP3XXX:
      return "MIFARE TNP3XXX";

    case MFRC522::PICC_TYPE_ISO_14443_4:
      return "ISO14443-4";

    case MFRC522::PICC_TYPE_ISO_18092:
      return "ISO18092";

    default:
      return "Unknown";
  }
}


// =====================================================
// RFID FINISH
// =====================================================

void finishRFID()
{
  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();

  rfid.PCD_AntennaOn();
}


// =====================================================
// RFID ROBUST READ
// =====================================================
//
// IMPORTANT FIX:
// Do NOT call WakeupA() continuously while no card
// is present. Normal polling is enough and is much
// smoother.
//
// =====================================================

bool readRFIDCardRobust()
{
  for (
    uint8_t attempt = 0;
    attempt < RFID_READ_RETRIES;
    attempt++
  )
  {
    if (
      !rfid.PICC_IsNewCardPresent()
    )
    {
      return false;
    }

    if (
      rfid.PICC_ReadCardSerial()
    )
    {
      return true;
    }

    finishRFID();

    delayMicroseconds(
      300
    );
  }

  return false;
}


// =====================================================
// RFID HARDWARE RESET
// =====================================================

void resetRFIDHardware()
{
  pinMode(
    RST_PIN,
    OUTPUT
  );

  digitalWrite(
    RST_PIN,
    LOW
  );

  delay(
    10
  );

  digitalWrite(
    RST_PIN,
    HIGH
  );

  delay(
    100
  );
}


// =====================================================
// READ RFID VERSION
// =====================================================

bool readRFIDVersion(
  byte& version
)
{
  version =
    rfid.PCD_ReadRegister(
      MFRC522::VersionReg
    );

  if (
    version == 0x00 ||
    version == 0xFF
  )
  {
    return false;
  }

  return true;
}


// =====================================================
// RFID INITIALIZE
// =====================================================

bool initializeRFID()
{
  Serial.println();

  Serial.println(
    "[RFID] Initializing RC522..."
  );

  pinMode(
    SS_PIN,
    OUTPUT
  );

  digitalWrite(
    SS_PIN,
    HIGH
  );

  resetRFIDHardware();

  SPI.begin(
    RFID_SCK_PIN,
    RFID_MISO_PIN,
    RFID_MOSI_PIN,
    SS_PIN
  );

  delay(
    20
  );

  rfid.PCD_Init();

  delay(
    80
  );

  rfid.PCD_AntennaOn();

  delay(
    20
  );

  rfid.PCD_SetAntennaGain(
    MFRC522::RxGain_max
  );

  delay(
    10
  );

  byte version;

  if (
    !readRFIDVersion(
      version
    )
  )
  {
    rfidReaderOK =
      false;

    Serial.println(
      "[RFID] INIT FAILED"
    );

    return false;
  }

  Serial.print(
    "[RFID] Version: 0x"
  );

  Serial.println(
    version,
    HEX
  );

  Serial.println(
    "[RFID] RC522 READY"
  );

  Serial.println(
    "[RFID] Antenna: ON"
  );

  Serial.println(
    "[RFID] RX Gain: MAX"
  );

  rfidReaderOK =
    true;

  rfidRecoveryFailures =
    0;

  rfidFailureMessageShown =
    false;

  rfidState =
    RFID_READY;

  currentRFIDUID[0] =
    '\0';

  lastRFIDUID[0] =
    '\0';

  lastRFIDProcessed =
    0;

  return true;
}


// =====================================================
// RFID RECOVERY
// =====================================================

void recoverRFID()
{
  unsigned long now =
    millis();

  if (
    now -
    rfidLastRecovery <
    RFID_RECOVERY_INTERVAL
  )
  {
    return;
  }

  rfidLastRecovery =
    now;

  rfidRecoveryFailures++;

  if (
    rfidRecoveryFailures <=
    RFID_MAX_SILENT_FAILURES ||
    (rfidRecoveryFailures % 5) == 0
  )
  {
    Serial.print(
      "[RFID] Recovery attempt #"
    );

    Serial.println(
      rfidRecoveryFailures
    );
  }

  resetRFIDHardware();

  SPI.begin(
    RFID_SCK_PIN,
    RFID_MISO_PIN,
    RFID_MOSI_PIN,
    SS_PIN
  );

  delay(
    10
  );

  rfid.PCD_Init();

  delay(
    60
  );

  rfid.PCD_AntennaOn();

  delay(
    10
  );

  rfid.PCD_SetAntennaGain(
    MFRC522::RxGain_max
  );

  delay(
    10
  );

  byte version;

  if (
    readRFIDVersion(
      version
    )
  )
  {
    rfidReaderOK =
      true;

    rfidRecoveryFailures =
      0;

    rfidFailureMessageShown =
      false;

    rfidState =
      RFID_READY;

    currentRFIDUID[0] =
      '\0';

    lastRFIDUID[0] =
      '\0';

    Serial.print(
      "[RFID] Recovery successful. Version: 0x"
    );

    Serial.println(
      version,
      HEX
    );
  }
  else
  {
    rfidReaderOK =
      false;

    if (
      !rfidFailureMessageShown
    )
    {
      Serial.println(
        "[RFID] Recovery pending."
      );

      Serial.println(
        "[RFID] Other system functions remain active."
      );

      rfidFailureMessageShown =
        true;
    }
  }
}


// =====================================================
// RFID HEALTH
// =====================================================

void updateRFIDHealth()
{
  unsigned long now =
    millis();

  if (
    now -
    rfidLastHealthCheck <
    RFID_HEALTH_INTERVAL
  )
  {
    return;
  }

  rfidLastHealthCheck =
    now;

  // Don't perform health reset while a card is
  // actively being processed or waiting for removal.
  if (
    rfidState ==
      RFID_PROCESSING ||
    rfidState ==
      RFID_WAIT_REMOVE
  )
  {
    return;
  }

  byte version;

  if (
    !readRFIDVersion(
      version
    )
  )
  {
    if (
      rfidReaderOK
    )
    {
      Serial.println(
        "[RFID] Communication lost."
      );
    }

    rfidReaderOK =
      false;

    recoverRFID();

    return;
  }

  rfidReaderOK =
    true;
}


// =====================================================
// RFID NORMAL
// =====================================================

void checkRFID()
{
  if (
    !rfidReaderOK ||
    rfidEnrollmentMode ||
    doorUnlocked ||
    deniedActive
  )
  {
    return;
  }

  unsigned long now =
    millis();

  // ---------------------------------------------------
  // WAIT UNTIL CARD IS REMOVED
  // ---------------------------------------------------

  if (
    rfidState ==
    RFID_WAIT_REMOVE
  )
  {
    if (
      now -
      rfidLastScan <
      RFID_SCAN_INTERVAL
    )
    {
      return;
    }

    rfidLastScan =
      now;

    if (
      rfid.PICC_IsNewCardPresent()
    )
    {
      return;
    }

    rfidState =
      RFID_READY;

    currentRFIDUID[0] =
      '\0';

    return;
  }

  // ---------------------------------------------------
  // POLLING
  // ---------------------------------------------------

  if (
    now -
    rfidLastScan <
    RFID_SCAN_INTERVAL
  )
  {
    return;
  }

  rfidLastScan =
    now;

  if (
    !readRFIDCardRobust()
  )
  {
    return;
  }

  rfidState =
    RFID_PROCESSING;

  char uid[
    MAX_RFID_LEN + 1
  ];

  getRFIDUID(
    uid,
    sizeof(uid)
  );

  if (
    uid[0] == '\0'
  )
  {
    finishRFID();

    rfidState =
      RFID_READY;

    return;
  }

  // ---------------------------------------------------
  // DUPLICATE CARD PROTECTION
  // ---------------------------------------------------

  if (
    strcmp(
      uid,
      lastRFIDUID
    ) == 0 &&
    now -
    lastRFIDProcessed <
    RFID_REPEAT_BLOCK_TIME
  )
  {
    finishRFID();

    rfidState =
      RFID_WAIT_REMOVE;

    return;
  }

  copyText(
    currentRFIDUID,
    sizeof(currentRFIDUID),
    uid
  );

  copyText(
    lastRFIDUID,
    sizeof(lastRFIDUID),
    uid
  );

  lastRFIDProcessed =
    now;

  Serial.println();
  Serial.println(
    "================================================"
  );

  Serial.println(
    "ðŸ’³ RFID DETECTED"
  );

  Serial.print(
    "ðŸ†” UID : "
  );

  Serial.println(
    uid
  );

  Serial.print(
    "ðŸ“ UID LENGTH : "
  );

  Serial.print(
    rfid.uid.size
  );

  Serial.println(
    " BYTE"
  );

  Serial.print(
    "ðŸ’³ PICC TYPE : "
  );

  Serial.println(
    getRFIDPICCTypeName()
  );

  Serial.println(
    "================================================"
  );

  int owner =
    findRFIDOwner(
      uid
    );

  if (
    owner != -1
  )
  {
    char name[
      MAX_NAME_LEN + 1
    ];

    bool active;

    if (
      getUserAccessInfo(
        owner,
        name,
        sizeof(name),
        active
      )
    )
    {
      if (
        active
      )
      {
        Serial.print(
          "ðŸ‘¤ USER : "
        );

        Serial.println(
          name
        );

        Serial.println(
          "ðŸ” STATUS : ACCESS GRANTED"
        );

        Serial.println(
          "ðŸ” METHOD : RFID"
        );

        grantAccess(
          name,
          "RFID",
          uid
        );
      }
      else
      {
        Serial.print(
          "ðŸ‘¤ USER : "
        );

        Serial.println(
          name
        );

        Serial.println(
          "ðŸ”’ STATUS : BLOCKED USER"
        );

        denyAccess(
          name,
          "RFID",
          "Blocked User",
          uid
        );
      }
    }
    else
    {
      denyAccess(
        "Unknown",
        "RFID",
        "User Error",
        uid
      );
    }
  }
  else
  {
    Serial.println(
      "ðŸ‘¤ USER : UNKNOWN"
    );

    Serial.println(
      "ðŸ”´ STATUS : ACCESS DENIED"
    );

    Serial.println(
      "âš ï¸ REASON : UNKNOWN USER"
    );

    denyAccess(
      "Unknown",
      "RFID",
      "Unknown User",
      uid
    );
  }

  finishRFID();

  rfidState =
    RFID_WAIT_REMOVE;
}


// =====================================================
// ENROLLMENT CANCEL / CLEANUP
// =====================================================

void cancelEnrollment(
  const char* reason,
  bool showStatus
)
{
  bool wasActive =
    rfidEnrollmentMode ||
    fingerprintEnrollmentMode;

  if (
    !wasActive
  )
  {
    enrollmentTimerActive = false;
    enrollmentStartTime = 0;
    temporaryLCDMessage = false;

    queueLCDMessage(
      "SYSTEM READY",
      "SCAN RFID/FINGER",
      0
    );

    return;
  }

  rfidEnrollmentMode =
    false;

  rfidEnrollMessageShown =
    false;

  fingerprintEnrollmentMode =
    false;

  fpEnrollState =
    FP_IDLE;

  enrollingFingerprintID =
    -1;

  enrollmentTimerActive =
    false;

  enrollmentStartTime =
    0;

  fpEnrollTimer =
    0;

  fingerprintState =
    FP_READY;

  rfidState =
    RFID_READY;

  currentRFIDUID[0] =
    '\0';

  // Enrollment messages use duration 0, so the normal LCD timer cannot
  // expire them. Explicitly restore the READY screen on cancel/timeout.
  temporaryLCDMessage =
    false;

  queueLCDMessage(
    "SYSTEM READY",
    "SCAN RFID/FINGER",
    0
  );

  if (
    showStatus &&
    reason != nullptr
  )
  {
    queueBlynkStatus(
      reason
    );
  }

  Serial.println(
    "[ENROLLMENT] CANCELLED / CLEANUP COMPLETE"
  );
}


// =====================================================
// GLOBAL ENROLLMENT TIMEOUT
// =====================================================

void updateEnrollmentTimeout()
{
  if (!enrollmentTimerActive)
  {
    return;
  }

  if (!rfidEnrollmentMode && !fingerprintEnrollmentMode)
  {
    enrollmentTimerActive = false;
    enrollmentStartTime = 0;
    return;
  }

  unsigned long now = millis();

  if (now - enrollmentStartTime < ENROLLMENT_TIMEOUT)
  {
    return;
  }

  if (rfidEnrollmentMode)
  {
    cancelEnrollment("RFID enrollment timeout", true);
  }
  else
  {
    cancelEnrollment("Fingerprint enrollment timeout", true);
  }
}


// =====================================================
// RFID ENROLLMENT
// =====================================================

void checkRFIDEnrollment()
{
  if (
    !rfidEnrollmentMode ||
    !rfidReaderOK
  )
  {
    return;
  }

  if (
    !rfidEnrollMessageShown
  )
  {
    lcdShow(
      "ADDING RFID...",
      "SCAN CARD"
    );

    rfidEnrollMessageShown =
      true;
  }

  unsigned long now =
    millis();

  if (
    enrollmentTimerActive &&
    now -
    enrollmentStartTime >=
    ENROLLMENT_TIMEOUT
  )
  {
    cancelEnrollment(
      "RFID enrollment timeout",
      true
    );

    return;
  }

  if (
    now -
    rfidLastScan <
    RFID_SCAN_INTERVAL
  )
  {
    return;
  }

  rfidLastScan =
    now;

  if (
    !readRFIDCardRobust()
  )
  {
    return;
  }

  char uid[
    MAX_RFID_LEN + 1
  ];

  getRFIDUID(
    uid,
    sizeof(uid)
  );

  Serial.println();
  Serial.println(
    "[RFID ENROLL] Card detected."
  );

  Serial.print(
    "[RFID ENROLL] UID: "
  );

  Serial.println(
    uid
  );

  int index =
    selectedUserID - 1;

  if (
    index < 0 ||
    index >= MAX_USERS
  )
  {
    queueBlynkStatus(
      "Invalid User ID"
    );

    rfidEnrollmentMode =
      false;

    rfidEnrollMessageShown =
      false;

    finishRFID();

    rfidState =
      RFID_READY;

    return;
  }

  UserSnapshot snapshot;

  if (
    !getUserSnapshot(
      index,
      snapshot
    )
  )
  {
    queueBlynkStatus(
      "User read error"
    );

    rfidEnrollmentMode =
      false;

    rfidEnrollMessageShown =
      false;

    finishRFID();

    rfidState =
      RFID_READY;

    return;
  }

  if (
    !snapshot.exists
  )
  {
    queueBlynkStatus(
      "User does not exist"
    );

    rfidEnrollmentMode =
      false;

    rfidEnrollMessageShown =
      false;

    finishRFID();

    rfidState =
      RFID_READY;

    return;
  }

  if (
    !snapshot.active
  )
  {
    queueBlynkStatus(
      "User is blocked"
    );

    rfidEnrollmentMode =
      false;

    rfidEnrollMessageShown =
      false;

    finishRFID();

    rfidState =
      RFID_READY;

    return;
  }

  int owner =
    findRFIDOwner(
      uid
    );

  if (
    owner != -1 &&
    owner != index
  )
  {
    queueBlynkStatus(
      "RFID already assigned"
    );

    queueLCDMessage(
      "RFID ALREADY USED",
      "TRY ANOTHER CARD",
      LCD_MESSAGE_TIME
    );

    rfidEnrollmentMode =
      false;

    rfidEnrollMessageShown =
      false;

    finishRFID();

    rfidState =
      RFID_READY;

    return;
  }

  if (
    lockData()
  )
  {
    users[index].rfidUID =
      uid;

    saveUserLocked(
      index
    );

    unlockData();
  }
  else
  {
    queueBlynkStatus(
      "Database busy"
    );

    finishRFID();

    rfidState =
      RFID_READY;

    return;
  }

  queueBlynkStatus(
    "RFID enrolled successfully"
  );

  queueBlynkShowUser(
    index
  );

  lcdShow(
    "RFID ENROLLED",
    String(
      snapshot.name
    )
  );

  Serial.println(
    "[RFID ENROLL] SUCCESS"
  );

  enrollmentTimerActive =
    false;

  enrollmentStartTime =
    0;

  rfidEnrollmentMode =
    false;

  rfidEnrollMessageShown =
    false;

  finishRFID();

  rfidState =
    RFID_WAIT_REMOVE;

  temporaryLCDMessage =
    true;

  messageTimer =
    millis();
}


// =====================================================
// FIND RFID OWNER
// =====================================================

int findRFIDOwner(
  const char* uid
)
{
  if (
    uid == nullptr
  )
  {
    return -1;
  }

  if (
    !lockData()
  )
  {
    return -1;
  }

  for (
    int i = 0;
    i < MAX_USERS;
    i++
  )
  {
    if (
      !users[i].exists
    )
    {
      continue;
    }

    if (
      users[i].rfidUID.equals(
        uid
      )
    )
    {
      unlockData();

      return i;
    }
  }

  unlockData();

  return -1;
}


// =====================================================
// FIND FINGERPRINT OWNER
// =====================================================

int findFingerprintOwner(
  int fingerprintID
)
{
  if (
    !lockData()
  )
  {
    return -1;
  }

  for (
    int i = 0;
    i < MAX_USERS;
    i++
  )
  {
    if (
      !users[i].exists
    )
    {
      continue;
    }

    if (
      users[i].fingerprintID ==
      fingerprintID
    )
    {
      unlockData();

      return i;
    }
  }

  unlockData();

  return -1;
}


// =====================================================
// USER ACCESS INFO
// =====================================================

bool getUserAccessInfo(
  int index,
  char* name,
  size_t nameSize,
  bool& active
)
{
  if (
    index < 0 ||
    index >= MAX_USERS
  )
  {
    return false;
  }

  if (
    !lockData()
  )
  {
    return false;
  }

  bool exists =
    users[index].exists;

  active =
    users[index].active;

  copyText(
    name,
    nameSize,
    users[index].name.c_str()
  );

  unlockData();

  return exists;
}


// =====================================================
// LOCAL ACCESS LOG
// =====================================================

void addAccessLogLocal(
  const AccessEvent& event
)
{
  String logEntry;

  logEntry.reserve(
    130
  );

  logEntry +=
    event.timestamp;

  logEntry +=
    " | ";

  logEntry +=
    event.user;

  logEntry +=
    " | ";

  logEntry +=
    event.method;

  logEntry +=
    " | ";

  logEntry +=
    event.result;

  if (
    !lockData()
  )
  {
    return;
  }

  for (
    int i = MAX_LOGS - 1;
    i > 0;
    i--
  )
  {
    accessLogs[i] =
      accessLogs[i - 1];
  }

  accessLogs[0] =
    logEntry;

  if (
    accessLogCount <
    MAX_LOGS
  )
  {
    accessLogCount++;
  }

  unlockData();
}


// =====================================================
// ACCESS LOG DISPLAY
// =====================================================

String buildAccessLogDisplay()
{
  String display;

  display.reserve(
    900
  );

  if (
    !lockData()
  )
  {
    return display;
  }

  for (
    int i = 0;
    i < accessLogCount;
    i++
  )
  {
    display +=
      String(
        i + 1
      );

    display +=
      ". ";

    display +=
      accessLogs[i];

    if (
      i <
      accessLogCount - 1
    )
    {
      display +=
        "\n";
    }
  }

  unlockData();

  return display;
}


// =====================================================
// ACCESS EVENT
// =====================================================

void prepareAccessEvent(
  AccessEvent& event,
  const char* user,
  const char* method,
  const char* result,
  const char* reason,
  const char* credential
)
{
  memset(
    &event,
    0,
    sizeof(event)
  );

  copyText(
    event.user,
    sizeof(event.user),
    user
  );

  copyText(
    event.method,
    sizeof(event.method),
    method
  );

  copyText(
    event.result,
    sizeof(event.result),
    result
  );

  copyText(
    event.reason,
    sizeof(event.reason),
    reason
  );

  copyText(
    event.credential,
    sizeof(event.credential),
    credential
  );

  String currentTime =
    getCurrentTime();

  copyText(
    event.timestamp,
    sizeof(event.timestamp),
    currentTime.c_str()
  );
}


// =====================================================
// QUEUE ACCESS EVENT
// =====================================================

void queueAccessEvent(
  const AccessEvent& event
)
{
  if (
    blynkEventQueue != nullptr
  )
  {
    BlynkEvent blynkEvent;

    memset(
      &blynkEvent,
      0,
      sizeof(blynkEvent)
    );

    blynkEvent.type =
      BLYNK_EVENT_ACCESS;

    blynkEvent.access =
      event;

    xQueueSend(
      blynkEventQueue,
      &blynkEvent,
      0
    );
  }

  if (
    telegramEventQueue != nullptr
  )
  {
    TelegramEvent telegramEvent;

    memset(
      &telegramEvent,
      0,
      sizeof(telegramEvent)
    );

    telegramEvent.type =
      TELEGRAM_EVENT_ACCESS;

    telegramEvent.access =
      event;

    xQueueSend(
      telegramEventQueue,
      &telegramEvent,
      0
    );
  }
}


// =====================================================
// QUEUE BLYNK STATUS
// =====================================================

void queueBlynkStatus(
  const char* text
)
{
  if (
    blynkEventQueue == nullptr
  )
  {
    return;
  }

  BlynkEvent event;

  memset(
    &event,
    0,
    sizeof(event)
  );

  event.type =
    BLYNK_EVENT_STATUS;

  copyText(
    event.text,
    sizeof(event.text),
    text
  );

  xQueueSend(
    blynkEventQueue,
    &event,
    0
  );
}


// =====================================================
// QUEUE SHOW USER
// =====================================================

void queueBlynkShowUser(
  int index
)
{
  if (
    blynkEventQueue == nullptr
  )
  {
    return;
  }

  BlynkEvent event;

  memset(
    &event,
    0,
    sizeof(event)
  );

  event.type =
    BLYNK_EVENT_SHOW_USER;

  event.userIndex =
    index;

  xQueueSend(
    blynkEventQueue,
    &event,
    0
  );
}


// =====================================================
// QUEUE DOOR STATE
// =====================================================

void queueDoorState(
  bool unlocked
)
{
  if (
    blynkEventQueue == nullptr
  )
  {
    return;
  }

  BlynkEvent event;

  memset(
    &event,
    0,
    sizeof(event)
  );

  event.type =
    BLYNK_EVENT_DOOR;

  event.doorState =
    unlocked;

  xQueueSend(
    blynkEventQueue,
    &event,
    0
  );
}


// =====================================================
// QUEUE TELEGRAM USER EVENT
// =====================================================

void queueTelegramUserEvent(
  uint8_t eventType,
  const char* name,
  int userID
)
{
  if (
    telegramEventQueue == nullptr
  )
  {
    return;
  }

  TelegramEvent event;

  memset(
    &event,
    0,
    sizeof(event)
  );

  event.type =
    eventType;

  copyText(
    event.userAction.name,
    sizeof(event.userAction.name),
    name
  );

  event.userAction.userID =
    userID;

  String currentTime =
    getCurrentTime();

  copyText(
    event.userAction.timestamp,
    sizeof(event.userAction.timestamp),
    currentTime.c_str()
  );

  xQueueSend(
    telegramEventQueue,
    &event,
    0
  );
}


// =====================================================
// BUZZER
// =====================================================

void startBuzzer()
{
  buzzerActive =
    true;

  buzzerStart =
    millis();

  ledcWrite(
    BUZZER,
    128
  );
}

void updateBuzzer()
{
  if (
    !buzzerActive
  )
  {
    return;
  }

  if (
    millis() -
    buzzerStart >=
    BUZZER_TIME
  )
  {
    ledcWrite(
      BUZZER,
      0
    );

    buzzerActive =
      false;
  }
}


// =====================================================
// GRANT ACCESS
// =====================================================

void grantAccess(
  const char* userName,
  const char* method,
  const char* credential
)
{
  if (
    !lockData()
  )
  {
    return;
  }

  if (
    doorUnlocked
  )
  {
    unlockData();

    return;
  }

  doorUnlocked =
    true;

  doorUnlockStart =
    millis();

  unlockData();

  // PRESERVED RELAY LOGIC
  digitalWrite(
    RELAY_PIN,
    HIGH
  );

  digitalWrite(
    GREEN_LED,
    HIGH
  );

  digitalWrite(
    RED_LED,
    LOW
  );

  startBuzzer();

  lcdShow(
    "ACCESS GRANTED",
    String(
      userName
    )
  );

  AccessEvent event;

  prepareAccessEvent(
    event,
    userName,
    method,
    "Granted",
    "",
    credential
  );

  if (
    lockData()
  )
  {
    accessCount++;

    if (
      accessCount > 999
    )
    {
      accessCount =
        999;
    }

    unlockData();
  }

  addAccessLogLocal(
    event
  );

  queueAccessEvent(
    event
  );

  queueDoorState(
    true
  );

  Serial.println();
  Serial.println(
    "================================================"
  );

  Serial.println(
    "ðŸŸ¢ ACCESS GRANTED"
  );

  Serial.print(
    "ðŸ‘¤ USER: "
  );

  Serial.println(
    userName
  );

  Serial.print(
    "ðŸ” METHOD: "
  );

  Serial.println(
    method
  );

  Serial.println(
    "ðŸšª DOOR: UNLOCKED"
  );

  Serial.println(
    "â±ï¸ AUTO LOCK: 5 SECONDS"
  );

  Serial.println(
    "================================================"
  );
}


// =====================================================
// DENY ACCESS
// =====================================================

void denyAccess(
  const char* userName,
  const char* method,
  const char* reason,
  const char* credential
)
{
  lcdShow(
    "ACCESS DENIED",
    "TRY AGAIN"
  );

  digitalWrite(
    GREEN_LED,
    LOW
  );

  deniedActive =
    true;

  deniedStep =
    0;

  deniedTimer =
    millis();

  AccessEvent event;

  prepareAccessEvent(
    event,
    userName,
    method,
    "Denied",
    reason,
    credential
  );

  addAccessLogLocal(
    event
  );

  queueAccessEvent(
    event
  );

  Serial.println();
  Serial.println(
    "================================================"
  );

  Serial.println(
    "ðŸ”´ ACCESS DENIED"
  );

  Serial.print(
    "ðŸ‘¤ USER: "
  );

  Serial.println(
    userName
  );

  Serial.print(
    "ðŸ” METHOD: "
  );

  Serial.println(
    method
  );

  Serial.print(
    "âš ï¸ REASON: "
  );

  Serial.println(
    reason
  );

  Serial.println(
    "================================================"
  );
}


// =====================================================
// DENIED ANIMATION
// =====================================================

void updateDenied()
{
  if (
    !deniedActive
  )
  {
    return;
  }

  if (
    millis() -
    deniedTimer <
    150
  )
  {
    return;
  }

  deniedTimer =
    millis();

  if (
    deniedStep == 0
  )
  {
    digitalWrite(
      RED_LED,
      HIGH
    );

    ledcWrite(
      BUZZER,
      128
    );

    deniedStep =
      1;
  }

  else if (
    deniedStep == 1
  )
  {
    digitalWrite(
      RED_LED,
      LOW
    );

    ledcWrite(
      BUZZER,
      0
    );

    deniedStep =
      2;
  }

  else if (
    deniedStep == 2
  )
  {
    digitalWrite(
      RED_LED,
      HIGH
    );

    ledcWrite(
      BUZZER,
      128
    );

    deniedStep =
      3;
  }

  else if (
    deniedStep == 3
  )
  {
    digitalWrite(
      RED_LED,
      LOW
    );

    ledcWrite(
      BUZZER,
      0
    );

    deniedStep =
      4;
  }

  else if (
    deniedStep == 4
  )
  {
    deniedActive =
      false;

    showReadyScreen();
  }
}


// =====================================================
// LCD TIMER
// =====================================================

void updateLCD()
{
  if (
    temporaryLCDMessage &&
    !doorUnlocked &&
    !deniedActive
  )
  {
    if (
      millis() -
      messageTimer >=
      LCD_MESSAGE_TIME
    )
    {
      temporaryLCDMessage =
        false;

      showReadyScreen();
    }
  }
}


// =====================================================
// FINGERPRINT ID USED
// =====================================================

bool fingerprintIDUsed(
  int id
)
{
  if (
    !lockData()
  )
  {
    return true;
  }

  for (
    int i = 0;
    i < MAX_USERS;
    i++
  )
  {
    if (
      !users[i].exists
    )
    {
      continue;
    }

    if (
      users[i].fingerprintID ==
      id
    )
    {
      unlockData();

      return true;
    }
  }

  unlockData();

  return false;
}


// =====================================================
// FIND AVAILABLE FINGERPRINT ID
// =====================================================

int findAvailableFingerprintID()
{
  if (
    !lockData()
  )
  {
    return -1;
  }

  for (
    int id = 1;
    id <= 127;
    id++
  )
  {
    bool used =
      false;

    for (
      int i = 0;
      i < MAX_USERS;
      i++
    )
    {
      if (
        users[i].exists &&
        users[i].fingerprintID ==
        id
      )
      {
        used =
          true;

        break;
      }
    }

    if (
      !used
    )
    {
      unlockData();

      return id;
    }
  }

  unlockData();

  return -1;
}


// =====================================================
// START FINGERPRINT ENROLLMENT
// =====================================================

void startFingerprintEnrollment()
{
  int index =
    selectedUserID - 1;

  if (
    index < 0 ||
    index >= MAX_USERS
  )
  {
    queueBlynkStatus(
      "Invalid User ID"
    );

    return;
  }

  if (
    !fingerprintSensorOK
  )
  {
    queueBlynkStatus(
      "Fingerprint sensor unavailable"
    );

    queueLCDMessage(
      "FINGERPRINT",
      "SENSOR OFFLINE",
      LCD_MESSAGE_TIME
    );

    return;
  }

  UserSnapshot snapshot;

  if (
    !getUserSnapshot(
      index,
      snapshot
    )
  )
  {
    queueBlynkStatus(
      "User read error"
    );

    return;
  }

  if (
    !snapshot.exists
  )
  {
    queueBlynkStatus(
      "User does not exist"
    );

    return;
  }

  if (
    !snapshot.active
  )
  {
    queueBlynkStatus(
      "User is blocked"
    );

    return;
  }

  enrollingFingerprintID =
    findAvailableFingerprintID();

  if (
    enrollingFingerprintID == -1
  )
  {
    queueBlynkStatus(
      "Fingerprint storage full"
    );

    return;
  }

  fingerprintEnrollmentMode =
    true;

  fpEnrollState =
    FP_WAIT_FIRST;

  enrollmentStartTime =
    millis();

  enrollmentTimerActive =
    true;

  fpEnrollTimer =
    enrollmentStartTime;

  Blynk.virtualWrite(
    V12,
    "Place finger..."
  );

  queueLCDMessage(
    "ENROLL FINGER",
    "PLACE FINGER",
    0
  );

  Serial.println();
  Serial.println(
    "ðŸ‘† FINGERPRINT ENROLLMENT"
  );

  Serial.print(
    "Assigned ID: "
  );

  Serial.println(
    enrollingFingerprintID
  );
}


// =====================================================
// FINGERPRINT ENROLLMENT
// =====================================================

void checkFingerprintEnrollment()
{
  if (
    !fingerprintEnrollmentMode
  )
  {
    return;
  }

  unsigned long now =
    millis();

  if (
    enrollmentTimerActive &&
    now -
    enrollmentStartTime >=
    ENROLLMENT_TIMEOUT
  )
  {
    cancelEnrollment(
      "Fingerprint enrollment timeout",
      true
    );

    return;
  }

  if (
    !fingerprintSensorOK
  )
  {
    fingerprintEnrollmentMode =
      false;

    fpEnrollState =
      FP_IDLE;

    enrollingFingerprintID =
      -1;

    queueBlynkStatus(
      "Fingerprint sensor unavailable"
    );

    return;
  }

  uint8_t p;

  if (
    fpEnrollState ==
    FP_WAIT_FIRST
  )
  {
    p =
      finger.getImage();

    if (
      p ==
      FINGERPRINT_NOFINGER
    )
    {
      return;
    }

    if (
      p !=
      FINGERPRINT_OK
    )
    {
      queueBlynkStatus(
        "First scan failed"
      );

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      return;
    }

    p =
      finger.image2Tz(
        1
      );

    if (
      p !=
      FINGERPRINT_OK
    )
    {
      queueBlynkStatus(
        "First scan failed"
      );

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      return;
    }

    queueBlynkStatus(
      "Remove finger..."
    );

    lcdShow(
      "FIRST SCAN OK",
      "REMOVE FINGER"
    );

    fpEnrollState =
      FP_REMOVE_FIRST;

    fpEnrollTimer =
      millis();

    return;
  }

  if (
    fpEnrollState ==
    FP_REMOVE_FIRST
  )
  {
    p =
      finger.getImage();

    if (
      p ==
      FINGERPRINT_NOFINGER
    )
    {
      queueBlynkStatus(
        "Place same finger again"
      );

      lcdShow(
        "PLACE SAME",
        "FINGER AGAIN"
      );

      fpEnrollState =
        FP_WAIT_SECOND;

      fpEnrollTimer =
        millis();

      return;
    }

    return;
  }

  if (
    fpEnrollState ==
    FP_WAIT_SECOND
  )
  {
    p =
      finger.getImage();

    if (
      p ==
      FINGERPRINT_NOFINGER
    )
    {
      return;
    }

    if (
      p !=
      FINGERPRINT_OK
    )
    {
      queueBlynkStatus(
        "Second scan failed"
      );

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      return;
    }

    p =
      finger.image2Tz(
        2
      );

    if (
      p !=
      FINGERPRINT_OK
    )
    {
      queueBlynkStatus(
        "Second scan failed"
      );

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      return;
    }

    p =
      finger.createModel();

    if (
      p !=
      FINGERPRINT_OK
    )
    {
      queueBlynkStatus(
        "Finger mismatch"
      );

      lcdShow(
        "FINGER MISMATCH",
        "TRY AGAIN"
      );

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      temporaryLCDMessage =
        true;

      messageTimer =
        millis();

      return;
    }

    p =
      finger.storeModel(
        enrollingFingerprintID
      );

    if (
      p !=
      FINGERPRINT_OK
    )
    {
      queueBlynkStatus(
        "Fingerprint save failed"
      );

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      return;
    }

    int index =
      selectedUserID - 1;

    UserSnapshot snapshot;

    if (
      !getUserSnapshot(
        index,
        snapshot
      )
    )
    {
      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      queueBlynkStatus(
        "User read error"
      );

      return;
    }

    if (
      lockData()
    )
    {
      users[index].fingerprintID =
        enrollingFingerprintID;

      saveUserLocked(
        index
      );

      unlockData();
    }

    enrollmentTimerActive =
      false;

    enrollmentStartTime =
      0;

    queueBlynkStatus(
      "Fingerprint enrolled successfully"
    );

    queueBlynkShowUser(
      index
    );

    lcdShow(
      "FINGERPRINT OK",
      String(
        snapshot.name
      )
    );

    fingerprintEnrollmentMode =
      false;

    fpEnrollState =
      FP_IDLE;

    enrollingFingerprintID =
      -1;

    temporaryLCDMessage =
      true;

    messageTimer =
      millis();

    Serial.println(
      "ðŸ‘† FINGERPRINT ENROLLMENT SUCCESS"
    );

    return;
  }
}


// =====================================================
// FINGERPRINT HANDSHAKE
// =====================================================

bool performFingerprintHandshake()
{
  // Clear stale response bytes first.
  while (
    mySerial.available()
  )
  {
    mySerial.read();
  }

  delay(
    30
  );

  uint8_t result =
    finger.verifyPassword();

  return (
    result ==
    FINGERPRINT_OK
  );
}


// =====================================================
// FINGERPRINT UART RE-SYNC
// =====================================================
//
// This does NOT reset ESP32.
// It only restarts UART2 on the host side.
//
// Useful when AS608 was already powered and ESP32
// started communicating too early.
//
// =====================================================

bool recoverFingerprintUART()
{
  unsigned long now =
    millis();

  if (
    now -
    fingerprintLastUARTRecovery <
    FINGERPRINT_UART_RECOVERY_INTERVAL
  )
  {
    return false;
  }

  fingerprintLastUARTRecovery =
    now;

  Serial.println(
    "[FINGERPRINT] UART re-sync..."
  );

  mySerial.flush();

  mySerial.end();

  delay(
    80
  );

  mySerial.begin(
    57600,
    SERIAL_8N1,
    FP_RX,
    FP_TX
  );

  delay(
    120
  );

  finger.begin(
    57600
  );

  delay(
    80
  );

  while (
    mySerial.available()
  )
  {
    mySerial.read();
  }

  delay(
    50
  );

  return performFingerprintHandshake();
}


// =====================================================
// START FINGERPRINT INITIALIZATION
// =====================================================

void startFingerprintInitialization()
{
  // UART2 is initialized once during normal startup.
  mySerial.begin(
    57600,
    SERIAL_8N1,
    FP_RX,
    FP_TX
  );

  delay(
    50
  );

  finger.begin(
    57600
  );

  fingerprintSensorOK =
    false;

  fingerprintInitState =
    FP_INIT_WAIT;

  fingerprintInitTimer =
    millis();

  fingerprintLastInitAttempt =
    0;

  fingerprintStartupRetries =
    0;

  fingerprintInitMessageShown =
    false;

  fingerprintLastUARTRecovery =
    0;

  fingerprintState =
    FP_READY;

  Serial.println(
    "[FINGERPRINT] UART initialized."
  );

  Serial.println(
    "[FINGERPRINT] AS608 startup stabilization..."
  );
}


// =====================================================
// FINGERPRINT INITIALIZATION UPDATE
// =====================================================

void updateFingerprintInitialization()
{
  if (
    fingerprintSensorOK
  )
  {
    return;
  }

  unsigned long now =
    millis();

  if (
    fingerprintInitState ==
    FP_INIT_WAIT
  )
  {
    if (
      now -
      fingerprintInitTimer <
      FINGERPRINT_STARTUP_DELAY
    )
    {
      return;
    }

    fingerprintInitState =
      FP_INIT_RETRY;
  }

  if (
    fingerprintInitState !=
    FP_INIT_RETRY
  )
  {
    return;
  }

  if (
    fingerprintStartupRetries >=
    FINGERPRINT_MAX_STARTUP_RETRIES
  )
  {
    if (
      now -
      fingerprintLastInitAttempt <
      FINGERPRINT_LONG_RECOVERY_INTERVAL
    )
    {
      return;
    }
  }
  else
  {
    if (
      fingerprintLastInitAttempt != 0 &&
      now -
      fingerprintLastInitAttempt <
      FINGERPRINT_RETRY_INTERVAL
    )
    {
      return;
    }
  }

  fingerprintLastInitAttempt =
    now;

  fingerprintStartupRetries++;

  bool success =
    performFingerprintHandshake();

  if (
    success
  )
  {
    fingerprintSensorOK =
      true;

    fingerprintInitState =
      FP_INIT_READY;

    fingerprintStartupRetries =
      0;

    fingerprintState =
      FP_READY;

    fingerprintLastError =
      0;

    Serial.println(
      "ðŸ‘† Fingerprint: READY"
    );

    Serial.println(
      "[FINGERPRINT] AS608 handshake successful."
    );

    queueBlynkStatus(
      "Fingerprint sensor ready"
    );

    return;
  }

  if (
    fingerprintStartupRetries <= 3 ||
    (fingerprintStartupRetries % 5) == 0
  )
  {
    Serial.print(
      "[FINGERPRINT] Handshake pending. Attempt #"
    );

    Serial.println(
      fingerprintStartupRetries
    );
  }

  while (
    mySerial.available()
  )
  {
    mySerial.read();
  }

  // After several failed startup attempts,
  // perform host UART re-sync instead of resetting ESP32.
  if (
    fingerprintStartupRetries >= 3
  )
  {
    if (
      recoverFingerprintUART()
    )
    {
      fingerprintSensorOK =
        true;

      fingerprintInitState =
        FP_INIT_READY;

      fingerprintStartupRetries =
        0;

      fingerprintState =
        FP_READY;

      Serial.println(
        "ðŸ‘† Fingerprint: READY after UART re-sync"
      );

      queueBlynkStatus(
        "Fingerprint sensor ready"
      );
    }
  }
}


// =====================================================
// NORMAL FINGERPRINT
// =====================================================

void checkFingerprint()
{
  if (
    !fingerprintSensorOK ||
    fingerprintEnrollmentMode ||
    doorUnlocked ||
    deniedActive
  )
  {
    return;
  }

  unsigned long now =
    millis();

  if (
    fingerprintState ==
    FP_PROCESSING
  )
  {
    if (
      now -
      fingerprintProcessingStart >
      FINGERPRINT_PROCESS_TIME
    )
    {
      fingerprintState =
        FP_WAIT_REMOVE;

      fingerprintLastScan =
        now;

      Serial.println(
        "[FINGERPRINT] Processing timeout."
      );
    }

    return;
  }

  if (
    fingerprintState ==
    FP_WAIT_REMOVE
  )
  {
    if (
      now -
      fingerprintLastScan <
      FINGERPRINT_SCAN_INTERVAL
    )
    {
      return;
    }

    fingerprintLastScan =
      now;

    uint8_t removeResult =
      finger.getImage();

    if (
      removeResult ==
      FINGERPRINT_NOFINGER
    )
    {
      fingerprintState =
        FP_READY;
    }

    return;
  }

  if (
    now -
    fingerprintLastScan <
    FINGERPRINT_SCAN_INTERVAL
  )
  {
    return;
  }

  fingerprintLastScan =
    now;

  fingerprintState =
    FP_DETECTING;

  uint8_t p =
    finger.getImage();

  if (
    p ==
    FINGERPRINT_NOFINGER
  )
  {
    fingerprintState =
      FP_READY;

    return;
  }

  if (
    p !=
    FINGERPRINT_OK
  )
  {
    fingerprintState =
      FP_READY;

    if (
      now -
      fingerprintLastError >=
      FINGERPRINT_ERROR_PRINT_INTERVAL
    )
    {
      fingerprintLastError =
        now;

      Serial.print(
        "[FINGERPRINT] getImage error: "
      );

      Serial.println(
        p
      );
    }

    return;
  }

  fingerprintState =
    FP_PROCESSING;

  fingerprintProcessingStart =
    now;

  Serial.println();
  Serial.println(
    "================================================"
  );

  Serial.println(
    "ðŸ‘† FINGERPRINT SCAN"
  );

  Serial.println(
    "STATUS : SCANNING..."
  );

  p =
    finger.image2Tz();

  if (
    p !=
    FINGERPRINT_OK
  )
  {
    fingerprintState =
      FP_WAIT_REMOVE;

    if (
      now -
      fingerprintLastError >=
      FINGERPRINT_ERROR_PRINT_INTERVAL
    )
    {
      fingerprintLastError =
        now;

      Serial.print(
        "[FINGERPRINT] image2Tz error: "
      );

      Serial.println(
        p
      );
    }

    return;
  }

  p =
    finger.fingerSearch();

  if (
    p ==
    FINGERPRINT_OK
  )
  {
    int fingerprintID =
      finger.fingerID;

    int owner =
      findFingerprintOwner(
        fingerprintID
      );

    if (
      owner != -1
    )
    {
      char name[
        MAX_NAME_LEN + 1
      ];

      bool active;

      if (
        getUserAccessInfo(
          owner,
          name,
          sizeof(name),
          active
        )
      )
      {
        if (
          active
        )
        {
          Serial.print(
            "RESULT : MATCH"
          );

          Serial.print(
            " | USER : "
          );

          Serial.println(
            name
          );

          Serial.println(
            "METHOD : FINGERPRINT"
          );

          Serial.println(
            "STATUS : ACCESS GRANTED"
          );

          char fpIDText[16];

          snprintf(
            fpIDText,
            sizeof(fpIDText),
            "%d",
            fingerprintID
          );

          grantAccess(
            name,
            "Fingerprint",
            fpIDText
          );
        }
        else
        {
          denyAccess(
            name,
            "Fingerprint",
            "Blocked User"
          );
        }
      }
      else
      {
        denyAccess(
          "Unknown",
          "Fingerprint",
          "User Error"
        );
      }
    }
    else
    {
      denyAccess(
        "Unknown",
        "Fingerprint",
        "Unknown Finger"
      );
    }

    fingerprintState =
      FP_WAIT_REMOVE;

    fingerprintLastScan =
      now;

    return;
  }

  if (
    p ==
    FINGERPRINT_NOTFOUND
  )
  {
    Serial.println(
      "RESULT : NO MATCH"
    );

    Serial.println(
      "STATUS : ACCESS DENIED"
    );

    denyAccess(
      "Unknown",
      "Fingerprint",
      "Fingerprint not recognized"
    );

    fingerprintState =
      FP_WAIT_REMOVE;

    fingerprintLastScan =
      now;

    return;
  }

  fingerprintState =
    FP_WAIT_REMOVE;

  if (
    now -
    fingerprintLastError >=
    FINGERPRINT_ERROR_PRINT_INTERVAL
  )
  {
    fingerprintLastError =
      now;

    Serial.print(
      "[FINGERPRINT] Search error: "
    );

    Serial.println(
      p
    );
  }
}


// =====================================================
// TOUCH EXIT
// =====================================================

void checkTouch()
{
  bool currentTouch =
    digitalRead(
      TOUCH_PIN
    );

  if (
    currentTouch == HIGH &&
    previousTouchState == LOW
  )
  {
    if (
      millis() -
      lastTouchTime >=
      TOUCH_DEBOUNCE
    )
    {
      lastTouchTime =
        millis();

      Serial.println();
      Serial.println(
        "ðŸ”˜ EXIT TOUCH DETECTED"
      );

      grantAccess(
        "Exit User",
        "Touch"
      );
    }
  }

  previousTouchState =
    currentTouch;
}


// =====================================================
// HARDWARE COMMAND PROCESSOR
// =====================================================

void processHardwareCommands()
{
  if (
    hardwareCommandQueue == nullptr
  )
  {
    return;
  }

  HardwareCommand command;

  while (
    xQueueReceive(
      hardwareCommandQueue,
      &command,
      0
    ) == pdTRUE
  )
  {
    if (
      command.type ==
      HW_COMMAND_UNLOCK
    )
    {
      grantAccess(
        command.user,
        command.method
      );
    }

    else if (
      command.type ==
      HW_COMMAND_LOCK
    )
    {
      digitalWrite(
        RELAY_PIN,
        LOW
      );

      digitalWrite(
        GREEN_LED,
        LOW
      );

      digitalWrite(
        RED_LED,
        LOW
      );

      doorUnlocked =
        false;

      buzzerActive =
        false;

      ledcWrite(
        BUZZER,
        0
      );

      showReadyScreen();

      queueDoorState(
        false
      );

      Serial.println(
        "ðŸ”’ DOOR: LOCKED"
      );

      Serial.println(
        "âœ… SYSTEM READY"
      );
    }

    else if (
      command.type ==
      HW_COMMAND_LCD
    )
    {
      lcdShow(
        String(
          command.lcdLine1
        ),
        String(
          command.lcdLine2
        )
      );

      if (
        command.lcdDuration > 0
      )
      {
        temporaryLCDMessage =
          true;

        messageTimer =
          millis();
      }
      else
      {
        temporaryLCDMessage =
          false;
      }
    }
  }
}


// =====================================================
// DOOR TIMER
// =====================================================

void updateDoor()
{
  if (
    !doorUnlocked
  )
  {
    return;
  }

  if (
    millis() -
    doorUnlockStart >=
    DOOR_UNLOCK_TIME
  )
  {
    digitalWrite(
      RELAY_PIN,
      LOW
    );

    digitalWrite(
      GREEN_LED,
      LOW
    );

    doorUnlocked =
      false;

    showReadyScreen();

    queueDoorState(
      false
    );

    Serial.println();
    Serial.println(
      "ðŸ”’ DOOR: LOCKED"
    );

    Serial.println(
      "âœ… SYSTEM READY"
    );
  }
}


// =====================================================
// BLYNK V5
// MANUAL UNLOCK / LOCK
// =====================================================

BLYNK_WRITE(V5)
{
  int value =
    param.asInt();

  if (
    hardwareCommandQueue == nullptr
  )
  {
    return;
  }

  HardwareCommand command;

  memset(
    &command,
    0,
    sizeof(command)
  );

  if (
    value == 1
  )
  {
    Serial.println(
      "â˜ï¸ BLYNK COMMAND: MANUAL UNLOCK"
    );

    command.type =
      HW_COMMAND_UNLOCK;

    copyText(
      command.user,
      sizeof(command.user),
      "Blynk User"
    );

    copyText(
      command.method,
      sizeof(command.method),
      "Blynk"
    );

    xQueueSend(
      hardwareCommandQueue,
      &command,
      0
    );

    Blynk.virtualWrite(
      V5,
      0
    );
  }
  else
  {
    Serial.println(
      "â˜ï¸ BLYNK COMMAND: LOCK"
    );

    command.type =
      HW_COMMAND_LOCK;

    xQueueSend(
      hardwareCommandQueue,
      &command,
      0
    );
  }
}


// =====================================================
// BLYNK V6
// USER NAME
// =====================================================

BLYNK_WRITE(V6)
{
  selectedName =
    param.asStr();

  selectedName.reserve(
    MAX_NAME_LEN
  );

  Serial.print(
    "[BLYNK] User name: "
  );

  Serial.println(
    selectedName
  );
}


// =====================================================
// BLYNK V7
// USER ID
// =====================================================

BLYNK_WRITE(V7)
{
  selectedUserID =
    param.asInt();

  Serial.print(
    "[BLYNK] Selected User ID: "
  );

  Serial.println(
    selectedUserID
  );

  if (
    selectedUserID < 1 ||
    selectedUserID > MAX_USERS
  )
  {
    Blynk.virtualWrite(
      V12,
      "Invalid User ID"
    );

    return;
  }

  showUser(
    selectedUserID - 1
  );
}


// =====================================================
// BLYNK V8
// USER ACTION
// =====================================================

BLYNK_WRITE(V8)
{
  int action =
    param.asInt();

  // ===================================================
  // CANCEL ENROLLMENT
  // V8 = 7
  //
  // Safety fallback: if the Blynk Menu sends option 6 while an
  // enrollment is active, never delete the selected user. Treat it as
  // CANCEL. When no enrollment is active, option 6 remains DELETE.
  // ===================================================

  if (
    action == 7 ||
    (
      action == 6 &&
      (
        rfidEnrollmentMode ||
        fingerprintEnrollmentMode
      )
    )
  )
  {
    Serial.print(
      "[BLYNK V8] CANCEL ENROLL REQUEST, action="
    );

    Serial.println(
      action
    );

    cancelEnrollment(
      "Enrollment cancelled",
      true
    );

    Blynk.virtualWrite(
      V8,
      0
    );

    return;
  }

  if (
    selectedUserID < 1 ||
    selectedUserID > MAX_USERS
  )
  {
    Blynk.virtualWrite(
      V12,
      "Invalid User ID"
    );

    Blynk.virtualWrite(
      V8,
      0
    );

    return;
  }

  int index =
    selectedUserID - 1;


  // ===================================================
  // ADD USER
  // ===================================================

  if (
    action == 1
  )
  {
    if (
      !lockData()
    )
    {
      Blynk.virtualWrite(
        V12,
        "Database busy"
      );

      return;
    }

    if (
      users[index].exists
    )
    {
      unlockData();

      Blynk.virtualWrite(
        V12,
        "User already exists"
      );
    }

    else if (
      selectedName.length() == 0
    )
    {
      unlockData();

      Blynk.virtualWrite(
        V12,
        "Enter user name first"
      );
    }

    else
    {
      String newUserName =
        selectedName;

      users[index].exists =
        true;

      users[index].name =
        newUserName;

      users[index].rfidUID =
        "";

      users[index].fingerprintID =
        -1;

      users[index].active =
        true;

      saveUserLocked(
        index
      );

      unlockData();

      updateRegisteredUserCount();

      Blynk.virtualWrite(
        V12,
        "User added successfully"
      );

      showUser(
        index
      );

      queueLCDMessage(
        "USER ADDED",
        newUserName.c_str(),
        LCD_MESSAGE_TIME
      );

      queueTelegramUserEvent(
        TELEGRAM_EVENT_USER_ADDED,
        newUserName.c_str(),
        index + 1
      );

      Serial.println();
      Serial.println(
        "ðŸ‘¤ NEW USER ADDED"
      );

      Serial.print(
        "Name: "
      );

      Serial.println(
        newUserName
      );

      Serial.print(
        "User ID: "
      );

      Serial.println(
        index + 1
      );

      Serial.println(
        "Source: Blynk"
      );
    }
  }


  // ===================================================
  // ADD RFID
  // ===================================================

  else if (
    action == 2
  )
  {
    UserSnapshot snapshot;

    if (
      !getUserSnapshot(
        index,
        snapshot
      )
    )
    {
      Blynk.virtualWrite(
        V12,
        "User read error"
      );
    }

    else if (
      !snapshot.exists
    )
    {
      Blynk.virtualWrite(
        V12,
        "User does not exist"
      );
    }

    else if (
      !snapshot.active
    )
    {
      Blynk.virtualWrite(
        V12,
        "User is blocked"
      );
    }

    else
    {
      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      rfidEnrollmentMode =
        true;

      enrollmentStartTime =
        millis();

      enrollmentTimerActive =
        true;

      rfidEnrollMessageShown =
        false;

      rfidState =
        RFID_READY;

      Blynk.virtualWrite(
        V12,
        "Waiting for RFID card..."
      );

      queueLCDMessage(
        "ADDING RFID...",
        "SCAN CARD",
        0
      );

      Serial.println(
        "ðŸ’³ RFID ENROLLMENT MODE"
      );
    }
  }


  // ===================================================
  // ADD FINGERPRINT
  // ===================================================

  else if (
    action == 3
  )
  {
    rfidEnrollmentMode =
      false;

    rfidEnrollMessageShown =
      false;

    enrollmentTimerActive =
      false;

    enrollmentStartTime =
      0;

    startFingerprintEnrollment();
  }


  // ===================================================
  // BLOCK
  // ===================================================

  else if (
    action == 4
  )
  {
    if (
      !lockData()
    )
    {
      Blynk.virtualWrite(
        V12,
        "Database busy"
      );

      return;
    }

    if (
      !users[index].exists
    )
    {
      unlockData();

      Blynk.virtualWrite(
        V12,
        "User does not exist"
      );
    }

    else
    {
      char userName[
        MAX_NAME_LEN + 1
      ];

      copyText(
        userName,
        sizeof(userName),
        users[index].name.c_str()
      );

      users[index].active =
        false;

      saveUserLocked(
        index
      );

      unlockData();

      rfidEnrollmentMode =
        false;

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      enrollmentTimerActive =
        false;

      enrollmentStartTime =
        0;

      Blynk.virtualWrite(
        V12,
        "User blocked"
      );

      showUser(
        index
      );

      queueLCDMessage(
        "USER BLOCKED",
        userName,
        LCD_MESSAGE_TIME
      );

      queueTelegramUserEvent(
        TELEGRAM_EVENT_USER_BLOCKED,
        userName,
        index + 1
      );

      Serial.print(
        "[USER] BLOCKED: "
      );

      Serial.println(
        userName
      );
    }
  }


  // ===================================================
  // UNBLOCK
  // ===================================================

  else if (
    action == 5
  )
  {
    if (
      !lockData()
    )
    {
      Blynk.virtualWrite(
        V12,
        "Database busy"
      );

      return;
    }

    if (
      !users[index].exists
    )
    {
      unlockData();

      Blynk.virtualWrite(
        V12,
        "User does not exist"
      );
    }

    else
    {
      char userName[
        MAX_NAME_LEN + 1
      ];

      copyText(
        userName,
        sizeof(userName),
        users[index].name.c_str()
      );

      users[index].active =
        true;

      saveUserLocked(
        index
      );

      unlockData();

      Blynk.virtualWrite(
        V12,
        "User unblocked"
      );

      showUser(
        index
      );

      queueLCDMessage(
        "USER UNBLOCKED",
        userName,
        LCD_MESSAGE_TIME
      );

      queueTelegramUserEvent(
        TELEGRAM_EVENT_USER_UNBLOCKED,
        userName,
        index + 1
      );

      Serial.print(
        "[USER] UNBLOCKED: "
      );

      Serial.println(
        userName
      );
    }
  }


  // ===================================================
  // DELETE
  // ===================================================

  else if (
    action == 6
  )
  {
    if (
      !lockData()
    )
    {
      Blynk.virtualWrite(
        V12,
        "Database busy"
      );

      return;
    }

    if (
      !users[index].exists
    )
    {
      unlockData();

      Blynk.virtualWrite(
        V12,
        "User does not exist"
      );
    }

    else
    {
      char deletedName[
        MAX_NAME_LEN + 1
      ];

      copyText(
        deletedName,
        sizeof(deletedName),
        users[index].name.c_str()
      );

      users[index].exists =
        false;

      users[index].name =
        "";

      users[index].rfidUID =
        "";

      users[index].fingerprintID =
        -1;

      users[index].active =
        false;

      saveUserLocked(
        index
      );

      unlockData();

      updateRegisteredUserCount();

      rfidEnrollmentMode =
        false;

      fingerprintEnrollmentMode =
        false;

      fpEnrollState =
        FP_IDLE;

      enrollingFingerprintID =
        -1;

      enrollmentTimerActive =
        false;

      enrollmentStartTime =
        0;

      Blynk.virtualWrite(
        V12,
        "User deleted"
      );

      showUser(
        index
      );

      queueLCDMessage(
        "USER DELETED",
        deletedName,
        LCD_MESSAGE_TIME
      );

      queueTelegramUserEvent(
        TELEGRAM_EVENT_USER_DELETED,
        deletedName,
        index + 1
      );

      Serial.print(
        "[USER] DELETED: "
      );

      Serial.println(
        deletedName
      );
    }
  }

  else
  {
    Blynk.virtualWrite(
      V12,
      "Invalid action"
    );
  }

  Blynk.virtualWrite(
    V8,
    0
  );
}


// =====================================================
// TELEGRAM HTML ESCAPE
// =====================================================

String htmlEscape(
  const char* text
)
{
  String result;

  result.reserve(
    100
  );

  if (
    text == nullptr
  )
  {
    return result;
  }

  for (
    size_t i = 0;
    text[i] != '\0';
    i++
  )
  {
    switch (
      text[i]
    )
    {
      case '&':
        result += "&amp;";
        break;

      case '<':
        result += "&lt;";
        break;

      case '>':
        result += "&gt;";
        break;

      case '"':
        result += "&quot;";
        break;

      default:
        result += text[i];
        break;
    }
  }

  return result;
}


// =====================================================
// TELEGRAM USERS
// =====================================================

String getTelegramUsers()
{
  UserSnapshot snapshots[
    MAX_USERS
  ];

  copyAllUserSnapshots(
    snapshots
  );

  String message;

  message.reserve(
    1200
  );

  message +=
    "ðŸ  PTA SMART DOOR\n\n";

  message +=
    "ðŸ‘¥ USER DATABASE\n";

  message +=
    "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n\n";

  int registered =
    getRegisteredUserCount();

  message +=
    "Registered: ";

  message +=
    String(
      registered
    );

  message +=
    "/";

  message +=
    String(
      MAX_USERS
    );

  message +=
    "\n\n";

  for (
    int i = 0;
    i < MAX_USERS;
    i++
  )
  {
    message +=
      "User ";

    message +=
      String(
        i + 1
      );

    message +=
      "\n";

    if (
      !snapshots[i].exists
    )
    {
      message +=
        "Status: EMPTY\n\n";

      continue;
    }

    message +=
      "Name: ";

    message +=
      htmlEscape(
        snapshots[i].name
      );

    message +=
      "\n";

    message +=
      "RFID: ";

    if (
      snapshots[i].rfidUID[0] ==
      '\0'
    )
    {
      message +=
        "-";
    }
    else
    {
      message +=
        "<code>";

      message +=
        snapshots[i].rfidUID;

      message +=
        "</code>";
    }

    message +=
      "\n";

    message +=
      "Fingerprint: ";

    if (
      snapshots[i].fingerprintID ==
      -1
    )
    {
      message +=
        "-";
    }
    else
    {
      message +=
        "ID ";

      message +=
        String(
          snapshots[i].fingerprintID
        );
    }

    message +=
      "\n";

    message +=
      "Status: <b>";

    message +=
      snapshots[i].active
        ? "ACTIVE"
        : "BLOCKED";

    message +=
      "</b>\n\n";
  }

  return message;
}


// =====================================================
// TELEGRAM LOGS
// =====================================================

String getTelegramLogs()
{
  String message;

  message.reserve(
    1000
  );

  message +=
    "ðŸ  PTA SMART DOOR\n\n";

  message +=
    "ðŸ“‹ ACCESS LOG\n";

  message +=
    "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n\n";

  if (
    !lockData()
  )
  {
    message +=
      "Unable to read log.";

    return message;
  }

  if (
    accessLogCount == 0
  )
  {
    unlockData();

    message +=
      "No access log yet.";

    return message;
  }

  for (
    int i = 0;
    i < accessLogCount;
    i++
  )
  {
    message +=
      String(
        i + 1
      );

    message +=
      ". ";

    message +=
      htmlEscape(
        accessLogs[i].c_str()
      );

    message +=
      "\n";
  }

  unlockData();

  return message;
}


// =====================================================
// TELEGRAM STATUS
// =====================================================

String getTelegramStatus()
{
  bool currentDoorState;

  int currentAccessCount;

  int registeredUsers;

  if (
    !lockData()
  )
  {
    return
      "Unable to read system status.";
  }

  currentDoorState =
    doorUnlocked;

  currentAccessCount =
    accessCount;

  registeredUsers =
    getRegisteredUserCountLocked();

  unlockData();

  String message;

  message.reserve(
    350
  );

  message +=
    "ðŸ  PTA SMART DOOR\n\n";

  message +=
    "ðŸ“Š SYSTEM STATUS\n";

  message +=
    "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n\n";

  message +=
    "ðŸšª Door: ";

  message +=
    currentDoorState
      ? "UNLOCKED"
      : "LOCKED";

  message +=
    "\n";

  message +=
    "ðŸ‘¥ Users: ";

  message +=
    String(
      registeredUsers
    );

  message +=
    "/";

  message +=
    String(
      MAX_USERS
    );

  message +=
    "\n";

  message +=
    "ðŸ“ˆ Access Count: ";

  message +=
    String(
      currentAccessCount
    );

  message +=
    "\n";

  message +=
    "ðŸ“¡ WiFi: ";

  message +=
    WiFi.status() ==
    WL_CONNECTED
      ? "CONNECTED"
      : "DISCONNECTED";

  message +=
    "\n";

  message +=
    "â˜ï¸ Blynk: ";

  message +=
    Blynk.connected()
      ? "CONNECTED"
      : "OFFLINE";

  message +=
    "\n";

  message +=
    "ðŸ• Time: ";

  message +=
    getCurrentTime();

  return message;
}


// =====================================================
// SEND TELEGRAM
// =====================================================

void sendTelegram(
  const String& message
)
{
  if (
    WiFi.status() !=
    WL_CONNECTED
  )
  {
    return;
  }

  bool success =
    bot.sendMessage(
      TELEGRAM_CHAT_ID,
      message,
      "HTML"
    );

  if (
    !success
  )
  {
    Serial.println(
      "âš ï¸ Telegram send failed."
    );
  }
}


// =====================================================
// TELEGRAM ACCESS NOTIFICATION
// =====================================================

void telegramAccessNotification(
  const AccessEvent& event
)
{
  String message;

  message.reserve(
    700
  );

  if (
    strcmp(
      event.result,
      "Granted"
    ) == 0
  )
  {
    message +=
      "ðŸ  PTA SMART DOOR\n\n";

    message +=
      "ðŸŸ¢ <b>ACCESS GRANTED</b>\n\n";

    message +=
      "ðŸ‘¤ User: <b>";

    message +=
      htmlEscape(
        event.user
      );

    message +=
      "</b>\n";

    message +=
      "ðŸ” Method: <b>";

    message +=
      htmlEscape(
        event.method
      );

    message +=
      "</b>\n";

    if (
      strcmp(
        event.method,
        "RFID"
      ) == 0
    )
    {
      message +=
        "ðŸ†” UID: <code>";

      message +=
        event.credential;

      message +=
        "</code>\n";
    }

    else if (
      strcmp(
        event.method,
        "Fingerprint"
      ) == 0
    )
    {
      message +=
        "ðŸ†” ID: <code>";

      message +=
        event.credential;

      message +=
        "</code>\n";
    }

    message +=
      "ðŸ• Time: <b>";

    message +=
      event.timestamp;

    message +=
      "</b>\n";

    message +=
      "ðŸšª Door: <b>UNLOCKED</b>\n";

    message +=
      "â±ï¸ Auto-lock: <b>5 seconds</b>\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n";

    message +=
      "âœ… Authentication successful\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”";
  }

  else
  {
    message +=
      "ðŸ  PTA SMART DOOR\n\n";

    message +=
      "ðŸ”´ <b>ACCESS DENIED</b>\n\n";

    message +=
      "âš ï¸ Authentication failed\n\n";

    message +=
      "ðŸ” Method: <b>";

    message +=
      htmlEscape(
        event.method
      );

    message +=
      "</b>\n";

    if (
      strcmp(
        event.method,
        "RFID"
      ) == 0
    )
    {
      message +=
        "ðŸ†” UID: <code>";

      message +=
        event.credential;

      message +=
        "</code>\n";
    }

    else if (
      strcmp(
        event.method,
        "Fingerprint"
      ) == 0
    )
    {
      message +=
        "ðŸ‘† Method: <b>Fingerprint</b>\n";
    }

    message +=
      "ðŸ• Time: <b>";

    message +=
      event.timestamp;

    message +=
      "</b>\n";

    if (
      event.reason[0] != '\0'
    )
    {
      message +=
        "âš ï¸ Reason: <b>";

      message +=
        htmlEscape(
          event.reason
        );

      message +=
        "</b>\n";
    }

    message +=
      "\nâ”â”â”â”â”â”â”â”â”â”â”â”â”â”\n";

    message +=
      "ðŸš« Unauthorized access attempt\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”";
  }

  sendTelegram(
    message
  );
}


// =====================================================
// TELEGRAM USER NOTIFICATION
// =====================================================

void telegramUserNotification(
  const TelegramEvent& event
)
{
  String message;

  message.reserve(
    600
  );

  String userName =
    htmlEscape(
      event.userAction.name
    );

  String userID =
    String(
      event.userAction.userID
    );

  String timestamp =
    event.userAction.timestamp;

  if (
    event.type ==
    TELEGRAM_EVENT_USER_ADDED
  )
  {
    message +=
      "ðŸ‘¤ NEW USER ADDED\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n\n";

    message +=
      "ðŸ†• Name: <b>";

    message +=
      userName;

    message +=
      "</b>\n";

    message +=
      "ðŸ†” User ID: <b>";

    message +=
      userID;

    message +=
      "</b>\n\n";

    message +=
      "ðŸ” Access:\n";

    message +=
      "<b>RFID + Fingerprint</b>\n";

    message +=
      "ðŸ“± Added via: <b>Blynk</b>\n";

    message +=
      "ðŸ• Time: <b>";

    message +=
      timestamp;

    message +=
      "</b>\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n";

    message +=
      "âœ… User registered successfully\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”";
  }

  else if (
    event.type ==
    TELEGRAM_EVENT_USER_DELETED
  )
  {
    message +=
      "ðŸ—‘ï¸ USER DELETED\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n\n";

    message +=
      "ðŸ‘¤ Name: <b>";

    message +=
      userName;

    message +=
      "</b>\n";

    message +=
      "ðŸ†” User ID: <b>";

    message +=
      userID;

    message +=
      "</b>\n";

    message +=
      "ðŸ“± Action: <b>Blynk</b>\n";

    message +=
      "ðŸ• Time: <b>";

    message +=
      timestamp;

    message +=
      "</b>\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n";

    message +=
      "âœ… User removed successfully\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”";
  }

  else if (
    event.type ==
    TELEGRAM_EVENT_USER_BLOCKED
  )
  {
    message +=
      "ðŸš« USER BLOCKED\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n\n";

    message +=
      "ðŸ‘¤ Name: <b>";

    message +=
      userName;

    message +=
      "</b>\n";

    message +=
      "ðŸ†” User ID: <b>";

    message +=
      userID;

    message +=
      "</b>\n";

    message +=
      "ðŸ”’ Status: <b>BLOCKED</b>\n";

    message +=
      "ðŸ“± Action: <b>Blynk</b>\n";

    message +=
      "ðŸ• Time: <b>";

    message +=
      timestamp;

    message +=
      "</b>\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n";

    message +=
      "âš ï¸ User can no longer access the door\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”";
  }

  else if (
    event.type ==
    TELEGRAM_EVENT_USER_UNBLOCKED
  )
  {
    message +=
      "ðŸ”“ USER UNBLOCKED\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n\n";

    message +=
      "ðŸ‘¤ Name: <b>";

    message +=
      userName;

    message +=
      "</b>\n";

    message +=
      "ðŸ†” User ID: <b>";

    message +=
      userID;

    message +=
      "</b>\n";

    message +=
      "ðŸŸ¢ Status: <b>ACTIVE</b>\n";

    message +=
      "ðŸ“± Action: <b>Blynk</b>\n";

    message +=
      "ðŸ• Time: <b>";

    message +=
      timestamp;

    message +=
      "</b>\n\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”\n";

    message +=
      "âœ… User access restored\n";

    message +=
      "â”â”â”â”â”â”â”â”â”â”â”â”â”â”";
  }

  sendTelegram(
    message
  );
}


// =====================================================
// TELEGRAM COMMANDS
// =====================================================

void handleTelegramMessages(
  int numNewMessages
)
{
  for (
    int i = 0;
    i < numNewMessages;
    i++
  )
  {
    String chat_id =
      bot.messages[i].chat_id;

    String text =
      bot.messages[i].text;

    if (
      chat_id !=
      TELEGRAM_CHAT_ID
    )
    {
      bot.sendMessage(
        chat_id,
        "Access denied.",
        ""
      );

      continue;
    }

    Serial.print(
      "[TELEGRAM] Command: "
    );

    Serial.println(
      text
    );

    if (
      text == "/start"
    )
    {
      String welcome;

      welcome.reserve(
        450
      );

      welcome +=
        "ðŸ  <b>PTA SMART DOOR</b>\n\n";

      welcome +=
        "ðŸ“± Telegram monitoring aktif.\n\n";

      welcome +=
        "<b>Commands:</b>\n";

      welcome +=
        "/users - Senarai user\n";

      welcome +=
        "/logs - Access log\n";

      welcome +=
        "/status - Status sistem\n";

      bot.sendMessage(
        chat_id,
        welcome,
        "HTML"
      );
    }

    else if (
      text == "/users"
    )
    {
      bot.sendMessage(
        chat_id,
        getTelegramUsers(),
        "HTML"
      );
    }

    else if (
      text == "/logs"
    )
    {
      bot.sendMessage(
        chat_id,
        getTelegramLogs(),
        "HTML"
      );
    }

    else if (
      text == "/status"
    )
    {
      bot.sendMessage(
        chat_id,
        getTelegramStatus(),
        "HTML"
      );
    }

    else
    {
      bot.sendMessage(
        chat_id,
        "â“ Command tidak dikenali.\n\nGunakan:\n/users\n/logs\n/status",
        ""
      );
    }
  }
}


// =====================================================
// PROCESS BLYNK EVENT
// =====================================================

void processBlynkEvent(
  const BlynkEvent& event
)
{
  if (
    event.type ==
    BLYNK_EVENT_ACCESS
  )
  {
    const AccessEvent& access =
      event.access;

    bool granted =
      strcmp(
        access.result,
        "Granted"
      ) == 0;

    if (
      Blynk.connected()
    )
    {
      Blynk.virtualWrite(
        V0,
        granted
          ? "Door Unlocked"
          : "Door Locked"
      );

      Blynk.virtualWrite(
        V1,
        access.user
      );

      Blynk.virtualWrite(
        V2,
        access.method
      );

      Blynk.virtualWrite(
        V3,
        access.result
      );

      int count =
        0;

      if (
        lockData()
      )
      {
        count =
          accessCount;

        unlockData();
      }

      Blynk.virtualWrite(
        V4,
        count
      );

      String display =
        buildAccessLogDisplay();

      if (
        display.length() == 0
      )
      {
        Blynk.virtualWrite(
          V13,
          "No access log yet"
        );
      }
      else
      {
        Blynk.virtualWrite(
          V13,
          display
        );
      }
    }

    return;
  }

  if (
    event.type ==
    BLYNK_EVENT_DOOR
  )
  {
    if (
      Blynk.connected()
    )
    {
      Blynk.virtualWrite(
        V0,
        event.doorState
          ? "Door Unlocked"
          : "Door Locked"
      );
    }

    return;
  }

  if (
    event.type ==
    BLYNK_EVENT_STATUS
  )
  {
    if (
      Blynk.connected()
    )
    {
      Blynk.virtualWrite(
        V12,
        event.text
      );
    }

    return;
  }

  if (
    event.type ==
    BLYNK_EVENT_SHOW_USER
  )
  {
    showUser(
      event.userIndex
    );

    return;
  }
}


// =====================================================
// BLYNK V9
// MANUAL WIFI SELECTION
// =====================================================
// V9 value:
//   0 = AUTO
//   1 = WiFi slot 1
//   2 = WiFi slot 2
//   3 = WiFi slot 3
//   4 = WiFi slot 4
//   5 = WiFi slot 5
// =====================================================

BLYNK_WRITE(V9)
{
  int selection =
    param.asInt();

  if (
    selection < 0 ||
    selection > WIFI_NETWORK_COUNT
  )
  {
    Blynk.virtualWrite(
      V9,
      wifiManualSelection
        ? currentWiFiIndex + 1
        : 0
    );

    return;
  }

  // AUTO mode: let the normal multi-WiFi manager choose.
  if (selection == 0)
  {
    wifiManualSelection = false;

    Serial.println(
      "â˜ï¸ BLYNK WIFI: AUTO MODE"
    );

    Blynk.virtualWrite(
      V12,
      "WiFi: AUTO mode"
    );

    return;
  }

  uint8_t requestedIndex =
    selection - 1;

  if (
    !hasConfiguredWiFi(requestedIndex)
  )
  {
    Serial.print(
      "âš ï¸ BLYNK WIFI: Slot "
    );

    Serial.print(
      selection
    );

    Serial.println(
      " is not configured."
    );

    Blynk.virtualWrite(
      V12,
      "WiFi slot not configured"
    );

    Blynk.virtualWrite(
      V9,
      currentWiFiIndex + 1
    );

    return;
  }

  wifiManualSelection = true;
  wifiManualIndex = requestedIndex;

  Serial.print(
    "â˜ï¸ BLYNK WIFI: MANUAL â†’ ["
  );

  Serial.print(
    selection
  );

  Serial.print(
    "] "
  );

  Serial.println(
    wifiSSIDs[requestedIndex]
  );

  // If already on this AP, do not unnecessarily disconnect it.
  if (
    WiFi.status() == WL_CONNECTED &&
    currentWiFiIndex == requestedIndex
  )
  {
    Blynk.virtualWrite(
      V12,
      String("WiFi: ") +
      wifiSSIDs[requestedIndex]
    );

    return;
  }

  startWiFiConnection(
    requestedIndex
  );

  Blynk.virtualWrite(
    V12,
    String("WiFi: trying ") +
    wifiSSIDs[requestedIndex]
  );
}


// =====================================================
// BLYNK CONNECTED
// =====================================================

BLYNK_CONNECTED()
{
  Serial.println(
    "â˜ï¸ BLYNK: CONNECTED"
  );

  bool currentDoor =
    false;

  int currentCount =
    0;

  if (
    lockData()
  )
  {
    currentDoor =
      doorUnlocked;

    currentCount =
      accessCount;

    unlockData();
  }

  Blynk.virtualWrite(
    V0,
    currentDoor
      ? "Door Unlocked"
      : "Door Locked"
  );

  Blynk.virtualWrite(
    V1,
    "-"
  );

  Blynk.virtualWrite(
    V2,
    "-"
  );

  Blynk.virtualWrite(
    V3,
    "-"
  );

  Blynk.virtualWrite(
    V4,
    currentCount
  );

  Blynk.virtualWrite(
    V5,
    0
  );

  Blynk.virtualWrite(
    V9,
    wifiManualSelection
      ? currentWiFiIndex + 1
      : 0
  );

  Blynk.virtualWrite(
    V10,
    "Select User"
  );

  Blynk.virtualWrite(
    V11,
    "Select User ID"
  );

  Blynk.virtualWrite(
    V12,
    "System Ready"
  );

  String display =
    buildAccessLogDisplay();

  if (
    display.length() == 0
  )
  {
    Blynk.virtualWrite(
      V13,
      "No access log yet"
    );
  }
  else
  {
    Blynk.virtualWrite(
      V13,
      display
    );
  }

  updateRegisteredUserCount();
}


// =====================================================
// MULTI-WIFI HELPERS
// =====================================================

bool hasConfiguredWiFi(uint8_t index)
{
  if (index >= WIFI_NETWORK_COUNT)
  {
    return false;
  }

  return (
    wifiSSIDs[index] != nullptr &&
    wifiSSIDs[index][0] != '\0'
  );
}

void startWiFiConnection(uint8_t index)
{
  if (!hasConfiguredWiFi(index))
  {
    return;
  }

  currentWiFiIndex = index;
  wifiAttemptStarted = millis();

  Serial.print("ðŸ“¡ WiFi: TRYING [");
  Serial.print(index + 1);
  Serial.print("] ");
  Serial.println(wifiSSIDs[index]);

  // Fully stop the previous STA connection before starting another AP.
  // persistent(false) above prevents this from rewriting flash credentials.
  WiFi.disconnect(true, false);
  delay(80);

  WiFi.mode(WIFI_STA);
  WiFi.begin(
    wifiSSIDs[index],
    wifiPasswords[index]
  );
}

void tryNextWiFiNetwork()
{
  if (WIFI_NETWORK_COUNT == 0)
  {
    return;
  }

  for (uint8_t offset = 1; offset <= WIFI_NETWORK_COUNT; offset++)
  {
    uint8_t nextIndex =
      (currentWiFiIndex + offset) % WIFI_NETWORK_COUNT;

    if (hasConfiguredWiFi(nextIndex))
    {
      startWiFiConnection(nextIndex);
      return;
    }
  }

  // Only one configured network exists.
  if (hasConfiguredWiFi(currentWiFiIndex))
  {
    startWiFiConnection(currentWiFiIndex);
  }
}


// =====================================================
// BLYNK TASK
// CORE 0
// =====================================================

void blynkTask(
  void* parameter
)
{
  bool lastWiFiState =
    false;

  bool lastBlynkState =
    false;

  bool stateInitialized =
    false;

  for (;;)
  {
    unsigned long now =
      millis();

    bool wifiConnected =
      WiFi.status() ==
      WL_CONNECTED;

    bool blynkConnected =
      Blynk.connected();

    if (
      !stateInitialized
    )
    {
      lastWiFiState =
        wifiConnected;

      lastBlynkState =
        blynkConnected;

      stateInitialized =
        true;

      if (
        wifiConnected
      )
      {
        Serial.println(
          "ðŸ“¡ WIFI: CONNECTED"
        );
      }
      else
      {
        Serial.println(
          "ðŸ“¡ WIFI: DISCONNECTED"
        );

        Serial.println(
          "âš ï¸ CLOUD SERVICES OFFLINE"
        );

        Serial.println(
          "ðŸŸ¢ LOCAL ACCESS: ACTIVE"
        );
      }
    }
    else
    {
      if (
        wifiConnected !=
        lastWiFiState
      )
      {
        lastWiFiState =
          wifiConnected;

        if (
          wifiConnected
        )
        {
          Serial.println(
            "ðŸ“¡ WIFI: RECONNECTED"
          );

          if (
            Blynk.connected()
          )
          {
            Blynk.virtualWrite(
              V9,
              currentWiFiIndex + 1
            );
          }
        }
        else
        {
          Serial.println(
            "ðŸ“¡ WIFI: DISCONNECTED"
          );

          Serial.println(
            "âš ï¸ CLOUD SERVICES OFFLINE"
          );

          Serial.println(
            "ðŸŸ¢ LOCAL ACCESS: ACTIVE"
          );

          telegramOnlineSent =
            false;
        }
      }

      if (
        blynkConnected !=
        lastBlynkState
      )
      {
        lastBlynkState =
          blynkConnected;

        if (
          blynkConnected
        )
        {
          Serial.println(
            "â˜ï¸ BLYNK: CONNECTED"
          );
        }
        else
        {
          Serial.println(
            "â˜ï¸ BLYNK: OFFLINE"
          );
        }
      }
    }

    if (
      !wifiConnected
    )
    {
      // One connection attempt owns the radio until its timeout.
      // When it fails, immediately rotate to the next configured SSID.
      bool attemptTimedOut =
        (now - wifiAttemptStarted) >=
        WIFI_NETWORK_TIMEOUT;

      if (attemptTimedOut)
      {
        lastWiFiReconnectAttempt = now;

        Serial.print("ðŸ“¡ WiFi: TIMEOUT [");
        Serial.print(currentWiFiIndex + 1);
        Serial.print("] ");
        Serial.println(wifiSSIDs[currentWiFiIndex]);

        if (
          wifiManualSelection
        )
        {
          Serial.println(
            "ðŸ“¡ WiFi: MANUAL MODE â†’ RETRY SELECTED NETWORK"
          );

          startWiFiConnection(
            wifiManualIndex
          );
        }
        else
        {
          Serial.println(
            "ðŸ“¡ WiFi: MOVING TO NEXT NETWORK..."
          );

          tryNextWiFiNetwork();

          if (
            Blynk.connected()
          )
          {
            Blynk.virtualWrite(
              V9,
              currentWiFiIndex + 1
            );
          }
        }
      }

      vTaskDelay(
        pdMS_TO_TICKS(50)
      );

      continue;
    }

    if (
      !Blynk.connected()
    )
    {
      if (
        now -
        lastBlynkConnectAttempt >=
        BLYNK_RECONNECT_INTERVAL
      )
      {
        lastBlynkConnectAttempt =
          now;

        Blynk.connect(
          1000
        );
      }
    }

    if (
      blynkEventQueue != nullptr
    )
    {
      BlynkEvent event;

      while (
        xQueueReceive(
          blynkEventQueue,
          &event,
          0
        ) == pdTRUE
      )
      {
        processBlynkEvent(
          event
        );
      }
    }

    if (
      Blynk.connected()
    )
    {
      Blynk.run();
    }

    vTaskDelay(
      pdMS_TO_TICKS(10)
    );
  }
}


// =====================================================
// TELEGRAM TASK
// CORE 0
// =====================================================

void telegramTask(
  void* parameter
)
{
  for (;;)
  {
    if (
      WiFi.status() !=
      WL_CONNECTED
    )
    {
      vTaskDelay(
        pdMS_TO_TICKS(100)
      );

      continue;
    }

    if (
      !telegramOnlineSent
    )
    {
      sendTelegram(
        String(
          "ðŸ  <b>PTA SMART DOOR</b>\n\n"
          "ðŸŸ¢ <b>TELEGRAM ONLINE</b>\n\n"
          "ðŸ“± Monitoring service is active.\n"
          "ðŸ• Time: <b>"
        ) +
        getCurrentTime() +
        "</b>"
      );

      telegramOnlineSent =
        true;
    }

    if (
      telegramEventQueue != nullptr
    )
    {
      TelegramEvent event;

      while (
        xQueueReceive(
          telegramEventQueue,
          &event,
          0
        ) == pdTRUE
      )
      {
        if (
          event.type ==
          TELEGRAM_EVENT_ACCESS
        )
        {
          telegramAccessNotification(
            event.access
          );
        }

        else
        {
          telegramUserNotification(
            event
          );
        }
      }
    }

    unsigned long now =
      millis();

    if (
      now -
      lastTelegramCheck >=
      TELEGRAM_CHECK_INTERVAL
    )
    {
      lastTelegramCheck =
        now;

      int numNewMessages =
        bot.getUpdates(
          bot.last_message_received + 1
        );

      if (
        numNewMessages > 0
      )
      {
        handleTelegramMessages(
          numNewMessages
        );
      }
    }

    vTaskDelay(
      pdMS_TO_TICKS(50)
    );
  }
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(
    115200
  );

  delay(
    100
  );

  Serial.println();
  Serial.println(
    "================================================"
  );

  Serial.println(
    "ðŸ  PTA SMART DOOR"
  );

  Serial.println(
    "ðŸš€ FINAL POLISH / EXHIBITION BUILD V2.2"
  );

  Serial.println(
    "================================================"
  );


  // ================================================
  // MUTEX
  // ================================================

  dataMutex =
    xSemaphoreCreateMutex();

  if (
    dataMutex == nullptr
  )
  {
    Serial.println(
      "âŒ ERROR: Mutex creation failed"
    );

    while (true)
    {
      delay(
        1000
      );
    }
  }


  // ================================================
  // QUEUES
  // ================================================

  blynkEventQueue =
    xQueueCreateStatic(
      EVENT_QUEUE_LENGTH,
      sizeof(BlynkEvent),
      blynkQueueStorage,
      &blynkQueueStruct
    );

  telegramEventQueue =
    xQueueCreateStatic(
      EVENT_QUEUE_LENGTH,
      sizeof(TelegramEvent),
      telegramQueueStorage,
      &telegramQueueStruct
    );

  hardwareCommandQueue =
    xQueueCreateStatic(
      12,
      sizeof(HardwareCommand),
      hardwareQueueStorage,
      &hardwareQueueStruct
    );

  if (
    blynkEventQueue == nullptr ||
    telegramEventQueue == nullptr ||
    hardwareCommandQueue == nullptr
  )
  {
    Serial.println(
      "âŒ ERROR: Queue creation failed"
    );

    while (true)
    {
      delay(
        1000
      );
    }
  }


  // ================================================
  // PREFERENCES
  // ================================================

  preferences.begin(
    "smartdoor",
    false
  );

  loadAllUsers();


  // ================================================
  // STRING RESERVATION
  // ================================================

  selectedName.reserve(
    MAX_NAME_LEN
  );

  for (
    int i = 0;
    i < MAX_LOGS;
    i++
  )
  {
    accessLogs[i].reserve(
      130
    );
  }


  // ================================================
  // LCD
  // ================================================

  Wire.begin(
    21,
    22
  );

  Wire.setClock(
    400000
  );

  lcd.init();

  lcd.backlight();

  lcdShow(
    "SMART DOOR",
    "STARTING..."
  );


  // ================================================
  // OUTPUT
  // ================================================

  pinMode(
    GREEN_LED,
    OUTPUT
  );

  pinMode(
    RED_LED,
    OUTPUT
  );

  pinMode(
    RELAY_PIN,
    OUTPUT
  );

  pinMode(
    TOUCH_PIN,
    INPUT
  );


  // ================================================
  // SAFE STATE
  // ================================================

  digitalWrite(
    GREEN_LED,
    LOW
  );

  digitalWrite(
    RED_LED,
    LOW
  );

  digitalWrite(
    RELAY_PIN,
    LOW
  );

  doorUnlocked =
    false;


  // ================================================
  // BUZZER
  // ================================================

  ledcAttach(
    BUZZER,
    2000,
    8
  );

  ledcWrite(
    BUZZER,
    0
  );


  // ================================================
  // HARDWARE STARTUP SETTLING
  // ================================================

  delay(
    250
  );


  // ================================================
  // RFID
  // ================================================

  if (
    initializeRFID()
  )
  {
    Serial.println(
      "ðŸ’³ RFID: READY"
    );
  }
  else
  {
    Serial.println(
      "âŒ RFID: INIT FAILED"
    );

    Serial.println(
      "âš ï¸ RFID recovery enabled."
    );
  }


  // ================================================
  // FINGERPRINT
  // ================================================

  startFingerprintInitialization();


  // ================================================
  // WIFI
  // ================================================

  WiFi.mode(
    WIFI_STA
  );

  WiFi.setSleep(
    false
  );

  // Custom multi-WiFi manager owns reconnect/rotation.
  // Do not let the ESP32 reconnect to the previous AP behind our back.
  WiFi.setAutoReconnect(
    false
  );

  WiFi.persistent(
    false
  );

  Serial.println(
    "ðŸ“¡ WiFi: STARTING MULTI-NETWORK MODE..."
  );

  bool wifiConfigured = false;

  for (uint8_t i = 0; i < WIFI_NETWORK_COUNT; i++)
  {
    if (hasConfiguredWiFi(i))
    {
      wifiConfigured = true;
      currentWiFiIndex = i;
      break;
    }
  }

  if (wifiConfigured)
  {
    // Ensure the first attempt gets its own timeout window.
    lastWiFiReconnectAttempt = millis();
    startWiFiConnection(currentWiFiIndex);
  }
  else
  {
    Serial.println(
      "âš ï¸ WiFi: NO NETWORK CONFIGURED"
    );
  }


  // ================================================
  // NTP
  // ================================================

  configTime(
    gmtOffset_sec,
    daylightOffset_sec,
    ntpServer
  );

  Serial.println(
    "ðŸ• NTP: CONFIGURED"
  );


  // ================================================
  // BLYNK
  // ================================================

  Blynk.config(
    BLYNK_AUTH_TOKEN
  );

  Serial.println(
    "â˜ï¸ Blynk: CONFIGURED"
  );


  // ================================================
  // TELEGRAM
  // ================================================

  telegramClient.setInsecure();

  // Keep Telegram requests responsive without blocking the system for long.
  telegramClient.setTimeout(
    300
  );

  bot.waitForResponse =
    500;

  Serial.println(
    "ðŸ“± Telegram: READY"
  );


  // ================================================
  // BACKGROUND TASKS
  // ================================================

  xTaskCreatePinnedToCore(
    blynkTask,
    "BlynkTask",
    6144,
    nullptr,
    1,
    &blynkTaskHandle,
    0
  );

  // Telegram uses WiFiClientSecure and Telegram Bot API calls.
  // Keep it on Core 1 at LOW priority so a blocking TLS/network
  // operation can never starve Core 0 IDLE and trigger the ESP32
  // task watchdog. The hardware loopTask remains higher priority.
  xTaskCreatePinnedToCore(
    telegramTask,
    "TelegramTask",
    8192,
    nullptr,
    0,
    &telegramTaskHandle,
    1
  );


  // ================================================
  // FINAL READY
  // ================================================

  showReadyScreen();

  Serial.println();
  Serial.println(
    "================================================"
  );

  Serial.println(
    "ðŸŸ¢ SYSTEM READY"
  );

  Serial.println(
    "================================================"
  );

  Serial.println(
    "ðŸ’³ RFID       : STARTED"
  );

  Serial.println(
    "ðŸ‘† FINGERPRINT: STARTING HANDSHAKE"
  );

  Serial.println(
    "ðŸ”˜ EXIT TOUCH : READY"
  );

  Serial.println(
    "ðŸšª DOOR LOCK  : READY"
  );

  Serial.println(
    "ðŸ“º LCD        : READY"
  );

  Serial.println(
    "ðŸ“¡ WIFI       : BACKGROUND"
  );

  Serial.println(
    "â˜ï¸ BLYNK      : BACKGROUND"
  );

  Serial.println(
    "ðŸ“± TELEGRAM   : BACKGROUND"
  );

  Serial.println(
    "================================================"
  );

  Serial.println(
    "SCAN RFID OR FINGER"
  );

  Serial.print(
    "Free Heap: "
  );

  Serial.println(
    ESP.getFreeHeap()
  );

  Serial.println();
}


// =====================================================
// LOOP
// =====================================================
//
// CORE 1
// HARDWARE ONLY
//
// NO BLYNK.RUN()
// NO TELEGRAM
// NO HTTP
// NO NETWORK
//
// =====================================================

void loop()
{
  // 0. Global enrollment timeout. Run before reader-specific processing.
  updateEnrollmentTimeout();

  // 1. Hardware commands from Blynk
  processHardwareCommands();

  // 2. Door auto-lock timer
  updateDoor();

  // 3. Buzzer
  updateBuzzer();

  // 4. Denied animation
  updateDenied();

  // 5. LCD timer
  updateLCD();

  // 6. RFID health
  updateRFIDHealth();

  // 7. RFID enrollment
  checkRFIDEnrollment();

  // 8. RFID authentication
  checkRFID();

  // 9. Fingerprint startup / recovery
  updateFingerprintInitialization();

  // 10. Fingerprint enrollment
  checkFingerprintEnrollment();

  // 11. Fingerprint authentication
  checkFingerprint();

  // 12. Touch exit
  checkTouch();

  // Small cooperative yield.
  // Prevents Core 1 from running an unnecessarily
  // aggressive empty loop while preserving response.
  delay(2);
}
