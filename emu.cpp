/*
Name : Vivekananda Katakam
Roll No : 2401CS52
Course : Computer Architecture (CS2206)
Mini-Project : Emulator for SIMPLEX Instruction Set
Language : C++
File : Main Emulator file
Declaration : I confirm that this is my own work

Usage: emu <object_file> [-trace] [-before] [-after] [-dump] [-T <n>]

Options:
-trace : Print each instruction as it executes
-before : Show register state BEFORE each instruction
-after : Show register state AFTER each instruction
-dump : Produce a memory dump after execution
-T <n> : Stop after <n> instructions (default: 1000000), Use this to detect/escape infinte loops
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>

using namespace std;

/*CONSTANTS*/
static const int MEMORY_SIZE = (1 << 24); /*16M words*/
static const int DEFAULT_LIMIT = 100000;  /*infinite-loop guard*/
static const int OPCODE_MASK = 0xFF;      /*lower 8 bits = opcode*/
static const int OPERAND_SHIFT = 8;       /*upper 24 bits = operand*/
static const int OPERAND_BITS = 24;

// Instruction Definition
struct InstructionDef
{
    const char *mnemonic;
    int opcode;
    bool hasOperand;
    bool isBranch;
};

static const InstructionDef InstrTable[] = {
    {"ldc", 0, true, false},
    {"adc", 1, true, false},
    {"ldl", 2, true, false},
    {"stl", 3, true, false},
    {"ldnl", 4, true, false},
    {"stnl", 5, true, false},
    {"add", 6, false, false},
    {"sub", 7, false, false},
    {"shl", 8, false, false},
    {"shr", 9, false, false},
    {"adj", 10, true, false},
    {"a2sp", 11, false, false},
    {"sp2a", 12, false, false},
    {"call", 13, true, true},
    {"return", 14, false, false},
    {"brz", 15, true, true},
    {"brlz", 16, true, true},
    {"br", 17, true, true},
    {"HALT", 18, false, false},
    {nullptr, 0, false, false} /*sentinel*/
};

/*Find an InstructionDef by opcode value - used for disassembly*/
static const InstructionDef *findInstructionByOpcode(int opcode)
{
    for (int i = 0; InstrTable[i].mnemonic != nullptr; i++)
    {
        if (InstrTable[i].opcode == opcode)
        {
            return &InstrTable[i];
        }
    }
    return nullptr;
}

/*CPU Registers*/
struct CPU
{
    int32_t A = 0;
    int32_t B = 0;
    int32_t PC = 0; // word address
    int32_t SP = 0; // word address
};

/*HELPER FUNCTIONS*/

/*Lower 8 bits hold the opcode*/
int decodeOpcode(uint32_t word)
{
    return (int)(word & OPCODE_MASK);
}

/*Upper 24 bits hold the operand*/
int32_t decodeOperand(uint32_t word)
{
    int32_t raw = (int32_t)(word >> OPERAND_SHIFT);
    raw &= 0x00FFFFFF; // only 24 bits
    if (raw & (1 << (OPERAND_BITS - 1)))
    {
        raw |= (int32_t)0xFF000000; // sign extend for negatives
    }
    return raw;
}

/*MEMORY: bounds-checked read/write + object file loader*/

static vector<int32_t> memory;
static ofstream traceFile; /*receives trace lines and memory dump*/

void initMemory()
{
    memory.assign(MEMORY_SIZE, 0);
}

/*Returns false and prints an error message if address is out of range*/
bool checkAddress(int32_t addr, int32_t pc, const string &op)
{
    if (addr < 0 || addr >= MEMORY_SIZE)
    {
        cerr << "RUNTIME ERROR: illegal memory " << op
             << " at address 0x" << hex << (uint32_t)addr
             << " (PC=0x" << (uint32_t)pc << ")\n";
        return false;
    }
    return true;
}

bool memRead(int32_t addr, int32_t pc, int32_t &out)
{
    if (!checkAddress(addr, pc, "read"))
        return false;
    out = memory[addr];
    return true;
}

bool memWrite(int32_t addr, int32_t value, int32_t pc)
{
    if (!checkAddress(addr, pc, "write"))
        return false;
    memory[addr] = value;
    return true;
}

/* Load little-endian 32-bit words from binary object file.
   Returns number of words loaded, or -1 on error.*/
