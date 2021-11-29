from m5.params import *
from m5.proxy import *
from m5.objects.MemCtrl import *

class MinirankMemCtrl(MemCtrl):
    type = 'MinirankMemCtrl'
    cxx_header = "mem/minirank_mem_ctrl.hh"
    cxx_class = 'gem5::memory::MinirankMemCtrl'

    # single-ported on the system interface side, instantiate with a
    # bus in front of the controller for multiple ports
    port = ResponsePort("This port responds to memory requests")

        # Interface to volatile, DRAM media
    minirank_dram = Param.MinirankDRAMInterface(NULL, "DRAM interface")