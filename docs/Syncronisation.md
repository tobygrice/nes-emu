The PPU runs at three times the speed of the CPU. That is, for every one CPU cycle, there are three PPU cycles.

Initially, I plan to implement the common and most simple "catch-up" method. This involves letting the CPU execute an entire instruction, then releasing the PPU for three times the number of cycles it took the CPU to execute that instruction. This is the least accurate option to emulate original hardware, but in most cases is "good enough".

The most commonly implemented syncronisation method is allowing the CPU to execute for **one cycle only**, then releasing the PPU for three. This often means stopping the CPU mid-instruction to allow the PPU to execute. 

The final option is most accurate to true hardware, but also the most processing-intensive and difficult to implement. It involves running each element of the NES in parallel threads, and coordinating timing with a master clock (as on original hardware). This requires very careful shared memory management to prevent race conditions or unexpected errors. This approach is very uncommon, as it is unnecessarily complex and better performance can be attained from serial execution. However, I am very interested in it as a distributed systems exercise, and from purist perspective this options sounds far more interesting.

My plan is to first implement the catch-up method, then once the emulator is complete and functioning correctly, refactor the code to implement the final option.