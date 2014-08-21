arduino.c
=========

This project is an effort to program for Arduino hardware using pure
C. You will need the Arduino base software (arduino-core Debian package)
to be installed.

Description
-----------

This package contains:

* a Makefile: self-sufficient (aside Arduino software itself) to compile
  C++ Arduino projects (including *.ino files) or a pure C Arduino project
  without libraries or serial access (no `/dev/ttyACM0` during execution).
* C core files: porting of a part of the C++ Arduino core for serial
  communications.
* examples:
 * **bargraph:** see http://arduino.cc/en/Tutorial/BarGraph
 * **2digits:** implements a two digit counter using two 7 segment displays
   and a single shift register (serial to parallel)


Set up
------

First, decide how you will call the Makefile.

### Option 1

The most convenient way is to add a symbolic link to the Makefile in
the project. For example:

    arduino.c/
      Makefile
      core/
      bargraph/
        Makefile -> ../Makefile
        ...

Then, you can just change dir to bargraph and run `make` (the Makefile
assumes core/ is in the same directory as the regular file Makefile).

### Option 2

If you cannot use symbolic link or are not sure how to use them, just
copy Makefile and core/ in your project directory:

    bargraph/
      Makefile
      core/
      ...

Again, you can compile by running just `make`.

### Option 3

Finally, you can of course use:

    make -f ../arduino.c/Makefile


Usage
-----

Then, check that the following variables are set to the right values:

VAR      | Preset value        | Description
---------|---------------------|-----------------------------------------------
TARGET   | program             | target program name (program.o, program.hex)
BOARD    | leonardo            | identifier of the board
ARD_BASE | /usr/share/arduino  | installation path of arduino (or sources)
PORT     | /dev/ttyACM0        | file pointing to the serial connection
LIB_OBJ  | $(BASE_PATH)/obj    | building directory for C core (e.g. /tmp/xyz)
LIB_C    | $(BASE_PATH)/core   | source directory for C core (c_*.c)

**Note:** `$(BASE_PATH)` is the directory where the Makefile is located

If you want to compile C++ Arduino project (like the Arduino IDE does),
you should add " MODE=cpp" to every call to the `make` command. If you
prefer, you can just uncomment the appropriate line in the Makefile.

The Makefile support the following commands:

* `make`           same as `make all`
* `make all`       build the .hex file to be sent to the board
* `make clean`     remove the project object files (*.o)
* `make cleanobj`  remove the global object files (obj/)
* `make destroy`   same as `make clean` and remove the .hex file
* `make rebuild`   same as `make destroy all`
* `make upload`    upload the .hex file to the board
