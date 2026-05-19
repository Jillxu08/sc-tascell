# Multi-node CPU-GPU H-Matrix Generation with Tascell

## Overview

This project investigates hybrid CPU–GPU execution and distributed-memory parallelism for H-matrix computations based on the SC-Tascell framework. The implementation integrates MPI, task parallelism, and GPU acceleration to improve scalability and resource utilization on modern heterogeneous systems.

The project includes:

* MPI-based distributed execution
* Hybrid CPU–GPU task scheduling
* Experimental H-matrix implementations
* Evaluation and benchmark scripts
* Benchmark datasets and utilities
* Tascell-based task-parallel implementations

---

## Directory Structure

```text id="d1p9f7"
.
├── bem_data/                 # BEM / H-matrix source files and datasets
├── hmat_*.sh                 # Experiment and execution scripts
├── hmat_ct_*.tcell           # Tascell implementations
├── Makefile                  # Build configuration
└── README.md
```

---

## Features

### Hybrid CPU–GPU Execution

The framework supports:

* CPU-only execution
* GPU-only execution
* Hybrid CPU–GPU execution

Tasks can be dynamically scheduled to CPUs or GPUs depending on resource availability.

---

### Task Parallelism

The implementation is based on the SC-Tascell task-parallel programming model.

Features include:

* Dynamic load balancing
* Work stealing
* Recursive task decomposition
* Parallel H-matrix computation

---

### GPU Acceleration

GPU acceleration is implemented using OpenACC with the NVIDIA HPC Compiler (`nvc`) on CUDA-enabled NVIDIA GPUs.

---

## Requirements

### SC-Tascell Environment

This project depends on the SC-Tascell environment and compiler toolchain.

Please install: Tascell runtime environment before building this project.

The implementation was tested with:

```text id="j7l8l6"
Tascell (May 15, 2022)
```

---

### Software

Recommended environment:

* Linux
* NVIDIA HPC Compiler (`nvc`)
* OpenMPI
* CUDA Toolkit
* OpenACC support
* OpenBLAS
* Git

### Hardware

Recommended hardware:

* Multi-core CPU
* NVIDIA GPU
* Multi-node cluster environment

---

## Build Instructions

### Clone Repository

```bash id="0w4u9q"
git clone https://github.com/Jillxu08/sc-tascell.git
cd sc-tascell
```

### Build

After installing the SC-Tascell environment:

```bash id="b4o0aq"
make
```

The provided Makefile uses:

* MPI (`mpicc`)
* NVIDIA HPC Compiler (`nvc`)
* OpenACC
* CUDA 12.2

Environment-specific modifications (e.g., CUDA version, GPU architecture, OpenBLAS paths, or MPI installation paths) may be required.

---

## Running Experiments

### Example Execution

```bash id="9xyrt5"
bash hmat_mpi.sh
```
---

## Benchmark Datasets

Datasets used in experiments include:

* Human models
* SphereCube
* SpherePyramid
* LargeSphere

Input files are located in:

```text id="n5mhdg"
bem_data/
```

---

## Research Focus

This project investigates:

* Efficient utilization of heterogeneous CPU–GPU systems
* Hybrid task scheduling strategies
* Distributed H-matrix computation
* Performance scalability on HPC systems

---

## Experimental Environment

The implementation has been tested on:

* Multi-node HPC clusters
* NVIDIA GPU platforms

Compiler environments include:

* GCC
* NVIDIA HPC SDK

---

## Notes

This repository is intended for:

* Research experiments
* HPC system studies
* Hybrid parallel programming research

Some scripts and implementations are experimental and may require environment-specific modifications.

---

## Future Work

Potential future improvements include:

* Improved GPU task scheduling
* Better MPI communication overlap
* Multi-GPU optimization
* Enhanced load balancing strategies
* Additional benchmark datasets

---

## Citation

```bibtex id="y1t1ez"
@inproceedings{xu2026hmatrix,
  author    = {Jing Xu and Tasuku Hiraishi and Zhengyang Bai and Akihiro Ida and Masahiro Yasugi and Takeshi Iwashita},
  title     = {Multi-node CPU-GPU H-Matrix Generation Using a Backtracking-based Load Balancing Framework},
  booktitle = {H3 Workshop (ISC 2026)},
  year      = {2026}
}
```

---

## Contact

Maintainer: Jing Xu

GitHub Repository:
[Jillxu08/sc-tascell](https://github.com/Jillxu08/sc-tascell?utm_source=chatgpt.com)
