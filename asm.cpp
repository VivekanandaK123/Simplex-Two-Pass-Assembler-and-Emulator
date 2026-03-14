/*
Name : Vivekananda Katakam
Roll No : 2401CS52
Course : Computer Architecture (CS2206)
Mini-project : Two Pass Assembler for SIMPLEX Instruction Set
Language : C++
File : Main assembler file
Declaration : I confirm that this is my own work
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <climits>

using namespace std;

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

// Structure defined for listing file
struct ListEntry
{
    int address;
    bool hasWord; /*false for label only lines*/
    uint32_t word;
    string label;    /*label defined on this line (may be empty)*/
    string mnemonic; /*original mnemonic (empty for data)*/
    bool isBranch;
    int32_t operandVal;
    bool hasOperand;
    bool operandWasLabel;
    string operandLabelName;
};

/* Assembler State */
static map<string, int32_t> labels;
static set<string> usedLabels;
static vector<ListEntry> listing;
static vector<uint32_t> objectCode;
static int errorCount = 0;
static int warningCount = 0;
static ofstream logFile;

/* Helper functions */

/* Emitting Error */
void emitError(const string &filename, int lineNo, const string &msg)
{
    string line = filename + ":" + to_string(lineNo) + ": error: " + msg + "\n";
    cerr << line;
    if (logFile.is_open())
        logFile << line;
    ++errorCount;
}

/* Emitting Warning */
void emitWarning(const string &filename, int lineNo, const string &msg)
{
    string line = filename + ":" + to_string(lineNo) + ": warning: " + msg + "\n";
    cerr << line;
    if (logFile.is_open())
        logFile << line;
    ++warningCount;
}

/* Finding mnemonic in table */
const InstructionDef *findInstr(const string &mnem)
{
    for (int i = 0; InstrTable[i].mnemonic != nullptr; i++)
    {
        if (mnem == InstrTable[i].mnemonic)
        {
            return &InstrTable[i];
        }
    }
    return nullptr;
}

/* Checking if label name is valid */
bool isValidLabel(const string &s)
{
    if (s.empty() || !isalpha(s[0]))
    {
        return false;
    }
    for (size_t i = 1; i < s.size(); i++)
    {
        if (!isalnum(s[i]))
        {
            return false;
        }
    }
    return true;
}

/* Stripping trailing whitespaces from a string */
string rtrim(const string &s)
{
    size_t end = s.size();
    while (end > 0 && isspace(s[end - 1]))
    {
        --end;
    }
    return s.substr(0, end);
}

/* Stripping leadinng whitespaces */
string ltrim(const string &s)
{
    size_t start = 0;
    while (start < s.size() && isspace(s[start]))
    {
        start++;
    }
    return s.substr(start);
}

/*Try to parse a number (decimal/hex/octal). Returns true on success. */
bool parseNumber(const string &tok, int32_t &val, const string &filename, int lineNo)
{
    if (tok.empty())
    {
        emitError(filename, lineNo, "missing operand");
        return false;
    }
    errno = 0;
    char *end = nullptr;
    long v = strtol(tok.c_str(), &end, 0);
    if (end == tok.c_str())
    {
        emitError(filename, lineNo, "invalid number '" + tok + "'");
        return false;
    }
    if (*end != '\0')
    {
        emitError(filename, lineNo, "invalid number (trailing garbage): '" + tok + "'");
        return false;
    }
    if (errno == ERANGE || v > INT32_MAX || v < INT32_MIN)
    {
        emitError(filename, lineNo, "number out of range: '" + tok + "'");
        return false;
    }
    val = (int32_t)v;
    return true;
}

