#!/bin/bash
#============ Slurm Options ===========
#SBATCH -p gr10561g        # ジョブキュー名（パーティション名）の指定。利用可能なキュー名は spartition コマンドで確認してください。
#SBATCH -t 24:00:00         # 経過時間の上限を1時間に指定。無駄のない上限を指定したほうがスケジューリングされやすくなります。
#SBATCH --rsc p=1:t=64:c=64  # 要求リソースの指定。
#SBATCH -o %x.%j.out       # ジョブの標準出力/エラー出力ファイルの指定。%xはジョブ名(ジョブスクリプト名)、%j はジョブIDに置換されます。
#============ Shell Script ============
set -x
# srun ./hmat_ct_reds-clos -n 64 -i "1 0 800000000 200"
# gr20100g gr10561g
# qgroup qs spartition
# 512G/node, 80G/GPU
# squeue -j 236729 -o "%.10i %.10P %.10j %.10u %.10t %.12M %.12L" 

# 运行时启用MPI调试
export MPICH_ASYNC_PROGRESS=1 
export MPICH_MAX_THREAD_SAFETY=multiple
# 或添加详细日志
export MPI_DEBUG=1
export GPU_REPORT_FILE="report_job${SLURM_JOB_ID}.txt"
export DS_BUF_INIT_CAP=700000
export GPU_PROFILING=1

# CPU-only
# srun ./hmat_ct_reds-clos -n 64 -i "1 800000000 800000000"
srun ./hmat_ct_reds_p-clos -n 64 -i "1 800000000 800000000"
# srun ./hmat_ct_reds_c-clos -n 64 -i "1 800000000 800000000"
# srun ./hmat_ct_reds_s-clos -n 64 -i "1 800000000 800000000"

# shperepyramid 70%	9700
# srun ./hmat_ct_reds-clos -n 68 -i "1 13035 800000000"

# srun ./hmat_ct_reds-clos -n 64 -i "1 9700 800000000"

# largesphere 80%	spherecube 40%	shperepyramid 70%	humans  90%
# 8100	5159	9700	13035

# srun ./hmat_ct_reds-clos -n 4 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 8 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 8 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 8 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 8 -i "1 0 0"

# srun ./hmat_ct_reds-clos -n 12 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 12 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 12 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 12 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 12 -i "1 0 0"

# srun ./hmat_ct_reds-clos -n 16 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 16 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 16 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 16 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 16 -i "1 0 0"

# srun ./hmat_ct_reds-clos -n 68 -i "1 8100 800000000"
# srun ./hmat_ct_reds-clos -n 68 -i "1 9700 800000000"
# srun ./hmat_ct_reds-clos -n 68 -i "1 13035 800000000"
# srun ./hmat_ct_reds-clos -n 68 -i "1 13035 800000000"
# srun ./hmat_ct_reds-clos -n 68 -i "1 13035 800000000"

# srun --ntasks=2 --gpus-per-task=1 --ntasks-per-node=1 ./hmat_ct_reds-clos -n 32 -i '1 5000 20000'
# srun ./hmat_ct_reds_s-clos -n 68 -i "1 8100 800000000"  #sphere
# srun ./hmat_ct_reds-clos -n 68 -i "1 5159 800000000"  #cube
# srun ./hmat_ct_reds-clos_p -n 68 -i "1 9700 800000000"  #pyramid
# srun ./hmat_ct_reds-clos_H -n 68 -i "1 5159 800000000"  #humans
# srun ./hmat_ct_reds-clos -n 68 -i "1 5159 800000000" 
# srun ./hmat_ct_reds-clos -n 68 -i "1 5159 800000000" 
# srun ./hmat_ct_reds-clos -n 68 -i "1 5159 800000000" 
# srun ./hmat_ct_reds-clos -n 68 -i "1 5159 800000000" 

# srun ./hmat_ct_reds-clos -n 68 -i "1 8100 800000000"  #sphere
# srun ./hmat_ct_reds-clos -n 68 -i "1 8100 800000000"  #sphere
# srun ./hmat_ct_reds-clos -n 68 -i "1 8100 800000000"  #sphere
# srun ./hmat_ct_reds-clos -n 68 -i "1 8100 800000000"  #sphere
# srun ./hmat_ct_reds-clos -n 68 -i "1 8100 800000000"  #sphere

# srun ./hmat_ct_reds-clos -n 68 -i "1 9700 800000000"  #pyramid
# srun ./hmat_ct_reds-clos -n 68 -i "1 9700 800000000" 
# srun ./hmat_ct_reds-clos -n 68 -i "1 9700 800000000" 
# srun ./hmat_ct_reds-clos -n 68 -i "1 9700 800000000" 
# srun ./hmat_ct_reds-clos -n 68 -i "1 9700 800000000" 


