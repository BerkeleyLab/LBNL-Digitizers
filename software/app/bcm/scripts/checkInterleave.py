from __future__ import print_function
import argparse
import math
import sys

parser = argparse.ArgumentParser(description='Find valid clock chain configurations.', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-b', '--buckets', type=int, default=328, help='Buckets per turn')
parser.add_argument('-d', '--divisor', type=int, default=4, help='Frf divisor to yield LMX2594 reference)')
parser.add_argument('-r', '--rf', type=float, default=499.64, help='Ring RF frequency (MHz)')
parser.add_argument('-p', '--pll', action='store_true', help='Enable FPGA PLL')
args = parser.parse_args()
if args.pll:
    mulFPGArange = range(1,25)
    divFPGArange = range(1,5)
else:
    mulFPGArange = range(1,2)
    divFPGArange = range(1,2)

Frf = args.rf * 1.0e6
Frev = Frf / args.buckets
Fcleaner = Frf / args.divisor
Trev = 1.0 / Frev

mulSynthLo = int(math.ceil(7.2e9 /  Fcleaner))
mulSynthHi = int(math.floor(14.8e9  / Fcleaner))
firstTime = True
for mulSynth in range(mulSynthLo, mulSynthHi + 1):
    for (fracNum, fracDen) in (
        (0,1),
        (1,2),
        (1,3),(2,3),
        (1,4),(3,4),
        (1,5),(2,5),(3,5),(4,5),
        (1,6),(5,6),
        (1,7),(2,7),(3,7),(4,7),(5,7),(6,7),
        (1,8),(3,8),(5,8),(7,8),
        (1,9),(2,9),(4,9),(5,9),(7,9),(8,9),
        (1,10),(3,10),(7,10),(9,10),
        (1,11),(2,11),(3,11),(4,11),(5,11),(6,11),(7,11),(8,11),(9,11),(10,11),
        (1,12),(5,12),(7,12),(11,12),
        (1,13),(2,13),(3,13),(4,13),(5,13),(6,13),(7,13),(8,13),(9,13),(10,13),(11,13),(12,13),
        (1,14),(3,14),(5,14),(7,14),(9,14),(11,14)):
        fracSynth = float(fracNum) / float(fracDen)
        FsynthVCO = Fcleaner * (mulSynth + fracSynth)
        if FsynthVCO < 7.5e9 or FsynthVCO > 15e9: continue
        for divSynth in (2, 4, 6, 8):
            Fsynth = FsynthVCO / divSynth
            if (args.pll and Fsynth > 1.2e9): continue
            for mulFPGA in mulFPGArange:
                for divFPGA in divFPGArange:
                    Fsamp = Fsynth * mulFPGA / divFPGA
                    if Fsamp < 3.15e9 or Fsamp > 3.75e9: continue
                    Tsamp = 1.0 / Fsamp
                    tMin = 1e6
                    readStride = 1
                    t = 0.0
                    i = 0
                    while (True):
                        i += 1
                        t += Tsamp
                        if (t >= (Trev - Trev / 1000000)):
                            t -= Trev
                        if (abs(t) < 1e-12):
                            if (i % args.buckets) == 0:
                                if firstTime:
                                    print("F_RF:%.7g  F_REF:%.7g  %d buckets per turn" % (Frf, Fcleaner, args.buckets))
                                    print("samplesPerTurn samplesPerBunch storeStride readStride mulSynth fracNum fracDen fracSynth divSynth mulFPGA divFPGA FsynthVCO Fsynth Fsamp Tsamp")
                                    sys.stdout.flush()
                                    firstTime = False
                                samplesPerBunch = i / args.buckets
                                storeStride = math.floor(Frf * samplesPerBunch / Fsamp + 0.5)
                                print("%d %d %d %d %d %d %d %.3f %d %d %d %.2f %.2f %.2f %.2f" % (i, samplesPerBunch, storeStride, readStride,mulSynth, fracNum, fracDen, fracSynth, divSynth, mulFPGA, divFPGA, FsynthVCO / 1e6, Fsynth / 1e6, Fsamp / 1e6, tMin * 1e12))
                                sys.stdout.flush()
                            break
                        if (t < tMin):
                            tMin = t
                            readStride = i
