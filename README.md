## cdcc - Compile flags database generating compiler wrapper

A wrapper for C/C++ compilers, e.g. GCC and clang, that will collect compile
flags used into a sqlite3 database, from which compile_commands.json files
can be generated.

### Usage

**Collect compile flags during compilation**

Use `cdcc-cc` instead of your preferred c/c++ compiler. Which compiler will
be invoked is controlled via the suffix of the name the binary is executed
with, i.e. `cdcc-gcc` (as a symlink to `cdcc-cc`) will call `gcc`.

The default location for the database is `$HOME/.cache/cdcc.db`; it can
be controlled via the `CDCC_DB` environment variable.


**Generation of `compile_commands.json`**

The `compile_commands.json` file can be generated from the database via
`cdcc-gen <PATH>`. The generated file will be place inside `<PATH>`.


**JHBuild**

To use `ccdc` with JHBuild put the following snippet in your
`~/.config/jhbuildrc` file.

``` python
if spawn.find_executable('cdcc-gcc') is not None:
	os.environ['CC'] = 'cdcc-gcc'
	os.environ['CXX'] = 'cdcc-g++'
```

