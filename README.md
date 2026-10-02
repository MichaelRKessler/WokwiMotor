# WokwiMotor

An ESP32 drives a brushed DC motor through an L293D H-bridge, simulated in [Wokwi](https://wokwi.com/). A potentiometer sets the speed and a pushbutton toggles between forward and reverse. Wokwi has no built-in L293D or DC motor, so both are implemented as custom chips in [chips/](chips/).

## Wiring

| ESP32 | Connects to | Purpose |
|-------|-------------|---------|
| D26 | L293D EN12 | PWM speed (1 kHz, 8-bit) |
| D25 | L293D 1A | Direction input 1 |
| D27 | L293D 2A | Direction input 2 |
| VP (GPIO36) | Potentiometer wiper | Speed setting (ADC1) |
| D23 | Pushbutton → GND | Forward/reverse (internal pull-up) |
| VIN | L293D VCC1 | Logic supply |
| GND | L293D GND.1, potentiometer, button | Common ground |

L293D VCC2 (motor supply) is fed from a separate VCC source, standing in for an external motor supply. Outputs 1Y and 2Y drive the motor terminals M1 and M2. Channels 3 and 4 are unused.

## Project layout

| Path | Contents |
|------|----------|
| [WokwiMotor.ino](WokwiMotor.ino) | ESP32 sketch |
| [diagram.json](diagram.json) | Wokwi circuit |
| [wokwi.toml](wokwi.toml) | Points Wokwi at the firmware in `build/` and the chips in `chips/build/` |
| [chips/l293d.chip.c](chips/l293d.chip.c) | L293D model: each output follows its input while the channel is enabled, otherwise high-impedance |
| [chips/dc-motor.chip.c](chips/dc-motor.chip.c) | Motor model: speed follows the average of M1 − M2 with a first-order lag, drawn as a spinning rotor |
| [build-chips.ps1](build-chips.ps1) | Compiles the custom chips to WebAssembly |

## Building

Requirements:

- [arduino-cli](https://arduino.github.io/arduino-cli/) with the ESP32 core (`arduino-cli core install esp32:esp32`)
- The [Wokwi for VS Code](https://docs.wokwi.com/vscode/getting-started) extension
- PowerShell, for the chip build script (it downloads wasi-sdk to `~/.wasi-sdk` the first time it runs)

Build the custom chips:

```powershell
./build-chips.ps1
```

Compile the sketch into `./build`:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 --build-path ./build .
```

Then open `diagram.json` in VS Code and start the simulator (**Wokwi: Start Simulator**).

## Using the simulation

- Turn the potentiometer to change speed. The serial monitor prints the direction and speed in steps of about 5%.
- Press the button to reverse direction.
- The motor's `maxRpm` (default 60) and `tau` (rotor time constant in seconds, default 0.25) attributes can be changed in `diagram.json`.

## Building it for real

On real hardware:

- **Power VCC2 (motor supply) from an external supply**, not the ESP32's VIN. Motor surges can brown out and reset the ESP32. The L293D also drops about 1.5–3 V, so choose a supply 2–3 V above the motor's rated voltage (VCC2 accepts 4.5–36 V).
- **VCC1 (logic supply) can stay on VIN.** It needs 4.5–7 V and draws little current.
- **Connect the external supply's ground to the ESP32's GND.**
