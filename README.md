# daemonize
##### Executes a program as detached from its parent

----

This program allows to run programs as children of init,
it can be useful if you don't care about a program's text output
and you want to free the shell, for example if you run a GUI program.

### How to use

#### Build the program

`make`

#### Run something with it

`./daemonize program arg1 arg2 arg3 ...`

