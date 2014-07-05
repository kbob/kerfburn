# kbmon - kerfburn monitor

A monitor screen that runs in a terminal.

kbmon is a friendly window into the laser cutter's state.  It displays
the firmware's variables, faults, and other state.  And most
importantly, it shows three status lines at the top of the screen:

  * **Overall Status**: running, idle, error?
  * **Laser Status**: Which laser is active?  What is keeping it from firing?
  * **Motor Status**: Which motors are enabled?  Why are the others disabled?


## Run kbmon with simulated firmware.

There is a firmware simulator here called `mock-fw`.  It just
simulates reading and writing all the variables and implements
the various report functions with dummy data.

Start the simulated firmware like this.

    $ thruport daemon --debug --fw-simulator=./mock-fw
    
Connect to it (or to the real firmware) by running kbmon in another
window

    $ ./kbmon
    
    
