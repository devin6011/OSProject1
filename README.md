# OSProject1

NTU CSIE Operating System Project 1 - Process Scheduling

Usage:

Compilation:

```
make
```

Run one testcase:

```
sudo ./scheduler < inputFile
```

Run all testcases:

```
sudo bash generateOutput.sh
```

Calculate one theoretical solution:

```
./theoretical < inputFile
```

Calculate all theoretical solution:

```
python3 genTheoreticalResult.py testcaseDir outputDir
```

Run evaluation:

```
python evaluate.py programOutputDir theoreticalSolutionDir
```