# libNeon
*Neopixel driver library, effects included. Pure ESP-IDF and C++17.*

## Developer guide

### Folder structure
Important folders:
* `libneon/`  
  Library source code, divided in headers, source code, examples.
    * `libneon/{include, src, examples}/neo/`  
      All sources are placed in the subfolders *neo*. This reflects the namespace in which
      all the objects are located, and keeps the includes clean.
    * `libneon/examples/sdkconfig.defaults`  
      This is the default ESP-IDF SDK config file that should be used when building the examples.  
* `tests/`  
  Subfolder containing the unit test project.
    * `tests/lib/libneon/`  
      Symlink to `libneon/`, to allow the unit tests to pick up the local library folder
    * `tests/test/.keep`  
      We need to keep this folder for PlatformIO to believe we are providing unit test in our own
      custom entry point.

Secondary folders:
* `cicd/` Helper files needed by CI/CD
* `docs/` Doxygen config and additional doxygen sources
* `misc/` Helper files needed for setting up development, logos, non-source material.

### Setting up development
0. [Install PlatformIO CLI](https://platformio.org/install/cli).
1. Prepare `tests/platformio.ini`. You can, for example
    * Customize `tests/platformio.ini.sample` to your board and setup, or
    * Copy `cicd/platformio.ini`, the file used by CI/CD
2. Generate a compilation database for your IDE of choice using
   ```shell
   $> ./misc/gen-compiledb.py tests/platformio.ini       
   ```
   **You have to regenerate this when a new file is added.**
3. You are now using the unit test project to "host" the library (so you will see all usages of instantiated templates,
   for example).
4. Use the provided `.clang-format` file to format the source, e.g. by
   ```shell
   $> clang-format --style file -i libneon/src/neo/my_file.cpp
   ```

### Running the tests
**Note on the test project structure.**
We set up the unit test project in such a way that we can use both `pio run` and `pio test` to run the unit tests.
The two commands are similar but different enough that some commands are available for one and not the other (for
example, the compilation database is generated for `pio run` but not `pio test`). We work around this by providing a
test transport (similar to the one provided by `pio test`), our own `app_main()` function and building sources and tests
together.

0. Make sure you have setup your `tests/platformio.ini` as above.
1. Change directory and use either `pio test` or `pio run`, as follows:
   ```shell
   $> cd tests/
   $> pio run -t upload -t monitor  # or
   $> pio test
   ```

### Building the documentation
1. Install Doxygen (or run through Docker), and run
   ```shell
   $> doxygen ./doxygen.conf
   ```
2. The documentation can be seen at `./docs/_build/html/index.html`.