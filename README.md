# MFOS-16-Stage-programmable-Phaser
An update of the MFOS 8 Stage phaser to 16 stages plus a programmer interface to store patches and MIDI

I built the MFOS 8 stage phaser and added another 8 stages to it, simple enough to replicate the extra 8 stages.

Here is a link to the MFOS project.

http://musicfromouterspace.com/analogsynth_new/PHASESHIFTER2007/PHASESHIFTER2007.php

I wanted to add this phaser to my programmable Crumar Trilogy to phase the strings and also make the phaser programmmable. So I set out firstly to make the Phaser CV controlled.

Easiest way was to replace the LFO section of the MFOS with an Electric Druid TapLFO3D based LFO which can be CV controlled for rate, level, waveform and many other options, but I only chose the fist 3.

I then added an AS3364 Quad VCA to control the feedback level, this requires a CV of 0-2V. Although I only used one of the VCA channels, it was all I had available.

To select the stages I used the MAX308 mux chip and using 5v logic like in a 4051 I can chose the 4, 8, 12 or 16 stages of the phaser.

Finally I used a 4053 to switch between external clock or MIDI clock for the LFO, it will free run until any of these are applied and selected.

Once it was all under CV control it was fairly straight forward to add a Teensy based controller and a couple of DACs to generate the 4 CV's and digital signals required to control the routing. I used a Teensy 2++ as that had enough I/O without the need for shift registers etc and I had a few spare laying around, the Teensy 2 did not have enough memory for the program and the T3.5 or 6 was overkill.

# Specifications are as follows

* 16 Stage phasing using LM13700 OTA
* LFO rate from 0.05Hz to 12.8Hz
* 8 LFO waveforms selectable
* LFO depth
* Feedback Level
* Push button stage selection 4/8/12/16
* MIDI Channel selection
* Encoder type selection
* External or MIDI clock selection
* Program Change and CC enabled over MIDI
* External clock input 0-5V
* 128 Patch Memories with Patch naming
* MIDI in/Thru


