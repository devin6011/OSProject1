import sys
import os

outputPath = sys.argv[1]
theoreticalPath = sys.argv[2]

def calcDuration(x):
    return x[1] - x[0]

def getTime(line):
    return list(map(float, line.strip().split()[-2:]))

with open(os.path.join(outputPath, 'TIME_MEASUREMENT_dmesg.txt')) as f:
    executionTimes = [calcDuration(getTime(line)) for line in f]
    unitTime = sum(executionTimes) / len(executionTimes) / 500

print(f'Unit time = {unitTime}')

filenames = os.listdir(theoreticalPath)
#filenames.remove('TIME_MEASUREMENT')
filenames.sort()

totalAverageError = 0.0
totalAverageRelativeError = 0.0

for filename in filenames:
    print('#####' + filename)

    with open(os.path.join(theoreticalPath, filename)) as f:
        theoreticalAnswer = {line.strip().split()[0] : int(line.strip().split()[2]) - int(line.strip().split()[1]) for line in f}
    with open(os.path.join(theoreticalPath, filename)) as f:
        theoreticalTimePoint = {line.strip().split()[0] : (int(line.strip().split()[1]), int(line.strip().split()[2])) for line in f}

    with open(os.path.join(outputPath, filename.split('.')[0] + '_stdout.txt')) as f:
        pid2name = {line.strip().split()[1] : line.strip().split()[0] for line in f}

    with open(os.path.join(outputPath, filename.split('.')[0] + '_dmesg.txt')) as f:
        outputAnswer = {line.strip().split()[-3] : calcDuration(getTime(line)) / unitTime for line in f}
    with open(os.path.join(outputPath, filename.split('.')[0] + '_dmesg.txt')) as f:
        outputTimePoint = {pid2name[line.strip().split()[-3]] : (float(line.strip().split()[-2]) / unitTime, float(line.strip().split()[-1]) / unitTime, ) for line in f}

    error = {pid2name[pid] : outputAnswer[pid] - theoreticalAnswer[pid2name[pid]] for pid in pid2name}

    relativeError = {pid2name[pid] : (outputAnswer[pid] - theoreticalAnswer[pid2name[pid]]) / theoreticalAnswer[pid2name[pid]] for pid in pid2name}

    averageError = sum(map(abs, error.values())) / len(error)
    averageRelativeError = sum(map(abs, relativeError.values())) / len(relativeError)

    if filename != "TIME_MEASUREMENT.txt":
        totalAverageError += averageError
        totalAverageRelativeError += averageRelativeError

    print(f'Average Error: {averageError}')
    print(f'Average Relative Error: {averageRelativeError}')

    print('|Name|TD|RD|Error|RError|')
    print('|----|--|--|-----|------|')

    for name in error:
        tStart, tEnd = theoreticalTimePoint[name]
        rStart, rEnd = outputTimePoint[name]
        tDiff = tEnd - tStart
        rDiff = rEnd - rStart
        err = error[name]
        relativeErr = relativeError[name]
        print(f'|{name}|{tDiff}|{rDiff:.3f}|{err}|{relativeErr}|')

    print()

totalAverageError /= 20
totalAverageRelativeError /= 20

print(f'Total Average Error: {totalAverageError}')
print(f'Total Average Relative Error: {totalAverageRelativeError}')
