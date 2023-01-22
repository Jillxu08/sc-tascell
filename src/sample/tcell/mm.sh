#!/bin/bash
#PJM -L "rscunit=ito-b"
#PJM -L "rscgrp=ito-g-4"
#PJM -L "vnode=1"
#PJM -L "vnode-core=36"
#PJM -L "elapse=10:00"

# ./mm_block2_para-gcc -n 32 -i "0"
./mm_block2_para -n 1 -i "0"