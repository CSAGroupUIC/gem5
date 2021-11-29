# !/usr/bin/python3
#
# Submit gem5 jobs to server
# Author: Yicheng Wang
# Date: 12/28/2019
#
# All rights reserved.

import subprocess
import os
import sys
import time
from pathlib import Path

# SPEC CPU2006 Benchmarks
# Benchmark #447 #481 #483 don't work
# integer = ["400","401","429","445","456",\
#     "458","462","464","471","473","483"]
# floating = ["410","416","433","434","435",\
#     "436","437","444","447","450",\
#     "453","454","459","465","470",\
#     "481","482"]

# 510
intrate = ["500","502","505","520","523",\
    "525","531","541","548","557"]
fprate = ["503","507","508","511",\
    "519","521","526","527","538",\
    "544","549","554"]
BM = {}
BM['high1'] = [503,519,549,505]
BM['low1'] = [541,507,531,538]
BM['mix'] = [511,531,538,541]
# configure gem5 running environment
gem5_location = "~/Research/gem5-pwu27"
gem5 = os.path.expanduser(gem5_location +
    "/build/X86/gem5.opt")
script = os.path.expanduser(gem5_location +
    "/configs/example/se_edited.py")

# Run simulation on selected benchmark

def compile():
    RUN_DIR = os.path.expanduser(gem5_location)
    os.chdir(RUN_DIR)
    os.system("scons build/X86/gem5.opt -j16")

def createSimpleJobs(workload, ncpu):
    if workload == "all":
        workloads = intrate + fprate
    elif workload == "intrate":
        workloads = intrate
    elif workload == "fprate":
        workloads = fprate
    else:
        workloads = [workload]

    jobs = {}

    for workload in workloads:
        print(workload)
        assert(workload in (intrate + fprate)), "\
            Wrong workload name."
        binary = "--binary=" + workload
        num_cpu = "--num_cpu=" + ncpu
        jobs[workload] = [gem5,script,binary,num_cpu]

    return jobs

def createMultiBenchJobs(workload, ncpu):
    if workload == "high1":
        workloads = BM['high1']
    elif workload == "low1":
        workloads = BM['low1']
    elif workload == "mix":
        workloads = BM['mix']

    jobs = {}

    SPEC_DIR = "/home/pwu27/Research/SPEC06_17/"
    RUN_DIR = "/home/pwu27/Research/temp/" + workload
    if os.path.exists(RUN_DIR):
        os.system('rm '+'-rf '+RUN_DIR)
    os.mkdir(RUN_DIR)
    BENCH = workloads
    for bm in BENCH:
        # For each of the benchmarks,
        # copy all files and folders into RUN_DIR
        benchDir = SPEC_DIR + str(bm)
        os.chdir(benchDir)
        os.system('cp -R * ' + RUN_DIR)
    binary = "--binary=" + workload
    same_bench = "--same_bench=" + "0"
    run_dir = "--rundir=" + RUN_DIR
    num_cpu = "--num_cpu=" + ncpu
    jobs[workload] = [gem5,script,binary,num_cpu,same_bench,run_dir]
    return jobs

def runJobs(jobs, display=False, wait=10):
    # Create an active job list
    active_jobs = []
    # Run the jobs until the
    # threshold of max_active_jobs is reached
    job_no = 0
    total_jobs = len(jobs)
    for key in jobs.keys():
        # If max_active_jobs is reached,
        # sleep for a while and update
        # active job list
        while len(active_jobs) >= 2:
            time.sleep(wait)
            active_jobs = [x for x in active_jobs if x.poll() is None]

        # Submit the next job
        job_no = job_no + 1
        if (display):
            print("JOB # %d/%d" % (job_no, total_jobs))
            print("Preform simulation " + key)
        job = subprocess.Popen(jobs[key])
        active_jobs.append(job)
    # Wait for all jobs to finish
    while len(active_jobs) > 0:
        time.sleep(wait)
        active_jobs = [job for job in active_jobs if job.poll() is None]

if __name__ == "__main__":
    # python3 minirankSim.py
    #core bench multidiff
    if len(sys.argv) > 3:
        sys.exit(1)
    else:
        # compile()
        same_bench = True
        ncpu = sys.argv[1]
        if same_bench:
            jobs = createSimpleJobs(sys.argv[2], ncpu)
        else:
            jobs = createMultiBenchJobs(sys.argv[2], ncpu)
        runJobs(jobs,display=True)

