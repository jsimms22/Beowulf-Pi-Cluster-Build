# Beowulf-style MPI Cluster on Raspberry Pi Using Ubuntu 22.04 LTS

This outlines my process in setting a 4-node Beowulf-style Cluster with Raspberry Pi 4s (2Gb).

Decide on a implementation of the MPI standard: MPICH or OpenMPI

Guide on manual install (OpenMPI): https://docs.open-mpi.org/en/v5.0.x/installing-open-mpi/quickstart.html
Guide on manual install (MPICH): https://www.mpich.org/static/downloads/4.1.2/mpich-4.1.2-installguide.pdf

Either is fine, the most recent MPICH is probably closer to the most current MPI Standard. OpenMPI is a more generalized implementation approach. Both use the same naming convention for the header files, and Bash scripts for compiling and running MPI apps.

For this guide I used OpenMPI.

## Cluster Specifications Used for this Guide:
-4 x Raspberry Pi 4s (quad-core, 2Gb RAM)
    with POE+ Hat, 64Gb SD Card
    
-POE+ capable networking switch (or 4 x dedicated power supplies)

-Ubuntu 22.04 LTS

## Step 1: Flash SD Cards
-download the raspbian image loader from https://www.raspberrypi.com/software/

-download OS .img file 

-use the image loader to flash your SD cards
    -at this point I chose input my default user/pass 
    -and, the device hostname
    -naming convention I chose:
        >rpi0-main (control node)
        >rpi1-node (worker node)
        >rpi2-node (worker node)
        >rpi3-node (worker node)

## repeat the following steps for each pi 
-Note: This will require a dedicated keyboard + monitor initially, at least until ssh server is installed and configured
-Note: It is strongly recommended to ensure your DNS assigns static IPs to all of your raspberry pis. Dynamic IPs may result in services being unable to connect to each other later on.

## Step 2: Acquire IP info and modify hosts file
-insert flashed SD Cards and supply power to boot up the pis

-login with default user credentials

-use ip -a to identify the IP

```
For me my IPs were assigned:
192.168.55.70 rpi0-main
192.168.55.71 rpi1-node
192.168.55.72 rpi2-node
192.168.55.73 rpi3-node
```

-modify /etc/hosts file

```
$ sudo nano /etc/hosts
```

-add the IPs/Hostnames for each node to the existing information

-verify changes were added (optional)

```
$ sudo cat /etc/hosts
```

## Step 3 (optional): Add new user or use default user if for personal use
-Whichever option, it is strongly recommended to have the same username for all nodes

-To add a new user, it will require sudo privelages

```
$ sudo adduser mpiuser
```

-Add new user to the sudo group

```
$ sudo usermod -aG sudo mpiuser
```

-Add or change the password for the new user

```
$ sudo passwd mpiuser
```

-Switch to the new user

```
$ su - mpiuser
```

Step 4: Install openssh server

```
$ sudo apt install openssh-server
$ sudo systemctl start openssh
```

-You may need to change the default options for your ssh server service

-options config file can be found here

``` 
$ cat /etc/ssh/sshd_config
``` 

-by default password and pubkey authentication are disallowed, and the listening port number is 22

-notes: 
    -password auth will be required for initial key sharing
    -recommend disallowing password auth after pub key auth is set up
    -recommend changing your default ssh server listening port to something non-standard and non-conflicting (example: 32323)
    -any changes to this config file will require the ssh server service to be restarted, this can be done with systemctl
        $ sudo systemctl restart ssh
    -you can check the status of ssh with:
        $ sudo systemctl status ssh

## Step 5: Enable Password-Less SSH
-Create keys with default options (or use -b option for changing key size complexity, -t option to change the type of encryption used) 
    $ ssh-keygen

-Follow prompts

-Navigate to where the keys were stored (unless default location - ~/.ssh)
    $ cd path/to/keys

-Copy the current node's public key to the other nodes for authentication
    $ ssh-copy-id mpiuser@"node IP or hostname here"
        -Alternatively
            $ ssh-copy-id "node IP or hostname here"
        -ssh into desired user of targeted machine (repeat for every other node)
            $ eval 'ssh-agent'
            $ ssh-add ~/.ssh/key-name

-Verify you can ssh into every other node from each node without needing a password

-Note: It may be useful to generate keys for a client computer at this point, using the same process. Especially if you plan to remove password authentication.

## Step 6a: Setting up NFS Server (Control Node Only)
-there are other services that can be used besides NFS, basic idea is providing a shared folder over your network that every node to can mount locally and have R+W access

-install required package for NFS
    $ sudo apt install nfs-kernel-server

-create the folder that will be used, we named ours cloud-nfs
    $ mkdir ~/cloud-nfs

-export the folder via an entry in /etc/exports
    $ sudo nano /etc/exports

-verify you added the entry with
    $ cat /etc/exports
    /home/mpiuser/cloud-nfs *(rw,sync,no_root_squash,no_subtree_check)

-note: * is a wildcard ip. It may be better, and more secure to specify the ip you intend to share the directory with. But, this may look like this
     $ cat /etc/exports
    /home/mpiuser/cloud-nfs 192.168.55.71(rw,sync,no_root_squash,no_subtree_check)
    /home/mpiuser/cloud-nfs 192.168.55.72(rw,sync,no_root_squash,no_subtree_check)
    /home/mpiuser/cloud-nfs 192.168.55.73(rw,sync,no_root_squash,no_subtree_check)

-export your altered /etc/exports folder (do this anytime after you modify it)
    $ exportfs -a

