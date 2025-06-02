#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <sstream>
#include <iostream>

// Enum para tipos de instrução
enum class InstructionType {
    LW,    // Load Word
    SW,    // Store Word
    ADD,   // Add
    SUB,   // Subtract
    MUL,   // Multiply
    DIV,   // Divide
    ADDI,  // Add Immediate
    SUBI,  // Subtract Immediate
    MULI,  // Multiply Immediate
    DIVI,  // Divide Immediate
    BEQ,   // Branch if Equal
    BNE,   // Branch if Not Equal
    J,     // Jump
    JAL,   // Jump and Link
    JR,    // Jump Register
    NOP    // No Operation
};

// Classe para representar uma instrução MIPS
class Instruction {
private:
    InstructionType type;
    std::string dest;      // Registrador de destino (ou registrador de valor para SW)
    std::string src1;      // Registrador fonte 1
    std::string src2;      // Registrador fonte 2 ou valor imediato
    int immediate;         // Valor imediato para instruções tipo I
    int address;           // Endereço de memória para load/store
    bool isImmediate;      // Flag para indicar se src2 é imediato

public:
    Instruction() {
        type = InstructionType::NOP;
        immediate = 0;
        address = 0;
        isImmediate = false;
    }
    
    // Getters
    InstructionType getType() const { return type; }
    std::string getDest() const { return dest; }  // Retorna registrador de destino ou registrador de valor para SW
    std::string getSrc1() const { return src1; }
    std::string getSrc2() const { return src2; }
    int getImmediate() const { return immediate; }
    int getAddress() const { return address; }
    bool getIsImmediate() const { return isImmediate; }

    // Setters
    void setType(InstructionType t) { type = t; }
    void setDest(const std::string& d) { dest = d; }  // Define registrador de destino ou registrador de valor para SW
    void setSrc1(const std::string& s) { src1 = s; }
    void setSrc2(const std::string& s) { src2 = s; }
    void setImmediate(int i) { immediate = i; }
    void setAddress(int a) { address = a; }
    void setIsImmediate(bool b) { isImmediate = b; }

    // Função para analisar instrução a partir de string
    static Instruction parseInstruction(const std::string& line) {
        Instruction inst;
        std::string typeStr;
        std::istringstream iss(line);
        
        iss >> typeStr;
        
        // Converter string para InstructionType
        if (typeStr == "lw") inst.type = InstructionType::LW;
        else if (typeStr == "sw") inst.type = InstructionType::SW;
        else if (typeStr == "add") inst.type = InstructionType::ADD;
        else if (typeStr == "sub") inst.type = InstructionType::SUB;
        else if (typeStr == "mul") inst.type = InstructionType::MUL;
        else if (typeStr == "div") inst.type = InstructionType::DIV;
        else if (typeStr == "addi") inst.type = InstructionType::ADDI;
        else if (typeStr == "subi") inst.type = InstructionType::SUBI;
        else if (typeStr == "muli") inst.type = InstructionType::MULI;
        else if (typeStr == "divi") inst.type = InstructionType::DIVI;
        else if (typeStr == "beq") inst.type = InstructionType::BEQ;
        else if (typeStr == "bne") inst.type = InstructionType::BNE;
        else if (typeStr == "j") inst.type = InstructionType::J;
        else if (typeStr == "jal") inst.type = InstructionType::JAL;
        else if (typeStr == "jr") inst.type = InstructionType::JR;
        else inst.type = InstructionType::NOP;

        // Analisar operandos baseado no tipo de instrução
        switch (inst.type) {
            case InstructionType::LW:
            case InstructionType::SW: {
                std::string dest, offsetBase;
                iss >> dest >> offsetBase;
                inst.dest = dest;  // Para SW, este é o registrador de valor
                // offsetBase é como 100(r14)
                size_t open = offsetBase.find('(');
                size_t close = offsetBase.find(')');
                if (open != std::string::npos && close != std::string::npos) {
                    inst.address = std::stoi(offsetBase.substr(0, open));
                    inst.src1 = offsetBase.substr(open + 1, close - open - 1);
                }
                break;
            }
            case InstructionType::ADDI:
            case InstructionType::SUBI:
            case InstructionType::MULI:
            case InstructionType::DIVI:
                iss >> inst.dest >> inst.src1 >> inst.immediate;
                inst.isImmediate = true;
                break;
            case InstructionType::BEQ:
            case InstructionType::BNE:
                iss >> inst.src1 >> inst.src2 >> inst.immediate;
                inst.isImmediate = true;
                break;
            case InstructionType::J:
            case InstructionType::JAL:
                iss >> inst.immediate;
                inst.isImmediate = true;
                break;
            case InstructionType::JR:
                iss >> inst.src1;
                break;
            default:
                iss >> inst.dest >> inst.src1 >> inst.src2;
                break;
        }

        return inst;
    }

    // Função para imprimir instrução
    void print() const {
        switch (type) {
            case InstructionType::LW:
                std::cout << "lw " << dest << " " << address << "(" << src1 << ")";
                break;
            case InstructionType::SW:
                std::cout << "sw " << dest << " " << address << "(" << src1 << ")";  // dest é registrador de valor para SW
                break;
            case InstructionType::ADD:
                std::cout << "add " << dest << " " << src1 << " " << src2;
                break;
            case InstructionType::SUB:
                std::cout << "sub " << dest << " " << src1 << " " << src2;
                break;
            case InstructionType::MUL:
                std::cout << "mul " << dest << " " << src1 << " " << src2;
                break;
            case InstructionType::DIV:
                std::cout << "div " << dest << " " << src1 << " " << src2;
                break;
            case InstructionType::ADDI:
                std::cout << "addi " << dest << " " << src1 << " " << immediate;
                break;
            case InstructionType::SUBI:
                std::cout << "subi " << dest << " " << src1 << " " << immediate;
                break;
            case InstructionType::MULI:
                std::cout << "muli " << dest << " " << src1 << " " << immediate;
                break;
            case InstructionType::DIVI:
                std::cout << "divi " << dest << " " << src1 << " " << immediate;
                break;
            case InstructionType::BEQ:
                std::cout << "beq " << src1 << " " << src2 << " " << immediate;
                break;
            case InstructionType::BNE:
                std::cout << "bne " << src1 << " " << src2 << " " << immediate;
                break;
            case InstructionType::J:
                std::cout << "j " << immediate;
                break;
            case InstructionType::JAL:
                std::cout << "jal " << immediate;
                break;
            case InstructionType::JR:
                std::cout << "jr " << src1;
                break;
            default:
                std::cout << "nop";
                break;
        }
        std::cout << std::endl;
    }
};

#endif // INSTRUCTION_H 