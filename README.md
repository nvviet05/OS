# OS Simulator - Operating System Simulator

An educational operating system simulator written in C that demonstrates core OS concepts including CPU scheduling, memory management, process loading, and system calls.

## Project Overview

This project is a lightweight OS simulator designed to teach and illustrate fundamental operating system principles. It implements a multi-level queue (MLQ) scheduler, virtual memory management with paging, and syscall handling on a simulated multi-CPU environment.

## Features

### Core Components

- **CPU Simulation**: Multi-CPU support with configurable time-slicing and context switching
- **Process Scheduling**: 
  - Multi-Level Queue (MLQ) scheduling with priority-based process management
  - 140 priority levels
  - Dynamic process loading and dispatching
  
- **Memory Management**:
  - Virtual memory with paging support
  - Physical RAM simulation (up to 4MB addressable space)
  - Swap space support for memory overflow
  - Page table management with present/dirty/swapped flags
  - 64-bit address mode support
  
- **System Calls**: Syscall infrastructure with extensible syscall table
  - Memory-related system calls
  - System call listing functionality
  
- **Process Management**:
  - Process loading from input files
  - Process control blocks (PCB)
  - Timer-based interrupts

## Directory Structure

```
├── src/                      # Source files
│   ├── os.c                 # Main OS simulator
│   ├── sched.c              # Scheduler implementation
│   ├── cpu.c                # CPU simulation
│   ├── mem.c                # Memory management
│   ├── mm.c                 # Virtual memory (paging)
│   ├── mm-vm.c              # VM implementation details
│   ├── mm-memphy.c          # Physical memory management
│   ├── mm64.c               # 64-bit address support
│   ├── loader.c             # Process loader
│   ├── timer.c              # Timer and interrupts
│   ├── syscall.c            # System call handler
│   ├── sys_mem.c            # Memory syscalls
│   ├── sys_listsyscall.c    # Syscall listing
│   ├── queue.c              # Ready queue implementation
│   ├── libmem.c             # Memory utility library
│   ├── libstd.c             # Standard library functions
│   ├── paging.c             # Paging implementation
│   ├── syscall.tbl          # Syscall table definition
│   └── syscalltbl.sh        # Syscall table generator
│
├── include/                 # Header files
│   ├── os-cfg.h             # OS configuration
│   ├── cpu.h                # CPU structures and functions
│   ├── sched.h              # Scheduler interface
│   ├── mm.h                 # Memory management definitions
│   ├── mm64.h               # 64-bit memory support
│   ├── os-mm.h              # OS-level memory management
│   ├── mem.h                # Memory structures
│   ├── loader.h             # Process loader interface
│   ├── timer.h              # Timer interface
│   ├── syscall.h            # Syscall interface
│   ├── queue.h              # Queue data structure
│   ├── libmem.h             # Memory library interface
│   ├── bitops.h             # Bit operation macros
│   └── common.h             # Common definitions
│
├── input/                   # Input test cases
│   ├── proc/                # Process test files
│   │   ├── m0s, m1s        # Memory test processes
│   │   ├── p0s, p1s, etc.  # Paging test processes
│   │   └── s0, s1, etc.    # Scheduler test processes
│   ├── sched_*              # Scheduler test inputs
│   └── os_*                 # Full OS simulation inputs
│
├── output/                  # Simulation output results
│   └── *.output             # Output files from test runs
│
├── obj/                     # Compiled object files
│
└── Makefile                 # Build configuration
```

## Configuration

The behavior of the OS simulator is controlled via compile-time flags in [include/os-cfg.h](include/os-cfg.h):

### Key Configuration Options

| Option | Default | Purpose |
|--------|---------|---------|
| `MLQ_SCHED` | Enabled | Multi-Level Queue scheduling |
| `MAX_PRIO` | 140 | Maximum priority level |
| `MM_PAGING` | Enabled | Virtual memory with paging |
| `MM64` | Enabled | 64-bit address support |
| `IODUMP` | Enabled | Dump I/O operations |
| `PAGETBL_DUMP` | Enabled | Dump page table operations |
| `MMDBG` | Disabled | Memory management debug output |
| `VMDBG` | Disabled | Virtual memory debug output |

## Building

### Requirements
- GCC compiler
- POSIX-compliant system (Linux/Unix/WSL)
- GNU Make
- pthread library

### Compilation

```bash
# Build the OS simulator (default target)
make

# Clean build artifacts
make clean

# Build memory management module only
make mem

# Build scheduler module only
make sched
```

The build process:
1. Compiles all source files into object files in the `obj/` directory
2. Generates the syscall table from `src/syscall.tbl`
3. Links all object files into the `os` executable

## Running Simulations

### Basic Usage

Run a simulation with an input file:
```bash
./os < input/<test_case>
```

### Available Test Cases

The `input/` directory contains pre-configured test cases:

**Scheduler Tests:**
- `sched` - Basic scheduler test
- `sched_0` - Single CPU scheduler test
- `sched_1` - Multi-CPU scheduler test

**Memory Tests:**
- `os_0_mlq_paging` - Paging with MLQ scheduling
- `os_1_mlq_paging` - Advanced paging test
- `os_1_mlq_paging_small_1K` - Small page size (1K) test
- `os_1_mlq_paging_small_4K` - Small page size (4K) test
- `os_1_singleCPU_mlq` - Single CPU with MLQ
- `os_1_singleCPU_mlq_paging` - Single CPU with paging

**System Call Tests:**
- `os_syscall` - Basic syscall test
- `os_syscall_list` - Syscall listing test
- `os_sc` - Syscall functionality test

### Example Output

```
Time slot   0
ld_routine
Time slot   1
...
Time slot  10
	CPU 0: Dispatched process  1
Time slot  11
	CPU 0: Processed  1 has finished
	CPU 0 stopped
```

Output files are saved to the `output/` directory with corresponding names.

## Simulation Components

### CPU Module
- Simulates one or more CPUs executing processes
- Time-sliced execution with context switching
- Timer interrupt handling
- Process state management

### Scheduler (MLQ)
- Priority-based process scheduling
- Multiple priority queues
- Process ready queue management
- CPU dispatch logic

### Memory Management
- **Physical Memory**: Simulated RAM with configurable size
- **Virtual Memory**: Page-based translation with page tables
- **Swap Space**: Secondary storage for page swapping
- **Page Table Management**: Tracks page presence, dirty state, and swapped pages

### Process Loader
- Reads process specifications from input files
- Initializes process control blocks
- Allocates memory for processes
- Enqueues processes to scheduler

### System Calls
- Memory allocation and deallocation
- Process information queries
- Extensible syscall table

### Timer
- Generates clock interrupts
- Enforces time slices
- Manages process scheduling events

## Project Purpose

This is an educational project for the CO2018 course at HCMC University of Technology. It provides hands-on experience with:
- OS kernel architecture
- CPU scheduling algorithms
- Memory management and virtual memory
- Process synchronization and control
- System call interfaces

## Files Generated

### Object Files (`obj/`)
- `.o` files compiled from source code

### Executable
- `os` - Main OS simulator executable

### System Files
- `src/syscalltbl.lst` - Generated syscall table listing

## Notes

- The simulator uses POSIX threads (`pthread`) for CPU simulation
- Process priorities range from 0 to 139 (lower number = higher priority)
- Page size is fixed at 256 bytes
- Addressable memory space: 4MB (22-bit bus)
- RAM size: 2MB
- Swap space: 512MB

## License

Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM

Source Code License Grant: The authors hereby grant to Licensee personal permission to use and modify the Licensed Source Code for the sole purpose of studying while attending the course CO2018.
