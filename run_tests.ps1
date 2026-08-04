function Run-Test {
    param(
        [string]$Name,
        [string]$ConfigContent,
        [array]$Commands
    )
    Write-Host "--- Running $Name ---"
    Set-Content -Path "config.txt" -Value $ConfigContent
    
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "build\src\os_emulator.exe"
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    
    $p = [System.Diagnostics.Process]::Start($psi)
    
    foreach ($cmdTuple in $Commands) {
        $cmd = $cmdTuple[0]
        $delay = $cmdTuple[1]
        
        if ($p.HasExited) {
            break
        }
        $p.StandardInput.WriteLine($cmd)
        if ($delay -gt 0) {
            Start-Sleep -Seconds $delay
        }
    }
    
    if (-not $p.HasExited) {
        $p.StandardInput.WriteLine("exit")
        $p.WaitForExit(2000)
        if (-not $p.HasExited) {
            $p.Kill()
        }
    }
    
    $out = $p.StandardOutput.ReadToEnd()
    $err = $p.StandardError.ReadToEnd()
    
    $outContent = $out
    if ($err) {
        $outContent += "`nSTDERR:`n" + $err
    }
    
    Set-Content -Path "$Name`_out.txt" -Value $outContent
    Write-Host "$Name completed. Output saved."
}

# Test 1
$config1 = @"
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
"@
$cmds1 = @(
    @("initialize", 1),
    @("screen -s process1", 5),
    @("screen -ls", 1),
    @("process-smi", 1),
    @("vmstat", 1)
)
Run-Test "test1" $config1 $cmds1

# Test 2
$config2 = @"
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
"@
$cmds2 = @(
    @("initialize", 1),
    @('screen -c faulty_process 256 "DECLARE varA 10; DECLARE varB 5; ADD varA varA varB; WRITE 0x500 varA; READ varC 0x500; PRINT(\"Variable A\" + varA); PRINT(\"Result: \" + varC)"', 3),
    @("screen -ls", 1),
    @("screen -r faulty_process", 1)
)
Run-Test "test2" $config2 $cmds2

# Test 3: Stress Test
$config3 = @"
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
"@
$cmds3 = [System.Collections.ArrayList]::new()
$cmds3.Add(@("initialize", 1)) > $null
$cmds3.Add(@("scheduler-start", 2)) > $null
for ($i=0; $i -lt 10; $i++) {
    $cmds3.Add(@("process-smi", 1)) > $null
    $cmds3.Add(@("vmstat", 1)) > $null
}
Run-Test "test3" $config3 $cmds3

# Edge Case 1: Insufficient Memory
$config_edge1 = @"
max-overall-mem 128
min-mem-per-proc 256
max-mem-per-proc 256
"@
$cmds_edge1 = @(
    @("initialize", 1),
    @("screen -s proc1", 1)
)
Run-Test "edge1" $config_edge1 $cmds_edge1

# Edge Case 2: Invalid Scheduler
$config_edge2 = @"
scheduler invalid_scheduler
"@
$cmds_edge2 = @(
    @("initialize", 1),
    @("screen -s proc1", 1)
)
Run-Test "edge2" $config_edge2 $cmds_edge2

# Edge Case 3: Zero Quantum Cycles
$config_edge3 = @"
scheduler rr
quantum-cycles 0
"@
$cmds_edge3 = @(
    @("initialize", 1),
    @("screen -s proc1", 1)
)
Run-Test "edge3" $config_edge3 $cmds_edge3

# Edge Case 4: Process > System Memory
$config_edge4 = @"
max-overall-mem 512
min-mem-per-proc 1024
max-mem-per-proc 1024
"@
$cmds_edge4 = @(
    @("initialize", 1),
    @("screen -s proc1", 1)
)
Run-Test "edge4" $config_edge4 $cmds_edge4

Write-Host "All tests completed."
