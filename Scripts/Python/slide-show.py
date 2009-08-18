# Display all patterns in Golly's Patterns folder.
# Author: Andrew Trevorrow (andrew@trevorrow.com), March 2006.

import golly as g
import os
# from os.path import join
# NOTE: We must not do the above import because an opened script might
# do "from golly import *" and from then on join(root, name) will call
# golly's join command, with very odd results!
import time

# ------------------------------------------------------------------------------

def slideshow ():
   oldalgo = g.getalgo()
   oldrule = g.getrule()
   
   message = "Hit space to continue or escape to exit the slide show..."
   g.show(message)
   for root, dirs, files in os.walk(g.getdir("app") + "Patterns"):
      for name in files:
         if name.startswith("."):
            # ignore hidden files (like .DS_Store on Mac)
            pass
         else:
            fullname = os.path.join(root, name)
            g.open(fullname, False)       # don't add file to Open/Run Recent submenu
            g.update()
            if name.endswith(".pl") or name.endswith(".py"):
               # reshow message in case it was changed by script
               g.show(message)
            
            while True:
               ch = g.getkey()
               if ch == " ": break
               g.dokey(ch)                # allow keyboard interaction
               time.sleep(0.01)           # avoid hogging cpu
      
      if "CVS" in dirs:
         dirs.remove("CVS")  # don't visit CVS directories
   
   # if all patterns have been displayed then restore original algo and rule
   # (don't do this if user hits escape in case they want to explore pattern)
   g.new("untitled")
   g.setalgo(oldalgo)
   g.setrule(oldrule)

# ------------------------------------------------------------------------------

# show status bar but hide other info to maximize viewport
oldstatus = g.setoption("showstatusbar", True)
oldtoolbar = g.setoption("showtoolbar", False)
oldlayerbar = g.setoption("showlayerbar", False)
oldeditbar = g.setoption("showeditbar", False)
oldscripts = g.setoption("showscripts", False)
oldpatterns = g.setoption("showpatterns", False)

try:
   slideshow()
finally:
   # this code is always executed, even after escape/error;
   # clear message line in case there was no escape/error
   g.show("")
   # restore original state
   g.setoption("showstatusbar", oldstatus)
   g.setoption("showtoolbar", oldtoolbar)
   g.setoption("showlayerbar", oldlayerbar)
   g.setoption("showeditbar", oldeditbar)
   g.setoption("showscripts", oldscripts)
   g.setoption("showpatterns", oldpatterns)
