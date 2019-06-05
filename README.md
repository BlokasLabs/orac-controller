# Midiboy &lt;-> Orac UI

![](screens/orac.gif?raw=true)

## Getting Started

### 1.0.2 Blokas Boards

It's required to be using 1.0.2 Blokas Boards definition files, here's how to upgrade if necessary:

1. Run Arduino IDE.
2. Tools->Board->Boards Manager...
3. Search for 'Blokas'.
4. If the version shows up as 1.0.2, update is not needed.
5. Otherwise, highlight the Blokas AVR boards entry and click Install at the bottom right.
6. Once complete, select Midiboy again in Tools->Board.

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

* Up and Down - move between the lines.
* Left and Right - move between the modules.
* A - activate the selected item.
* B - go to the parameters screen.

On the parameters screen:

* Up and Down - move between the parameters.
* Left and Right:
    * If a param is activated, decrease and increase its value respectively.
    * Otherwise go to previous or next parameter page.
* A - activate the currently selected parameter for changing the value. If MIDI Learn is enabled, after the parameters' value is changed using Left or Right, the parameter can be MIDI mapped by moving a control on a MIDI controller.
* B goes to the menu screen.