int loadObjectFile(const string &path)
{
    ifstream objFile(path, ios::binary);
    if (!objFile)
    {
        cerr << "error : cannot open object file: " << path << "\n";
        return -1;
    }

    int count = 0;
    unsigned char buf[4];
    while (objFile.read((char *)buf, 4)) // reading 4 bytes at a time and storing in buf
    {
        if (count >= MEMORY_SIZE)
        {
            cerr << "error: object file too large for memory\n";
            return -1;
        }
        uint32_t word = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
        memory[count++] = (int32_t)word;
    }

    /*A failed (non-EOF) read  means the file ended mid-word - it is corrupt*/
    if (!objFile.eof() && objFile.fail()) // eof = end of file
    {
        cerr << "error: object file has a partial trailing word (corrupt?)\n";
        return -1;
    }

    return count;
}

/*DISASSEMBLER: turn one encoded word into a human readable string*/
string disassemble(uint32_t word, int32_t instrPC)
{
    int opcode = decodeOpcode(word);
    int32_t operand = decodeOperand(word);

    const InstructionDef *instr = findInstructionByOpcode(opcode);

    ostringstream oss;
    if (instr == nullptr)
    {
        oss << "??? (opcode=0x" << hex << opcode << ")";
        return oss.str();
    }

    oss << left << setw(8) << instr->mnemonic; // left is used for alignment and setw for setting width

    if (instr->hasOperand)
    {
        if (instr->isBranch)
        {
            /*Show raw offset AND the resolved absolute target for readability*/
            int32_t target = instrPC + 1 + operand;
            oss << dec << operand << " ; -> 0x" << hex << (uint32_t)target;
        }
        else
        {
            oss << dec << operand;
        }
    }
    return oss.str();
}

/*Format register state as a compact string*/
string formatRegisters(const CPU &cpu)
{
    ostringstream oss;
    oss << "A=0x" << hex << setw(8) << setfill('0') << (uint32_t)cpu.A
        << " B=0x" << setw(8) << setfill('0') << (uint32_t)cpu.B
        << " PC=0x" << setw(8) << setfill('0') << (uint32_t)cpu.PC
        << " SP=0x" << setw(8) << setfill('0') << (uint32_t)cpu.SP
        << dec << setfill(' ');
    return oss.str();
}

/* PRINT REGISTER STATE */
void printRegisters(const CPU &cpu, const string &label)
{
    ostringstream oss;
    oss << label << " " << formatRegisters(cpu) << "\n";
    cout << oss.str();
    if (traceFile.is_open())
        traceFile << oss.str();
}

/*
 * Emit one combined trace line:
 *   BEFORE: <regs>  |  PC=0x.. <mnemonic>  |  AFTER: <regs>
 * Any of the three sections can be omitted depending on flags.
 */
void emitTraceLine(const CPU &before, const CPU &after,
                   uint32_t word, int32_t instrPC,
                   bool showBefore, bool showTrace, bool showAfter)
{
    ostringstream oss;

    if (showBefore)
    {
        oss << "BEFORE: " << formatRegisters(before);
    }

    if (showTrace)
    {
        if (showBefore)
            oss << "  |  ";
        oss << "PC=0x" << hex << setw(8) << setfill('0') << (uint32_t)instrPC
            << dec << setfill(' ') << "  " << disassemble(word, instrPC);
    }

    if (showAfter)
    {
        if (showBefore || showTrace)
            oss << "  |  ";
        oss << "AFTER: " << formatRegisters(after);
    }

    if (showBefore || showTrace || showAfter)
    {
        oss << "\n\n"; /* blank line separator between instructions */
        cout << oss.str();
        if (traceFile.is_open())
            traceFile << oss.str();
    }
}

/* MEMORY DUMP */
void dumpMemory(int words)
{
    ostringstream oss;
    oss << "\n====================== Memory Dump ==========================\n";
    int dumpingSize = max(words, 4096);

    // Header row
    oss << string(6, ' ')
        << setw(12) << right << "(+0)"
        << setw(12) << right << "(+1)"
        << setw(12) << right << "(+2)"
        << setw(12) << right << "(+3)" << "\n";

    for (int i = 0; i < dumpingSize && i < MEMORY_SIZE; i += 4)
    {
        oss << "0x" << hex << setw(8) << setfill('0') << (uint32_t)i << setfill(' ');

        for (int j = 0; j < 4 && (i + j) < dumpingSize && (i + j) < MEMORY_SIZE; j++)
        {
            oss << "   " << hex << setw(8) << setfill('0') << (uint32_t)memory[i + j] << setfill(' ');
        }

        oss << dec << "\n";
    }

    cout << oss.str();
    if (traceFile.is_open())
        traceFile << oss.str();
}

