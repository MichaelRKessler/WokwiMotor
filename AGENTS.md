# Agent Guidelines

## Framework & Build System
- **Framework:** Arduino framework.
- **CLI & Build Tool:** Use `arduino-cli` for compiling, library management, and flashing.
- **Build Path:** Always compile using an explicit build path pointing to the local `./build` directory rather than relying on the OS temp directory.
- **Version Control:** Ensure `./build` is excluded from version control in `.gitignore`.

## Simulation & Hardware Configuration (Wokwi)
- **Wokwi Configuration:** Configure `wokwi.toml` at the project root to point directly to the binary and ELF outputs in `./build`.
- **Simulation Environment:** Wokwi is used to simulate the setup.
- **Hardware Synchronization:**
  - Whenever hardware components or pin assignments are added, removed, or modified, keep both the source code and `diagram.json` synchronized.
  - Make necessary code updates (in the `.ino` sketch or libraries) and update `diagram.json` if hardware connections changed.

