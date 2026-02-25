# Boot Sequence Specification — RISC-V QEMU Bootloader

Purpose: define the implemented and auditable boot state behavior, serial token contract, and deterministic protocol flow.

## 1. Deterministic behavior model

- Startup path is fixed and explicit: assembly entry (`_start`) then C `main()`
- Boot decision is explicit on UART input (`u`/`U` enters update mode)
- Validation checks are deterministic (magic, size bounds, CRC32)
- No random branch selection

## 2. Serial token contract (canonical)

Canonical machine-parsable format:

`BL_EVT:<TOKEN>[:VALUE]`

Canonical flow tokens:

- `BL_EVT:INIT`
- `BL_EVT:HW_READY`
- `BL_EVT:DECISION_NORMAL`
- `BL_EVT:APP_CRC_CHECK`
- `BL_EVT:APP_CRC_OK`
- `BL_EVT:APP_CRC_FAIL`
- `BL_EVT:LOAD_APP`
- `BL_EVT:HANDOFF`
- `BL_EVT:HANDOFF_APP`
- `BL_EVT:DECISION_RECOVERY`
- `BL_EVT:FATAL_RESET`

Compatibility note:

- Human-readable messages (`BOOT?`, `OK`, `READY`, `ERR:*`) may coexist during migration.
- New tooling should prioritize `BL_EVT:*` tokens.

## 3. Protocol baseline

Update protocol sequence:

1. Bootloader emits readiness (`BOOT?`) and canonical state tokens
2. Host requests update (`u`/`U`)
3. Host sends `SEND <size>`
4. Bootloader erases app region and receives binary
5. Bootloader computes CRC and emits pass/fail tokens
6. Bootloader reboots or hands off according to platform policy

## 4. Recovery baseline

When no valid app exists:

- bootloader enters recovery loop
- update remains available via UART
- no application handoff occurs until valid image is written

## 5. Visual signaling

This QEMU reference has no mandatory LED contract.
Visual evidence is terminal/log based.

## 6. Evidence links

- Validation profile: `VALIDATION_PROFILE.md`
- Setup and execution: `SETUP.md`
- Release evidence: `docs/evidence/<release>/`
