#!/bin/bash
#============ Slurm Options ===========
#SBATCH -p gr10561g        # ジョブキュー名（パーティション名）の指定。利用可能なキュー名は spartition コマンドで確認してください。
#SBATCH -t 4:00:00         # 経過時間の上限を1時間に指定。無駄のない上限を指定したほうがスケジューリングされやすくなります。
#SBATCH --rsc p=1:t=16:c=16:m=500G  # 要求リソースの指定。
#SBATCH -o %x.%j.out       # ジョブの標準出力/エラー出力ファイルの指定。%xはジョブ名(ジョブスクリプト名)、%j はジョブIDに置換されます。
#============ Shell Script ============
set -x

# srun ./a.out

# export NVCOMPILER_ACC_NOTIFY=2
# export NVCOMPILER_ACC_TIME=1
# NV_ACC_DEBUG=0x800 
# srun env LD_PRELOAD=libnvblas.so ./drop 
# srun env LD_PRELOAD=/opt/system/app/nvhpc/23.9/Linux_x86_64/23.9/math_libs/lib64/libnvblas.so ./mm_cu-gcc -n 6 -i "0"
# gcc drop.c -lnvblas -lm -o drop 
#module switch PrgEnvNvidia/2022 PrgEnvGCC

srun env LD_PRELOAD=-L/usr/lib64/libopenblas.so ./mm_cu-gcc -n 1 -i "0"