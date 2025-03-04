# SKA Mid.CBF First Order Delay Model Generation Library

This library supports the generation of first order delay models in the Mid.CBF Resampler Delay Tracker (RDT). It is expected to be used in both the TDC (TalonDX) version and the Agilex version of the RDT software.

## Build

X86_64 build
`make cpp-build-x86`

X86_64 build debug for unit tests. 
`make cpp-build-debug`

ARMv8 build for TalonDX
`make cpp-build-armv8`

## Unit test

To run the unit test suite:
`make cpp-test`