# Description
This repo contains Da Shi, Jason Yu and Yunlan Li's implementation of
various features for the Linux v5.4 kernel as part of the *W4118 Operating
System I* course offered at Columbia University in the City of New York in
Spring 2021, taught by Professor Jason Nieh.

# Branch
The details of each Linux kernel feature we implemented can be found in the
pdf on master branch. Additionally, each branch contains our work for a
specific feature. As an example, the branch *PPAGEFS* contains our
implementation for a in-memory filesystem where in its root directory,
there's a PID.PROCESSNAME directory for each currently running process and
within this directory, there are 2 files *zero* and *total* that contains
the number of distinct zero and total ppages respectively mapped by the given
process.

## Branch -> Homework No. Mapping

  Branch              |     Homework No.
  ---                 |     ---
  PSTRACE             |     3
  WRR\_SCHEDULER      |     4
  EXPOSE\_PAGE\_TABLE |     5
  PPAGEFS             |     6

