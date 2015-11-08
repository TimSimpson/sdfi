Word Max
--------

Given a directory, iterates all files and returns a list of the max word count.
Words are considered on a case insensitive basis. They can also include numbers.


Building
--------

Prereqs:

    * git
    * CMake
    * A newer version of g++ or MSVC (note: so full testing has only been
      performed on GCC / Linux).

First run initialize all Git submodules:

.. code-block:: bash

    $ git submodule update --init

The project uses CMake to build. If you aren't familar with it, you can use
"build.sh" or "build.bat" to build the project initially and then "rebuild.sh"
or "rebuild.bat" to build again.


Running
-------

All executables will be created in build/msvc or build/gcc depending on your
platform.

Single Process
~~~~~~~~~~~~~~

The simplest way to do a word count is to use the "read_directory" command,
which accepts an absolute path to a directory and returns the top 10 words
appearing in all files in that directory.

For example, say you wanted to read the directory "/books" and had built the
program using gcc. To see the top word count you could run:

.. code-block:: bash

    $ ./build/gcc/read_directory /books

Multiple Processes
~~~~~~~~~~~~~~~~~~

You can also run the program with multiple processes / machines. This involves
starting worker processes on each machine which are connected to by a master
process.

To do this, you need to make sure the directory you are going to read has the
same path and contents on every machine a worker will run.

The worker commands require a port. For example, say you wanted to run using
port 3451, you could start it like so:

.. code-block:: bash

    $ ./build/gcc/worker 3451

After you've started as many workers as you wish, kick them off using the
master process:

.. code-block:: bash

    $ ./build/gcc/master /books localhost 3451 192.168.1.2 3451 192.168.1.3 3451

The above command will start the master and connect to three worker processes-
localhost:3451, 192.168.1.2:3451, 192.168.1.3:3451 - and ask them to count
the words in the directory /books which should be present and identical on
each machine a worker process runs on.


Multiple Processes II
~~~~~~~~~~~~~~~~~~~~~

If the worker processes don't have access to the directories they need to read
then master2 / worker2 can be used instead. These work much like the earlier
programs except that master2 will read all the data and send it to worker2 to
read.

The second version chunks data into arbitrary points, forcing all worker2 to
send back a list of all the word counts which master2 then sums up before
finding the top words.


Multiple Processes III
~~~~~~~~~~~~~~~~~~~~~~

This approach is just like II above except that the master3 only sends words
beginning with a certain prefix to various worker3 processes.

For example, if there are three worker3 processes, the first will get all words
starting with a, d, g, j, etc. while the second gets all starting with b, e, h,
etc.

Because no two worker3 instances see the same word, they are free to send back
only the top ten words, of master3 takes the maximum of.


Performance
~~~~~~~~~~~

Here are my results from using "read_directory" on a single Rackspace Cloud
Server.

.. code-block:: bash

    Top words:

    1. the  7645440
    2. and  7055104
    3. i    5979392
    4. to   5395200
    5. of   4743424
    6. a    3974400
    7. you  3651840
    8. my   3318784
    9. that 3060736
    10. in  3032064
    Elapsed time: 489159ms

Here's the same result running worker processes on three Rackspace Cloud
Servers, with one running the master process:

.. code-block:: bash

    Top words:

    1. the  7645440
    2. and  7055104
    3. i    5979392
    4. to   5395200
    5. of   4743424
    6. a    3974400
    7. you  3651840
    8. my   3318784
    9. that 3060736
    10. in  3032064
    Elapsed time: 181320ms

Future Plans
~~~~~~~~~~~~

Because work is split via files, luck may have it that a worker might end up
having to read several large files while its peers read smaller ones and
finish earlier. If the workers could instead receive one (or maybe a handful)
of files from the master and ask for more as they finished the workload would
probably be better distributed in the presence of infrequent, very large files.

Currently all workers create a massive string to send back to the master. While
there haven't been any noticable problems doing this in theory it would mean
that any series of files with a large number of arbitrary words might require
too much memory to send back to the master process (though I'd imagine the
overcrowded map would be a problem before then).
