import subprocess
import argparse
import os
import sys

def build_emulator():
    print("Building OS Emulator...")
    try:
        # Run the build command
        subprocess.run(["cmake", "--build", "build"], check=True, capture_output=True, text=True)
        print("Build successful.")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Build failed:\n{e.stderr}")
        return False
    except FileNotFoundError:
        print("CMake not found. Is it in your PATH?")
        return False

def run_test(input_file, executable_path="build/src/os_emulator.exe"):
    if not os.path.exists(input_file):
        print(f"Input file not found: {input_file}")
        return None, None
    
    # On Windows, executable paths might have backslashes or require .exe extension check
    if not os.path.exists(executable_path):
        print(f"Executable not found: {executable_path}. Did the build succeed?")
        return None, None

    print(f"Running test with input: {input_file}")
    try:
        with open(input_file, 'r') as infile:
            result = subprocess.run(
                [executable_path],
                stdin=infile,
                capture_output=True,
                text=True,
                timeout=10 # Stop if it hangs
            )
            return result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        print("Test run timed out.")
        return "", "Timeout expired"
    except Exception as e:
        print(f"Error running test: {e}")
        return "", str(e)

def main():
    parser = argparse.ArgumentParser(description="OS Emulator QA Harness")
    parser.add_argument("--build", action="store_true", help="Build the emulator before running tests")
    parser.add_argument("--input", "-i", type=str, help="Path to input test file", required=False)
    parser.add_argument("--output", "-o", type=str, help="Path to save output file", required=False)
    parser.add_argument("--expected", "-e", type=str, help="Path to expected output for assertion", required=False)
    
    args = parser.parse_args()

    if args.build:
        if not build_emulator():
            sys.exit(1)

    if args.input:
        stdout, stderr = run_test(args.input)
        if stdout is None and stderr is None:
            sys.exit(1)
            
        if args.output:
            with open(args.output, 'w') as outfile:
                outfile.write(stdout)
            print(f"Output written to {args.output}")
        else:
            print("--- STDOUT ---")
            print(stdout)
            if stderr:
                print("--- STDERR ---")
                print(stderr)
                
        if args.expected:
            if not os.path.exists(args.expected):
                print(f"Expected output file not found: {args.expected}")
                sys.exit(1)
            with open(args.expected, 'r') as expfile:
                expected_content = expfile.read()
                # Basic string comparison for assertion
                if stdout.strip() == expected_content.strip():
                    print("Test PASSED: Output matches expected.")
                else:
                    print("Test FAILED: Output does not match expected.")
                    sys.exit(1)

if __name__ == "__main__":
    main()
