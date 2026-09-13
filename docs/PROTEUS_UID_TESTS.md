# RFID UID Test Cases

## Purpose

This reference records the RFID values used to verify the Smart Door Proteus simulation. Each test case should be entered in the Virtual Terminal exactly as shown.

## Test Matrix

| RFID UID | Authorisation status | Expected result |
| --- | --- | --- |
| `E280689401A9` | Authorised | Access granted; user displayed as ALI; door unlocked |
| `E2000019060C` | Authorised | Access granted; user displayed as ABU; door unlocked |
| `123456789ABC` | Unauthorised | Access denied; door remains locked; system returns to card-ready state |

## How to Use This Reference

1. Run the Proteus simulation.
2. Copy one UID from the table, without the name or expected-result text.
3. Paste the UID into the Virtual Terminal.
4. Press **Enter** and compare the observed response with the expected result.

For complete setup instructions, see the [Proteus Smart Door Simulation Guide](PROTEUS_SIMULATION_GUIDE.md).