/*MAIN EMULATION LOOP*/
int runEmulator(int loadedWords, bool optTrace, bool optBefore,
                bool optAfter, bool optDump, int traceLimit)
{
    CPU cpu;
    int steps = 0;
    bool halted = false;

    /* Print column header if any per-instruction output is requested */
    if (optBefore || optTrace || optAfter)
    {
        ostringstream hdr;
        hdr << string(80, '-') << "\n";
        if (optBefore)
            hdr << "BEFORE: A B PC SP";
        if (optBefore && (optTrace || optAfter))
            hdr << "  |  ";
        if (optTrace)
            hdr << "INSTRUCTION";
        if (optTrace && optAfter)
            hdr << "  |  ";
        if (optAfter)
            hdr << "AFTER: A B PC SP";
        hdr << "\n"
            << string(80, '-') << "\n\n";
        cout << hdr.str();
        if (traceFile.is_open())
            traceFile << hdr.str();
    }

    while (!halted)
    {
        /* Infinite loop guard*/
        if (steps >= traceLimit)
        {
            cerr << "WARNING: instruction limit (" << dec << traceLimit
                 << ") reached - possible infinite loop. Stopping.\n";

            if (traceFile.is_open())
                traceFile << "WARNING: instruction limit (" << dec << traceLimit
                          << ") reached - possible infinite loop. Stopping.\n";
            break;
        }

        /* Fetch: PC must be within the loaded program*/
        if (cpu.PC < 0 || cpu.PC >= loadedWords)
        {
            cerr << "RUNTIME ERROR: PC=0x" << hex << (uint32_t)cpu.PC
                 << " jumped outside the loaded program (0x0 - 0x"
                 << (uint32_t)(loadedWords - 1) << ")\n";
            return EXIT_FAILURE;
        }

        uint32_t word = (uint32_t)memory[cpu.PC];
        int opcode = decodeOpcode(word);
        int32_t operand = decodeOperand(word);

        /* Snapshot of state BEFORE execution */
        CPU cpuBefore = cpu;
        int32_t instrPC = cpu.PC;

        cpu.PC++;

        bool ok = true;

        switch (opcode)
        {
        case 0: /*ldc - B:=A, A:=value*/
            cpu.B = cpu.A;
            cpu.A = operand;
            break;

        case 1: /*adc - A:=A+value*/
            cpu.A = cpu.A + operand;
            break;

        case 2: /*ldl - B:=A, A:=mem[SP+offset]*/
            cpu.B = cpu.A;
            ok = memRead(cpu.SP + operand, cpu.PC - 1, cpu.A);
            break;

        case 3: /*stl - mem[SP+offset]:=A, A:=B*/
            ok = memWrite(cpu.SP + operand, cpu.A, cpu.PC - 1);
            cpu.A = cpu.B;
            break;

        case 4: /*ldnl - A:=mem[A+offset]*/
            ok = memRead(cpu.A + operand, cpu.PC - 1, cpu.A);
            break;

        case 5: /*stnl - mem[A+offset]:=B*/
            ok = memWrite(cpu.A + operand, cpu.B, cpu.PC - 1);
            break;

        case 6: /*add - A:=B+A*/
            cpu.A = cpu.B + cpu.A;
            break;

        case 7: /*sub - A:=B-A*/
            cpu.A = cpu.B - cpu.A;
            break;

        case 8: /*shl - A:=B<<A*/
            if (cpu.A < 0 || cpu.A > 31)
            {
                cerr << "WARNING: shl shift amount " << dec << cpu.A
                     << " out of range [0,31] at PC=0x"
                     << hex << (uint32_t)(cpu.PC - 1) << "\n";
            }
            cpu.A = cpu.B << (cpu.A & 31);
            break;

        case 9: /*shr - A:=B>>A (logical/unsigned right shift)*/
            if (cpu.A < 0 || cpu.A > 31)
            {
                cerr << "WARNING: shr shift amount " << dec << cpu.A
                     << " out of range [0,31] at PC=0x"
                     << hex << (uint32_t)(cpu.PC - 1) << "\n";
            }
            cpu.A = (int32_t)((uint32_t)cpu.B >> (cpu.A & 31));
            break;

        case 10: /*adj - SP:=SP+value*/
            cpu.SP = cpu.SP + operand;
            break;

        case 11: /*a2sp - SP:=A, A:=B*/
            cpu.SP = cpu.A;
            cpu.A = cpu.B;
            break;

        case 12: /*sp2a - B:=A, A:=SP*/
            cpu.B = cpu.A;
            cpu.A = cpu.SP;
            break;

        case 13: /*call - B:=A, A:=PC, PC:=PC+offset*/
        {
            int32_t retAddr = cpu.PC;
            cpu.B = cpu.A;
            cpu.A = retAddr;
            cpu.PC = retAddr + operand;
            break;
        }

        case 14: /*return - PC:=A, A:=B*/
            cpu.PC = cpu.A;
            cpu.A = cpu.B;
            break;

        case 15: /*brz - if A==0 then PC:=PC+offset*/
            if (cpu.A == 0)
            {
                cpu.PC = cpu.PC + operand;
            }
            break;

        case 16: /*brlz - if A<0 then PC:=PC+offset*/
            if (cpu.A < 0)
            {
                cpu.PC = cpu.PC + operand;
            }
            break;

        case 17: /*br - PC:=PC+offset*/
            cpu.PC = cpu.PC + operand;
            break;

        case 18: /*HALT - stop the emulator*/
            halted = true;
            cout << "HALT reached after " << dec << steps
                 << " instruction(s). (PC was 0x"
                 << hex << (uint32_t)(cpu.PC - 1) << ")\n";
            break;

        default:
            cerr << "RUNTIME ERROR: unknown opcode 0x"
                 << hex << (uint32_t)opcode
                 << " at PC=0x" << (uint32_t)(cpu.PC - 1) << "\n";
            return EXIT_FAILURE;
        }

        if (!ok)
        {
            cerr << "Execution aborted due to memory fault.\n";
            return EXIT_FAILURE;
        }

        /* Emit the combined BEFORE | TRACE | AFTER line */
        emitTraceLine(cpuBefore, cpu, word, instrPC,
                      optBefore, optTrace, optAfter);

        steps++;
    }

    if (optDump)
    {
        dumpMemory(loadedWords);
    }

    cout << dec << steps << " instructions(s) executed. \n";

    cout << "\n ----FINAL REGISTER STATE----\n";
    printRegisters(cpu, "FINAL ");

    return EXIT_SUCCESS;
}

