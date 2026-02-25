# Expected vs Observed — v0.1

Scope: protocol baseline validation for the QEMU reference bootloader.

Observed artifact example:

- `docs/evidence/v0.1/logs/qemu_update_protocol.log`

## Checklist

- Normal handoff: expected `BL_EVT:HANDOFF_APP` + `APP_EVT:START` — observed: TODO
- Recovery behavior: expected no handoff on invalid image — observed: TODO
- Update pass path: expected `BL_EVT:APP_CRC_OK` — observed: TODO
- Update fail path: expected `BL_EVT:APP_CRC_FAIL` and safe path — observed: TODO

Summary: TODO (pending fresh capture run).

Notes:

- Keep expected and observed results explicit.
- Attach command/procedure used for reproduction and date/version context.
