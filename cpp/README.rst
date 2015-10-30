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
    $ mkdir build/msvc
    $ cmake -H. -Bbuild
    $ cmake --build build --config Release --clean-first -G "Visual Studio 14 2015"

---

Next
1. Send all words in map to top word, print out list of top words.
2. Make it so a directory of words is read recursively.
    First it has to generate a list of files.
3. Add subprocess that accept the directory, and their index out of some count. From this they figure out which chunk of a directory they will read-
    3.5 and send back word counts to some other proc which then figures out what the top word count was.
4. Change it so if the proc is done, it, then says it's "bored" and steals work from another proc. Still sends all words back at end.
5. Change to read files from web.
6. Figure out way to send words back frequently, see if it matters.
