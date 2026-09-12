# PTA Smart Door

An ESP32-based access-control project developed for the PTA. The system combines RFID identification, AS608 fingerprint verification, a 16x2 I2C LCD, relay-driven solenoid locking, touch exit, Blynk, and Telegram integration.

> **Project scope:** academic prototype for demonstrating multi-method door access control.

## Project Resources

- [Arduino firmware](firmware/FULL_CODE_SmartDoor_PTA1.ino) — public-safe version; service credentials are placeholders.
- [Proteus simulation project](docs/Litar%20simulasi%20Smart%20Door.pdsprj)
- [Proteus simulation guide](docs/PROTEUS_SIMULATION_GUIDE.md)
- [RFID UID test cases](docs/PROTEUS_UID_TESTS.md)
- [Cirkit Designer project](https://app.cirkitdesigner.com/project/c76d481d-00aa-41fe-8f70-778ca7574145)

## Wiring Reference

![Cirkit Designer wiring diagram](docs/Smart-Door-Cirkit-Designer-Wiring.png)

*Figure 1. Cirkit Designer wiring reference for the physical Smart Door prototype.*

## Access-Control Flow

1. The ESP32 initialises the connected hardware, local records, display, and configured network services.
2. A user presents an RFID card, scans a fingerprint, or uses the touch-exit input.
3. The system checks the received identity against the authorised local records.
4. For a valid request, the LCD shows the authorised user and the relay unlocks the solenoid for the configured interval.
5. For an invalid request, the door remains locked and the user receives LCD, LED, and buzzer feedback.
6. When configured, Blynk and Telegram support remote monitoring and control.

## System Architecture

```text
RFID RC522 ─────┐
                ├──> ESP32 ───> LCD / LEDs / Buzzer
AS608 Fingerprint┤       │
Touch Exit ──────┘       └──> Relay ───> Solenoid Lock
                        │
                        ├──> Blynk (optional)
                        └──> Telegram (optional)
```

## Verified ESP32 Pin Map

| Module | ESP32 connection |
| --- | --- |
| RC522 RFID | SS GPIO 5; RST GPIO 2; SCK GPIO 18; MISO GPIO 19; MOSI GPIO 23 |
| AS608 fingerprint sensor | RX GPIO 16; TX GPIO 17 |
| I2C LCD | SDA GPIO 21; SCL GPIO 22; I2C address `0x27` |
| Relay / solenoid control | GPIO 26 |
| Touch exit | GPIO 27 |
| Green LED | GPIO 25 |
| Red LED | GPIO 33 |
| Buzzer | GPIO 32 |

## Implemented Capabilities

- RFID and fingerprint authentication
- RFID and fingerprint enrolment
- Local user records using ESP32 Preferences
- LCD, LED, buzzer, and access-log feedback
- Automatic door relock after five seconds
- Blynk dashboard and Telegram notification support
- Multi-Wi-Fi retry logic and sensor recovery handling

## Proteus Demonstration

Use the [Proteus Simulation Guide](docs/PROTEUS_SIMULATION_GUIDE.md) together with the [UID Test Cases](docs/PROTEUS_UID_TESTS.md) to demonstrate both authorised and unauthorised RFID access outcomes.

## Security Notice

Do not publish Wi-Fi passwords, Blynk tokens, Telegram tokens, or chat IDs. Configure these values only in a private local copy before flashing the ESP32.

## Project Status

- [x] Firmware published
- [x] Proteus simulation project uploaded
- [x] Cirkit Designer wiring reference uploaded
- [x] UID test cases documented
- [ ] Demo video to be added
