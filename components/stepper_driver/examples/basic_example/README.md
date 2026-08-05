# Basic Example
This example demonstrates how to use the stepper_driver directory.

It will energize the motor, rotate in both directions, then denergize the motor.


## How to use example
This example requires a step and direction stepper driver. 
Use the sdk configuration to set the pins and levels for the step, direction, and enable pins of the driver.

### Build and flash
Build the project by entering the basic_example directory and running the following commands.
```
idf.py build
```
Flash the project to a device with the following command.
```
idf.py -p PORT flash monitor
```