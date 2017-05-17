obj-m := assoofs.o

all: ko mkassoofs

ko:
	make -C /lib/modules/4.4.0-71-generic/build  M=$(PWD) modules


mkassoofs_SOURCES:
	mkassoofs.c assoofs.h

clean:
	make -C /lib/modules//lib/modules/4.4.0-71-generic/build M=$(PWD) clean
	rm mkassoofs
