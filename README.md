# cscs-bug-mpi-nvidia

## Issue
Running multiple MPI applications on hohgant works with `PrgEnv-cray`, but fails with `PrgEnv-nvidia` and inside a container (Sarus). 

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
# run the test app once : OK
$ srun -p amdgpu -N 1 -n 2 bash -c "./test-mpi"
# run the test app twice : OK
$ srun -p amdgpu -N 1 -n 2 bash -c "./test-mpi && ./test-mpi"
```

### Fails with nvidia environment
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
Again, the first run of `./test-mpi` succeeds, the second one fails, as with the nvidia environment.
This is likely because Sarus replaces the MPI library in the container with the one from the nvidia environment. Sarus does this in the MPI hook.
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
It seems we can blame the nvidia MPI library (libmpi_nvidia.so.12), as we can see the same problem with this library even when using different compilers (`cc` wrapper on the node and `mpicc` in a container).
