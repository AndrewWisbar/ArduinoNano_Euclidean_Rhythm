# ArduinoNano_Euclidean_Rhythm
Source code and plans for a Eurorack format Euclidean Rhythm Generator, with an arbitrary number of independent channels, and real-time user controls for various parameters. 

## Current Features

- Independent channels (4 by default)
- User input for the overall rate/tempo
- User input for the number of active beats on a per channel basis
- User input for the number of total steps on a per channel basis
- User input for rotating the generated pattern of beats on a per channel basis
- Display using the AdaFruit NeoPixel system

## Planned Features

- Design for a eurorack panel + stripboard/pcb layout
- Circuit Shematics
- A CV control system to allow external signals to control the module, including an external clock signal for the tempo
- Switch the timing of the unit to use the arduinos built in timers, instead of the millis() function
- A 7 segment display to show the tempo of the module in BPM

## Known Issues

- Current timing system often produces inconsistent timing, especially an apparent pause when going from step 15 back to step 0

## Description

This project is meant to be used in a Eurorack context to generate gate signals whose patterns follow a Euclidean Rhythm.
In very simple terms a Euclidean Rhythm is a rhythm that is created by distributing a given number of beats as evenly as possible across a given time. 

For example a pattern of 4 beats and 16 steps would give the following pattern:

![4 beats distributed on 16 steps](/assets/images/4_16.PNG)

And a pattern of 10 beats and 16 steps:

![10 beats distributed on 16 steps](/assets/images/10_16.PNG)

The rotation control allows the user to shift the beginning of the pattern by 0 - 15 steps so with the original pattern:

![5 beats distributed on 16 steps](/assets/images/5_16.PNG)

Adding a rotation of 2 would result in the following pattern:

![5 beats distributed on 16 steps with a rotation of two steps](/assets/images/5_16_rot2.PNG)

When the pattern has completed it moves back to the beginning of the pattern and as such, Euclidean Rhythms are often depicted as circular.

If a rotation applied to the pattern would cause an active beat to move past the number of total steps (outside the bar in the above images), then that beat is likewise moved to the beginning of pattern (beat 15 would become beat 0)
