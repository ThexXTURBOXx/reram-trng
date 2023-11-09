# Notation

- `RPi`: Raspberry Pi
- `ReRAM`: Resistive Random Access Memory
- `PCB`: Printed Circuit Board
- `TRNG`: True Random Number Generator
- `Supervisor RPi`: The Raspberry Pi which supervises the progress (the one with a `1` in its ID)
- `ReRAM RPi`: The RPi which is connected to the ReRAM PCB (the one with a `2` in its ID)

# Wiring

![Wiring diagram](./Wiring_Steckplatine.png?raw=true)

# Compilation

## Use a Pre-built Kernel

1. Go to the [latest pipeline build](https://git.fim.uni-passau.de/mexis/rpi_measurement_kernel/-/pipelines/latest)
1. Open its build log
1. On the right sidebar, click on `Download` under `Job artifacts`
1. The zip file contains all the relevant files. Be sure to rename the kernel that you want to deploy to just `kernel8.img`!

## Doing It Yourself

1. Clone this Git repository (remember to also initialise the `circle` submodule!)
1. Install either a cross-compiling `gcc` instance (`gcc-aarch64-linux-gnu`) or the native `gcc` if you are already on the ARMv8/AARCH64v8 platform
1. Install `make` and `wget`
1. Run `./init.sh`
1. Run `make`
1. You should get a `kernel8.img` file after a few moments
1. All the other required files can be found in `circle/boot`

# ReRAM RPi Setup

1. Start Raspberry Pi Imager
1. Select `Raspberry Pi OS Lite 64 bit` as image
1. Select the **correct** SD card
1. Advanced settings: don't matter
1. Select `Write`
1. Wait until done
1. Briefly disconnect the SD card from the computer and insert it again
1. You can remove all the files on the SD-card and copy the required files from the previous step onto its boot partition
1. Put the SD card in RPi with the `2` in its ID
1. **Do not turn it on yet**

# Supervisor RPi Setup

## Firmware Setup

1. Start Raspberry Pi Imager
1. Select `Raspberry Pi OS Lite 32 bit` as image
1. Select **correct** SD card
1. Advanced settings: 
    - `Hostname`: Self-selectable (preferably name corresponding to letters on RPi, e.g. `A1 -> alice`, `B1 -> bob` etc.).
    - `Enable SSH`: Check the box and select `Use password for authentication`
    - `Set up Wifi`: Check the box and insert the data from your hotspot; also select the correct wifi country (probably corresponding to the language of the mobile phone)
    - Remaining parameters do not matter
1. Select `Write`
1. Wait until ready
1. Put SD card in RPi with the `1` in its ID
1. Turn on your hotspot
1. Turn on the RPi
1. Connect laptop to hotspot
1. Wait until RPi is logged into the hotspot (may take a while at the first start...)
1. Connect to RPi via `ssh` (try the host name as the IP address, otherwise use `ip neigh` or brute-force via e.g. "Angry IP Scanner")
1. Execute command: `sudo apt update && sudo apt install minicom screen && sudo raspi-config`
1. Navigate to "Interface Options" and select "Serial Port".
1. For `Would you like a login shell [...]` select: `<No>`
1. For `Do you want to enable the serial port hardware [...]` select: `<Yes>`
1. Confirm with `<Ok>` and change to `<Finish>` with <kbd>Tab</kbd> and confirm with <kbd>Enter</kbd>
1. When asked for a reboot, confirm with `<Yes>` and wait

## Minicom Setup

1. Once the RPi is restarted, connect again via SSH
1. Execute command: `ls /dev/ttyS0` and check that the file is displayed. If not, something went wrong during the setup
1. Execute command: `sudo minicom -s`
1. Navigate to `Serial port setup` in the menu and select it by pressing <kbd>Enter</kbd>
1. Type `A`; the `Serial device` field should be selected now
1. Change to `/dev/ttyS0` and confirm with <kbd>Enter</kbd>
1. Check that `E` says `115200 8N1`
1. Go to the main menu with <kbd>Enter</kbd>
1. Under `Screen and keyboard` change `A` to <kbd>Ctrl</kbd>+<kbd>X</kbd> (it must then say `^X`)
1. Go back to the main menu with <kbd>Enter</kbd>
1. Select `Save setup as dfl` and wait until the small message disappears
1. Select `Exit`
1. The `minicom` console should now be opened

# Test Run

1. Make sure that you are inside `minicom` in the Supervisor RPi
1. Make sure that everything is wired up correctly
1. Turn on the ReRAM RPi
1. You should see logs inside `minicom`. If not, something went wrong!