// ARGUMENT PARSING AND ENTRY POINT
void usage(const char *prog)
{
    cerr << "Usage: " << prog
         << " <object_file> [-trace] [-before] [-after] [-dump] [-T <n>]\n"
         << "\n"
         << " -trace   Print each instruction as it executes\n"
         << " -before  Show registers before each instruction\n"
         << " -after   Show registers after each instruction\n"
         << " -dump    Dump memory after execution\n"
         << " -T <n>   Stop after <n> instructions (default: "
         << DEFAULT_LIMIT << ") - catches infinite loops\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    string objPath;
    bool optTrace = false;
    bool optBefore = false;
    bool optAfter = false;
    bool optDump = true;
    int limit = DEFAULT_LIMIT;

    for (int i = 1; i < argc; i++)
    {
        string arg(argv[i]);

        if (arg == "-trace")
            optTrace = true;
        else if (arg == "-before")
            optBefore = true;
        else if (arg == "-after")
            optAfter = true;
        else if (arg == "-dump")
            optDump = true;
        else if (arg == "-T")
        {
            if (i + 1 >= argc)
            {
                cerr << "error: -T requires a numeric argument\n";
                return EXIT_FAILURE;
            }
            limit = atoi(argv[++i]);
            if (limit <= 0)
            {
                cerr << "error: -T value must be positive integer\n";
                return EXIT_FAILURE;
            }
        }
        else if (arg[0] == '-')
        {
            cerr << "error: unknown option '" << arg << "'\n";
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        else
        {
            if (!objPath.empty())
            {
                cerr << "error: multiple input files specified\n";
                return EXIT_FAILURE;
            }
            objPath = arg;
        }
    }

    if (objPath.empty())
    {
        cerr << "error: no object file specified\n";
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    initMemory();

    int loadedWords = loadObjectFile(objPath);
    if (loadedWords < 0)
        return EXIT_FAILURE;

    if (loadedWords == 0)
    {
        cerr << "error: object file is empty\n";
        return EXIT_FAILURE;
    }

    cout << "Loaded " << dec << loadedWords
         << " word(s) from '" << objPath << "'\n";

    /*Open trace file - strip extension and add .trace*/
    {
        string tracePath = objPath;
        size_t dot = tracePath.rfind('.');
        size_t sep = tracePath.rfind('/');
        if (dot != string::npos && (sep == string::npos || dot > sep))
        {
            tracePath = tracePath.substr(0, dot);
        }
        tracePath += ".trace";
        traceFile.open(tracePath);
        if (!traceFile)
        {
            cerr << "warning: cannot open trace file '" << tracePath << "' (continuing without it)\n";
        }
        else
        {
            cout << "Trace file : " << tracePath << "\n";
        }
    }

    return runEmulator(loadedWords, optTrace, optBefore, optAfter, optDump, limit);
}
