# SWAGGER
## A Tool for programming ARM microcontrollers via SWD

Swagger is flash programming tool for ARM microcontrollers based on the [Squirrel scripting language](http://www.squirrel-lang.org/).

The Suirrel scripts have access to a very low-level wrapper of the SWD protocol. This allows new processors or features to be added without having to recompile the Swagger binary.

Communication with the programming adapter hardware is done through the standard USB serial communications protocol. Any 3.3V powered board that supports this interface, such as an Arduino Due, can be used.

Currently only the Freescale/NXP MKV10Z32 processor is supported.

Note: this is work-in-progress.
