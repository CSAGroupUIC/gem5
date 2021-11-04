import os
import time

EXE = {}
OP = {}
IN = {}

EXE[500]="perlbench_r_base.mytest-m64"
OP[500]="-I lib checkspam.pl 2500 5 25 11 150 1 1 1 1"
IN[500]=""

EXE[502]="cpugcc_r_base.mytest-m64"
OP[502]="gcc-pp.c -O3 -finline-limit=0 \
    -fif-conversion -fif-conversion2 -o gcc-pp.s"
IN[502]=""

EXE[503]="bwaves_r_base.mytest-m64"
OP[503]="bwaves_1"
IN[503]="bwaves_1.in"

EXE[505]="mcf_r_base.mytest-m64"
OP[505]="inp.in"
IN[505]=""

# colorful
EXE[507]="cactusBSSN_r_base.mytest-m64"
OP[507]="spec_ref.par"
IN[507]=""

EXE[508]="namd_r_base.mytest-m64"
OP[508]="--input apoa1.input --output \
    apoa1.ref.output --iterations 65"
IN[508]=""

# NOT WORKIN[G tried to read unmapped addr
EXE[510]="parest_r_base.mytest-m64"
OP[510]="ref.prm"
IN[510]=""

EXE[511]="povray_r_base.mytest-m64"
OP[511]="SPEC-benchmark-ref.ini"
IN[511]=""

EXE[519]="lbm_r_base.mytest-m64"
OP[519]="3000 reference.dat 0 0 100_100_130_ldc.of"
IN[519]=""

# NOT WORKIN[G Error convert 0 to simtime
EXE[520]="omnetpp_r_base.mytest-m64"
OP[520]="-c General -r 0"
IN[520]=""

# NOT WORKIN[G
EXE[521]="wrf_r_base.mytest-m64"
OP[521]=""
IN[521]=""

EXE[523]="cpuxalan_r_base.mytest-m64"
OP[523]="-v t5.xml xalanc.xsl"
IN[523]=""

# NOT WORKIN[G CLZ miscompiled
EXE[525]="264_r_base.mytest-m64"
OP[525]="--pass 1 --stats x264_stats.log --bitrate\
     1000 --frames 1000 -o BuckBunny_New.264 BuckBunny.yuv 1280x720"
IN[525]=""

EXE[526]="blender_r_base.mytest-m64"
OP[526]="sh3_no_char.blend --render-output sh3_no_char_ \
    --threads 1 -b -F RAWTGA -s 849 -e 849 -a"
IN[526]=""

# NOT WORKIN[G Expected int but got char, gfortran error
EXE[527]="cam4_r_base.mytest-m64"
OP[527]=""
IN[527]=""

# colorful
EXE[531]="deepsjeng_r_base.mytest-m64"
OP[531]="ref.txt"
IN[531]=""

EXE[538]="imagick_r_base.mytest-m64"
OP[538]="-limit disk 0 refrate_input.tga -edge 41 \
    -resample 181% -emboss 31 -colorspace YUV \
        -mean-shift 19x19+15% -resize 30% refrate_output.tga"
IN[538]=""

EXE[541]="leela_r_base.mytest-m64"
OP[541]="ref.sgf"
IN[541]=""

EXE[544]="nab_r_base.mytest-m64"
OP[544]="1am0 1122214447 122"
IN[544]=""

# NOT WORKIN[G
EXE[548]="exchange2_r_base.mytest-m64"
OP[548]="6"
IN[548]=""

# colorful
EXE[549]="fotonik3d_r_base.mytest-m64"
OP[549]=""
IN[549]=""

# NOT WORKIN[G
EXE[554]="roms_r_base.mytest-m64"
OP[554]=""
IN[554]="ocean_benchmark2.in.x"

EXE[557]="xz_r_base.mytest-m64"
OP[557]="cld.tar.xz 160 \
    19cf30ae51eddcbefda78dd06014b4b96281456e07\
        8ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697\
            718cede6bd5454852652806a657bb56e07d6\
                1128434b474 59796407 61004416 6"
IN[557]=""

####################################################
### SPEC 2006
######## 400 ######### GOOD
EXE[400]="perlbench_base.amd64-m64-gcc42-nn"
OP[400]="-I lib checkspam.pl 2500 5 25 11 150 1 1 1 1"
IN[400]=""

########## bzip2 ########## GOOD
EXE[401]="bzip2_base.amd64-m64-gcc42-nn"
OP[401]="chicken.jpg 30"
IN[401]=""

 ########## gcc ########## GOOD
