import subprocess
from glob import glob
import sys

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


def unitTestFile(loxbinary, filepath):

    # just run the file and see if '// expected: something' is the next element in our stdout 
    def checkExpectedOut(filepath, outLines):
        with open(filepath) as f:
            for line in f:
                current = line.strip()
                if (len(outLines)<=0):
                    print ("sucess: " + filepath)
                    return True
                if ("expect: " +outLines[0]) in current:
                    #print("found: '"+ ("expect: " +outLines[0])+ "' == " + current)
                    outLines.pop(0)

        print(bcolors.FAIL +"FAILED: for file: "+ filepath +bcolors.ENDC)
        return False
    # get all lines our *.lox file outputs
    result = subprocess.run([loxbinary, filepath], capture_output=True, universal_newlines = True )
    outLines = result.stdout.splitlines() + result.stderr.splitlines()
    return checkExpectedOut(filepath, outLines)

def getAllLoxFiles(loxbinary, pathToTests):
    return glob(pathToTests, recursive=True)


loxbinary = sys.argv[1] #"./binary.out"
pathTestFiles = sys.argv[2].rstrip("/")+"/**/*.lox" #"./tests" + 
if len(sys.argv) != 3:
    print("lox-test, usage:\n\tlox-test [pathToBinary] [pathToLoxTestfiles]\n\tpython3 ./tests/unit.py ./binary.out ./tests")
    exit(2)
files = getAllLoxFiles(loxbinary, pathTestFiles)
print(F"found {len(files)} *.lox-files. Starting testing...")

good = 0
bad = 0
for file in files:
    didSucceed = unitTestFile(loxbinary,file)
    if didSucceed: good += 1
    else: bad +=1
print (F"Finished {len(files)} tests: {bcolors.OKGREEN}passed: {good}{bcolors.ENDC}, {bcolors.FAIL}failed: {bad}{bcolors.ENDC}")

if bad != 0: exit(1)
else: exit(0)