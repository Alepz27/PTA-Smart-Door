# PTA Smart Door

PTA Smart Door is an ESP32-based access-control prototype developed for engineering learning and academic demonstration. It combines RFID and fingerprint authentication, touch-to-exit operation, local user records, physical status feedback, relay-controlled door locking, and optional Blynk and Telegram services.

> **Project scope:** This is a functional academic prototype. It is not a certified or production-ready security system and must not be used as the sole access-control measure for an occupied building.

## Project Overview

| Area | Current implementation |
| --- | --- |
| Main controller | ESP32 DevKit V1 |
| Entry methods | RC522 RFID and AS608 fingerprint authentication |
| Exit method | TTP223 touch sensor |
| Lock actuation | 5 V active-low relay controlling a 12 V *fail-secure* solenoid lock |
| Local feedback | LCD1602 I2C, green and red LEDs, and an active buzzer |
| User storage | Up to 50 local user slots using ESP32 Preferences |
| Connected services | Blynk for control and status; Telegram for monitoring |
| Offline operation | Local authentication remains available without cloud connectivity |
| Published evidence | Public-safe firmware, Proteus RFID simulation, UID test matrix, and wiring diagram |

## Problem Statement

A conventional mechanical lock does not provide electronic identity verification, event feedback, or connected monitoring. This project investigates how one microcontroller can coordinate several access methods and a physical lock while preserving essential local operation when Internet services are unavailable.

## Objectives

- Integrate RFID, fingerprint, and touch-exit inputs on one ESP32.
- Make access decisions locally rather than relying on a cloud service.
- Provide clear status feedback through the LCD, LEDs, and buzzer.
- Control a *fail-secure* solenoid lock through a relay and relock it automatically.
- Support optional monitoring and user-management functions through Blynk and Telegram.
- Recover from temporary network or reader faults without intentionally restarting the ESP32.
- Document the design, evidence, and limitations clearly for technical review.

## Current Scope and Proposed Development

The two columns below deliberately separate implemented functions from proposed work.

| Implemented in the published prototype | Proposed for Smart Door V2 |
| --- | --- |
| RFID and fingerprint authentication | Revised magnetic-lock driver stage |
| TTP223 touch-to-exit input | Door-position sensing using a reed/contact sensor |
| Local user records in ESP32 Preferences | Forced-entry and door-left-open detection |
| LCD1602, LED, and buzzer feedback | Colour TFT/IPS interface |
| Automatic relocking after five seconds | NTP time with RTC backup |
| Blynk status, remote lock control, and user actions | Custom mobile application with role-based permissions |
| Telegram status, user, and recent-log queries | Authenticated OTA updates and device-health monitoring |
| Wi-Fi retry and reader-recovery logic | Tamper detection and optional CCTV event linkage |
| Ten recent access events held in RAM | Persistent, exportable audit storage |

Items in the right-hand column are proposals only. They are not part of the current build.

## System Architecture

| Layer | Elements | Responsibility |
| --- | --- | --- |
| Inputs | RC522, AS608, TTP223 | Capture entry credentials and exit requests |
| Controller | ESP32 access logic, user records, timers, and recovery states | Validate users and coordinate system behaviour |
| Outputs | LCD, LEDs, buzzer, relay, and solenoid lock | Communicate status and control the physical lock |
| Optional services | Blynk and Telegram | Provide remote management, status, and notifications |

The ESP32 makes access decisions locally. Blynk and Telegram extend management and visibility but are not required for RFID, fingerprint, or touch-exit operation.

## Operating Sequence

1. On start-up, the firmware sets the relay output to `LOW` and records the door state as locked.
2. The ESP32 loads saved users and initialises the LCD, SPI, UART, readers, and background network tasks.
3. The system waits for an RFID card, a fingerprint, or a touch-exit request.
4. RFID and fingerprint inputs are checked against active local user records. A valid touch input is treated as an internal exit request.
5. An authorised request activates the relay, green LED, buzzer, and LCD feedback.
6. After approximately five seconds, `updateDoor()` returns the relay to `LOW` without blocking the main control loop.
7. A denied request leaves the lock engaged and activates the denial feedback sequence.

### Access Outcomes

| Condition | Lock response | Feedback and record |
| --- | --- | --- |
| Active RFID user | Unlocks and then relocks | User and status on LCD; green LED and buzzer; event added to the recent log |
| Active fingerprint user | Unlocks and then relocks | User and status on LCD; green LED and buzzer; event added to the recent log |
| Touch-exit request | Unlocks and then relocks | Exit feedback; event recorded as `Exit User` using the `Touch` method |
| Unknown credential or blocked user | Remains locked | Denial sequence on LCD, red LED, and buzzer; attempt added to the recent log |

### Timing Behaviour

