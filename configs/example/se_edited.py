# This file is used as a debug run script using command line:
# ./build/X86/gem5.opt configs/example/se_edited.py
# build/Z86/gem5.opt configs/example/se.py -c \
#     /home/pwu27/Research/CPU2017/502.gcc_r/run/run_b\
#       ase_refrate_O2Static-m64.0000/cpugcc_r_base.O2Static-m64 -o \
#         'ref32.c -03 -fselective-scheduling -fselective-scheduling2\
#           ' --cpu-type=Deriv03CPU --caches\
#             --mem-type=DDR4_2400_8x8
from __future__ import print_function
from __future__ import absolute_import

import optparse
import argparse

import sys
import os
import specCPU
import specCPU_new

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.params import NULL
from m5.util import addToPath, fatal, warn

gem5 = "gem5-pwu27"
addToPath('../')

from ruby import Ruby

from common import Options
from common import Simulation
from common import CacheConfig
from common import CpuConfig
from common import ObjectList
from common import MemConfig
from common.FileSystemConfig import config_filesystem
from common.Caches import *
from common.cpu2000 import *

def get_processes(options):

    multiprocesses = []
    inputs = []
    outputs = []
    errouts = []
    pargs = []

    workloads = options.cmd.split(';')
    if options.input != "":
        inputs = options.input.split(';')
    if options.output != "":
        outputs = options.output.split(';')
    if options.errout != "":
        errouts = options.errout.split(';')
    if options.options != "":
        pargs = options.options.split(';')

    idx = 0
    for wrkld in workloads:
        process = Process(pid = 100 + idx)
        process.executable = wrkld
        # os.chdir(options.rundir)
        os.chdir(options.rundir)
        process.cwd = os.getcwd()

        if options.env:
            with open(options.env, 'r') as f:
                process.env = [line.rstrip() for line in f]

        if len(pargs) > idx:
            process.cmd = [wrkld] + pargs[idx].split()
        else:
            process.cmd = [wrkld]

        if len(inputs) > idx:
            process.input = inputs[idx]
        if len(outputs) > idx:
            process.output = outputs[idx]
        if len(errouts) > idx:
            process.errout = errouts[idx]

        multiprocesses.append(process)
        idx += 1

    if options.smt:
        assert(options.cpu_type == "DerivO3CPU")
        return multiprocesses, idx
    else:
        return multiprocesses, 1


# parser = optparse.OptionParser()
# Options.addCommonOptions(parser)
# Options.addSEOptions(parser)


parser = argparse.ArgumentParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)

# args = parser.parse_args()


parser.add_argument("--binary",action="store", type=str,
    default=None,
    help="base names for --take-checkpoint \
        and --checkpoint-restore")
parser.add_argument("--rundir",
    default=os.path.expanduser("~/Research/"+gem5+"/"),
    help="Set current working directory. ")
parser.add_argument("--num_cpu", action="store", type=int,
    default=None,
    help="number of cores for multicore configs")
parser.add_argument("--same_bench", action="store", type=int,
    default=1,
    help="use different checkpoint for not same benchmarks")
if '--ruby' in sys.argv:
    Ruby.define_options(parser)

# (options, args) = parser.parse_args()
args = parser.parse_args()
print(args)

### configurations

# binary
# args.binary = "500"
# args.num_cpu = 4
binary = args.binary
num_cpus = args.num_cpu
same_bench = args.same_bench
run_dir = args.rundir
# raim
enable_dram_powerdown = False
enable_dram_self_refresh = False  # test
enable_raim5 = False
mem_type = "DDR4_2400_8x8"
flags = "refresh"

# system
sys_voltage = '1.0V'
sys_clock = '4GHz'
cpu_voltage_domain = '1.0V'
cpu_clock = "4GHz"

# cpu
cpu_type = "DerivO3CPU"  ##
# wrong! restore_with_cpu  standard switch?
mem_size = "8GB"#  for same workloads
# 4 diff bench"16GB"
cache_line_size = 64

# external cache
caches = True
l2cache = True
l1i_size = "32kB"
l1i_assoc = 2
l1d_size = "64kB"
l1d_assoc = 2
l2_size = "2MB"
l2_assoc = 4

