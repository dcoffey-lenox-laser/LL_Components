# GPIO Control Example
This example demonstrates simple user defined stepper motor control

It allows for energizing and denergizing the motor, moving clockwise/counter-clockwise, and changing speed.

## How to use example
This example requires a step and direction stepper driver.
Use the sdk configuration to set the pins and levels for the step, direction, and enable pins of the driver.

Use the sdk project configuration to set the GPIO pins for user control as well as speed configurations. 

### Build and flash
Build the project by entering the gpio_control_example directory from an espidf terminal and run the following commands. 
```
idf.py build
```
Flash the project to a device with the following command.
```
idf.py -p PORT flash monitor
```