| Function | Firmware value |
| --- | --- |
| Automatic relock | `DOOR_UNLOCK_TIME = 5000` ms |
| Touch-input debounce | `TOUCH_DEBOUNCE = 300` ms |
| Duplicate RFID suppression | `RFID_REPEAT_BLOCK_TIME = 1200` ms |
| Enrolment timeout | `ENROLLMENT_TIMEOUT = 30000` ms |
| Wi-Fi connection timeout | `WIFI_NETWORK_TIMEOUT = 10000` ms |

## Hardware Components

| Component | Role |
| --- | --- |
| ESP32 DevKit V1 | Main controller and network interface |
| RC522 | RFID reader connected through SPI |
| AS608 | Fingerprint reader connected through UART2 at 57,600 baud |
| TTP223 | Touch-based exit request |
| LCD1602 I2C | Local prompts and system status |
| 5 V active-low relay module | Electrical control interface for the lock circuit |
| 12 V *fail-secure* solenoid lock | Mechanical locking actuator |
| Green and red LEDs | Access-granted and access-denied indicators |
| Active buzzer | Audible status feedback |
| LM2596 converter | Voltage step-down for the low-voltage electronics |
| 12 V adapter | Primary prototype supply |

## Verified ESP32 Pin Mapping

| Module | ESP32 connection |
| --- | --- |
| LCD1602 I2C | SDA GPIO21; SCL GPIO22; address `0x27` |
| RC522 RFID | SS/SDA GPIO5; RST GPIO2; SCK GPIO18; MOSI GPIO23; MISO GPIO19 |
| AS608 fingerprint | Sensor TX to ESP32 RX GPIO16; sensor RX to ESP32 TX GPIO17; `HardwareSerial(2)` at 57,600 baud |
| TTP223 touch exit | GPIO27 |
| Active-low relay | GPIO26 |
| Green LED | GPIO25 |
| Red LED | GPIO33 |
| Active buzzer | GPIO32 |

## Firmware Design

- **Hardware-first control loop:** Processes reader input, touch input, output timers, health checks, and queued hardware commands.
- **Local persistence:** Stores up to 50 user records using ESP32 Preferences.
- **State-based processing:** Uses bounded states for RFID handling, fingerprint initialisation, authentication, and enrolment.
- **Task coordination:** Uses FreeRTOS queues and a mutex to coordinate local state with Blynk and Telegram tasks.
- **Reader recovery:** Schedules RC522 reinitialisation and AS608 UART resynchronisation without intentionally rebooting the ESP32.
- **Safe start state:** Drives the relay `LOW` and sets `doorUnlocked` to `false` during `setup()`.

### User Records

Each local record contains a name, RFID UID, fingerprint-template ID, active flag, and existence flag. Record metadata is stored in ESP32 Preferences. Fingerprint templates remain in the AS608 sensor; the firmware stores only the associated template ID.

### Access Log

The firmware keeps the ten most recent formatted events in RAM for display through Blynk or Telegram. This record is not persistent across an ESP32 restart, is not an independent audit trail, and relies on network time for meaningful timestamps.

## Connected Services

### Blynk

The published firmware includes handlers for:

- manual unlock and lock commands;
- user-slot selection and status display;
- user creation, RFID or fingerprint enrolment, blocking, unblocking, and deletion;
- cancellation of an active enrolment process;
- automatic or manual Wi-Fi network selection; and
- door state, access count, system status, and recent-log display.

The Blynk widget and datastream configuration is not included. The dashboard therefore cannot be reproduced from the firmware alone.

### Telegram

The bot validates the configured chat ID before serving `/users`, `/logs`, and `/status`. It also receives queued access and user-management notifications.

The current firmware calls `telegramClient.setInsecure()`, which disables TLS certificate verification. This is a documented prototype limitation and must be corrected before security-sensitive deployment.

### Offline Operation

Local authentication does not depend on Blynk or Telegram. Background tasks rotate through configured Wi-Fi slots after a ten-second connection timeout and retry Blynk separately. Remote commands and cloud notifications are unavailable while the network is disconnected.

## Power and Electrical Notes

The 12 V adapter supplies the solenoid branch and the LM2596 converter. The converter provides the regulated low-voltage rail for the controller, sensors, and relay module.

The repository does not contain a measured power budget or a complete terminal-level electrical schematic. Before reproducing the prototype, verify:

- LM2596 output voltage;
- relay contact rating;
- solenoid current;
- flyback suppression;
- conductor size;
- shared ground/reference connections; and
- appropriate electrical isolation.

![Cirkit Designer wiring reference](docs/Smart-Door-Cirkit-Designer-Wiring.png)

*Figure 1. Existing Cirkit Designer wiring reference. This is not a certified electrical drawing or a measured as-built inspection.*

## Verification Evidence

