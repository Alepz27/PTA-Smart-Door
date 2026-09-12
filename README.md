# PTA Smart Door

A final-year-project (PTA) smart access-control system built around an ESP32. The system combines RFID and fingerprint verification, supports Blynk connectivity, presents status on an LCD, and actuates a relay-driven solenoid lock.

> **Project status:** Documentation scaffold created. The original Arduino sketch, Proteus schematic, web circuit design, and verified pin assignments have not yet been added to this repository.

## Highlights

- Multi-factor access workflow using RFID and an AS608 fingerprint sensor
- ESP32 as the central controller
- LCD interface for local prompts and status feedback
- Relay-controlled solenoid door lock
- Blynk integration for connected monitoring or control
- Documentation-first repository layout for firmware and circuit artefacts

## System Architecture

~~~mermaid
flowchart LR
  User[Authorised user] --> RFID[RFID reader]
  User --> Fingerprint[AS608 fingerprint sensor]
  RFID --> ESP32[ESP32 controller]
  Fingerprint --> ESP32
  Blynk[Blynk cloud/app] <--> ESP32
  ESP32 --> LCD[LCD status display]
  ESP32 --> Relay[Relay module]
  Relay --> Lock[Solenoid door lock]
~~~

The specific access policy, fallback behaviour, enrolment flow, and cloud commands will be documented from the final firmware rather than assumed here.

## Hardware

| Component | Documented role |
| --- | --- |
| ESP32 | Main microcontroller and integration point |
| RFID reader | Card/tag authentication input |
| AS608 | Fingerprint verification input |
| LCD | Local status and user feedback |
| Relay module | Electrical switching interface |
| Solenoid lock | Door locking actuator |
| Blynk | Connected application/cloud platform |

## Wiring and Pin Map

Pin assignments are intentionally not published yet because no verified schematic or firmware source was available at setup time.

| Interface | ESP32 pin | Notes |
| --- | --- | --- |
| RFID | To be confirmed | Add from final schematic/firmware |
| AS608 | To be confirmed | Add UART pins and power details |
| LCD | To be confirmed | Add I²C/SPI address and pins |
| Relay | To be confirmed | Add control pin and relay power details |

## Repository Layout

~~~text
PTA-Smart-Door/
├── README.md
├── firmware/        # Arduino IDE / ESP32 source code
├── schematics/      # Proteus project, exports and wiring diagrams
├── web-circuit/     # Web circuit-design source and exports
├── docs/            # Architecture, setup and testing notes
└── media/           # Photos, screenshots and demo assets
~~~

## Adding Project Evidence

Please add the original project files to preserve a complete engineering record:

1. **Arduino IDE** — .ino files, library list, Blynk template details kept in local configuration, and any serial-monitor test notes.
2. **Proteus** — project file, schematic export (PDF/PNG), component/library notes, and simulation screenshots.
3. **Web circuit design** — share/export file plus a rendered diagram image or PDF.
4. **Media** — labelled hardware photographs and a short demonstration video link, if available.

Do not commit credentials, Wi-Fi passwords, Blynk auth tokens, API keys, or personal data. Use a local secrets/config file excluded through .gitignore.

## Current Progress

- [x] Public project repository created
- [x] Project scope and high-level architecture documented
- [x] Hardware roles documented
- [ ] Verified pin map added
- [ ] Arduino IDE firmware added
- [ ] Proteus schematic and exports added
- [ ] Web circuit design added
- [ ] Test results and demonstration media added

## Future Improvements

- Add audit logging for every access event
- Define secure remote-access rules and failure handling
- Add an enclosure, backup power, and tamper considerations
- Create a repeatable hardware test checklist
- Document sensor enrolment and recovery procedures

## Contributing

This is an academic project repository. Changes should include a concise description, relevant schematic or firmware evidence, and testing notes where applicable.
