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
| [.github/workflows/chips.yml](.github/workflows/chips.yml) | Builds the chips in CI and publishes them as a release when a `v*` tag is pushed |

## Building

Requirements:

- [arduino-cli](https://arduino.github.io/arduino-cli/) with the ESP32 core (`arduino-cli core install esp32:esp32`)
- The [Wokwi for VS Code](https://docs.wokwi.com/vscode/getting-started) extension
- PowerShell (7 or later on macOS and Linux), for the chip build script (it downloads wasi-sdk to `~/.wasi-sdk` the first time it runs)

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

## Using the chips in another project

The motor and L293D chips can be added to any Wokwi for VS Code project without building them:

1. Download [wokwi-motor-chips.zip](https://github.com/MichaelRKessler/WokwiMotor/releases/latest/download/wokwi-motor-chips.zip) and unzip it into a folder named `chips` in your project. Keep each `.wasm` file next to its `.json` file, because Wokwi reads the chip's pins from the `.json` beside the binary. Commit the folder with your project.
2. Add the chips to your project's `wokwi.toml`:

   ```toml
   [[chip]]
   name = "dc-motor"
   binary = "chips/dc-motor.chip.wasm"

   [[chip]]
   name = "l293d"
   binary = "chips/l293d.chip.wasm"
   ```

3. Add the parts to `diagram.json`. The part type is `chip-` followed by the name from `wokwi.toml`:

   ```json
   { "type": "chip-l293d", "id": "drv", "top": 0, "left": 0, "attrs": {} },
   { "type": "chip-dc-motor", "id": "motor", "top": 0, "left": 150, "attrs": { "maxRpm": "60", "tau": "0.25" } }
   ```

The motor has two terminals, `M1` and `M2`. The L293D pins use the datasheet names (`EN12`, `1A`, `1Y`, `VCC1`, `VCC2`, and so on); see [chips/l293d.chip.json](chips/l293d.chip.json) for the full list and [diagram.json](diagram.json) for an example of wiring them.

## Building it for real

On real hardware:

- **Power VCC2 (motor supply) from an external supply**, not the ESP32's VIN. Motor surges can brown out and reset the ESP32. The L293D also drops about 1.5–3 V, so choose a supply 2–3 V above the motor's rated voltage (VCC2 accepts 4.5–36 V).
- **VCC1 (logic supply) can stay on VIN.** It needs 4.5–7 V and draws little current.
- **Connect the external supply's ground to the ESP32's GND.**

## Releasing the chips

The [Chips workflow](.github/workflows/chips.yml) builds the chips whenever they change. To publish a new download, tag a commit and push the tag:

```powershell
git tag v1.0.0
git push origin v1.0.0
```

The workflow creates a GitHub release for the tag with `wokwi-motor-chips.zip` attached. The download link in [Using the chips in another project](#using-the-chips-in-another-project) always points to the newest release.
