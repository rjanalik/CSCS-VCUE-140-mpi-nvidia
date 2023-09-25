# cscs-bug-mpi-nvidia

## Issue
Running multiple MPI applications on hohgant works with `PrgEnv-cray`, but fails with `PrgEnv-nvidia`. 

## Background
Executing multiple MPI applications in one `srun` command is important, especially for running tests. MPI standard does not allow MPI commands after `MPI_Finalize()` (with few exceptions). In order to run various tests, test frameworks have to execute every test as an independent application. Test frameworks are often executed with a single `srun` command as `srun ctest` and then `ctest` runs all the test MPI applications.

In order to test the applications properly, it is important to use different programing environments. And many applications, especially those that use CUDA, require the NVIDIA programming environment (`PrgEnv-nvidia`).

## Steps to reproduce
### Works with cray environment
```bash
$ ssh hohgant
$ cd cscs-bug-mpi-nvidia
# Load cray environment
$ . setup-cray.sh
$ cc test-mpi.c -o test-mpi
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
$ cc test-mpi.c -o test-mpi
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

## Conclusion
From the steps described above, it seems that there is something wrong in the NVIDIA MPI library (libmpi_nvidia.so.12), as we can see the same problem with this library even when using different compilers (`cc` wrapper on the node and `mpicc` in a container).