/*
Single pass over the source.
pass == 1: collect labels, count PC, no output, ignore undefined labels
pass == 2: resolve labels, fill listing & objectCode
*/
bool runPass(const string &filename, const vector<string> &lines, int pass)
{
    int pc = 0; /* program counter (word address) */

    if (pass == 2)
    {
        listing.clear();
        objectCode.clear();
    }

    for (int lineNo = 1; lineNo <= (int)lines.size(); lineNo++)
    {
        string raw = lines[lineNo - 1];

        /* Stripping comments */
        size_t not_comment = raw.find(";");
        if (not_comment != string::npos)
        {
            raw = raw.substr(0, not_comment);
        }
        raw = rtrim(raw);
        string line = ltrim(raw);

        if (line.empty())
            continue; /* blank or comment-only line*/

        /* --- Extract label definition --- */
        string labelDef;
        size_t colon = line.find(":");
        if (colon != string::npos)
        {
            labelDef = line.substr(0, colon);
            line = ltrim(line.substr(colon + 1));

            /* Validate  label name */
            if (!isValidLabel(labelDef))
            {
                if (pass == 1)
                {
                    emitError(filename, lineNo, "invalid label name: '" + labelDef + "'");
                }
                labelDef.clear();
            }
            else
            {
                if (pass == 1)
                {
                    if (labels.count(labelDef))
                    {
                        emitError(filename, lineNo, "duplicate label: '" + labelDef + "'");
                    }
                    else
                    {
                        labels[labelDef] = pc;
                    }
                }
            }
        }

        istringstream ss(line);
        string mnem;
        ss >> mnem;

        if (mnem.empty())
        {
            /*Label-only line*/
            if (pass == 2 && !labelDef.empty())
            {
                ListEntry e;
                e.address = pc;
                e.hasWord = false;
                e.label = labelDef;
                e.mnemonic = "";
                e.isBranch = false;
                e.operandVal = 0;
                e.hasOperand = false;
                e.operandWasLabel = false;
                e.operandLabelName = "";
                listing.push_back(e);
            }
            continue;
        }

        /* Handle pseudo-instructions */

        /* data <value> */
        if (mnem == "data")
        {
            /*data without a label can never be referenced, hence warning*/
            if (pass == 2 && labelDef.empty())
            {
                emitWarning(filename, lineNo, "data has no label and cannot be referenced");
            }

            string operand; // operand token
            ss >> operand;

            /* Check for extra tokens */
            string extra;
            if ((ss >> extra) && pass == 2)
            {
                emitError(filename, lineNo, "extra text after operand: '" + extra + "'");
            }

            int32_t val = 0;
            bool ok = false;
            if (operand.empty())
            {
                if (pass == 2)
                    emitError(filename, lineNo, "data requires an operand");
            }
            else
            {
                if (pass == 2)
                {
                    ok = parseNumber(operand, val, filename, lineNo);
                }
            }

            if (pass == 2)
            {
                ListEntry e;
                e.address = pc;
                e.hasWord = true;
                e.word = ok ? (uint32_t)val : 0u;
                e.label = labelDef;
                e.mnemonic = "data";
                e.isBranch = false;
                e.operandVal = val;
                e.hasOperand = true;
                e.operandWasLabel = false;
                e.operandLabelName = "";
                listing.push_back(e);
                objectCode.push_back(e.word);
            }
            pc++;
            continue;
        }

        /* SET <value> - sets current label to given value instead of PC*/
        if (mnem == "SET")
        {
            /*SET without label is meaningless*/
            if (pass == 2 && labelDef.empty())
            {
                emitError(filename, lineNo, "SET must have a label");
            }

            string operand;
            ss >> operand;

            string extra;
            if ((ss >> extra) && pass == 2)
            {
                emitError(filename, lineNo, "extra text after operand: '" + extra + "'");
            }

            int32_t val = 0;
            bool ok = false;
            if (operand.empty())
            {
                if (pass == 2)
                    emitError(filename, lineNo, "SET requires an operand");
            }
            else
            {
                if (pass == 1)
                {
                    // parse silently on pass 1, just need the value
                    errno = 0;
                    char *endp = nullptr;
                    long v = strtol(operand.c_str(), &endp, 0);
                    if (endp != operand.c_str() && *endp == '\0' && errno == 0)
                    {
                        val = (int32_t)v;
                        ok = true;
                    }
                }
                else
                {
                    ok = parseNumber(operand, val, filename, lineNo);
                }
            }

            /*PASS 1: override label value*/
            if (ok && pass == 1 && !labelDef.empty())
            {
                /* Override the label value set above to the SET value*/
                labels[labelDef] = val;
            }

            if (pass == 2 && !labelDef.empty())
            {
                ListEntry e;
                e.address = pc;
                e.hasWord = true;
                e.word = (uint32_t)val;
                e.label = labelDef;
                e.mnemonic = "SET";
                e.isBranch = false;
                e.operandVal = val;
                e.hasOperand = true;
                e.operandWasLabel = false;
                e.operandLabelName = "";
                listing.push_back(e);
                // objectCode.push_back(e.word);
            }
            // pc++;
            continue;
        }

        /* Normal Instruction */
        const InstructionDef *instr = findInstr(mnem);
        if (instr == nullptr)
        {
            if (pass == 2)
                emitError(filename, lineNo, "unknown mnemonic: '" + mnem + "'");
            /*Try to keep PC sync by not advancing (we dont know word count)*/
            continue;
        }

        /*Read optional operand token*/
        string operand;
        ss >> operand;

        /*Check for unexpected extra tokens */
        string extra;
        if ((ss >> extra) && pass == 2)
        {
            emitError(filename, lineNo, "extra text after operand: '" + extra + "'");
        }

        /*Validate operand presence - pass 2 only*/
        if (pass == 2)
        {
            if (instr->hasOperand && operand.empty())
            {
                emitError(filename, lineNo, "instruction '" + mnem + "' requires an operand");
            }
            if (!instr->hasOperand && !operand.empty())
            {
                emitError(filename, lineNo, "instruction '" + mnem + "' takes no operand");
            }
        }

        /* Resolve operand */
        int32_t operVal = 0;
        bool operOk = true;
        bool operandWasLabel = false;
        string operandLabelName;
        if (instr->hasOperand && !operand.empty())
        {
            /* Try as a number first */
            errno = 0;
            char *endp = nullptr;
            long tmp = strtol(operand.c_str(), &endp, 0);
            if (endp != operand.c_str() && *endp == '\0' && errno == 0)
            {
                operVal = (int32_t)tmp;
            }
            else
            {
                /* Must be a label*/
                operandWasLabel = true;
                operandLabelName = operand;
                if (!isValidLabel(operand))
                {
                    if (pass == 2)
                        emitError(filename, lineNo, "invalid operand: '" + operand + "'");
                    operOk = false;
                }
                else
                {
                    if (pass == 2)
                        usedLabels.insert(operand);
                    if (labels.count(operand))
                    {
                        int32_t labelVal = labels.at(operand);
                        if (instr->isBranch)
                        {
                            /* displacement = traget - (pc + 1)*/
                            operVal = labelVal - (pc + 1);
                        }
                        else
                        {
                            operVal = labelVal;
                        }
                    }
                    else
                    {
                        if (pass == 2)
                        {
                            emitError(filename, lineNo, "undefined label: '" + operand + "'");
                            operOk = false;
                        }
                        /*pass 1 : ignore undefined labels*/
                    }
                }
            }
        }

        /*Encode instruction : upper 24 bits = operand, lower 8 bits = opcode*/
        uint32_t word = ((uint32_t)(operVal & 0x00FFFFFF) << 8) | (uint32_t)(instr->opcode & 0xFF);

        if (pass == 2)
        {
            ListEntry e;
            e.address = pc;
            e.hasWord = true;
            e.word = operOk ? word : 0u;
            e.label = labelDef;
            e.mnemonic = mnem;
            e.isBranch = instr->isBranch;
            e.operandVal = operVal;
            e.hasOperand = instr->hasOperand;
            e.operandWasLabel = operandWasLabel;
            e.operandLabelName = operandLabelName;
            listing.push_back(e);
            objectCode.push_back(e.word);
        }
        pc++;
    }
    return true;
}

