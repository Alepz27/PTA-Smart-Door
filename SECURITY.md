# Security and Public Repository Hygiene

This repository documents an academic access-control prototype. Do not treat it as security certification or deployment guidance for a real building.

## Never Commit

- Wi-Fi SSIDs and passwords
- Blynk template/authentication tokens
- Telegram bot tokens or chat IDs
- API keys, passwords, private certificates or recovery codes
- Real access logs or user-identifying records
- Unreviewed dashboard screenshots or serial logs

The published firmware deliberately uses `YOUR_...` placeholders. Keep the working credential copy outside the repository or in an ignored local file.

## Before Publishing a Change

1. Review the complete diff, including deleted lines and binary files.
2. Search text for `password`, `token`, `secret`, `chat_id`, `ssid`, `api_key` and known private values.
3. Inspect screenshots for names, faces, email addresses, private network information and credentials.
4. Confirm generated archives do not contain editor backups or local configuration.
5. Revoke and rotate any credential that was ever pushed; deleting a later commit does not make the old value safe.

## Prototype Security Boundaries

- RFID UID comparison is not resistant to cloning.
- The current Telegram client disables certificate verification with `setInsecure()`.
- Remote unlock depends on the security of the connected account and token.
- The design has no door-position, forced-entry or enclosure-tamper sensor.
- The ten-entry access view is volatile and is not a tamper-resistant audit log.
- Electrical, egress, fire and fail-safe/fail-secure requirements must be reviewed for any real installation.

Report suspected credential exposure privately and rotate the affected secret immediately. Do not open a public issue containing the secret.
