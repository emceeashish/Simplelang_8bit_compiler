#!/usr/bin/env python3

import re
import sys

# Check for correct usage
if len(sys.argv) < 3:
    print("Usage: python asm.py <input_file> <output_file>")
    sys.exit(1)

# Input assembly file
input_file = sys.argv[1]
output_file = sys.argv[2]

# Instruction set mapping
inst = {
    "nop": 0x00,
    "call": 0x01,
    "ret": 0x02,
    "lda": 0x87,
    "out": 0x03,
    "in": 0x04,
    "hlt": 0x05,
    "cmp": 0x06,
    "sta": 0xB8,
    "jmp": 0x18,
    "jz": 0x19,
    "jnz": 0x1A,
    "je": 0x19,
    "jne": 0x1A,
    "jc": 0x1B,
    "jnc": 0x1C,
    "push": 0x20,
    "pop": 0x28,
    "add": 0x40,
    "sub": 0x48,
    "inc": 0x50,
    "dec": 0x58,
    "and": 0x60,
    "or": 0x68,
    "xor": 0x70,
    "adc": 0x78,
    "ldi": 0x10,
    "mov": 0x80,
}

# Register mappings
reg = {
    "A": 0x0,
    "B": 0x1,
    "C": 0x2,
    "D": 0x3,
    "E": 0x4,
    "F": 0x5,
    "G": 0x6,
    "M": 0x7,
}

# Constants
MEM_SIZE = 256

# Initialize memory and variables
mem = [0 for _ in range(MEM_SIZE)]
cnt = 0

labels = {}
data = {}
data_addr = {}

# Helper to handle numbers in various formats
def rich_int(v):
    if v.startswith("0x") or v.startswith("0X"):
        return int(v, 16)
    elif v.startswith("0b") or v.startswith("0B"):
        return int(v, 2)
    else:
        return int(v)

# Read and process the assembly file
with open(input_file) as f:
    section = 'TEXT'  # Default to TEXT section
    for l in f:
        # Remove comments and whitespace
        l = re.sub(";.*", "", l).strip()
        if not l:
            continue

        # Handle sections
        if l.lower() == ".text":
            section = 'TEXT'
            continue
        elif l.lower() == ".data":
            section = 'DATA'
            continue

        if section == 'DATA':
            # Expecting lines like: var = value
            n, v = map(str.strip, l.split("=", 1))
            data[n] = rich_int(v)
        elif section == 'TEXT':
            # Expecting instructions or labels
            kw = l.split()
            if kw[0].endswith(":"):
                # Label definition
                labels[kw[0].rstrip(":")] = cnt
            else:
                current_inst = kw[0].lower()
                if current_inst == "ldi":
                    reg_name = kw[1].rstrip(",")
                    value = kw[2]
                    r = reg[reg_name]
                    mem[cnt] = (inst[current_inst] & 0b11111000) | r
                    cnt += 1
                    mem[cnt] = rich_int(value)
                    cnt += 1
                elif current_inst in ("push", "pop"):
                    reg_name = kw[1]
                    r = reg[reg_name]
                    mem[cnt] = (inst[current_inst] & 0b11111000) | r
                    cnt += 1
                elif current_inst == "cmp":
                    if len(kw) != 3:
                        print("Error: 'cmp' instruction requires two operands (e.g., cmp A B).")
                        sys.exit(1)
                    
                    reg1 = kw[1]
                    reg2 = kw[2]

                    if reg1 not in reg or reg2 not in reg:
                        print(f"Error: Invalid operands in cmp instruction: {reg1}, {reg2}")
                        sys.exit(1)

                    mem[cnt] = inst["cmp"]
                    cnt += 1
                    mem[cnt] = (reg[reg1] << 4) | reg[reg2]  # Encode registers into memory
                    cnt += 1
                elif current_inst == "mov":
                    dest = kw[1].rstrip(",")
                    src = kw[2].rstrip(",")
                    addr = kw[3]
                    op1 = reg[dest]
                    op2 = reg[src]
                    mem[cnt] = (inst[current_inst] & 0b11111000) | op2
                    mem[cnt] = (mem[cnt] & 0b11000111) | (op1 << 3)
                    cnt += 1
                    mem[cnt] = rich_int(addr)
                    cnt += 1
                elif current_inst in ("add", "sub"):
                    operand = kw[1]
                    mem[cnt] = inst[current_inst]
                    cnt += 1
                    mem[cnt] = rich_int(operand) if operand not in reg else reg[operand]
                    cnt += 1
                elif current_inst in ("jmp", "jz", "jnz", "je", "jne", "jc", "jnc"):
                    label = kw[1]
                    mem[cnt] = inst[current_inst]
                    cnt += 1
                    mem[cnt] = label  # Store label (resolve later)
                    cnt += 1
                elif current_inst == "hlt":
                    mem[cnt] = inst[current_inst]
                    cnt += 1
                else:
                    print(f"Error: Unknown instruction '{current_inst}'")
                    sys.exit(1)

# Resolve labels in memory
for i in range(MEM_SIZE):
    if isinstance(mem[i], str):
        label = mem[i]
        if label in labels:
            mem[i] = labels[label]
        else:
            print(f"Error: Undefined label '{label}' used in instructions.")
            sys.exit(1)

# Write data into memory after instructions
for k, v in data.items():
    data_addr[k] = cnt
    mem[cnt] = v
    cnt += 1

data_addr.update(labels)

# Replace variables with their memory addresses
for i in range(MEM_SIZE):
    if isinstance(mem[i], str) and mem[i].startswith("%"):
        var = mem[i].lstrip("%")
        if var in data_addr:
            mem[i] = data_addr[var]
        else:
            print(f"Error: Undefined variable '{var}' used in instructions.")
            sys.exit(1)

# Write the memory to the output file as hex values
with open(output_file, "w") as out:
    for value in mem:
        if isinstance(value, int):  # Ensure only valid numbers are written
            out.write(f"{value:02X}\n")  # Write each hex value on a new line
