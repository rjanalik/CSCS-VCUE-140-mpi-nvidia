#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int rank;
    int size;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Process %i / %i start.\n", rank, size);

    // Each process waits different time
    sleep(rank * 5);

    printf("Process %i / %i finish.\n", rank, size);

    MPI_Finalize();

    return 0;
}
