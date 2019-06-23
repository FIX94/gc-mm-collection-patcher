# About this Project  
I created this to improve on control and audio issues found in the NES Ports of Mega Man 1 to 6 in the GameCube Mega Man Anniversary Collection.  
As of right now all included NES titles (Mega Man 1 to 5) get audio patches that completely replace how audio is played back and also control patches that invert the A and B buttons to fix its controls (it was backwards in the collection).  
Now instead of playing through low quality recordings, I instead added a stripped down version of my NES emulator to play it back exactly as the original.  
This also corrects audio issues such as files not looping/looping incorrectly and straight up wrong songs being played at times.   Also there is a small patch in place that allows you to hold start when the game starts up to skip over all the logo and intro movies straight into the menu. I created this patch mainly because when I play from sd card on my gamecube, it often crashes when it tries to play back the intro movies, maybe this is useful if you run into a similar issue.  
One note regarding the audio patches and Mega Man 6, you may notice the intro music is different to what you are used to, this is due to the NES ROM included in the game being japanese, which has a shorter intro and thus a different intro tune than the US version, leading to the intro now having some silence for a bit. Also in Mega Man 6 after you get a weapon, the text that normally tells you what you got had a sound effect in the original NES version playing on each letter, this is absent here, for some reason this port never sends the commands to play that sound.    

# How to apply this  
Simply go to the releases tab and grab the current executable, then you just have to drag and drop your .iso file in, that should be it!  
Make sure to keep a backup of it in case something goes wrong, and if anything does go wrong, maybe open up a issue on this repository for me to look into it.    

# How to compile  
Just have a look at the build.bat, specifically you need some form of gcc to compile the executables for your pc and devkitPPC r29-1 for the gamecube files.  
It probably could be somehow done with a newer devkitPPC version but thats just what most of my stuff used for some years so I know how to work with it.    

This project was only possible thanks to all of the NES titles having the original ROM in a weird way stored in them, going completely unused and being in japanese and having only 1/4th of their graphics still in place, I assume at some point they used the rest as some form of pointers to some graphics stored elsewhere.  
Thankfully, the code in these ROMs is untouched, meaning it still works perfectly fine when trying to play back audio from it :)
