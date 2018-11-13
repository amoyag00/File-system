# File system
Implementation of a file system based on the one made by psankar https://github.com/psankar/simplefs

Troubleshooting about compiling:
when using make, the route which used uname "/lib/modules/4.4.0-72-generic" didn't contain the build folder.
Instead, I have used the version 4.4.0-71 writing its route directlty.If it does not work, change
"make -C /lib/modules/4.4.0-71-generic/build  M=$(PWD) modules" for
 "make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules"
and "make -C /lib/modules//lib/modules/4.4.0-71-generic/build M=$(PWD) clean" for
"make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean"
