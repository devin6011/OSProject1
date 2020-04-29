import sys
import os
import subprocess

inputDir = sys.argv[1]
outputDir = sys.argv[2]

for x in os.listdir(inputDir):
    with open(os.path.join(inputDir, x)) as inputFile:
        with open(os.path.join(outputDir, x), 'w') as outputFile:
            subprocess.run(['./theoretical'], stdin=inputFile, stdout=outputFile)
