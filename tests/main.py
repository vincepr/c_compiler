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

def getAllLoxFiles(loxbinary, pathToTests):
    return glob(pathToTests, recursive=True)



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
                    if "expect: "+outLines[0] in line :
                        outLines.pop(0) # we matched this output
                    else:
                        print(F"{FAILED} {PATHTESTED} @line[{idx}] '{line}' got: '{outLines[0]}'")
                        return False
            ## we ran trough all lines check if we matched all errors and output from our cmd
            if len(outLines)!=0 and len(errLines)!=0:
                print(F"{FAILED} {PATHTESTED} to many Errors or std-Output\n")
                return False
            else: return True

            


def unitTestFile(loxbinary, filepath):

    # just run the file and see if '// expected: something' is the next element in our stdout 
    def checkExpectedOut(filepath, outLines):
        with open(filepath) as f:
            for line in f:
                current = line.strip()
                if (len(outLines)<=0):
                    #print ("sucess: " + filepath)
                    return True
                if ("expect: " +outLines[0]) in current:
                    #print("found: '"+ ("expect: " +outLines[0])+ "' == " + current)
                    outLines.pop(0)

        print(bcolors.FAIL +"FAILED: " + loxbinary + " " + filepath +bcolors.ENDC)
        return False
    # get all lines our *.lox file outputs
    #print(F"starting file: {filepath}")
    result = subprocess.run([loxbinary, filepath], capture_output=True, universal_newlines = True )
    outLines = result.stdout.splitlines() + result.stderr.splitlines()
    return checkExpectedOut(filepath, outLines)


## our main process:
loxbinary = sys.argv[1] #"./binary.out"
pathTestFiles = sys.argv[2].rstrip("/")+"/**/*.lox" #"./tests" + 
if len(sys.argv) != 3:
    print("lox-test, usage:\n\tlox-test [pathToBinary] [pathToLoxTestfiles]\n\tpython3 ./tests/unit.py ./binary.out ./tests")
    exit(2)
files = getAllLoxFiles(loxbinary, pathTestFiles)
print(F"found {len(files)} *.lox-files. Starting testing...")

bad = 0
for file in files:
    didSucceed = testFile(loxbinary,file)
    if not didSucceed: bad += 1

print (F"{bcolors.HEADER}Finished {len(files)} tests: {bcolors.ENDC} {bcolors.OKGREEN}passed: {len(files)-bad}{bcolors.ENDC}, {bcolors.FAIL}failed: {bad}{bcolors.ENDC}")

if bad != 0: exit(1)
else: exit(0)