# srun ./hmat_ct_reds-clos -n 64 -i "1 800000000 800000000"
# srun ./hmat_ct_reds-clos -n 64 -i "1 800000000 800000000"
# srun --ntasks=1 --gpus-per-task=1 ./hmat_ct_reds-clos -n 1 -i '1 0 0'
# srun --ntasks=1 --gpus-per-task=1 ./hmat_ct_reds-clos -n 1 -i '1 0 0'
# srun --ntasks=1 --gpus-per-task=1 ./hmat_ct_reds-clos -n 1 -i '1 0 0'
# srun --ntasks=1 --gpus-per-task=1 ./hmat_ct_reds-clos -n 1 -i '1 0 0'
# ★ Profiling 开关（仅方案B -DGPU_PROFILING_RUNTIME 编译时有效）
# 需要精确时间分解时设为 1，正式计算时注释掉或设为 0
# export GPU_PROFILING=1

# srun ./hmat_ct_reds-clos -n 1 -i "1 0 0"
# srun --ntasks=1 --gpus-per-task=4 ./hmat_ct_reds-clos -n 64 -i "1 0 0"

# srun --ntasks=1 --gpus-per-task=1 ./hmat_ct_reds-clos -n 64 -i '1 0 0'
# nsys profile --trace=openacc,cuda srun --ntasks=2 --gpus-per-task=1 ./hmat_ct_reds-clos -n 32 -i '1 5000 20000'


# srun ./hmat_ct_reds-clos -n 1 -i '1'
# srun ./hmat_ct_reds-clos -n 64 -i '1 8000 80000000 500 1'
# srun --ntasks=1 --gpus-per-task=4 ./hmat_ct_reds-clos -n 64 -i '1 8000 80000000 500 3'
# 在 srun 之前添加
# srun --ntasks=4 --gpus-per-task=1 --ntasks-per-node=4 bash -c '
#     echo "Rank $SLURM_PROCID on $(hostname):"
#     echo "  SLURM_LOCALID=$SLURM_LOCALID"
#     echo "  SLURM_STEP_GPUS=$SLURM_STEP_GPUS"
#     echo "  SLURM_JOB_GPUS=$SLURM_JOB_GPUS"
#     echo "  CUDA_VISIBLE_DEVICES=$CUDA_VISIBLE_DEVICES"
#     nvidia-smi -L 2>/dev/null || echo "  (nvidia-smi not available)"
# '


# srun ./hmat_ct_reds-clos -n 65 -i "1 0 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 0"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 0"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 6650"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 6650"


# srun ./hmat_ct_reds-clos -n 65 -i "1 0 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 8400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 8400"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 10600"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 10600"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 13200"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 13200"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 17000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 17000"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 23500"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 23500"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 39550"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 39550"


# srun ./hmat_ct_reds-clos -n 65 -i "1 0 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 75750"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 75750"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 143400"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 143400"

# srun ./hmat_ct_reds-clos -n 65 -i "1 0 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 2640 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 3477 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4092 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 4644 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 5476 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 6461 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 7800 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 9559 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 13035 200000000"
# srun ./hmat_ct_reds-clos -n 65 -i "1 20000000 200000000"





# srun --ntasks=1 --gpus-per-task=4 --ntasks-per-node=1 ./hmat_ct_reds-clos -n 64 -i '1 8000 80000000'
# srun --ntasks=2 --gpus-per-task=1 --ntasks-per-node=1 ./hmat_ct_reds-clos -n 32 -i '1 5000 20000'
# srun --ntasks=4 --gpus-per-task=1 --ntasks-per-node=4 ./hmat_ct_reds-clos -n 64 -i '1 8000 80000000'
# srun --ntasks=3 --gpus-per-task=1 --ntasks-per-node=1 \
#   ./hmat_ct_reds-clos -n 32 -i '1 8000 80000000 500 3'
# srun --ntasks=1 env HMAT_NUM_GPUS=0 ./hmat_ct_reds-clos -n 64 -i '1 8000 80000000 500'
# srun ./hmat_ct_reds-clos -n 64 -i '1 9000 800000000 100' 2>&1 | tee debug.log

# srun compute-sanitizer --tool memcheck ./hmat_ct_reds-clos -n 64 -i "1 10 80000000"

# int th_ds, th_lr, minsz; 硬性上限：$2,147,483,647$ (INT_MAX)。

# export UCX_TLS=rc,cuda_ipc,gdr_copy,sm  # 启用高效传输协议
# export PAMI_ENABLE_CUDA=1               # 如果是 IBM 系统
# export NV_ACC_NOTIFY=1
# export NV_ACC_TIME=1

# srun ./hmat_ct_reds-clos -n 1 -i "1 0 0 100"
# srun ./hmat_ct_reds-clos -n 16 -i "1 900000000 800000000 100"
# srun ./hmat_ct_reds-clos -n 16 -i "1 9000 800000000 100"




