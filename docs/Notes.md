# Entrance

## Components


| Component                     | Use and location                                           | Connection to the outer world                                                                                                                                                                                                                                                                                                                                                                                                              | Notes                                                                                                                                                                                                                                                                                                                       |     |
| ----------------------------- | ---------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --- |
| Arduino Mega 2560 Pro (Embed) | Main, inside the box                                       | Needs several connections to the other components, but only listing here the power connection, which should be 12V via VIN.<br>- Use 2-pin panel-mount terminal block                                                                                                                                                                                                                                                                      |                                                                                                                                                                                                                                                                                                                             |     |
| Ticket Printer                | Print the tickets, secured to the panel. Needs adaptation  | Needs to be powered via its JST connector (12V - 2A)<br>- How will this be wired? Directly from the power source? Or maybe better to wire the power source to separate branches on the box and then connect the printer to its dedicated 2-pin, panel-mount terminal block? If this type of panel mount terminal block can stand 5A, using one to get the 12V power source inside the box and then distribute it should still work, right? | Connects directly to the Arduino serial port<br>- Use Serial1 (RX1 > D19; TX1 > D18)                                                                                                                                                                                                                                        |     |
| Barcode Scanner               | Read card barcodes, secured to the panel. Needs adaptation | DB9                                                                                                                                                                                                                                                                                                                                                                                                                                        | Symbol Technologies - Model: DS9208<br>- Use Serial2 (RX2 > D17; TX2 > D16)                                                                                                                                                                                                                                                 |     |
| MAX232                        | To connect the barcode scanner                             |                                                                                                                                                                                                                                                                                                                                                                                                                                            | Remember that this board inverts RX and TX. So RX needs to be connected to RX (Arduino), and TX to TX (Arduino)                                                                                                                                                                                                             |     |
| MAX485                        | PC communication                                           |                                                                                                                                                                                                                                                                                                                                                                                                                                            | RO > RX3<br>DI > TX3<br>RE > Control pin (LOW to enable receive) – use a digital pin<br>DE > Same as RE (HIGH to enable transmit) – use same control pin<br>VCC > 5V<br>GND > GND<br>A > RS485 A line > Differential signal (+)<br>B > RS485 B line > Differential signal (-)<br><br>Use Serial3 (RX3 > D15; TX3 > D14)<br> |     |
| Relay box                     | Switch power for the motor to lift bar                     |                                                                                                                                                                                                                                                                                                                                                                                                                                            |                                                                                                                                                                                                                                                                                                                             |     |
| DB9 extension                 |                                                            |                                                                                                                                                                                                                                                                                                                                                                                                                                            |                                                                                                                                                                                                                                                                                                                             |     |
| Power Supply                  | See if this can power both panels                          |                                                                                                                                                                                                                                                                                                                                                                                                                                            |                                                                                                                                                                                                                                                                                                                             |     |
| Car presence sensors          | Let the bar down                                           |                                                                                                                                                                                                                                                                                                                                                                                                                                            |                                                                                                                                                                                                                                                                                                                             |     |
| Bar motor                     | Lift the bar                                               |                                                                                                                                                                                                                                                                                                                                                                                                                                            |                                                                                                                                                                                                                                                                                                                             |     |
| Button                        | Generate ticket and lift bar                               |                                                                                                                                                                                                                                                                                                                                                                                                                                            |                                                                                                                                                                                                                                                                                                                             |     |

## Connections

| Arduion Mega Pro |                      |
| ---------------- | -------------------- |
| TX1 (D18)        | Printer (RX)         |
| RX1 (D19)        |                      |
| TX2 (D16)        | MAX232 TX (Scanner)* |
| RX2 (D17)        | MAX232 RX (Scanner)* |
| TX3 (D14)        | MAX485 DI            |
| RX3 (D15)        | MAX485 RO            |
|                  |                      |
|                  |                      |
|                  |                      |
|                  |                      |
|                  |                      |
|                  |                      |
|                  |                      |

\*These are crossed on purpose because the communication doesn't work otherwise


### MAX485

| PIN                  | CONNECTION                             |
| -------------------- | -------------------------------------- |
| DI (Driver In)       | TX (TX3 - D14)                         |
| RO (Receiver Out)    | RX (RX3 - D15)                         |
| VCC                  | 5V                                     |
| GND                  | GND                                    |
| B                    | T/R-                                   |
| A                    | T/R+                                   |
| DE (Driver Enable)   | Any digital pin (HIGH = Transmit mode) |
| RE (Receiver Enable) | Any digital pin (LOW = Receive mode)   |

### MAX232

| PIN | CONNECTION      |
| --- | --------------- |
| VCC | 5V              |
| GND | GND             |
| RX  | RX (RX2 - D17)* |
| TX  | TX (TX2 - D16)* |
\*These are crossed on purpose because the communication doesn't work otherwise


### Printer

| PIN | CONNECTION                           |
| --- | ------------------------------------ |
| VCC | 12V                                  |
| GND | GND (needs to be shared with 5V GND) |
| RX  | TX1 (D18)                            |
| TX  | Not used                             |


# Exit




## Connections


| Arduion Mega Pro | MAX232 Male |
| ---------------- | ----------- |
| TX               | TX2 (D16)** |
| RX               | RX2 (D17)** |
| GND              | GND         |
| VCC              | VCC         |
** Connected TX with TX and RX with RX because it didn't work otherwise. See below

"Male MAX232 modules are often wired for PC use.

In RS232 communication, the devices are often categorized as:

- **DTE (Data Terminal Equipment)**: like a **PC**
    
- **DCE (Data Communication Equipment)**: like a **modem** or **scanner**
    

When connecting two devices, their **TX/RX lines must be crossed**:

- TX (transmit) from one side must go to RX (receive) on the other
    
- RX ↔ TX, TX ↔ RX


When you buy a MAX232 **with a DB9 male connector**, it’s often built to **mimic a PC's serial port**.

That means:

- **Pin 2 = RX (input)** from the **scanner's TX**
    
- **Pin 3 = TX (output)** to the **scanner's RX**


If your scanner is also acting like a PC (DTE), **both sides try to transmit on pin 3 and receive on pin 2** → they both send data on TX at the same time and **nothing gets received**.
"




| Arduion Mega Pro | MAX485      |
| ---------------- | ----------- |
| TX               | TX2 (D16)** |
| RX               | RX2 (D17)** |
| GND              | GND         |
| VCC              | VCC         |
| VCC              | RE          |
| VCC              | DE          |
| D14              | DI          |
| D15              | RO          |


| MAX485 | DTECH 485 <> USB converter |
| ------ | -------------------------- |
| A      | T/R+                       |
|  B     | T/R-                       |
| GND    | GND                        |