| Evidence | What it supports | Evidence boundary |
| --- | --- | --- |
| [Published firmware](firmware/FULL_CODE_SmartDoor_PTA1.ino) | Control paths, timing values, pin assignments, persistence, and recovery logic | Source review does not prove that every path has been tested physically |
| [Proteus project](docs/Litar%20simulasi%20Smart%20Door.pdsprj) | Repeatable RFID-oriented simulation | Does not model the complete ESP32, AS608, and cloud-connected system |
| [Simulation guide](docs/PROTEUS_SIMULATION_GUIDE.md) | Repeatable demonstration procedure | Expected outcomes must not be presented as recorded test results |
| [UID test matrix](docs/PROTEUS_UID_TESTS.md) | Two authorised RFID cases and one unauthorised case | These are simulation inputs, not security-performance measurements |
| [Wiring diagram](docs/Smart-Door-Cirkit-Designer-Wiring.png) | Component-interconnection reference | Not a certified schematic or physical inspection record |

See [Testing and Evidence](docs/TESTING_AND_EVIDENCE.md) for the detailed verification matrix and the evidence that is still required.

## Reliability Boundaries

The published firmware contains recovery mechanisms for temporary RC522, AS608, Wi-Fi, and Blynk faults. The repository does not yet provide evidence for:

- controlled brownout or repeated power-interruption testing;
- measured recovery time for each fault type;
- long-duration soak testing; or
- physical lock-state verification for every power-supply failure mode.

## Security Considerations

- Public firmware uses placeholders instead of real Wi-Fi, Blynk, and Telegram credentials.
- RFID UID matching is susceptible to cloning and is not high-assurance authentication.
- Remote unlock depends on the security of the connected accounts, tokens, and authorised chat ID.
- The prototype has no door-position, forced-entry, or enclosure-tamper sensing.
- The in-memory access record is neither persistent nor tamper-resistant.
- Telegram TLS certificate verification is disabled in the current firmware.
- Fire, emergency-egress, electrical, and *fail-safe*/*fail-secure* requirements must be assessed before any real installation.

See [SECURITY.md](SECURITY.md) for public-repository and credential-handling guidance.

## Current Limitations

- No certification, formal threat assessment, or compliance review.
- No door-position sensor to confirm that the door has physically closed.
- No forced-entry, door-left-open, or enclosure-tamper detection.
- Access history is limited to ten RAM entries and is lost after restart.
- No exported Blynk dashboard configuration.
- No RTC-backed time source.
- No documented power budget, UPS protection, or controlled power-failure test.
- Proteus evidence covers an RFID demonstration rather than the complete integrated system.
- No privacy-reviewed prototype photographs or suitable public demonstration video.

## Smart Door V2 Roadmap

| Current limitation | Proposed improvement | Intended benefit |
| --- | --- | --- |
| Lock command is not confirmed by door position | Add a reed/contact sensor | Detect open, closed, and door-left-open states |
| No forced-entry signal | Compare door state with authorised unlock state | Identify unexpected opening |
| Limited character display | Add a colour TFT/IPS interface | Improve prompts and maintenance information |
| Network-dependent timestamps | Add an RTC synchronised through NTP | Preserve useful timestamps during outages |
| Volatile recent logs | Add bounded persistent and exportable storage | Improve traceability across restarts |
| Dashboard-dependent administration | Develop an authenticated application with roles | Improve control of user-management privileges |
| No firmware-update mechanism | Add authenticated or signed OTA updates with rollback | Improve maintainability without weakening trust |
| No tamper awareness | Add enclosure and wiring-tamper inputs | Provide earlier warning of physical interference |
| Single credential per access event | Evaluate risk-based multi-factor authentication | Increase assurance where required |

These items remain proposals and must not be described as implemented features.

## Repository Structure

```text
PTA-Smart-Door/
|-- README.md
|-- SECURITY.md
|-- firmware/
|   `-- FULL_CODE_SmartDoor_PTA1.ino
`-- docs/
    |-- Litar simulasi Smart Door.pdsprj
    |-- PROTEUS_SIMULATION_GUIDE.md
    |-- PROTEUS_UID_TESTS.md
    |-- Smart-Door-Cirkit-Designer-Wiring.png
    `-- TESTING_AND_EVIDENCE.md
```

## Project Resources

- [Arduino firmware](firmware/FULL_CODE_SmartDoor_PTA1.ino) — contains public-safe credential placeholders.
- [Proteus simulation guide](docs/PROTEUS_SIMULATION_GUIDE.md)
- [RFID UID test cases](docs/PROTEUS_UID_TESTS.md)
- [Cirkit Designer project](https://app.cirkitdesigner.com/project/c76d481d-00aa-41fe-8f70-778ca7574145)

## Demonstrated Learning

The project demonstrates practical work with embedded C++, SPI/I2C/UART peripherals, local persistence, state-based control, FreeRTOS task coordination, IoT integration, fault recovery, troubleshooting, and engineering documentation. These are learning outcomes from a functional prototype, not claims of production-security expertise.
