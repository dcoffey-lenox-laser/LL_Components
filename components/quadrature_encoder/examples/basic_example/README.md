# Basic Example
This example demonstrates how to use the quadrature encoder component.

It will write the encoder's current position to the serial monitor.

## How to use example
This example requires a quadrature encoder to be connected to the esp32.
Use the sdk configuration menu to set the pins used for the A,B, and Z channel.

### Build and flash
Build the project by entering the basic_example directory and running the follow commands.
```
idf.py build
```
Flash the project to a device with the following command.
```
idf.py -p PORT flash monitor
```