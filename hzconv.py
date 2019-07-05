#questionable python script I wrote up just for some math
#to figure out the various clocks and stuff we want -FIX94
import math

#going by nesdev, this is the master clock calc by definition
master_freq_ntsc = 236250000.0/11.0
print "master_freq_ntsc: " + str(master_freq_ntsc) + "Hz"
#the cpu runs every 12 cycles from there
cpu_freq_ntsc = master_freq_ntsc/12.0
print "cpu_freq_ntsc: " + str(cpu_freq_ntsc) + "Hz"
#in my emu, the apu audio updates divides again by 8 cycles
apu_calls_div_8 = cpu_freq_ntsc/8.0
print "apu_calls_div_8: " + str(apu_calls_div_8) + "Hz"
#finally, this is the "conversion ratio" to get that to 32kHz
val_for_32khz = apu_calls_div_8/32000.0
print "val_for_32khz: " + str(val_for_32khz) + " for division to 32kHz"

print "calculating closest add/total value combination"
ADD_VAL_32K = 0
TOTAL_VAL_32K = 0
besttotal = 0
bestdeviation = 0.999
tmpadd = 0

while(1):
    tmpadd = tmpadd+1
    tmptotal = val_for_32khz*tmpadd
    #break search after 0xFFFE just for easy 16bit cmplwi
    if(tmptotal > 65534.0):
        break
    tmpdeviation = tmptotal - float(int(tmptotal))
    if(tmpdeviation < bestdeviation):
        ADD_VAL_32K = tmpadd
        TOTAL_VAL_32K = int(tmptotal)
        besttotal = tmptotal
        bestdeviation = tmpdeviation

print "values to copy into apu.h:"
print "ADD_VAL_32K: " + str(ADD_VAL_32K)
print "TOTAL_VAL_32K: " + str(TOTAL_VAL_32K)
print "additional stat vals:"
print "besttotal: " + str(besttotal)
print "bestdeviation: " + str(bestdeviation)

#new way I now calculate things with, based much more on actually thought
#out data and much less total cycles of things being added up and compared
def confemu(cycles):
    totalClocks = 0
    apuClock2 = ADD_VAL_32K
    print "Calculating list position after " + str(cycles) + " calls"
    for i in xrange(cycles):
        #only save at 32kHz using nearest resample
        if apuClock2 >= TOTAL_VAL_32K:
            apuClock2 = apuClock2-TOTAL_VAL_32K
            #where we would save clipped output
            totalClocks = totalClocks+1
        apuClock2 = apuClock2+ADD_VAL_32K
    #print out how many positions we saved
    print "Next list position would be: " + str(totalClocks)
    return totalClocks

#the old way in previous builds, I hacked this together without too
#much thought and approximation using a calculator, not quite accurate
def confemu_old(cycles):
    totalClocks = 0
    apuClock2 = 0
    apuClock3 = 0
    print "Calculating list position after " + str(cycles) + " calls"
    for i in xrange(cycles):
        #only save at 32001Hz using nearest resample
        if apuClock2 == 56:
            if apuClock3 == 13:
                apuClock2 = 1 #so 55 cycles for next data block
                apuClock3 = 0
            else:
                apuClock2 = 0
                apuClock3 = apuClock3+1
            #where we would save clipped output
            totalClocks = totalClocks+1
        apuClock2 = apuClock2+1
    #print out how many positions we saved
    print "Next list position would be: " + str(totalClocks)
    return totalClocks

print "Enter how many seconds you want to run this demo: "
itr = int(input())
print "Running new way of resampling demo first"
#using my apu frequency which is only 1/8th of the cpu one
outnum = confemu(int(math.ceil(apu_calls_div_8*itr)))
print "After " + str(itr) + " seconds the frequency is:"
print str(float(outnum)/float(itr)) + "Hz"

print "Now running old way of resampling demo to compare"
#using full cpu frequency, eating lots more cycles as a result
outnum = confemu_old(int(math.ceil(cpu_freq_ntsc*itr)))
print "After " + str(itr) + " seconds the frequency is:"
print str(float(outnum)/float(itr)) + "Hz"
