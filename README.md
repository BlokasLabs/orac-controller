# Midiboy &lt;-> Orac UI

## Getting Started

### 1.0.2 Blokas Boards

It's required to be using 1.0.2 Blokas Boards definition files, here's how to upgrade if necessary:

1. Run Arduino IDE.
2. Tools->Board->Boards Manager...
3. Search for 'Blokas'.
4. If the version shows up as 1.0.2, update is not needed.
5. Otherwise, highlight the Blokas AVR boards entry and click Install at the bottom right.

### Sketch and Bridge Install

1. Upload orac-controller.ino sketch from orac-controller/ folder to your Midiboy.
2. Install OracCtlBridge.py on your Raspberry Pi:
    ```
    git clone https://github.com/BlokasLabs/orac-controller
    cd orac-controller
    sudo ./install.sh
    ```
3. Connect Midiboy via USB to your Raspberry Pi, it should automatically connect to Orac and display the UI.
   If it doesn't, make sure Orac and MEC are running and reconnect Midiboy.

## Controls

On the menu screen:

* Up and Down move between the lines.
* Left and Right move between the modules.
* A activates the selected item.
* B goes to the parameters screen.

On the parameters screen:

* Up and Down move between the parameters. If MIDI learn is enabled, the just selected param can be MIDI mapped.
* Left and Right decrease and increase the highlighted parameter value.
* A activates the currently selected parameter for MIDI learning. Useful if the focus was 'lost' by external param change.
* B goes to the menu screen.
