# About this Project  
I created this to improve on control and audio issues found in the NES Ports of Mega Man 1 to 6 in the GameCube Mega Man Anniversary Collection.  
As of right now the audio patches that completely replace how audio is played back apply to Mega Man 1-3 and the control patches that invert the A and B buttons to fix its controls apply to all included NES titles (Mega Man 1-6).  
Now instead of playing through low quality recordings, I instead added a stripped down version of my NES emulator to play it back exactly as the original.  
This also corrects audio issues such as files not looping/looping incorrectly and straight up wrong songs being played at times.    

# How to apply this  
Simply go to the releases tab and grab the current executable, then you just have to drag and drop your .iso file in, that should be it!  
Make sure to keep a backup of it in case something goes wrong, and if anything does go wrong, maybe open up a issue on this repository for me to look into it.    

# How to compile  
Just have a look at the build.bat, specifically you need some form of gcc to compile the executables for your pc and devkitPPC r29-1 for the gamecube files.  
It probably could be somehow done with a newer devkitPPC version but thats just what most of my stuff used for some years so I know how to work with it.    

In the future I will look at the other NES titles and try patch them too, I just recently remembered a fact that made me want to make this project,
for some reason all the NES titles have the original ROM in a weird way stored in them, going completely unused and being in japanese and having only
1/4th of their graphics still in place, I assume at some point they used the rest as some form of pointers to some graphics stored elsewhere.  
Thankfully, the code in these ROMs is untouched, meaning it still works perfectly fine when trying to play back audio from it :)
