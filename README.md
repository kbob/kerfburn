# Kerfburn

Firmware for a laser cutter.

I've been building a CNC laser cutter, and I'm writing my own firmware
for it.  What firmware I have written is here, but it's far from a
complete solution.

Current status:

   - Move, cut, home, and dwell are implemented.  (No engraving yet.)
   - Laser(s) fire in continuous, pulsed, or PPI mode.
   - Translates G-Code to firmware's command set.
   - Translates Postscript and PDF to firmware's command set using pstoedit.
   - There is no UI -- command line only.

This is the laser cutter I built.
http://www.buildlog.net/blog/2011/02/buildlog-net-2-x-laser/

My build log is here.
http://buildlog.net/buildlog/view_log.php?id=1505
http://www.buildlog.net/forum/viewtopic.php?f=16&t=1505
