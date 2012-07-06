#! /usr/bin/env python

import sys
import os
import re
from subprocess import Popen

test_dir = "Compression_3x1x1/np3"
base_name = "Compression_3x1x1"

if __name__ == "__main__":

    result = 0

    # log file will be dumped if verbose option is given
    verbose = False
    if "-verbose" in sys.argv:
        verbose = True

    # change to the specified test directory
    os.chdir(test_dir)

    # open log file
    log_file_name = base_name + ".log"
    if os.path.exists(log_file_name):
        os.remove(log_file_name)
    logfile = open(log_file_name, 'w')

    # remove old output files, if any
    # use regular expression module since differentiating
    # between gold files and old output files can be tricky
    files_to_remove = base_name + ".e"
    for file in os.listdir(os.getcwd()):
      if file in files_to_remove:
        os.remove(file)

    # run Peridigm
    command = ["mpiexec", "-np", "3", "../../../../src/Peridigm", "../"+base_name+".xml"]
    p = Popen(command, stdout=logfile, stderr=logfile)
    return_code = p.wait()
    if return_code != 0:
        result = return_code

    # compare output files against gold files
    command = ["../../../../scripts/epu", "-p", "3", base_name]
    p = Popen(command, stdout=logfile, stderr=logfile)
    return_code = p.wait()
    if return_code != 0:
        result = return_code
    command = ["../../../../scripts/exodiff", \
                   "-stat", \
                   base_name+".e", \
                   "../"+base_name+"_gold.e", \
                   "-f", \
                   "../"+base_name+".comp"]
    p = Popen(command, stdout=logfile, stderr=logfile)
    return_code = p.wait()
    if return_code != 0:
        result = return_code

    logfile.close()

    # dump the output if the user requested verbose
    if verbose == True:
        os.system("cat " + log_file_name)

    sys.exit(result)
