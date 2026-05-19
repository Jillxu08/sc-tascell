#!/bin/bash
# extract_stats.sh - 从输出中提取关键数据

OUTPUT_FILE=$1
RESULT_CSV=$2

# 提取数据
TOTAL_TIME=$(grep "Total time" $OUTPUT_FILE | awk '{print $3}')
GPU_ATTEMPTS=$(grep "Attempts:" $OUTPUT_FILE | awk '{print $2}')
GPU_FAILURES=$(grep "Failures:" $OUTPUT_FILE | awk '{print $2}')
SUCCESS_RATE=$(grep "Success Rate:" $OUTPUT_FILE | awk '{print $3}' | tr -d '%')
DS_GPU_CNT=$(grep "Dense.*GPU:" $OUTPUT_FILE | head -1 | awk '{print $2}' | cut -d'=' -f2)
DS_CPU_CNT=$(grep "Dense.*CPU:" $OUTPUT_FILE | head -1 | awk '{print $2}' | cut -d'=' -f2)
LR_GPU_CNT=$(grep "Low-Rank.*GPU:" $OUTPUT_FILE | head -1 | awk '{print $2}' | cut -d'=' -f2)
LR_CPU_CNT=$(grep "Low-Rank.*CPU:" $OUTPUT_FILE | head -1 | awk '{print $2}' | cut -d'=' -f2)
DS_GPU_TIME=$(grep "GPU: dense=" $OUTPUT_FILE | awk -F'dense=' '{print $2}' | awk '{print $1}')
DS_CPU_TIME=$(grep "CPU: dense=" $OUTPUT_FILE | awk -F'dense=' '{print $2}' | awk '{print $1}')
LR_GPU_TIME=$(grep "GPU: dense=" $OUTPUT_FILE | awk -F'lowrank=' '{print $2}' | awk '{print $1}')
LR_CPU_TIME=$(grep "CPU: dense=" $OUTPUT_FILE | awk -F'lowrank=' '{print $2}' | awk '{print $1}')

# 写入CSV
echo "$TOTAL_TIME,$GPU_ATTEMPTS,$GPU_FAILURES,$SUCCESS_RATE,$DS_GPU_CNT,$DS_CPU_CNT,$LR_GPU_CNT,$LR_CPU_CNT,$DS_GPU_TIME,$DS_CPU_TIME,$LR_GPU_TIME,$LR_CPU_TIME" >> $RESULT_CSV