/*
    First move file or folders to remote server using: (LAN based pi-cluster)
        $ scp -r /local/file/path username@<server-ip>:/remote/file/path

    (on remote server) build executable
        $ mpic++ my_file.cpp -o my_exe

    (on remote server) run exe
        $ mpirun -np <desired ## of nodes for job> my_exe

    example:
        mpirun -np 15 --hostfile host_file hello_mpi++

    console output:
        Hello world from processor rpi0-main, rank 0 out of 15 processors
        Hello world from processor rpi0-main, rank 1 out of 15 processors
        Hello world from processor rpi0-main, rank 2 out of 15 processors
        Hello world from processor rpi0-main, rank 3 out of 15 processors
        Hello world from processor rpi3-node, rank 15 out of 15 processors
        Hello world from processor rpi1-node, rank 5 out of 15 processors
        Hello world from processor rpi2-node, rank 8 out of 15 processors
        Hello world from processor rpi3-node, rank 12 out of 15 processors
        Hello world from processor rpi1-node, rank 4 out of 15 processors
        Hello world from processor rpi2-node, rank 9 out of 15 processors
        Hello world from processor rpi3-node, rank 13 out of 15 processors
        Hello world from processor rpi1-node, rank 7 out of 15 processors
        Hello world from processor rpi2-node, rank 11 out of 15 processors
        Hello world from processor rpi3-node, rank 14 out of 15 processors
        Hello world from processor rpi1-node, rank 6 out of 15 processors
        Hello world from processor rpi2-node, rank 10 out of 15 processors
*/

#include "mpi.h"
#include <iostream>

int main (int argc, char** argv) 
{
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Print off a hello world message
    std::cout << "Hello world from processor " << processor_name << ", rank " 
              << world_rank << " out of " << world_size - 1 << " processors\n";
              
    // Finalize the MPI environment.
    MPI_Finalize();
}