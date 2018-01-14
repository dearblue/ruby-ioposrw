# QUICK REFERENCE FOR IOPOSRW

## LIBRARIES

  * ``require "ioposrw"`` - main library
  * ``require "ioposrw/stringio"`` - need when use ``StringIO#readat/writeat``

## METHODS

  * ``IO#readat(offset, size = nil, buf = "") -> buf``<br>
    Similar functionality to the pread(2) system call. Implemented as ``IOPositioningReadWrite::IO#readat``.
  * ``IO#writeat(offset, buf) -> io``<br>
    Similar functionality to the pwrite(2) system call. Implemented as ``IOPositioningReadWrite::IO#writeat``.
  * ``StringIO#readat(offset, size = nil, buf = "") -> buf``<br>
      StringIO version pread(2). Implemented as ``IOPositioningReadWrite::StringIO#readat``.
  * ``StringIO#writeat(offset, buf) -> io``<br>
    StringIO version pwrite(2). Implemented as ``IOPositioningReadWrite::StringIO#writeat``.
