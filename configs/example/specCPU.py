# SPEC CPU 2006/2017
# gem5 integration via python
#
# Authors: Yicheng Wang
# update: 04/27/2020

import os

class CPU2006(object):
    """
    CPU2006() -> class
    Set SPEC CPU 2006 benchmark configurations for gem5 simulation
    Input capitaliz I to avoid name conflicts
    """
    def __init__(self,workload):
        """
        Initialize the workload instance with its name
        """
        sections = {}

        if workload in ['400','perlbench']:
            sections["binary_name"] = "perlbench"
            sections["dir_name"] = "400.perlbench"
            sections["options"] = "-I./lib diffmail.pl 4 800 10 17 19 300"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 8646

        elif workload in ['401', 'bzip2']:
            sections["binary_name"] = "bzip2"
            sections["dir_name"] = "401.bzip2"
            sections["options"] = "chicken.jpg 30"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 3078

        elif workload in ['403', 'gcc']:
            sections["binary_name"] = "gcc"
            sections["dir_name"] = "403.gcc"
            sections["options"] = "166.i -o 166.s"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 5681

        elif workload in ['410', 'bwaves']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "bwaves"
            sections["dir_name"] = "410.bwaves"
            sections["options"] = ""
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 195834

        elif workload in ['416','gamess']:
            sections["binary_name"] = "gamess"
            sections["dir_name"] = "416.gamess"
            sections["options"] = ""
            sections["Input"] = "cytosine.2.config"
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 52844

        elif workload in ['429', 'mcf']:
            sections["binary_name"] = "mcf"
            sections["dir_name"] = "429.mcf"
            sections["options"] = "inp.in"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 24182

        elif workload in ['433', 'milc']:
            sections["binary_name"] = "milc"
            sections["dir_name"] = "433.milc"
            sections["options"] = ""
            sections["Input"] = "su3imp.in"
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 42171

        elif workload in ['434', 'zeusmp']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "zeusmp"
            sections["dir_name"] = "434.zeusmp"
            sections["options"] = ""
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 143391

        elif workload in ['435', 'gromacs']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "zeusmp"
            sections["dir_name"] = "434.zeusmp"
            sections["options"] = ""
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 73566

        elif workload in ['436', 'cactusADM']:
            sections["binary_name"] = "cactusADM"
            sections["dir_name"] = "436.cactusADM'"
            sections["options"] = "benchADM.par"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 41811

        elif workload in ['437', 'leslie3d']:
            sections["binary_name"] = "leslie3d"
            sections["dir_name"] = "437.leslie3d"
            sections["options"] = ""
            sections["Input"] = "leslie3d.in"
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 102866

        elif workload in ['444', 'namd']:
            sections["binary_name"] = "namd"
            sections["dir_name"] = "444.namd"
            sections["options"] = "--input namd.input \
                --iterations 38"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 14239

        elif workload in ['445', 'gobmk']:
            sections["binary_name"] = "gobmk"
            sections["dir_name"] = "445.gobmk"
            sections["options"] = "--quiet --mode gtp"
            sections["Input"] = "13x13.tst"
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 2922

        elif workload in ['450','soplex']:
            sections["binary_name"] = "soplex"
            sections["dir_name"] = "450.soplex"
            sections["options"] = "-m3500 ref.mps"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 3988

        elif workload in ['453', 'povray']:
            sections["binary_name"] = "povray"
            sections["dir_name"] = "453.povray"
            sections["options"] = "SPEC-benchmark-ref.ini"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 61412

        elif workload in ['454', 'calculix']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "calculix"
            sections["dir_name"] = "454.calculix"
            sections["options"] = "-i hyperviscoplastic"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 210709

        elif workload in ['456', 'hmmer']:
            sections["binary_name"] = "hmmer"
            sections["dir_name"] = "456.hmmer"
            sections["options"] = "nph3.hmm swiss41"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 6286

        elif workload in ['458', 'sjeng']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "sjeng"
            sections["dir_name"] = "458.sjeng"
            sections["options"] = "ref.txt"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 134141

        elif workload in ['459', 'GemsFDTD']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "GemsFDTD"
            sections["dir_name"] = "459.GemsFDTD"
            sections["options"] = ""
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 131941

        elif workload in ['462', 'libquantum']:
            sections["binary_name"] = "libquantum"
            sections["dir_name"] = "462.libquantum"
            sections["options"] = "1297 8"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 41938

        elif workload in ['464', 'h264ref']:
            sections["binary_name"] = "h264ref"
            sections["dir_name"] = "464.h264ref"
            sections["options"] = "-d \
                foreman_ref_encoder_baseline.cfg"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 4289

        elif workload in ['465', 'tonto']:
            sections["binary_name"] = "tonto"
            sections["dir_name"] = "465.tonto"
            sections["options"] = ""
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 23

        elif workload in ['470', 'lbm']:
            sections["binary_name"] = "lbm"
            sections["dir_name"] = "470.lbm"
            sections["options"] = "3000 reference.dat 0 0 100_100_130_ldc.of"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 1820

        elif workload in ['471', 'omnetpp']:
            sections["binary_name"] = "omnetpp"
            sections["dir_name"] = "471.omnetpp"
            sections["options"] = "omnetpp.ini"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 34112

        elif workload in ['473', 'astar']:
            sections["binary_name"] = "astar"
            sections["dir_name"] = "473.astar"
            sections["options"] = "rivers.cfg"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 18680

        elif workload in ['481','wrf']:
            raise ValueError("Can't run in simulation.")
            # sections["binary_name"] = "wrf"
            # sections["dir_name"] = "481.wrf"
            # sections["options"] = ""
            # sections["Input"] = ""
            # sections["output"] = ""
            # sections["errout"] = ""
            # sections["simpt"] = 228382

        elif workload in ['482', 'sphinx3']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "sphinx_livepretend"
            sections["dir_name"] = "482.sphinx3"
            sections["options"] = "ctlfile . args.an4"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 96412

        elif workload in ['483','xalancbmk']:
            raise ValueError("Checkpoints not ready..")
            # sections["binary_name"] = "xalancbmk"
            # sections["dir_name"] = "483.xalancbmk"
            # sections["options"] = "-v test.xml xalanc.xsl"
            # sections["Input"] = ""
            # sections["output"] = ""
            # sections["errout"] = ""
            # sections["simpt"] = 1820

        else:
            raise ValueError("Wrong workload name.")

        # specify the path
        head = os.path.expanduser("~/Research/CPU2006/")
        tail = '/run/run_base_ref_amd64-m64-gcc41-nn.0000/'
        filePath = head + sections["dir_name"] + tail
        self.filePath = filePath
        self.executable = sections["binary_name"] + '_base\
            .amd64-m64-gcc41-nn'
        self.options = sections["options"] if sections["options"] else ""
        self.Input = sections["Input"] if sections["Input"] else ""
        self.output = sections["output"] if sections["output"] else ""
        self.errout = sections["errout"] if sections["errout"] else ""
        self.simpt = sections["simpt"]


