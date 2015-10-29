Word Count
----------

Counts words.

Building
--------

Prereqs:

    git
    CMake

First run initialize all Git submodules:

    $ git submodule update --init

If you haven't run CMake before, it's pretty easy and installers exist for all
major platforms. One important thing is that by default it may try to polute
the root directory with artifacts. The following will create a out of source
build in a different directory:

    $ mkdir build
    $ cmake -H. -Bbuild
    $ cmake --build build --config Release --clean-first
