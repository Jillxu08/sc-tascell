# Multi-node CPU-GPU H-Matrix Generation with Tascell

## Overview

This project investigates hybrid CPU–GPU execution and distributed-memory parallelism for H-matrix computations based on the SC-Tascell framework. The implementation integrates MPI, task parallelism, and GPU acceleration to improve scalability and resource utilization on modern heterogeneous systems.

The project includes:

* MPI-based distributed execution
* Hybrid CPU/GPU task scheduling
* Experimental H-matrix implementations
* Multiple execution configurations and evaluation scripts
* Benchmark datasets and experiment utilities
* Tascell-based task-parallel implementations

The implementation is designed for research and experimental evaluation on HPC environments.

---

# Directory Structure

```text
.
├── bem_data/                 # BEM and H-matrix related source files and datasets
├── hmat_*.sh                 # Experiment and execution scripts
├── hmat_ct_*.tcell           # Tascell implementations
├── Makefile                  # Build configuration
└── README.md
```

---

# Features

## Hybrid CPU-GPU Execution

The framework supports multiple execution modes:

* CPU-only execution
* GPU-only execution
* Hybrid CPU-GPU execution

Workers can dynamically execute CPU-oriented or GPU-oriented tasks depending on resource availability.

## MPI Support

MPI is used for distributed-memory parallelism across multiple nodes.

Supported configurations include:

* Multi-node execution
* Multi-process execution
* Hybrid MPI + GPU execution

## Task Parallelism

The implementation is based on the Tascell task-parallel programming model.

Features include:

* Dynamic load balancing
* Work stealing
* Recursive task decomposition
* Parallel H-matrix computation

## GPU Acceleration

GPU acceleration is implemented using:

* OpenACC
* CUDA
* NVIDIA HPC Compiler (nvc)

Several computational kernels can be offloaded to GPUs for improved performance.

---

# Requirements

## Software

Recommended environment:

* Linux
* GCC or NVIDIA HPC Compiler (nvc)
* MPI implementation

  * OpenMPI
* CUDA Toolkit
* OpenACC support
* Git

## Hardware

Recommended:

* Multi-core CPU
* NVIDIA GPU
* Multi-node cluster environment

---

# Build Instructions

## Clone Repository

```bash
git clone https://github.com/Jillxu08/sc-tascell.git
cd sc-tascell
```

## Build

Example build command:

```bash
make
```

For NVIDIA HPC Compiler:

```bash
make CC=nvc
```

Depending on the environment, additional MPI or CUDA flags may be required.

---

# Running Experiments

## Example Execution

```bash
bash hmat_mpi.sh
```

## MPI Execution Example

```bash
mpirun -np 4 ./executable
```

## GPU Execution

Example:

```bash
bash hmat_mpi.sh
```

---

# Benchmark Datasets

Datasets used in experiments include:

* Human models
* SphereCube
* SpherePyramid
* LargeSphere

Input files are located in:

```text
bem_data/
```

---

# Research Focus

This project investigates:

* Efficient utilization of heterogeneous CPU-GPU systems
* Hybrid task scheduling strategies
* Distributed H-matrix computation
* Dynamic load balancing for irregular workloads
* Performance scalability on HPC systems

---

# Experimental Environment

The implementation has been tested on:

* Multi-node HPC clusters
* NVIDIA GPU platforms

Compiler environments include:

* GCC
* NVIDIA HPC SDK

---

# Notes

This repository is primarily intended for:

* Research experiments
* Performance evaluation
* HPC system studies
* Hybrid parallel programming research

Some scripts and implementations are experimental and may require environment-specific modifications.

---

# Future Work

Potential future improvements include:

* Improved GPU task scheduling
* Better MPI communication overlap
* Multi-GPU optimization
* Enhanced load balancing strategies
* Additional benchmark datasets

---

# Citation

@inproceedings{xu2026hmatrix,
  author = {Xu, Jing and Hiraishi, Tasuku and Bai, Zhengyang and Ida, Akihiro and Yasugi, Masahiro and Iwashita, Takeshi},
  title = {Multi-node CPU-GPU H-Matrix Generation Using a Backtracking-based Load Balancing Framework},
  booktitle = {H3 Workshop (ISC 2026)},
  year = {2026}
}
---

# Contact

Maintainer: Jing Xu

GitHub Repository:

[https://github.com/Jillxu08/sc-tascell](https://github.com/Jillxu08/sc-tascell)
