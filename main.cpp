#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "instruction.h"
#include "tomasulo.h"

// Função para ler instruções do arquivo
std::vector<Instruction> readInstructionsFromFile(const std::string& filename) {
    std::vector<Instruction> instructions;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return instructions;
    }

    while (std::getline(file, line)) {
        // Pular linhas vazias e comentários
        if (line.empty() || line[0] == '#') continue;
        
        // Remover comentários da linha
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Remover espaços em branco
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (!line.empty()) {
            instructions.push_back(Instruction::parseInstruction(line));
        }
    }

    file.close();
    return instructions;
}

int main() {
    // Ler instruções do arquivo
    std::vector<Instruction> instructions = readInstructionsFromFile("instructions.txt");
    
    if (instructions.empty()) {
        std::cerr << "No instructions found in file!" << std::endl;
        return 1;
    }

    // Imprimir as instruções que serão executadas
    std::cout << "Instructions to be executed:" << std::endl;
    for (const Instruction& inst : instructions) {
        inst.print();
    }
    std::cout << std::endl;

    // Criar e executar o simulador Tomasulo
    Tomasulo simulator(instructions);
    simulator.run();

    return 0;
}
