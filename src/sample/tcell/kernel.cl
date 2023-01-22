__kernel void mixtran(__global const float *input,
	__global  float *inputT) {
	int gid = get_global_id(0);
	for (int i = 0; i < 65; i++) {
		inputT[i*8192+gid] = input[gid*65+i];
	}
}

__kernel void mixmul(__global const float *input1,
	__global const float *input2,
	__global float *result) {
	int gid = get_global_id(0);
	for (int j = 0; j < 65; j ++ ) {
		for (int i = 0; i < 8192; i++) {
			result[gid*65+j] += input1[i + gid * 8192] * input2[i + j * 8192];
		}
	}
}