class CPU2017(object):
    """
    CPU2006() -> class
    Set SPEC CPU 2006 benchmark configurations for gem5 simulation
    Input capitaliz I to avoid name conflicts
    """
    def __init__(self,workload):
        """
        Initialize the workload instance with its name
        """
        sections = {}

        if workload in ['500', 'perlbench_r']:
            sections["binary_name"] = "perlbench_r"
            sections["dir_name"] = "500.perlbench_r"
            sections["options"] = "-I./lib splitmail.pl \
                6400 12 26 16 100 0"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 25489

        elif workload in ['502', 'gcc_r']:
            sections["binary_name"] = "cpugcc_r"
            sections["dir_name"] = "502.gcc_r"
            sections["options"] = "ref32.c -O3 -fselective-scheduling \
                -fselective-scheduling2"
            sections["Input"] = ""
            sections["output"] = "ref32.opts-O3_-fselective-scheduling_\
                -fselective-scheduling2.s"
            sections["errout"] = "ref32.opts-O3_-fselective-scheduling_\
                -fselective-scheduling2.err"
            sections["simpt"] = 2077

        elif workload in ['503', 'bwaves_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "bwaves_r"
            sections["dir_name"] = "503.bwaves_r"
            sections["options"] = "bwaves_1"
            sections["Input"] = "bwaves_4.in"
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 22745

        elif workload in ['505', 'mcf_r']:
            sections["binary_name"] = "mcf_r"
            sections["dir_name"] = "505.mcf_r"
            sections["options"] = "inp.in"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 41104

        elif workload in ['507', 'cactuBSSN_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "cactuBSSN_r"
            sections["dir_name"] = "507.cactuBSSN_r"
            sections["options"] = "spec_ref.par"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 1235


        elif workload in ['508', 'namd_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "namd_r"
            sections["dir_name"] = "508.namd_r"
            sections["options"] = "--input apoa1.input --output \
                apoa1.ref.output --iterations 65"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 31873 #252468

        elif workload in ['510', 'parest_r']:
            # raise ValueError("Checkpoints not ready.")
            # need re-run simpoint
            sections["binary_name"] = "parest_r"
            sections["dir_name"] = "510.parest_r"
            sections["options"] = "ref.prm"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = None

        elif workload in ['511', 'povray_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "povray_r"
            sections["dir_name"] = "511.povray_r"
            sections["options"] = "SPEC-benchmark-ref.ini"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 9688

        elif workload in ['519', 'lbm_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "lbm_r"
            sections["dir_name"] = "519.lbm_r"
            sections["options"] = "3000 reference.dat 0 0 \
                100_100_130_ldc.of"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 22184

        elif workload in ['520', 'omnetpp_r']:
            sections["binary_name"] = "omnetpp_r"
            sections["dir_name"] = "520.omnetpp_r"
            sections["options"] = "-c General -r 0"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 79014

        elif workload in ['521', 'wrf_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "wrf_r"
            sections["dir_name"] = "521.wrf_r"
            sections["options"] = ""
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 4488

        elif workload in ['523', 'xalancbmk_r']:
            sections["binary_name"] = "xalancbmk_r"
            sections["dir_name"] = "523.xalancbmk_r"
            sections["options"] = "-v t5.xml xalanc.xsl"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 12499

        elif workload in ['525', 'x264_r']:
            sections["binary_name"] = "x264_r"
            sections["dir_name"] = "525.x264_r"
            sections["options"] = "--seek 500 --dumpyuv 200 \
                --frames 1250 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 31334 #40984

        elif workload in ['526', 'blender_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "blender_r"
            sections["dir_name"] = "526.blender_r"
            sections["options"] = "sh3_no_char.blend \
                --render-output sh3_no_char_ --threads 1 \
                    -b -F RAWTGA -s 849 -e 849 -a"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 41602

        elif workload in ['527', 'cam4_r']:
            raise ValueError("Checkpoints not ready.")
            # need re-run simpoint
            # sections["binary_name"] = "cam4_r"
            # sections["dir_name"] = "527.cam4_r"
            # sections["options"] = ""
            # sections["Input"] = ""
            # sections["output"] = ""
            # sections["errout"] = ""
            # sections["simpt"] = 38

        elif workload in ['531', 'deepsjeng_r']:
            sections["binary_name"] = "deepsjeng_r"
            sections["dir_name"] = "531.deepsjeng_r"
            sections["options"] = "ref.txt"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 11356 #85840

        elif workload in ['538', 'imagick_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "imagick_r"
            sections["dir_name"] = "538.imagick_r"
            sections["options"] = "-limit disk 0 refrate_input.tga \
                -edge 41 -resample 181% -emboss 31 -colorspace YUV \
                    -mean-shift 19x19+15% -resize 30% refrate_output.tga"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 3185 #499

        elif workload in ['541', 'leela_r']:
            sections["binary_name"] = "leela_r"
            sections["dir_name"] = "541.leela_r"
            sections["options"] = "ref.sgf"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 9221 # 140716

        elif workload in ['544', 'nab_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "nab_r"
            sections["dir_name"] = "544.nab_r"
            sections["options"] = "1am0 1122214447 122"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 56451

        elif workload in ['548', 'exchange2_r']:
            sections["binary_name"] = "exchange2_r"
            sections["dir_name"] = "548.exchange2_r"
            sections["options"] = "6"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 26999 #13390

        elif workload in ['549', 'fotonik3d_r']:
            # raise ValueError("Checkpoints not ready.")
            sections["binary_name"] = "fotonik3d_r"
            sections["dir_name"] = "549.fotonik3d_r"
            sections["options"] = ""
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 37327

        elif workload in ['554', 'roms_r']:
            raise ValueError("Checkpoints not ready.")
            # need re-run simpoint
            # sections["binary_name"] = "roms_r"
            # sections["dir_name"] = "554.roms_r"
            # sections["options"] = ""
            # sections["Input"] = "ocean_benchmark2.in.x"
            # sections["output"] = ""
            # sections["errout"] = ""
            # sections["simpt"] = None

        elif workload in ['557', 'xz_r']:
            sections["binary_name"] = "xz_r"
            sections["dir_name"] = "557.xz_r"
            sections["options"] = "input.combined.xz 250 \
                a841f68f38572a49d86226b7ff5baeb31bd19dc637\
                    a922a972b2e6d1257a890f6a544ecab967c313\
                        e370478c74f760eb229d4eef8a8d2836d23\
                            3d3e9dd1430bf 40401484 41217675 7"
            sections["Input"] = ""
            sections["output"] = ""
            sections["errout"] = ""
            sections["simpt"] = 19372 #52856

        else:
            raise ValueError("Wrong workload name.")


        # specify the path
        head = os.path.expanduser("~/Research/CPU2017/")
        if int(workload) < 600:
            tail = '/run/run_base_refrate_O2Static-m64.0000/'
        else:
            tail = '/run/run_base_refspeed_O2Static-m64.0000/'
        filePath = head + sections["dir_name"] + tail
        self.filePath = filePath
        self.executable = sections["binary_name"] + '_base.O2Static-m64'
        self.options = sections["options"] if sections["options"] else ""
        self.Input = sections["Input"] if sections["Input"] else ""
        self.output = sections["output"] if sections["output"] else ""
        self.errout = sections["errout"] if sections["errout"] else ""
        self.simpt = sections["simpt"]