# memory
mem_channels = 1
mem_ranks = 2

# simulation
at_instruction = True
instructions = 20000000#20000000
# warmup_insts = 2000000
# fast_forward = 2000000  # add more inst

# checkpoint directory
if same_bench != 0 :
    checkpoint = "~/Research/NewCheckpoints/checkpoints/" + binary
    if int(binary) < 500:
        workload = specCPU.CPU2006(binary)
        checkpoint_dir = os.path.expanduser("~/Research/new_ckpts\
            /ckpts2006/singleCore/" + binary)
    elif int(binary) < 600:
        workload = specCPU.CPU2017(binary)
        if num_cpus == 1:
            checkpoint_dir = os.path.expanduser(checkpoint + "/1-core")
        elif num_cpus == 4:
            checkpoint_dir = os.path.expanduser(checkpoint + "/4-core")
        elif num_cpus == 8:
            checkpoint_dir = os.path.expanduser(checkpoint + "/8-core")
        else:
            print("Error: Only support 1,4,8 cores")
    else:
        print("Error: Only support workloads from 400-599.")
        sys.exit(1)
    args.cmd = workload.executable
    print("cmd is " + args.cmd)
    args.input =  workload.Input
    args.output = workload.output
    args.options = workload.options
    args.rundir = workload.filePath
    args.errout = workload.errout
    for i in range(2,num_cpus + 1):
        if len(args.cmd):
            args.cmd += ";" + workload.executable
        print("cmd is " + args.cmd)
        if len(args.input):
            args.input += ";" + workload.Input
        if len(args.output):
            args.output += ";" + workload.output
        if len(args.options):
            args.options += ";" + workload.options
    simpt = int(workload.simpt*(1e7))
else:
    key = binary
    BM = {}
    BM['high1'] = [503,519,549,505]
    BM['low1'] = [541,507,531,538]
    BM['mix'] = [511,531,538,541]
    checkpoint = "~/Research/NewCheckpoints/checkpoints/" + key
    checkpoint_dir = os.path.expanduser(checkpoint + "/4-core")
    temp = BM[key]
    b = temp[0]
    if int(b) < 500:
        workload = specCPU_new.CPU(int(b))
    elif int(b) < 600:
        workload = specCPU_new.CPU(int(b))
    args.cmd = workload.executable
    print("cmd is " + args.cmd)
    args.input =  workload.Input
    args.output = workload.output
    args.options = workload.options
    # args.rundir = workload.filePath
    args.errout = workload.errout
    for i in range(1,num_cpus):
        b = temp[i]
        if int(b) < 500:
            workload = specCPU_new.CPU(int(b))
        elif int(b) < 600:
            workload = specCPU_new.CPU(int(b))
        if len(args.cmd):
            args.cmd += ";" + workload.executable
        print("cmd is " + args.cmd)
        if len(args.input):
            args.input += ";" + workload.Input
        if len(args.output):
            args.output += ";" + workload.output
        if len(args.options):
            args.options += ";" + workload.options
    simpt = key    # new checkpoint updated

print(args.input)
print(args.output)
print(args.options)
print(args.cmd)
print(args.rundir)
# set up args with parameters above
# args.binary = binary
# binary = args.binary
args.sys_voltage = sys_voltage
args.sys_clock = sys_clock
args.cpu_clock = cpu_clock

args.num_cpus = num_cpus
args.cpu_type = cpu_type
args.restore_with_cpu = cpu_type
args.mem_size = mem_size
args.cacheline_size = cache_line_size

args.caches = caches
args.l2cache = l2cache

args.mem_type = mem_type
args.mem_channels = mem_channels
args.mem_ranks = mem_ranks

args.instructions = instructions
args.maxinsts = instructions
# args.warmup_insts = warmup_insts
# args.fast_forward = fast_forward
args.at_instruction = at_instruction

args.checkpoint_dir = checkpoint_dir
#DEBUG
print("Checkpoint Directory: " + args.checkpoint_dir + "\n")

