# SPC16_SBC
A single board computer based on the elusive Philips/Signetics SPC16/10 microprocessor.

Target of this project is to document the obscure SPC16/10 microprocessor.
This is done both by documenting the chip itself and design a single board computer around this processor.

This processor is a single-chip variant of the Philips P851 minicomputer. 
It dates around the 1982 timeframe.
The processor has a 16 bit wide datapath, and accesses 2 separate memories of 32Kx16 each ("Firmware" and "Software" memory.)
The instruction set is in common with the P851 minicomputer. 
Some additional instructions deal with the distiction between FWM(firmware) and SWM (softwarememory)
The SPC16/10 can also address up to 64 IO devices and 256 external registers. 

Other chips in the SPC16 system are the SPC16/11 interrupt controller and the SPC16/12 unified bus manager.
PINOUT :

![FAST_pinout](https://github.com/user-attachments/assets/b17f39b2-1407-43a2-ab6a-e7912ef892b7)




The SBC computer has been shown to work and is pictured here :
![front](https://github.com/user-attachments/assets/42818bc2-bb06-42cc-8cc9-f7295dcad3e4)



