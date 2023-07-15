import subprocess
from glob import glob
import sys

# quick and easy test-suite
# usage:
#       python3 [pathTo/tester.py] [pathTo/binary.out] [pathToLoxTestfiles/tests]
#
# - it just runs every *.lox file in the specified folder.
# - "// expect: 1234" to expect 1234 as print output in that line (outputs get parsed one after the other)
# - "// [line 22] Error message expected" errors can be written at any place in the file


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def getAllLoxFiles(loxbinary, pathToTests):
    return glob(pathToTests, recursive=True)

def substring_after(s, delim):
     return s.partition(delim)[2]

# executes file and checks for expected output marked with "expect: ....."
# for errors we just get them and search if they are referenced anywhere in the file -> then were fine
def testFile(loxbinary, filepath):
    result = subprocess.run([loxbinary, filepath], capture_output=True, universal_newlines = True )
    outLines = result.stdout.splitlines()
    errLines = result.stderr.splitlines()
    with open(filepath) as f:
            idx = 0 # line-nr
            FAILED = F"{bcolors.FAIL}FAILED:{bcolors.ENDC}"
            PATHTESTED = F"{bcolors.WARNING}{loxbinary} {filepath}{bcolors.ENDC}"
            for line in f:
                idx+=1
                for err in errLines:
                    if err in line: errLines.remove(err) #  we just see if our errors get mentioned somewhere
                if "expect: " in line:                  # check if we expect an std-output in this line?
                    if len(outLines) <=0:
                        print(F"{FAILED} {PATHTESTED} @line[{idx}] '{line.strip()}'")
                        return False
                    if outLines[0].strip() == substring_after(line, "expect: ").strip() :
                        outLines.pop(0) # we matched this output
                    else:
                        print(F"{FAILED} {PATHTESTED} @line[{idx}] '{line}' got: '{outLines[0]}'")
                        return False
            ## we ran trough all lines check if we matched all errors and output from our cmd
            if len(outLines)!=0 or len(errLines)!=0:
                print(F"{FAILED} {PATHTESTED} to many Errors or std-Output {outLines}{errLines}")
                return False
            else: return True

## our main process:
loxbinary = sys.argv[1] #"./binary.out"
pathTestFiles = sys.argv[2].rstrip("/")+"/**/*.lox" #"./tests" + 
if len(sys.argv) != 3:
    print("lox-test, usage:\n\tlox-test [pathToBinary] [pathToLoxTestfiles]\n\tpython3 ./tests/unit.py ./binary.out ./tests")
    exit(2)
files = getAllLoxFiles(loxbinary, pathTestFiles)
print(F"Found {bcolors.WARNING}{len(files)} *.lox-files.{bcolors.ENDC} Starting testing...")

count = 0
bad = 0
for count, file in enumerate(files, start=1):
    didSucceed = testFile(loxbinary,file)
    if not didSucceed: bad += 1
    if count%50 == 0:
            print (F"{bcolors.HEADER}Finished {count} tests:\t{bcolors.ENDC}{bcolors.OKGREEN}passed: {count-bad}{bcolors.ENDC}, \t{bcolors.FAIL}failed: {bad}{bcolors.ENDC}")


if bad != 0: 
    print (F"{bcolors.HEADER}Completed {count} tests: {bcolors.ENDC} {bcolors.OKGREEN}passed: {count-bad}{bcolors.ENDC}, {bcolors.FAIL}failed: {bad}{bcolors.ENDC}")
    exit(1)
else: 
    print (F"{bcolors.OKGREEN}Completed all {count} tests: passed: {count-bad} failed: {bad}{bcolors.ENDC},")
    exit(0)