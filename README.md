# Analysis of MPI programs

The following operations will be analysed:

Point-to-point-Communication:
- send, isend
- bsend, ibsend
- rsend, irsend
- ssend, issend
- recv, irecv

	basically integrated into the analysingsystem but not analysed and added into queue yet
	- (mrecv, imrecv)
	- (sendrecv)
	- (sendrecv_replace)

Collective-communication:
- bcast, ibcast
- gather, igather
- gatherv, igatherv
- scatter, iscatter
- scatterv, iscatterv

One-sided-communication:


INFO BTLs:
template-btl and btl-usnic are not implemented for the analysis.

# Open MPI

[The Open MPI Project](https://www.open-mpi.org/) is an open source
implementation of the [Message Passing Interface (MPI)
specification](https://www.mpi-forum.org/docs/) that is developed and
maintained by a consortium of academic, research, and industry
partners.  Open MPI is therefore able to combine the expertise,
technologies, and resources from all across the High Performance
Computing community in order to build the best MPI library available.
Open MPI offers advantages for system and software vendors,
application developers and computer science researchers.

## Official documentation

The Open MPI documentation can be viewed in the following ways:

1. Online at https://docs.open-mpi.org/
1. In self-contained (i.e., suitable for local viewing, without an
   internet connection) in official distribution tarballs under
   `docs/_build/html/index.html`.

## Building the documentation locally

The source code for Open MPI's docs can be found in the Open MPI Git
repository under the `docs` folder.

Developers who clone the Open MPI Git repository will not have the
HTML documentation and man pages by default; it must be built.
Instructions for how to build the Open MPI documentation can be found
here:
https://docs.open-mpi.org/en/main/developers/prerequisites.html#sphinx.