-restart your nfs server service (do this anytime after you modify /etc/exports)
    $ sudo service restart nfs-kernel-server

## Step 6b: Setting up NFS Client (Worker Nodes Only)
-install required package
    $ sudo apt install nfs-common

-make a mirrored folder directory to sync with the NFS server (it does not have to be the same name as your control node's folder)
    $ mkdir ~/cloud-nfs

-mount the shared directory to the folder you just made
    $ sudo mount -t nfs <control node ip>:/home/mpiuser/cloud-nfs ~/cloud-nfs
    
-verify the directory was mounted
    $ df -h

-if you want to automate this mounting to occur at boot, add an entry to /etc/fstab
    $ sudo nano /etc/fstab

-verify changes
    $ sudo cat /etc/fstab
    <control node ip>:/home/mpiuser/cloud-nfs /home/mpiuser/cloud-nfs nfs

-if this does not allow the mount to happen during boot, view your boot logs and fix the issue, or manually mount via
    $ sudo mount -a

-Note: alternatively to diving into the boot logs you could set up a script that runs the above command during boot

## Step 7: Install make and other basic build tools (like GNU compilers)
    $ sudo apt install build-essential

## Step 7: Install MPI
-Note: you must install the version with the same configuration on each node. Or you can create a symbolic link in the shared folder to the MPI header/scripts/libraries and only install MPI on the control node. If you are manually installing MPI this may be easier over time. But, I do not recommend this unless you are comfortable troubleshooting symbolically linked folders/files and have a decent grasp on the concept of linking libraries for code compilation.

-Easier method, with less control over the installation, run this on each node:
    $ sudo apt install openmp-bin

-Verify the locations of the libraries, header files, and other useful things for the linker to know with the MPI compile script of your chosen language (see OpenMPI doc for all)
    $ mpic++ -showme
    g++ -I/usr/lib/aarch64-linux-gnu/openmpi/include -I/usr/lib/aarch64-linux-gnu/openmpi/include/openmpi -L/usr/lib/aarch64-linux-gnu/openmpi/lib -lmpi_cxx -lmpi

-It is strongly recommended to let the provided MPI scripts to build and run MPI programs. They will handle the linking for you so you do not need to worry about. Otherwise you now have the information to do it manually

## Step 7: Create a MPI version of hello world
-if coding directly on the control node:
    $ cd ~/cloud-nfs
    $ mkdir hello_mpi
    $ nano hello_mpi.cpp

-if coding on a client device with your favorite IDE, and planning to only compile and run on your control node, use the scp command to transfer files to your cluster:
    $ scp -r /your/local/path/to/hello_mpi.cpp mpiuser@<control node ip>:~/cloud-nfs/learning_mpi

-here is an example, a C++ version based on the C implementation from:
    https://github.com/mpitutorial/mpitutorial/blob/gh-pages/tutorials/mpi-hello-world/code/mpi_hello_world.c:

```
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
``` 

-compile your program:
    $ mpic++ mpic++ hello_mpi.cpp -o hello_mpi++

-run your program:
    $ mpirun -np 4 hello_mpi++
note: -np is where you tell the script how many processors you want to employ for this executable, here we are using 4.

-sample output:
```
Hello world from processor rpi0-main, rank 0 out of 4 processors
Hello world from processor rpi0-main, rank 1 out of 4 processors
Hello world from processor rpi0-main, rank 2 out of 4 processors
Hello world from processor rpi0-main, rank 3 out of 4 processors
```

-you may have noticed that all 4 processors were from the same node... but didn't we build a cluster with 4 different machines?

-OpenMPI will do what is efficient first, spawn tasks for the most available processors, my raspberry pies are quad-core so each one pi can ingest 4 MPI "slots"

-run your program with more requested processors then maximum cores on a single machine:
    $ mpirun -np 5 hello_mpi++

-sample output:
```
--------------------------------------------------------------------------
There are not enough slots available in the system to satisfy the 5
slots that were requested by the application:

  hello_mpi++

Either request fewer slots for your application, or make more slots
available for use.

A "slot" is the Open MPI term for an allocatable unit where we can
launch a process.  The number of slots available are defined by the
environment in which Open MPI processes are run:

  1. Hostfile, via "slots=N" clauses (N defaults to number of
     processor cores if not provided)
  2. The --host command line parameter, via a ":N" suffix on the
     hostname (N defaults to 1 if not provided)
  3. Resource manager (e.g., SLURM, PBS/Torque, LSF, etc.)
  4. If none of a hostfile, the --host command line parameter, or an
     RM is present, Open MPI defaults to the number of processor cores

In all the above cases, if you want Open MPI to default to the number
of hardware threads instead of the number of processor cores, use the
--use-hwthread-cpus option.

Alternatively, you can use the --oversubscribe option to ignore the
number of available slots when deciding the number of processes to
launch.
--------------------------------------------------------------------------
```

-Okay, so obviously I have not told you everything you need to properly say hello from your whole cluster. mpirun does not know instinctively where all the nodes are, it needs a list of nodes and their ips (or hostnames if ips are static, however I recommend using IPs anyways, unless you have a specific reason).

-In the same directory as your hello_mpi program, create a host_file 
    $ nano host_file
    
-Add the nodes you wish to target via ips in here, for example:
```
192.168.55.70
192.168.55.71
192.168.55.72
192.168.55.73
```

-You can all and use this as a template for all other programs, or request less and have specific host_files per each program directory

-Now run your program again with the --hostname [filename] option added after your requested slots, this time I am going to request the max number for my whole cluster.
    $ mpirun -np 16 --hostfile host_file hello_mpi++

-output:
```
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
```