/* Binary obj file*/
bool writeObjectFile(const string &outPath)
{
    ofstream objFile(outPath, ios::binary);
    if (!objFile)
    {
        cerr << "error: cannot open object file: " << outPath << "\n";
        return false;
    }
    for (uint32_t w : objectCode)
    {
        unsigned char buf[4];
        buf[0] = (unsigned char)(w & 0xFF);
        buf[1] = (unsigned char)((w >> 8) & 0xFF);
        buf[2] = (unsigned char)((w >> 16) & 0xFF);
        buf[3] = (unsigned char)((w >> 24) & 0xFF);
        objFile.write((char *)buf, 4);
    }

    return objFile.good();
}

/*Reverse look-up: given an address, find a label name (if any)*/
string labelAtAddress(int32_t addr)
{
    for (const auto &labelPair : labels)
    {
        if (labelPair.second == addr)
        {
            return labelPair.first;
        }
    }
    return "";
}

/*Write listing file*/
void writeListingFile(const string &lstPath)
{
    ofstream listingFile(lstPath);
    if (!listingFile)
    {
        cerr << "error: cannot open listing file: " << lstPath << "\n";
        return;
    }

    for (const ListEntry &e : listing)
    {
        /* If this entry has only label (no word)*/
        if (!e.hasWord)
        {
            if (!e.label.empty())
            {
                /*Format: "XXXXXXXX label:" */
                char buf[64];
                snprintf(buf, sizeof(buf), "%08X            %s:", (unsigned)e.address, e.label.c_str());
                listingFile << buf << "\n";
            }
            continue;
        }

        /*Address*/
        char buf[256];
        int pos = 0;
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%08X %08X", (unsigned)e.address, e.word);

        /* optional label prefix */
        if (!e.label.empty())
        {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "   %s:", e.label.c_str());
        }
        else
        {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "  ");
        }

        /*Mnemonic*/
        if (!e.mnemonic.empty())
        {
            pos += snprintf(buf + pos, sizeof(buf) - pos, " %s", e.mnemonic.c_str());

            /*Operand*/
            if (e.hasOperand)
            {
                if (e.isBranch)
                {
                    if (e.operandWasLabel)
                    {
                        pos += snprintf(buf + pos, sizeof(buf) - pos, " %s", e.operandLabelName.c_str());
                    }
                    else
                    {
                        pos += snprintf(buf + pos, sizeof(buf) - pos, " %d", e.operandVal);
                    }
                }
                else
                {
                    /*For non-branch, try to find a matching label*/
                    if (e.operandWasLabel)
                    {
                        pos += snprintf(buf + pos, sizeof(buf) - pos, " %s", e.operandLabelName.c_str());
                    }
                    else
                    {
                        pos += snprintf(buf + pos, sizeof(buf) - pos, " %d", e.operandVal);
                    }
                }
            }
        }
        listingFile << buf << "\n";
    }
}

