XEN_ROOT = $(CURDIR)/../..

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
include $(XEN_ROOT)/Config.mk
endif

all: main.a

main.a: main.o c_timers.o wtime.o c_print_results.o randdp.o
	$(AR) cr $@ $^

clean:
	rm -f *.a *.o
