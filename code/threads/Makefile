# --- SETTINGS ---
DEFINES = -DTHREADS
INCPATH = -I../threads -I../machine
CPPFLAGS = -g -Wall -m32 -fno-pie $(INCPATH) $(DEFINES) -DHOST_i386 -DHOST_LINUX

# --- OBJECT GROUPS ---
# Added security.o to this list
THREADS_OBJ = main.o scheduler.o synch.o system.o thread.o utility.o threadtest.o \
              preemptive.o globalbuffer.o producer.o consumer.o auth.o security.o

MACHINE_OBJ = interrupt.o stats.o sysdep.o timer.o

ALL_OBJ = $(THREADS_OBJ) $(MACHINE_OBJ) switch.o

# --- RULES ---
all: nachos

# Compile C++ files from threads directory
$(THREADS_OBJ): %.o: ../threads/%.cc
	g++ $(CPPFLAGS) -c $< -o $@

# Compile C++ files from machine directory
$(MACHINE_OBJ): %.o: ../machine/%.cc
	g++ $(CPPFLAGS) -c $< -o $@

# Assemble the context switch code
switch.o: ../threads/switch.s
	gcc -m32 -c ../threads/switch.s -o switch.o

# Link everything together
nachos: $(ALL_OBJ)
	g++ -m32 -no-pie -z noexecstack $(ALL_OBJ) -o nachos

clean:
	rm -f *.o nachos swtch.s

depend:
	@echo "Dependencies skipped."