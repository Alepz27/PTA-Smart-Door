# Proteus Smart Door Simulation Guide

## Purpose

This document explains how to run and verify the RFID access-control simulation supplied with the PTA Smart Door project. It is written as a concise laboratory guide for demonstration and assessment use.

## Required Files and Software

- [Proteus project: Litar simulasi Smart Door.pdsprj](Litar%20simulasi%20Smart%20Door.pdsprj)
- [RFID UID test cases](PROTEUS_UID_TESTS.md)
- Proteus 8 or a compatible installation capable of opening `.pdsprj` projects

## Procedure

### 1. Download the simulation project

1. Open the linked `.pdsprj` file in this repository.
2. Select **Raw** or **Download raw file** to save the project locally.
3. Keep the original filename and `.pdsprj` extension.
4. Open the downloaded file in Proteus.

### 2. Start the simulation

1. Confirm that the project schematic is displayed.
2. Select the **Run** button (play icon).
3. Observe the LCD and the Virtual Terminal. The LCD should display the card-ready state, while the Virtual Terminal is used to enter an RFID UID.

### 3. Test authorised access

1. Open the [RFID UID test cases](PROTEUS_UID_TESTS.md).
2. Copy the UID only; do not copy the user name.
3. Click inside the Virtual Terminal, paste the UID, and press **Enter**.
4. Use `E280689401A9` to test the recorded user **ALI**, or `E2000019060C` to test **ABU**.
5. Verify that the LCD reports access granted and that the access output indicates an unlocked door.

### 4. Test unauthorised access

1. Enter `123456789ABC` in the Virtual Terminal.
2. Press **Enter**.
3. Verify that the door remains locked and that the LCD returns to the card-ready state.

## Expected Results

| Test input | Expected LCD / system response |
| --- | --- |
| `E280689401A9` | Access granted; user identified as ALI; door unlocked |
| `E2000019060C` | Access granted; user identified as ABU; door unlocked |
| `123456789ABC` | Access denied; door remains locked; system returns to card-ready state |

## What the Simulation Demonstrates

The simulation models a basic RFID access-control sequence: the Virtual Terminal receives a UID, the Arduino compares it with the authorised list, and the system either permits or rejects access. The LCD, LEDs, buzzer, and relay output provide observable feedback for the outcome.

## Troubleshooting

- Click inside the Virtual Terminal before pasting a UID.
- Start the simulation before entering any test value.
- Press **Enter** after each UID.
- Enter only the hexadecimal UID; do not include a user name or additional spaces.
- If the project opens as text in the browser, download it and open it locally in Proteus.
