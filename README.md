# spassgen

This is a very simple password generator written in C using true
randomness from `/dev/random` or `dev/urandom`. Thats basically
all. With *simple* i mean, that it should be simple to code, to
maintain, to read and to use.

Compile with `make`. You can also use `make debug` if you want to
compile assert-statements and other useful debugging stuff.

`make dist` creates a tarball and a arch-linux package.

Any improvements, suggestions, pullrequests are very welcome.
