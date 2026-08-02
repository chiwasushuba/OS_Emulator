The parameters for the "config.txt" should be:
num-cpu 2
scheduler fcfs
quantum-cycles 1
batch-process-freq 1
min-ins 4000
max-ins 4000
delays-per-exec 0
max-overall-mem 512
mem-per-frame 256
min-mem-per-proc 512
max-mem-per-proc 512

This is the sequence:
1. Run the "initialize" command.
2. Run the command: screen -s process1
3. Wait for 5 sec
3. Type the "screen -ls" command
4. Type the "process-smi" command


Expected output:

The expected output should allow 2nd process to run, but it will result in very high page-ins and page-outs because only 1 process can fit the main memory. The system will be in a constant state of swapping pages to and from the backing store. 

For vmstat, it should show the following information such as total memory, used memory, and free memory. used memory = 32768KB Free memory = 0KB

{2nd part}

The parameters for the "config.txt" should be :
num-cpu 1
scheduler rr
quantum-cycles 10
batch-process-freq 1
min-ins 1000
max-ins 1000
delays-per-exec 0
max-overall-mem 256
mem-per-frame 256
min-mem-per-proc 256
max-mem-per-proc 256

This is the sequence:
1. Run the "initialize" command.
2. Execute the following command: screen -c faulty_process 256 "DECLARE varA 10; DECLARE varB 5; ADD varA varA varB; WRITE 0x500 varA; READ varC 0x500; PRINT(\"Variable A\" + varA); PRINT(\"Result: \" + varC)"
3. Type the "screen -ls" command
3. Type the "screen -r" command to access faulty_process

Expected output:
The correct var. A and C are printed in the console.




{THis is the stress Test , making sure that the program + computer is prepared}

The parameters for the "config.txt" should be:
num-cpu 32
scheduler fcfs
quantum-cycles 5
batch-process-freq 1
min-ins 100
max-ins 100
delays-per-exec 0
max-overall-mem 4096
mem-per-frame 64
min-mem-per-proc 512
max-mem-per-proc 512

This is the sequence:
1. Run the "initialize" command.
2. Run the "scheduler-start" command
3. Wait for 20 seconds
3. At every 2-second interval, type the "process-smi" command and then type the "vmstat" command. Repeat this process for 20 seconds.

Expected Outpu:

No Deadlock occurs.

The vmstat should clearly indicate a full memory, and hte number of paged in/outs are continuously increasing

If possible the processes are moving to the "finished processes" list as they have short instructions

{Edge Cases}

**Edge Case 1: Insufficient Memory**
*   **config.txt:** max-overall-mem 128, min-mem-per-proc 256, max-mem-per-proc 256
*   **Sequence:** Run `initialize` then `screen -s proc1` to create a process.
*   **Expected Output:** The system should reject the process creation or gracefully handle the out-of-memory error without crashing.

**Edge Case 2: Invalid Scheduler**
*   **config.txt:** scheduler invalid_scheduler
*   **Sequence:** Start the emulator and run `initialize`.
*   **Expected Output:** The emulator should fall back to a default scheduler (like FCFS) or exit gracefully with a descriptive error message.

**Edge Case 3: Zero Quantum Cycles in Round Robin**
*   **config.txt:** scheduler rr, quantum-cycles 0
*   **Sequence:** Start the emulator, run `initialize`, and run a process.
*   **Expected Output:** The system should either set a default quantum size (e.g., 1) or prevent the emulator from starting with an invalid configuration.

**Edge Case 4: Process requiring more memory than maximum system memory**
*   **config.txt:** max-overall-mem 512, min-mem-per-proc 1024
*   **Sequence:** Run `initialize` then `screen -s proc1` to create a process.
*   **Expected Output:** System should reject the process since it requires more memory than the physical limit.
