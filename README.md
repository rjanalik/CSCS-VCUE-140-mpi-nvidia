# cscs-bug-mpi-nvidia

## Issue
Running multiple MPI applications on hohgant works with `PrgEnv-cray`, but fails with `PrgEnv-nvidia` and inside a container (Sarus). 

## Background
Executing multiple MPI applications in one `srun` command is important, especially for running tests. MPI standard does not allow MPI commands after `MPI_Finalize()` (with few exceptions). In order to run various tests, test frameworks have to execute every test as an independent application. Test frameworks are often executed with a single `srun` command as `srun ctest` and then `ctest` runs all the test MPI applications.

In order to test the applications properly, it is important to use different programing environments. And many applications, especially those that use CUDA, require the NVIDIA programming environment (`PrgEnv-nvidia`).

Another important use case for the NVIDIA programming environment is any containerized application that uses MPI. Sarus, the container engine developed at CSCS, replaces the MPI library in the container with the one from `PrgEnv-nvidia` on the host. This cannot be changed, as most of the applications also use GPUs. Again, testing such applications lead to the same use case, running multiple MPI applications in one `srun` command.

## Steps to reproduce
### Works with cray environment
```bash
$ ssh hohgant
$ cd cscs-bug-mpi-nvidia
# Load cray environment
$ . setup-cray.sh
$ make -B
# Check the MPI library
$ ldd test-mpi
...
	libmpi_cray.so.12 => /opt/cray/pe/lib64/libmpi_cray.so.12 (0x00001543fb2cf000)
...
$ ls -l /opt/cray/pe/lib64/libmpi_nvidia.so.12
lrwxrwxrwx 1 root root 65 Oct 27  2022 /opt/cray/pe/lib64/libmpi_nvidia.so.12 -> /opt/cray/pe/mpich/8.1.21/ofi/nvidia/20.7/lib/libmpi_nvidia.so.12
# run the test app once : OK
$ srun -p amdgpu -N 1 -n 2 bash -c "./test-mpi"
# run the test app twice : OK
$ srun -p amdgpu -N 1 -n 2 bash -c "./test-mpi && ./test-mpi"
```

### Fails with NVIDIA environment
Note: Use `srun` with `--mpi=pmi2`
```bash
$ ssh hohgant
$ cd cscs-bug-mpi-nvidia
# Load nvidia environment
$ . setup-nvidia.sh
$ make -B
# Check the MPI library
$ ldd test-mpi
...
	libmpi_nvidia.so.12 => /opt/cray/pe/lib64/libmpi_nvidia.so.12 (0x000014d6ddfab000)
...
$ ls -l /opt/cray/pe/lib64/libmpi_cray.so.12
lrwxrwxrwx 1 root root 61 Oct 27  2022 /opt/cray/pe/lib64/libmpi_cray.so.12 -> /opt/cray/pe/mpich/8.1.21/ofi/cray/10.0/lib/libmpi_cray.so.12
# run the test app once : OK
$ srun --mpi=pmi2 -p amdgpu -N 1 -n 2 bash -c "./test-mpi"
# run the test app twice : fail (the first one succeeds, the second one fails)
$ srun --mpi=pmi2 -p amdgpu -N 1 -n 2 bash -c "./test-mpi && ./test-mpi"
```
The first run of `./test-mpi` succeeds, the second one fails.

### Fails inside a container (Sarus)
Note: Use `srun` with `--mpi=pmi2` and `sarus` with `--mpi`
```bash
$ ssh hohgant
$ cd cscs-bug-mpi-nvidia
# Do not load any environment, Sarus does not use it
# compile the test app inside a container
$ srun -N 1 -n 1 --mpi=pmi2 -p amdgpu sarus run --mpi --mount=type=bind,source=$HOME,destination=$HOME quay.io/madeeks/osu-mb:6.2-mpich4.1-ubuntu22.04 bash -c "cd ~/cscs-bug-mpi-nvidia && make -B CC=mpicc && ldd ./test-mpi"
...
	libmpi.so.12 => /usr/lib/libmpi.so.12 (0x00007f70bb01b000)
...
$ srun -N 1 -n 2 --mpi=pmi2 -p amdgpu sarus run --mpi --mount=type=bind,source=$HOME,destination=$HOME quay.io/madeeks/osu-mb:6.2-mpich4.1-ubuntu22.04 bash -c "cd ~/cscs-bug-mpi-nvidia && ./test-mpi"
$ srun -N 1 -n 2 --mpi=pmi2 -p amdgpu sarus run --mpi --mount=type=bind,source=$HOME,destination=$HOME quay.io/madeeks/osu-mb:6.2-mpich4.1-ubuntu22.04 bash -c "cd ~/cscs-bug-mpi-nvidia && ./test-mpi && ./test-mpi"
```
Again, the first run of `./test-mpi` succeeds, the second one fails, as with the NVIDIA environment.
This is likely because Sarus replaces the MPI library in the container with the one from the NVIDIA environment. Sarus does this in the MPI hook.
```bash
$ cat /opt/sarus/1.6.0/etc/hooks.d/070-mpi-hook.json
...
            "MPI_LIBS=/opt/sarus/1.6.0/mpi_links/libmpi.so.12.1.8:/opt/sarus/1.6.0/mpi_links/libmpifort.so.12.1.8",
...
$ ls -l /opt/sarus/1.6.0/mpi_links/libmpi.so.12.1.8 /opt/sarus/1.6.0/mpi_links/libmpifort.so.12.1.8
lrwxrwxrwx 1 root root 38 Sep 19 16:15 /opt/sarus/1.6.0/mpi_links/libmpi.so.12.1.8 -> /opt/cray/pe/lib64/libmpi_nvidia.so.12
lrwxrwxrwx 1 root root 42 Sep 19 16:15 /opt/sarus/1.6.0/mpi_links/libmpifort.so.12.1.8 -> /opt/cray/pe/lib64/libmpifort_nvidia.so.12
```

## Conclusion
From the steps described above, it seems that there is something worng in the NVIDIA MPI library (libmpi_nvidia.so.12), as we can see the same problem with this library even when using different compilers (`cc` wrapper on the node and `mpicc` in a container).
