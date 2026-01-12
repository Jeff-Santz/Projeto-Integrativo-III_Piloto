RF-Link 125kHz - Custom Wireless Communication System

This repository centralizes the source code developed for the Custom Wireless Communication System, conceived within the scope of the Projeto Integrativo III (Integrative Project III) course, part of the Percurso Competências (Competencies Track - Pilot) at the Escola Politécnica da USP (Polytechnic School of the University of São Paulo).

The project consists of implementing a proprietary low-frequency (125 kHz) data network based on a linear mesh topology. It utilizes ESP32 microcontrollers to drive custom-built RF hardware, enabling wireless data transmission of sensor readings through modified On-Off Keying (OOK) modulation.

Development Team
The system was architected and developed by the students of the Percurso Competências:

Isadora Ribeiro Vital

Janos Biezok Neto

Jefferson Santos Monteiro

Jorge Ricardo Barbosa França

Repository Structure
For organization, maintenance, and evaluation purposes, the project was structured as a monorepo, divided into main directories corresponding to the functional nodes and hardware dependencies:

1. src (Firmware)
    This directory contains the source code for the three types of nodes in the network:

    Transmissor (Origin Node): Responsible for reading environmental data, formatting packets, and executing the 125kHz carrier modulation.

    Bypass (Repeater Node): Implements a Store-and-Forward mechanism to extend the network range.

    Receptor (Destination Node): Handles signal decoding, data integrity validation, and final output.

2. RadioHead-master (Modified Library)
This directory contains a customized version of the RadioHeadlibrary. The `RH_ASK` driver was modified to implement OOK modulation via software-generated PWM (125kHz) on GPIO 4. This change allows the ESP32 to interface directly with the custom discrete RF power stage without the need for commercial modules.

3. docs (Technical Documentation)
This directory contains the full technical report, including hardware schematics, PCB layouts designed for the 125kHz resonance, and the communication protocol specifications.

Technologies and Tools
Hardware: Espressif ESP32 (DevKit V1), Custom 125kHz Analog RF Stage.
Firmware: Arduino Framework (C++), FreeRTOS.
Protocols: Custom OOK (Physical Layer), JSON (Application Layer).
Techniques: Software PWM Modulation, Predictive Deep Sleep, NTP Synchronization.

Credits and Third-Party References
The development of this project relied on the **RadioHead** library, authored by Mike McCauley. 

Library Customization:
To meet the specific requirements of the project's analog hardware, the `RH_ASK::writeTx` function was heavily modified. Instead of standard digital output, the driver was adapted to drive the ESP32 LEDC (PWM) peripheral. This ensures that a 125kHz carrier wave is generated whenever a logic '1' is transmitted, fulfilling the requirements for inductive coupling and RF transmission at low frequencies.

Note on Versioning
This repository consolidates modules that were developed and validated in a laboratory environment. Temporary build files, binaries, and local dependencies were excluded via `.gitignore` to ensure source code integrity and cleanliness.