#DEBUG
# print(simpt)
args.checkpoint_restore = simpt
args.enable_raim5 = enable_raim5
# args.enable_dram_powerdown = enable_dram_powerdown
# args.enable_dram_self_refresh = enable_dram_self_refresh


### Display simulation configurations
print("\n" + "=" * 30)
print("Simulation workload is " + args.binary + "\nThe simulation \
will run " + str(instructions) + " instructions on \
    " + str(num_cpus) + " cpus.\n\
This memory system consists of " + str(args.mem_channels) +" channels.")
print("=" * 30 + "\n")

if num_cpus == 4:
    foldercore = "quadcore"
elif num_cpus == 1:
    foldercore = "singlecore"

if "6400" in mem_type:
    folder = "6400"
elif "2400" in mem_type:
    folder = "2400"
elif "3200" in mem_type:
    folder = "3200"

if "3200_debug" in mem_type:
    folder = "3200_debug"

if "x8" in mem_type:
    folderWidth = "x8"
elif "x16" in mem_type:
    folderWidth = "x16"
else:
    folderWidth = "x4"

if not enable_dram_powerdown:
    folderPower = "no_powerdown"
elif not enable_dram_self_refresh:
    folderPower = "no_self_refresh"
else:
    folderPower = "powerdown"
    ## normal powerdown mode with self refresh enabled

print(binary)
if "raim5" in mem_type:
    outdir = os.path.expanduser("~/Research/Gem5Output/\
        " + foldercore + "/" + folder + "/" + folderWidth + "/\
            " + folderPower + "/raim5/"+ binary)
elif "mr" in mem_type:
    outdir = os.path.expanduser("~/Research/Gem5Output/\
        " + foldercore + "/" + folder + "/" + folderWidth + "/\
            " + folderPower + "/mr/"+ binary)
else:
    outdir = os.path.expanduser("~/Research/Gem5Output/\
        " + foldercore + "/" + folder + "/" + folderWidth + "/\
            " + folderPower + "/baseline/"+ binary)

print(outdir)
if not os.path.exists(outdir):
    os.mkdir(outdir)
# Specify the output directory
m5.options.outdir = outdir
m5.core.setOutputDir(outdir)

# redirect stdout and stderr
stdout_file = "simout_" + "_" + args.binary
stdout_file = os.path.join(outdir, stdout_file)
redir_fd = os.open(stdout_file, os. O_WRONLY | os.O_CREAT | os.O_TRUNC)
os.dup2(redir_fd, sys.stdout.fileno())

stderr_file = "simerr_" + "_" + args.binary
stderr_file = os.path.join(outdir, stderr_file)
redir_fd = os.open(stderr_file, os. O_WRONLY | os.O_CREAT | os.O_TRUNC)
os.dup2(redir_fd, sys.stderr.fileno())

# Specify the debug flag and enable them
args.flags = flags
# m5.debug.flags[args.flags].enable()



multiprocesses = []
numThreads = 1

if args.bench:
    apps = args.bench.split("-")
    if len(apps) != args.num_cpus:
        print("number of benchmarks not equal to set num_cpus!")
        sys.exit(1)

    for app in apps:
        try:
            if buildEnv['TARGET_ISA'] == 'arm':
                exec("workload = %s('arm_%s', 'linux', '%s')" % (
                        app, args.arm_iset, args.spec_input))
            else:
                exec("workload = %s(buildEnv['TARGET_ISA', 'linux', '%s')" % (
                        app, args.spec_input))
            multiprocesses.append(workload.makeProcess())
        except:
            print("Unable to find workload for %s: %s" %
                  (buildEnv['TARGET_ISA'], app),
                  file=sys.stderr)
            sys.exit(1)
elif args.cmd:
    multiprocesses, numThreads = get_processes(args)
else:
    print("No workload specified. Exiting!\n", file=sys.stderr)
    sys.exit(1)


(CPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(args)
CPUClass.numThreads = numThreads

# Check -- do not allow SMT with multiple CPUs
if args.smt and args.num_cpus > 1:
    fatal("You cannot use SMT with multiple CPUs!")

np = args.num_cpus
system = System(cpu = [CPUClass(cpu_id=i) for i in range(np)],
                mem_mode = test_mem_mode,
                mem_ranges = [AddrRange(args.mem_size)],
                cache_line_size = args.cacheline_size,
                workload = NULL)

if numThreads > 1:
    system.multi_thread = True

# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage = args.sys_voltage)

# Create a source clock for the system and set the clock period
system.clk_domain = SrcClockDomain(clock =  args.sys_clock,
                                   voltage_domain = system.voltage_domain)

# Create a CPU voltage domain
system.cpu_voltage_domain = VoltageDomain()

# Create a separate clock domain for the CPUs
system.cpu_clk_domain = SrcClockDomain(clock = args.cpu_clock,
                                       voltage_domain =
                                       system.cpu_voltage_domain)

# If elastic tracing is enabled, then configure the cpu and attach the elastic
# trace probe
if args.elastic_trace_en:
    CpuConfig.config_etrace(CPUClass, system.cpu, args)

# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
for cpu in system.cpu:
    cpu.clk_domain = system.cpu_clk_domain

if ObjectList.is_kvm_cpu(CPUClass) or ObjectList.is_kvm_cpu(FutureClass):
    if buildEnv['TARGET_ISA'] == 'x86':
        system.kvm_vm = KvmVM()
        for process in multiprocesses:
            process.useArchPT = True
            process.kvmInSE = True
    else:
        fatal("KvmCPU can only be used in SE mode with x86")

# Sanity check
if args.simpoint_profile:
    if not ObjectList.is_noncaching_cpu(CPUClass):
        fatal("SimPoint/BPProbe should be done with an atomic cpu")
    if np > 1:
        fatal("SimPoint generation not supported with more than one CPUs")

for i in range(np):
    if args.smt:
        system.cpu[i].workload = multiprocesses
    elif len(multiprocesses) == 1:
        system.cpu[i].workload = multiprocesses[0]
    else:
        system.cpu[i].workload = multiprocesses[i]

    if args.simpoint_profile:
        system.cpu[i].addSimPointProbe(args.simpoint_interval)

    if args.checker:
        system.cpu[i].addCheckerCpu()

    if args.bp_type:
        bpClass = ObjectList.bp_list.get(args.bp_type)
        system.cpu[i].branchPred = bpClass()

    if args.indirect_bp_type:
        indirectBPClass = \
            ObjectList.indirect_bp_list.get(args.indirect_bp_type)
        system.cpu[i].branchPred.indirectBranchPred = indirectBPClass()

    system.cpu[i].createThreads()

if args.ruby:
    Ruby.create_system(args, False, system)
    assert(args.num_cpus == len(system.ruby._cpu_ports))

    system.ruby.clk_domain = SrcClockDomain(clock = args.ruby_clock,
                                        voltage_domain = system.voltage_domain)
    for i in range(np):
        ruby_port = system.ruby._cpu_ports[i]

        # Create the interrupt controller and connect its ports to Ruby
        # Note that the interrupt controller is always present but only
        # in x86 does it have message ports that need to be connected
        system.cpu[i].createInterruptController()

        # Connect the cpu's cache ports to Ruby
        system.cpu[i].icache_port = ruby_port.slave
        system.cpu[i].dcache_port = ruby_port.slave
        if buildEnv['TARGET_ISA'] == 'x86':
            system.cpu[i].interrupts[0].pio = ruby_port.master
            system.cpu[i].interrupts[0].int_master = ruby_port.slave
            system.cpu[i].interrupts[0].int_slave = ruby_port.master
            system.cpu[i].itb.walker.port = ruby_port.slave
            system.cpu[i].dtb.walker.port = ruby_port.slave
else:
    MemClass = Simulation.setMemClass(args)
    system.membus = SystemXBar()
    system.system_port = system.membus.slave
    CacheConfig.config_cache(args, system)
    MemConfig.config_mem(args, system)
    config_filesystem(system, args)

if args.wait_gdb:
    for cpu in system.cpu:
        cpu.wait_for_remote_gdb = True

root = Root(full_system = False, system = system)
Simulation.run(args, root, system, FutureClass)

print("Workload " + args.binary + " exits from \
    simulation after simulating " + str(args.instructions))