EXE[403]="gcc_base.amd64-m64-gcc42-nn"
OP[403]="166.i -o 166.s"
IN[403]=""

########## bWAVES ########## GOOD
EXE[410]="bwaves_base.amd64-m64-gcc42-nn"
OP[410]=""
IN[410]=""

########## gamess ########## GOOD
EXE[416]="gamess_base.amd64-m64-gcc42-nn"
OP[416]=""
IN[416]="cytosine.2.config"

########## mcf ########## GOOD
EXE[429]="mcf_base.amd64-m64-gcc42-nn"
OP[429]="inp.in"
IN[429]=""

########## milc ########## GOOD
EXE[433]="milc_base.amd64-m64-gcc42-nn"
OP[433]=""
IN[433]="su3imp.in"

########## zeusmp ########## GOOD
EXE[434]="zeusmp_base.amd64-m64-gcc42-nn"
OP[434]=""
IN[434]=""

########## gromacs ########## GOOD
EXE[435]="gromacs_base.amd64-m64-gcc42-nn"
OP[435]="-silent -deffnm gromacs -nice 0"
IN[435]=""

########## cactusADM ########## GOOD
EXE[436]="cactusADM_base.amd64-m64-gcc42-nn"
OP[436]="benchADM.par"
IN[436]=""

########## leslie3d ########## GOOD
EXE[437]="leslie3d_base.amd64-m64-gcc42-nn"
OP[437]=""
IN[437]="leslie3d.in"

########## namd ########## GOOD
EXE[444]="namd_base.amd64-m64-gcc42-nn"
OP[444]="--input namd.input --output \
    namd.out --iterations 38"
IN[444]=""

########## gobmk ########## GOOD
EXE[445]="gobmk_base.amd64-m64-gcc42-nn"
OP[445]="--quiet --mode gtp"
IN[445]="13x13.tst"

########## soplex ########## GOOD
EXE[450]="soplex_base.amd64-m64-gcc42-nn"
OP[450]="-s1 -e -m4500 pds-50.mps"
IN[450]=""

########## 453 ########## GOOD
EXE[453]="povray_base.amd64-m64-gcc42-nn"
OP[453]="SPEC-benchmark-ref.ini"
IN[453]=""

########## 454 ########## GOOD
EXE[454]="calculix_base.amd64-m64-gcc42-nn"
OP[454]="-i hyperviscoplastic"
IN[454]=""

########## 456 ########## GOOD
EXE[456]="hmmer_base.amd64-m64-gcc42-nn"
OP[456]="nph3.hmm swiss41"
IN[456]=""

########## 458 ########## GOOD
EXE[458]="sjeng_base.amd64-m64-gcc42-nn"
OP[458]="ref.txt"
IN[458]=""

########## 459 ########## GOOD
EXE[459]="GemsFDTD_base.amd64-m64-gcc42-nn"
OP[459]="yee.dat"
IN[459]=""

########## 462 ########## GOOD
EXE[462]="libquantum_base.amd64-m64-gcc42-nn"
OP[462]="1397 8"
IN[462]=""

########## 464 ########## GOOD
EXE[464]="h264ref_base.amd64-m64-gcc42-nn"
OP[464]="-d foreman_ref_encoder_baseline.cfg"
IN[464]=""

########## 465 ########## GOOD
EXE[465]="tonto_base.amd64-m64-gcc42-nn"
OP[465]=""
IN[465]="stdin"

########## 470 ########## GOOD
EXE[470]="lbm_base.amd64-m64-gcc42-nn"
OP[470]="3000 reference.dat 0 0 100_100_130_ldc.of"
IN[470]=""

########## 471 ########## GOOD
EXE[471]="omnetpp_base.amd64-m64-gcc42-nn"
OP[471]="omnetpp.ini"
IN[471]=""

########## 473 ########## GOOD
EXE[473]="astar_base.amd64-m64-gcc42-nn"
OP[473]="BigLakes2048.cfg"
IN[473]=""

# ########## 481 ########## ERROR
# EXE[481]="481/wrf_base.amd64-m64-gcc42-nn"
# OP[481]=""
# IN[481]=""

########## 482 ########## GOOD
EXE[482]="sphinx_livepretend_base.amd64-m64-gcc42-nn"
OP[482]="ctlfile . args.an4"
IN[482]=""

class CPU(object):
    def __init__(self,number):
        self.executable = EXE[number]
        self.options = OP[number]
        self.Input = IN[number]
        self.output = ""
        self.errout = ""
