# QUICK REFERENCE FOR IOPOSRW

## LIBRARIES

*   ``require "ioposrw"`` - main library
*   ``require "ioposrw/stringio"`` - need when use ``StringIO#pread/pwrite``

## METHODS

*   ``IO#pread`` - Similar functionality to the pread(2) system call. Implemented as ``IOPositioningReadWrite::IO#pread``.
*   ``IO#pwrite`` - Similar functionality to the pwrite(2) system call. Implemented as ``IOPositioningReadWrite::IO#pwrite``.
*   ``StringIO#pread`` - StringIO version pread(2). Implemented as ``IOPositioningReadWrite::StringIO#pread``.
*   ``StringIO#pwrite`` - StringIO version pwrite(2). Implemented as ``IOPositioningReadWrite::StringIO#pwrite``.
