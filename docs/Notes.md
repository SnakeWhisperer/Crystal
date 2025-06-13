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
