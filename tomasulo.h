#ifndef TOMASULO_H
#define TOMASULO_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <memory>
#include "instruction.h"

// Declarações antecipadas
class FunctionalUnit;
class ReservationStation;
class LoadBuffer;
class StoreBuffer;

// Estrutura para representar o status de um registrador
struct RegisterStatus {
    bool busy;
    std::string reservationStation;
    RegisterStatus() : busy(false), reservationStation("") {}
};

// Estrutura para representar uma estação de reserva
struct ReservationStation {
    std::string name;
    bool busy;
    InstructionType op;
    double vj, vk;        // Valores dos operandos fonte
    std::string qj, qk;   // Estações de reserva produzindo os operandos fonte
    int address;          // Para instruções de load/store
    int instructionIndex; // Índice da instrução no programa

    ReservationStation(const std::string& n) 
        : name(n), busy(false), op(InstructionType::NOP), 
          vj(0), vk(0), address(0), instructionIndex(-1) {}
};

// Estrutura para representar um buffer de load/store
struct LoadStoreBuffer {
    std::string name;
    bool busy;
    InstructionType op;
    int address;
    std::string qAddr;     // Estação de reserva produzindo o endereço
    std::string qValue;    // Estação de reserva produzindo o valor (para store)
    double v;             // Valor para store
    std::string dest;     // Registrador de destino para load
    int instructionIndex; // Índice da instrução no programa

    LoadStoreBuffer(const std::string& n) 
        : name(n), busy(false), op(InstructionType::NOP), address(0), 
          instructionIndex(-1) {}
};

// Classe para representar uma unidade funcional
class FunctionalUnit {
public:
    std::string name;
    bool busy;
    int executionTime;
    int remainingTime;
    std::shared_ptr<ReservationStation> rs;
    std::shared_ptr<LoadStoreBuffer> buffer;

    FunctionalUnit(const std::string& n, int execTime) 
        : name(n), busy(false), executionTime(execTime), remainingTime(0) {}

    bool isBusy() const { return busy; }
    void setBusy(bool b) { busy = b; }
    void setReservationStation(std::shared_ptr<ReservationStation> r) { rs = r; }
    void setBuffer(std::shared_ptr<LoadStoreBuffer> b) { buffer = b; }
    void execute() { if (remainingTime > 0) remainingTime--; }
    bool isFinished() const { return remainingTime == 0; }
    void startExecution(int time) { remainingTime = executionTime; }
    std::shared_ptr<ReservationStation> getReservationStation() const { return rs; }
    std::shared_ptr<LoadStoreBuffer> getBuffer() const { return buffer; }
};

// Classe principal Tomasulo
class Tomasulo {
private:
    // Estado do programa
    std::vector<Instruction> instructions;
    int currentInstruction;
    int clock;
    int pc;  // Contador de Programa
    bool branchTaken;
    int branchTarget;

    // Componentes de hardware
    std::map<std::string, RegisterStatus> registerStatus;
    std::vector<std::shared_ptr<ReservationStation>> reservationStations;
    std::vector<std::shared_ptr<LoadStoreBuffer>> loadBuffers;
    std::vector<std::shared_ptr<LoadStoreBuffer>> storeBuffers;
    std::vector<std::shared_ptr<FunctionalUnit>> functionalUnits;
    std::map<std::string, double> registers;
    std::map<int, double> memory;

    // Configuração
    const int NUM_ADD_RS = 3;
    const int NUM_MUL_RS = 2;
    const int NUM_LOAD_BUFFERS = 3;
    const int NUM_STORE_BUFFERS = 3;

    // Métodos auxiliares
    void initializeHardware();
    bool issueInstruction();
    void execute();
    void writeResult();
    void updateReservationStations(const std::string& source, double value);
    int getExecutionTime(InstructionType type);
    std::string getReservationStationName(InstructionType type);
    bool isLoadStore(InstructionType type);
    bool isBranch(InstructionType type);
    bool isJump(InstructionType type);
    bool areRegistersReady(const Instruction& inst);
    void handleBranch(const Instruction& inst);
    void handleJump(const Instruction& inst);
    void handleLoadStore(const Instruction& inst);
    void updatePC();
    void flushPipeline();

public:
    Tomasulo(const std::vector<Instruction>& insts);
    void run();
    void printState();
};

#endif // TOMASULO_H 