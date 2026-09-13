# Testing and Evidence

This record separates repository-verifiable evidence from behavior that still needs a dated physical test record.

## Evidence Basis

| Claim | Evidence | Status |
| --- | --- | --- |
| RFID grant and denial simulation cases exist | Proteus project, simulation guide and UID matrix | Documented simulation evidence |
| RFID and fingerprint authentication paths exist | Published firmware | Source verified |
| Touch exit calls the common grant path | `checkTouch()` in the firmware | Source verified |
| Relay automatically returns LOW after about five seconds | `DOOR_UNLOCK_TIME = 5000` and `updateDoor()` | Source verified |
| User records persist | `Preferences` load/save implementation | Source verified |
| Recent access log is limited to ten entries in RAM | `MAX_LOGS = 10` and in-memory array | Source verified |
| Local access continues without Wi-Fi/cloud | Hardware loop is independent of background network tasks | Source verified by architecture; physical outage result not recorded here |
| RC522 and AS608 recovery routines exist | Scheduled RC522 reinitialisation and UART2 re-sync state logic | Source verified |
| Complete integrated hardware passes all flows | No dated test report or suitable video in repository | Requires user verification |
| Recovery after brownout/power interruption | No controlled test record in repository | Not verified |

## Reproducible Proteus RFID Check

Follow [PROTEUS_SIMULATION_GUIDE.md](PROTEUS_SIMULATION_GUIDE.md) and record the observed result rather than treating the expected result as proof.

| Case | Input | Expected result | Repository record |
| --- | --- | --- | --- |
| Authorised user ALI | `E280689401A9` | Access granted and unlock indication | Test vector documented |
| Authorised user ABU | `E2000019060C` | Access granted and unlock indication | Test vector documented |
| Unknown card | `123456789ABC` | Access denied and locked state | Test vector documented |

## Recommended Physical Verification Matrix

The following is a test plan, not a claim that the tests have passed.

| Test | Method | Acceptance evidence to capture |
| --- | --- | --- |
| Cold start safe state | Power the complete prototype from off | Relay/lock state, LCD start sequence and serial log |
| RFID grant | Present enrolled card | User feedback, unlock interval and relock |
| RFID denial | Present unknown card | Locked state and denied feedback |
| Fingerprint grant/denial | Use enrolled and unknown fingers | Match result, actuator state and feedback |
| Touch exit | Trigger TTP223 once and repeatedly | Debounce behavior and five-second relock |
| Offline local access | Disconnect Wi-Fi before tests | Local paths remain responsive; cloud functions unavailable |
| Wi-Fi recovery | Restore configured network | Reconnection time and restored Blynk/Telegram state |
| RC522 interruption | Use a safe, defined lab fault method | Recovery attempt and restored reads without ESP32 restart |
| AS608 start/recovery | Power-cycle according to a safe test procedure | Bounded retry/re-sync and restored matching |
| Hostile input/admin flow | Invalid IDs, cancelled enrolment and blocked user | No unintended deletion or unlock |
| Power interruption | Remove/restore supply under a controlled procedure | Physical lock state, record persistence and clean reboot |
| Soak test | Run for a defined duration and event count | Reset count, memory trend and missed-event count |

## Missing Evidence

- Privacy-reviewed photographs of the physical prototype and lock mechanism.
- Blynk dashboard and Telegram notification screenshots with identities and tokens removed.
- A dated integrated-system test log.
- Measured voltage/current readings and a power budget.
- A controlled outage/recovery record.
- A suitable demonstration video.

Only add evidence after checking it for names, faces, email addresses, network details, tokens, passwords, chat IDs and other private information.
