import subprocess
import time
import sys
import os

# Windows CreateProcess will not accept a forward-slash relative path even when
# it exists, so normalize to an absolute native path before launching.
EXE = os.path.abspath(os.path.join('build', 'src', 'os_emulator.exe'))

def write_config(config_text):
    with open('config.txt', 'w') as f:
        f.write(config_text)

def run_test(name, config_text, commands_with_delays):
    print(f"--- Running {name} ---")
    write_config(config_text)
    
    # Start process
    p = subprocess.Popen([EXE], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    output = []
    
    # helper to write to stdin
    def send_cmd(cmd):
        if p.poll() is None:
            p.stdin.write(cmd + "\n")
            p.stdin.flush()
    
    # Every command except `initialize` is refused until the system is initialized,
    # so without this the tests only ever exercised the initialize gate.
    send_cmd("initialize")
    time.sleep(1)

    for cmd, delay in commands_with_delays:
        if p.poll() is not None:
            break
        send_cmd(cmd)
        time.sleep(delay)
        
    send_cmd("exit")
    
    out, err = p.communicate()
    with open(f'{name}_out.txt', 'w') as f:
        f.write(out)
        if err:
            f.write("\nSTDERR:\n" + err)
            
    print(f"{name} completed. Output saved to {name}_out.txt")


# Test 1
config1 = """num-cpu 2
scheduler "fcfs"
quantum-cycles 0
batch-process-freq 1
min-ins 4000
max-ins 4000
delays-per-exec 0
max-overall-mem 512
mem-per-frame 256
min-mem-per-proc 512
max-mem-per-proc 512
"""

cmds1 = [
    ("screen", 5),
    ("screen -ls", 1),
    ("process-smi", 1),
    ("vmstat", 1)
]

run_test("test1", config1, cmds1)

# Test 2
config2 = """num-cpu 1
scheduler "rr"
quantum-cycles 10
batch-process-freq 1
min-ins 1000
max-ins 1000
delays-per-exec 0
max-overall-mem 256
mem-per-frame 256
min-mem-per-proc 256
max-mem-per-proc 256
"""

cmds2 = [
    ('screen -c faulty_process "DECLARE varA 10; DECLARE varB 5; ADD varA varA varB; WRITE 0x500 varA; READ varC 0x500; PRINT(\\"Variable A\\" + varA); PRINT(\\"Result: \\" + varC)"', 3),
    ("screen -ls", 1),
    ("screen -r faulty_process", 1),
]

run_test("test2", config2, cmds2)

# Edge Case 1: Insufficient Memory
config_edge1 = """max-overall-mem 128
min-mem-per-proc 256
max-mem-per-proc 256
"""
cmds_edge1 = [
    ("screen", 1)
]
run_test("edge1", config_edge1, cmds_edge1)

# Edge Case 2: Invalid Scheduler
config_edge2 = """scheduler "invalid_scheduler"
"""
cmds_edge2 = [
    ("screen", 1)
]
run_test("edge2", config_edge2, cmds_edge2)

# Edge Case 3: Zero Quantum Cycles
config_edge3 = """scheduler "rr"
quantum-cycles 0
"""
cmds_edge3 = [
    ("screen", 1)
]
run_test("edge3", config_edge3, cmds_edge3)

# Edge Case 4: Process > System Memory
config_edge4 = """max-overall-mem 512
min-mem-per-proc 1024
max-mem-per-proc 1024
"""
cmds_edge4 = [
    ("screen", 1)
]
run_test("edge4", config_edge4, cmds_edge4)

print("All automated tests completed.")
