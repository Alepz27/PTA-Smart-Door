# Running the Proteus Smart Door Simulation

## Download the project

1. Open docs/Litar simulasi Smart Door.pdsprj in this repository.
2. Click Raw or Download raw file. Raw opens the original project data in the browser; save it as Litar simulasi Smart Door.pdsprj if the download does not start automatically.
3. Open the saved .pdsprj file with Proteus 8.

## Run and test

1. In Proteus, click the Run button (play icon).
2. The Virtual Terminal black window and LCD will show that the card is ready to scan.
3. Open the UID test notes, copy only the UID value, paste it into the Virtual Terminal, then press Enter.

| UID | Expected LCD result |
| --- | --- |
| E280689401A9 | Access Granted; User: ALI; Door: Unlocked |
| E2000019060C | Access Granted; User: ABU; Door: Unlocked |
| 123456789ABC | Access denied; LCD returns to Card Ready / Scan Your Card |

## Demonstration evidence

The simulation screenshots should show the initial Card Ready state, a successful ALI/ABU access state, and an invalid UID state.

## Troubleshooting

- If the black terminal does not accept typing, click inside it first.
- If no LCD change appears, confirm the simulation is running and press Enter after pasting the UID.
- Do not paste the name; use the UID only.
