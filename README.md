# practica-assoofs-amoyag00
practica-assoofs-amoyag00 created by GitHub Classroom

Nota para compilar: en mi caso al usar make la ruta que usaba uname "/lib/modules/4.4.0-72-generic" no contenia la carpeta build.
En su lugar he usado la versión 4.4.0-71 poniendo su ruta directamente. Si no funciona tal y como est cambiar 
"make -C /lib/modules/4.4.0-71-generic/build  M=$(PWD) modules" por
 "make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules"
y "make -C /lib/modules//lib/modules/4.4.0-71-generic/build M=$(PWD) clean" por
"make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean"
