# daemonize
## Detachedly execute a program from its parent

This utility allows programs to run as children of `init`, which can be useful
if you don't care about a program's text output and you want to free the shell,
as in cases of GUI programs.

## Usage
### Build the program

```
./autogen.sh && ./configure && make
```

### Install the program (optional)
```
make install
```

Use the --prefix option of configure if you want a custom
install location.


### Run it with something
```
./daemonize program arg1 arg2 arg3 ...
```
