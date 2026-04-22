# MyKey Marchy — COGES MyKey NFC Tool

**Fork by Marchy** | Based on the original work by **luhf** | Encryption algorithm by the **MIKAI team**

---

<<<<<<< HEAD
Released for **educational and research purposes only.**

**License notice (from original):** You are NOT allowed to sell or publish this application without the source code and this README. Redistribution requires full source + credits. Non-negotiable.

---

## Features

- **Read Card** — Read any COGES MyKey dongle via NFC
- **Add / Set Credit** — Add or set an arbitrary credit amount
- **[Q] Quick Charge** — One-tap shortcut to set the configured amount (default 19.00 €)
- **Set Quick Amount** — Configure the Quick Charge amount and save it to SD
- **Reset Card** — Unbind the dongle from a vendor (re-associate)
- **Save / Load File** — Persist card state to/from SD card
- **Debug Info** — Dump raw block data for analysis
- **Auto-Backup** — Silent backup to SD every time a card is read (no popup)

---

## Changelog

### v1.1-marchy 

- **Bug fix — `mykey_reset` loop**: The refactored loop used `(offset % 4)` arithmetic
  that produced wrong block addresses (e.g. group `0x14` was silently overwriting `0x10`).
  Fixed with an explicit `group_bases[] = {0x10, 0x14, 0x3F, 0x43}` array.
- **Optimized NFC write**: Saves a baseline snapshot at read time; write path now skips
  unchanged blocks, reducing NFC transaction time and flash wear on the dongle.
- **Quick Charge repositioned** in the menu — now sits directly under *Set Credit* for fast access.
- **Fork branding**: About screen, log TAG (`MyKeyMarchy`), `application.fam` updated.
- **Cleaner comments** throughout `mykey_core.c` and `nfc_srix.c`.

### v1.0-marchy

- **Quick Charge 19 €**: Added one-tap function to set credit to 19.00 € with automatic NFC write.
- **Core Refactoring**: Replaced the massive switch-case in `mykey_reset` with data-driven logic.
- **Constant Centralization**: Eliminated magic numbers — all block addresses and constants
  moved to `cogs_mikai.h`.
- **GUI Abstraction Layer**: Introduced popup/dialog utility functions in `cogs_mikai_app.c`,
  reducing boilerplate in every scene.
- **Logic Optimization**: Implemented `update_block_optimized` helper to handle checksum,
  encoding and XOR mask in a single step.
- **NFC Driver**: Improved stability in `nfc_srix.c`.

## Credits

**luhf** ([monocul.us](https://monocul.us/))

**MIKAI team** (libmikai) 


---

## Disclaimer

For educational and research use only. The authors are not responsible for any misuse.
If you use this to steal, you're not a hacker — you're a thief. Act accordingly.
=======
Credit for the original encryption algorithm goes to the MIKAI team.ù


* **Quick Charge 19€**: Aggiunta funzione rapida per impostare il credito a 19.00€ con scrittura automatica.
* **Refactoring Core**: Sostituito lo switch-case massivo in `mykey_reset` con logica data-driven.
* **Centralizzazione Costanti**: Eliminazione dei magic numbers in `cogs_mikai.h`.
* **Abstraction Layer GUI**: Introdotte utility per popup e dialoghi in `cogs_mikai_app.c`, riducendo il boilerplate nelle scene.

* **Logic Optimization**: Implementato helper `update_block_optimized` per gestire checksum, encoding e XOR in un unico step.
* **NFC Driver**: Migliorata la stabilità in `nfc_srix.c`.
>>>>>>> origin/master