/* Warn about unused labels*/
void warnUnusedLabels(const string &filename)
{
    for (const auto &labelPair : labels)
    {
        if (usedLabels.find(labelPair.first) == usedLabels.end())
        {
            string line = filename + ": warning: label '" + labelPair.first + "' is defined but never used\n";
            cerr << line;
            if (logFile.is_open())
                logFile << line;
            warningCount++;
        }
    }
}

/* Derive output filename by replacing/adding extension*/
string replaceExt(const string &path, const string &newExt)
{
    size_t dot = path.rfind('.');
    size_t sep = path.rfind('/');
    if (dot != string::npos && (sep == string::npos || dot > sep))
    {
        return path.substr(0, dot) + newExt;
    }
    return path + newExt;
}

/*MAIN*/
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage : " << argv[0] << " <source.asm>\n";
        return 1;
    }

    string srcPath = argv[1];
    string objPath = replaceExt(srcPath, ".o");
    string lstPath = replaceExt(srcPath, ".lst");
    string logPath = replaceExt(srcPath, ".log");

    /*Opne log file for errors and warnings*/
    logFile.open(logPath);
    if (!logFile)
    {
        cerr << "warning: cannot open log file: " << logPath << " ( continuing wihtout it)\n";
    }

    /*Read entire source file into memory*/
    ifstream source_file(srcPath);
    if (!source_file)
    {
        cerr << "error: cannot open source file: " << srcPath << "\n";
        return 1;
    }

    vector<string> lines;
    {
        string ln;
        while (getline(source_file, ln))
            lines.push_back(ln);
    }
    source_file.close();

    /*Pass 1 : Collect labels*/
    errorCount = 0;
    warningCount = 0;
    labels.clear();
    usedLabels.clear();

    runPass(srcPath, lines, 1);

    /*Pass 2 : generate code*/
    runPass(srcPath, lines, 2);

    warnUnusedLabels(srcPath);

    /*Summary*/

    if (errorCount > 0)
    {
        string summary = to_string(errorCount) + " error(s), " + to_string(warningCount) + " warning(s). No output produced.\n";
        cerr << summary;
        if (logFile.is_open())
            logFile << summary;
        return 1;
    }

    if (warningCount > 0)
    {
        string summary = to_string(warningCount) + " warning(s)\n";
        cerr << summary;
        if (logFile.is_open())
            logFile << summary;
    }

    /*Write outputs*/
    if (!writeObjectFile(objPath))
    {
        cerr << "error: failed to write object file: " << objPath << "\n";
        return 1;
    }

    writeListingFile(lstPath);

    cout << "Assembled " << objectCode.size() << " word(s).\n";
    cout << "Object file : " << objPath << "\n";
    cout << "Listing file : " << lstPath << "\n";
    cout << "Log file : " << logPath << "\n";

    return 0;
}
