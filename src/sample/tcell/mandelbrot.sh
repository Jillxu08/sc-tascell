#!/bin/bash
#============ Slurm Options ===========
#SBATCH -p gr10561g        # ジョブキュー名（パーティション名）の指定。利用可能なキュー名は spartition コマンドで確認してください。
#SBATCH -t 4:00:00         # 経過時間の上限を1時間に指定。無駄のない上限を指定したほうがスケジューリングされやすくなります。
#SBATCH --rsc p=1:t=64:c=64  # 要求リソースの指定。
#SBATCH -o %x.%j.out       # ジョブの標準出力/エラー出力ファイルの指定。%xはジョブ名(ジョブスクリプト名)、%j はジョブIDに置換されます。
#============ Shell Script ============
set -x

# srun nsys profile --trace=cuda,osrt -o mandelbrot_profile ./mandelbrot-clos -n 65 -i "0 4 50000 2000 2 5"

# # NVCOMPILER_ACC_NOTIFY=2 
# # NVCOMPILER_ACC_TIME=1 
# # NV_ACC_DEBUG=0x128 #error info

# tssrun -gpu -gpuhost gp-0001 -t 20:00 bash

# #  65536, 32768, 16384, 4096, 4096, 2048
# # # TH MAX_ITERATIONS, th_gpu1, th_gpu2,th_cpu

# #!/bin/bash
# # # bash ./mm_v.sh 

# srun ./mandelbrot-clos -n 1 -i "0 4 50000 8000 0 0"
srun ./mandelbrot-clos -n 2 -i "0 4 50000 2000 3 6"
# srun ./mandelbrot-clos -n 3 -i "0 4 50000 2000 6 7"
# srun ./mandelbrot-clos -n 5 -i "0 4 50000 2000 3 6"
# srun ./mandelbrot-clos -n 9 -i "0 4 50000 2000 8 8"
# srun ./mandelbrot-clos -n 17 -i "0 4 50000 2000 0 4"
# srun ./mandelbrot-clos -n 33 -i "0 4 50000 2000 3 6"
# srun ./mandelbrot-clos -n 65 -i "0 4 50000 2000 2 5"

# srun ./mandelbrot-clos -n 1 -i "0 4 50000 8000 0 0"
# srun ./mandelbrot-clos -n 1 -i "0 4 50000 8000 0 0"
# srun ./mandelbrot-clos -n 1 -i "0 4 50000 8000 0 0"
# srun ./mandelbrot-clos -n 1 -i "0 4 50000 8000 0 0"

# srun ./mandelbrot-clos -n 2 -i "0 4 50000 2000 3 6"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 2000 0 4"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 2000 0 4"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 2000 0 4"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 2000 0 4"

# srun ./mandelbrot-clos -n 3 -i "0 4 50000 2000 6 7"
# srun ./mandelbrot-clos -n 3 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 3 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 3 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 3 -i "0 4 50000 2000 2 8"

# srun ./mandelbrot-clos -n 5 -i "0 4 50000 2000 3 6"
# srun ./mandelbrot-clos -n 5 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 5 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 5 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 5 -i "0 4 50000 2000 2 8"

# srun ./mandelbrot-clos -n 9 -i "0 4 50000 2000 8 8"
# srun ./mandelbrot-clos -n 9 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 9 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 9 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 9 -i "0 4 50000 2000 2 8"

# srun ./mandelbrot-clos -n 17 -i "0 4 50000 2000 0 4"
# srun ./mandelbrot-clos -n 17 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 17 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 17 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 17 -i "0 4 50000 2000 2 8"

# srun ./mandelbrot-clos -n 33 -i "0 4 50000 2000 3 6"
# srun ./mandelbrot-clos -n 33 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 33 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 33 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 33 -i "0 4 50000 2000 2 8"

# srun ./mandelbrot-clos -n 65 -i "0 4 50000 2000 2 5"
# srun ./mandelbrot-clos -n 65 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 65 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 65 -i "0 4 50000 2000 2 8"
# srun ./mandelbrot-clos -n 65 -i "0 4 50000 2000 2 8"

# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 1"

# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 50"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 50"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 50"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 50"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 50"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 50"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 50"

# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 100"

# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 500"


# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 1000"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 1000"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 1000"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 1000"
# # srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 1000"
# # srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 1000"
# # srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 1000"


# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 4000"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 4000"
# # srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 4000"
# # srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 4000"
# # srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 4000"
# # srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 4000"
# # srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 4000"

# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 8000"

# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 1 -i "0 4 50000 -1 -2 1"

# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 2 -i "0 4 50000 -1 -2 1"

# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 4 -i "0 4 50000 -1 -2 1"

# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 8 -i "0 4 50000 -1 -2 1"

# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 16 -i "0 4 50000 -1 -2 1"

# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 32 -i "0 4 50000 -1 -2 1"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 1"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 1"


# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 100"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 100"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 200"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 200"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 200"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 200"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 200"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 300"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 300"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 300"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 300"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 300"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 400"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 400"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 400"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 400"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 400"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 500"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 500"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 600"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 600"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 600"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 600"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 600"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 700"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 700"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 700"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 700"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 700"

# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 800"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 800"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 800"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 800"
# srun ./mandelbrot-clos -n 64 -i "0 4 50000 -1 -2 800"

