#include "tomasulo.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

Tomasulo::Tomasulo(const std::vector<Instruction>& insts) 
    : instructions(insts), currentInstruction(0), clock(0), pc(0),
      branchTaken(false), branchTarget(0) {
    initializeHardware();
    // Inicializar todos os registradores usados nas instruções
    for (const Instruction& inst : instructions) {
        if (!inst.getDest().empty()) registers[inst.getDest()] = 0;
        if (!inst.getSrc1().empty()) registers[inst.getSrc1()] = 0;
        if (!inst.getSrc2().empty() && !inst.getIsImmediate()) registers[inst.getSrc2()] = 0;
    }
}

void Tomasulo::initializeHardware() {
    // Inicializar estações de reserva
    for (int i = 0; i < NUM_ADD_RS; i++) {
        reservationStations.push_back(std::make_shared<ReservationStation>("Add" + std::to_string(i)));
    }
    for (int i = 0; i < NUM_MUL_RS; i++) {
        reservationStations.push_back(std::make_shared<ReservationStation>("Mult" + std::to_string(i)));
    }

    // Inicializar buffers de load/store
    for (int i = 0; i < NUM_LOAD_BUFFERS; i++) {
        loadBuffers.push_back(std::make_shared<LoadStoreBuffer>("Load" + std::to_string(i)));
    }
    for (int i = 0; i < NUM_STORE_BUFFERS; i++) {
        storeBuffers.push_back(std::make_shared<LoadStoreBuffer>("Store" + std::to_string(i)));
    }

    // Inicializar unidades funcionais
    functionalUnits.push_back(std::make_shared<FunctionalUnit>("Add1", 2));
    functionalUnits.push_back(std::make_shared<FunctionalUnit>("Add2", 2));
    functionalUnits.push_back(std::make_shared<FunctionalUnit>("Mult1", 10));
    functionalUnits.push_back(std::make_shared<FunctionalUnit>("Mult2", 10));
    functionalUnits.push_back(std::make_shared<FunctionalUnit>("Load", 2));
    functionalUnits.push_back(std::make_shared<FunctionalUnit>("Store", 2));
}

bool Tomasulo::issueInstruction() {
    if (currentInstruction >= instructions.size()) return false;

    const Instruction& inst = instructions[currentInstruction];
    std::string rsName = getReservationStationName(inst.getType());
    
    // Encontrar estação de reserva disponível
    std::shared_ptr<ReservationStation> rs = nullptr;
    for (auto& station : reservationStations) {
        if (!station->busy && station->name.find(rsName) != std::string::npos) {
            rs = station;
            break;
        }
    }

    if (!rs) {
        std::cout << "[DEBUG] Nenhuma estação de reserva disponível para " << rsName << " na instrução " << currentInstruction << std::endl;
        return false; // Nenhuma estação de reserva disponível
    }

    std::cout << "[DEBUG] Emitindo instrução " << currentInstruction << ": ";
    inst.print();

    // Emitir a instrução
    rs->busy = true;
    rs->op = inst.getType();
    rs->instructionIndex = currentInstruction;

    // Tratar operandos fonte
    if (!inst.getSrc1().empty()) {
        if (registerStatus[inst.getSrc1()].busy) {
            rs->qj = registerStatus[inst.getSrc1()].reservationStation;
        } else {
            rs->vj = registers[inst.getSrc1()];
            rs->qj = "";
        }
    }

    if (!inst.getSrc2().empty() && !inst.getIsImmediate()) {
        if (registerStatus[inst.getSrc2()].busy) {
            rs->qk = registerStatus[inst.getSrc2()].reservationStation;
        } else {
            rs->vk = registers[inst.getSrc2()];
            rs->qk = "";
        }
    }

    if (inst.getIsImmediate()) {
        rs->vk = inst.getImmediate();
        rs->qk = "";
    }

    // Tratar registrador de destino
    if (!inst.getDest().empty()) {
        registerStatus[inst.getDest()].busy = true;
        registerStatus[inst.getDest()].reservationStation = rs->name;
    }

    // Tratar endereço de memória para load/store
    if (isLoadStore(inst.getType())) {
        rs->address = inst.getAddress();
    }

    currentInstruction++;
    return true;
}

// Função auxiliar para saber se todas as estações e unidades funcionais estão livres
template <typename T>
bool allFree(const std::vector<std::shared_ptr<T>>& vec) {
    for (const std::shared_ptr<T>& item : vec) {
        if (item->busy) return false;
    }
    return true;
} 

void Tomasulo::execute() {
    // Executar instruções nas unidades funcionais
    for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
        if (fu->isBusy()) {
            fu->execute();
        }
    }

    // Iniciar execução para instruções prontas
    for (auto& rs : reservationStations) {
        if (rs->busy && rs->qj.empty() && rs->qk.empty()) {
            // Verificar se esta estação de reserva já está executando em alguma unidade funcional
            bool alreadyExecuting = false;
            for (const std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
                if (fu->isBusy() && fu->getReservationStation() == rs) {
                    alreadyExecuting = true;
                    break;
                }
            }
            
            // Só iniciar execução se não estiver já executando
            if (!alreadyExecuting) {
                // Encontrar unidade funcional disponível
                for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
                    if (!fu->isBusy() && 
                        ((rs->name.find("Add") != std::string::npos && fu->name.find("Add") != std::string::npos) ||
                         (rs->name.find("Mult") != std::string::npos && fu->name.find("Mult") != std::string::npos))) {
                        fu->setBusy(true);
                        fu->setReservationStation(rs);
                        fu->startExecution(getExecutionTime(rs->op));
                        break;
                    }
                }
            }
        }
    }

    // Verificar buffers de store prontos
    for (std::shared_ptr<LoadStoreBuffer>& buffer : storeBuffers) {
        if (buffer->busy && buffer->qAddr.empty() && buffer->qValue.empty()) {
            // Verificar se este buffer já está executando
            bool alreadyExecuting = false;
            for (const std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
                if (fu->isBusy() && fu->getBuffer() == buffer) {
                    alreadyExecuting = true;
                    break;
                }
            }
            
            // Só iniciar execução se não estiver já executando
            if (!alreadyExecuting) {
                // Encontrar unidade funcional disponível para store
                for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
                    if (!fu->isBusy() && fu->name == "Store") {
                        fu->setBusy(true);
                        fu->setBuffer(buffer);
                        fu->startExecution(getExecutionTime(buffer->op));
                        std::cout << "[DEBUG] Store iniciado: " << buffer->name << " -> Memory[" << buffer->address << "]" << std::endl;
                        break;
                    }
                }
            }
        }
    }
}

void Tomasulo::writeResult() {
    // Tratar resultados das unidades funcionais
    for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
        if (fu->isBusy() && fu->isFinished()) {
            std::shared_ptr<ReservationStation> rs = fu->getReservationStation();
            if (rs) {
                double result = 0;
                switch (rs->op) {
                    case InstructionType::ADD:
                        result = rs->vj + rs->vk;
                        break;
                    case InstructionType::SUB:
                        result = rs->vj - rs->vk;
                        break;
                    case InstructionType::MUL:
                        result = rs->vj * rs->vk;
                        break;
                    case InstructionType::DIV:
                        if (rs->vk != 0) {
                            result = rs->vj / rs->vk;
                        } else {
                            result = 0;  // Divisão por zero retorna 0
                        }
                        break;
                    case InstructionType::ADDI:
                        result = rs->vj + rs->vk;
                        break;
                    case InstructionType::SUBI:
                        result = rs->vj - rs->vk;
                        break;
                    case InstructionType::MULI:
                        result = rs->vj * rs->vk;
                        break;
                    case InstructionType::DIVI:
                        if (rs->vk != 0) {
                            result = rs->vj / rs->vk;
                        } else {
                            result = 0;  // Divisão por zero retorna 0
                        }
                        break;
                    default:
                        break;
                }

                // Atualizar registradores e estações de reserva
                for (std::pair<const std::string, RegisterStatus>& reg : registerStatus) {
                    if (reg.second.reservationStation == rs->name) {
                        registers[reg.first] = result;
                        reg.second.busy = false;
                        reg.second.reservationStation = "";
                        std::cout << "[DEBUG] Atualizando registrador " << reg.first << " = " << result << std::endl;
                    }
                }

                updateReservationStations(rs->name, result);
                rs->busy = false;
                fu->setBusy(false);
            }
        }
    }

    // Tratar resultados dos buffers de load/store
    for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
        if (fu->isBusy() && fu->isFinished()) {
            std::shared_ptr<LoadStoreBuffer> buffer = fu->getBuffer();
            if (buffer) {
                if (buffer->op == InstructionType::LW) {
                    // Load está pronto para executar
                    double value = memory[buffer->address];
                    registers[buffer->dest] = value;
                    registerStatus[buffer->dest].busy = false;
                    registerStatus[buffer->dest].reservationStation = "";
                    updateReservationStations(buffer->name, value);
                    std::cout << "[DEBUG] Load completado: " << buffer->dest << " = " << value << std::endl;
                } else if (buffer->op == InstructionType::SW) {
                    // Store está pronto para executar
                    memory[buffer->address] = buffer->v;
                    std::cout << "[DEBUG] Store completado: Memory[" << buffer->address << "] = " << buffer->v << std::endl;
                }
                buffer->busy = false;
                fu->setBusy(false);
            }
        }
    }
}

void Tomasulo::updateReservationStations(const std::string& source, double value) {
    std::cout << "[DEBUG] Forwarding valor " << value << " de " << source << std::endl;
    for (std::shared_ptr<ReservationStation>& rs : reservationStations) {
        if (rs->busy) {
            if (rs->qj == source) {
                rs->vj = value;
                rs->qj = "";
                std::cout << "[DEBUG] Forwarding para " << rs->name << ".qj = " << value << std::endl;
            }
            if (rs->qk == source) {
                rs->vk = value;
                rs->qk = "";
                std::cout << "[DEBUG] Forwarding para " << rs->name << ".qk = " << value << std::endl;
            }
        }
    }

    // Atualizar buffers de load/store
    for (std::shared_ptr<LoadStoreBuffer>& buffer : loadBuffers) {
        if (buffer->busy && buffer->qAddr == source) {
            buffer->address += value;  // Atualiza o endereço com o valor do registrador base
            buffer->qAddr = "";
            std::cout << "[DEBUG] Forwarding para " << buffer->name << ".qAddr = " << value << std::endl;
        }
    }
    for (std::shared_ptr<LoadStoreBuffer>& buffer : storeBuffers) {
        if (buffer->busy) {
            if (buffer->qAddr == source) {
                buffer->address += value;  // Atualiza o endereço com o valor do registrador base
                buffer->qAddr = "";
                std::cout << "[DEBUG] Forwarding para " << buffer->name << ".qAddr = " << value << std::endl;
            }
            if (buffer->qValue == source) {
                buffer->v = value;  // Atualiza o valor a ser armazenado
                buffer->qValue = "";
                std::cout << "[DEBUG] Forwarding para " << buffer->name << ".qValue = " << value << std::endl;
            }
        }
    }
}

int Tomasulo::getExecutionTime(InstructionType type) {
    switch (type) {
        case InstructionType::ADD:
        case InstructionType::SUB:
        case InstructionType::ADDI:
        case InstructionType::SUBI:
            return 2;
        case InstructionType::MUL:
        case InstructionType::DIV:
        case InstructionType::MULI:
        case InstructionType::DIVI:
            return 10;
        case InstructionType::LW:
        case InstructionType::SW:
            return 2;
        default:
            return 1;
    }
}

std::string Tomasulo::getReservationStationName(InstructionType type) {
    switch (type) {
        case InstructionType::ADD:
        case InstructionType::SUB:
        case InstructionType::ADDI:
        case InstructionType::SUBI:
            return "Add";
        case InstructionType::MUL:
        case InstructionType::DIV:
        case InstructionType::MULI:
        case InstructionType::DIVI:
            return "Mult";
        case InstructionType::LW:
            return "Load";
        case InstructionType::SW:
            return "Store";
        default:
            return "";
    }
}

bool Tomasulo::isLoadStore(InstructionType type) {
    return type == InstructionType::LW || type == InstructionType::SW;
}

bool Tomasulo::isBranch(InstructionType type) {
    return type == InstructionType::BEQ || type == InstructionType::BNE;
}

bool Tomasulo::isJump(InstructionType type) {
    return type == InstructionType::J || type == InstructionType::JAL || type == InstructionType::JR;
}

bool Tomasulo::areRegistersReady(const Instruction& inst) {
    return !registerStatus[inst.getSrc1()].busy && 
           !registerStatus[inst.getSrc2()].busy;
}

void Tomasulo::handleBranch(const Instruction& inst) {
    // Verificar se os registradores estão ocupados
    if (registerStatus[inst.getSrc1()].busy || registerStatus[inst.getSrc2()].busy) {
        std::cout << "[DEBUG] Branch aguardando registradores: " << inst.getSrc1() << " ou " << inst.getSrc2() << std::endl;
        return; // Aguardar registradores ficarem prontos
    }
    
    double val1 = registers[inst.getSrc1()];
    double val2 = registers[inst.getSrc2()];
    bool taken = false;

    std::cout << "[DEBUG] Avaliando branch: " << inst.getSrc1() << "=" << val1 << ", " << inst.getSrc2() << "=" << val2 << std::endl;

    switch (inst.getType()) {
        case InstructionType::BEQ:
            taken = (val1 == val2);
            break;
        case InstructionType::BNE:
            taken = (val1 != val2);
            break;
        default:
            return;
    }

    if (taken) {
        branchTaken = true;
        // Calcular destino do branch relativo à próxima instrução (estilo MIPS)
        branchTarget = pc + 1 + inst.getImmediate();
        std::cout << "[DEBUG] Branch tomado: PC = " << pc << " -> " << branchTarget << std::endl;
        std::cout << "[DEBUG] Flushando pipeline..." << std::endl;
        flushPipeline();
        currentInstruction = branchTarget; // Atualizar currentInstruction para o destino do branch
    } else {
        std::cout << "[DEBUG] Branch não tomado: PC = " << pc << " -> " << (pc + 1) << std::endl;
        currentInstruction = pc + 1; // Mover para próxima instrução
    }
}

void Tomasulo::handleJump(const Instruction& inst) {
    switch (inst.getType()) {
        case InstructionType::J:
            // Jump simples para endereço imediato
            branchTaken = true;
            branchTarget = inst.getImmediate();
            std::cout << "[DEBUG] Jump para endereço " << branchTarget << std::endl;
            break;
            
        case InstructionType::JAL:
            // Salvar endereço de retorno em $ra e fazer jump
            registers["$ra"] = pc + 1;  // Salvar endereço de retorno
            registerStatus["$ra"].busy = false;  // Marcar $ra como não ocupado
            branchTaken = true;
            branchTarget = inst.getImmediate();
            std::cout << "[DEBUG] Jump and Link: $ra = " << (pc + 1) << ", jump para " << branchTarget << std::endl;
            break;
            
        case InstructionType::JR:
            // Jump para endereço no registrador
            if (registerStatus[inst.getSrc1()].busy) {
                std::cout << "[DEBUG] Jump Register aguardando registrador " << inst.getSrc1() << std::endl;
                return;  // Não pode executar jump ainda
            }
            branchTaken = true;
            branchTarget = static_cast<int>(registers[inst.getSrc1()]);
            std::cout << "[DEBUG] Jump Register para endereço em " << inst.getSrc1() << " = " << branchTarget << std::endl;
            break;
            
        default:
            return;
    }

    if (branchTaken) {
        std::cout << "[DEBUG] Flushando pipeline..." << std::endl;
        flushPipeline();
    }
    currentInstruction++;
}

void Tomasulo::handleLoadStore(const Instruction& inst) {
    if (inst.getType() == InstructionType::LW) {
        // Encontrar buffer de load disponível
        for (std::shared_ptr<LoadStoreBuffer>& buffer : loadBuffers) {
            if (!buffer->busy) {
                buffer->busy = true;
                buffer->op = inst.getType();
                buffer->dest = inst.getDest();
                buffer->address = inst.getAddress();
                buffer->instructionIndex = currentInstruction;
                
                // Verificar se o registrador base está pronto
                if (registerStatus[inst.getSrc1()].busy) {
                    buffer->qAddr = registerStatus[inst.getSrc1()].reservationStation;
                    std::cout << "[DEBUG] Load aguardando registrador " << inst.getSrc1() << " em " << buffer->qAddr << std::endl;
                } else {
                    buffer->address += registers[inst.getSrc1()];
                    buffer->qAddr = "";  // Sem dependência
                    std::cout << "[DEBUG] Load pronto para executar: " << buffer->name << " -> " << buffer->dest << " (addr=" << buffer->address << ")" << std::endl;
                }
                
                registerStatus[inst.getDest()].busy = true;
                registerStatus[inst.getDest()].reservationStation = buffer->name;
                
                // Encontrar unidade funcional disponível para load
                for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
                    if (!fu->isBusy() && fu->name == "Load") {
                        fu->setBusy(true);
                        fu->setBuffer(buffer);
                        fu->startExecution(getExecutionTime(inst.getType()));
                        std::cout << "[DEBUG] Load iniciado: " << buffer->name << " -> " << buffer->dest << std::endl;
                        currentInstruction++;
                        return;
                    }
                }
                std::cout << "[DEBUG] Nenhuma unidade funcional de load disponível" << std::endl;
                return;
            }
        }
        std::cout << "[DEBUG] Nenhum buffer de load disponível" << std::endl;
    } else if (inst.getType() == InstructionType::SW) {
        // Encontrar buffer de store disponível
        for (std::shared_ptr<LoadStoreBuffer>& buffer : storeBuffers) {
            if (!buffer->busy) {
                buffer->busy = true;
                buffer->op = inst.getType();
                buffer->address = inst.getAddress();
                buffer->instructionIndex = currentInstruction;
                
                // Verificar se o registrador base está pronto
                if (registerStatus[inst.getSrc1()].busy) {
                    buffer->qAddr = registerStatus[inst.getSrc1()].reservationStation;
                    std::cout << "[DEBUG] Store aguardando registrador " << inst.getSrc1() << " em " << buffer->qAddr << std::endl;
                } else {
                    buffer->address += registers[inst.getSrc1()];
                    buffer->qAddr = "";  // Sem dependência
                }
                
                // Verificar se o registrador de valor está pronto
                if (registerStatus[inst.getDest()].busy) {
                    buffer->qValue = registerStatus[inst.getDest()].reservationStation;
                    std::cout << "[DEBUG] Store aguardando valor em " << buffer->qValue << std::endl;
                } else {
                    buffer->v = registers[inst.getDest()];
                    std::cout << "[DEBUG] Store pronto para executar: " << buffer->name << " -> Memory[" << buffer->address << "] = " << buffer->v << std::endl;
                }
                
                // Encontrar unidade funcional disponível para store
                for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
                    if (!fu->isBusy() && fu->name == "Store") {
                        fu->setBusy(true);
                        fu->setBuffer(buffer);
                        fu->startExecution(getExecutionTime(inst.getType()));
                        std::cout << "[DEBUG] Store iniciado: " << buffer->name << " -> Memory[" << buffer->address << "]" << std::endl;
                        currentInstruction++;
                        return;
                    }
                }
                std::cout << "[DEBUG] Nenhuma unidade funcional de store disponível" << std::endl;
                return;
            }
        }
        std::cout << "[DEBUG] Nenhum buffer de store disponível" << std::endl;
    }
}

void Tomasulo::updatePC() {
    if (branchTaken) {
        std::cout << "[DEBUG] Atualizando PC: " << pc << " -> " << branchTarget << std::endl;
        pc = branchTarget;
        currentInstruction = pc;  // Sincroniza currentInstruction com pc
        branchTaken = false;
        flushPipeline();
    } else {
        pc++;
    }
}

void Tomasulo::flushPipeline() {
    // Limpar todas as estações de reserva
    for (auto& rs : reservationStations) {
        if (rs->instructionIndex >= pc) {
            rs->busy = false;
        }
    }
    
    // Limpar todos os buffers de load/store
    for (std::shared_ptr<LoadStoreBuffer>& buffer : loadBuffers) {
        if (buffer->instructionIndex >= pc) {
            buffer->busy = false;
        }
    }
    for (std::shared_ptr<LoadStoreBuffer>& buffer : storeBuffers) {
        if (buffer->instructionIndex >= pc) {
            buffer->busy = false;
        }
    }
    
    // Limpar todas as unidades funcionais
    for (std::shared_ptr<FunctionalUnit>& fu : functionalUnits) {
        if (fu->isBusy()) {
            auto rs = fu->getReservationStation();
            auto buffer = fu->getBuffer();
            if ((rs && rs->instructionIndex >= pc) || 
                (buffer && buffer->instructionIndex >= pc)) {
                fu->setBusy(false);
            }
        }
    }
    
    // Resetar status dos registradores para registradores que estavam sendo escritos por instruções descartadas
    for (std::pair<const std::string, RegisterStatus>& reg : registerStatus) {
        if (reg.second.busy) {
            auto rs = std::find_if(reservationStations.begin(), reservationStations.end(),
                [&](const std::shared_ptr<ReservationStation>& rs) {
                    return rs->name == reg.second.reservationStation;
                });
            if (rs != reservationStations.end() && (*rs)->instructionIndex >= pc) {
                reg.second.busy = false;
                reg.second.reservationStation = "";
            }
        }
    }
}

void Tomasulo::run() {
    int maxCycles = 1000; // segurança para evitar loop infinito
    while ((pc < instructions.size()) ||
           !allFree(reservationStations) ||
           !allFree(functionalUnits)) {
        if (clock > maxCycles) {
            std::cerr << "\n[ERRO] Loop infinito detectado. Abortando após " << maxCycles << " ciclos.\n";
            break;
        }
        std::cout << "\nClock Cycle: " << clock << std::endl;
        printState();

        writeResult();
        execute();
        
        if (pc < instructions.size()) {
            const Instruction& inst = instructions[pc];
            std::cout << "\n[DEBUG] Processando instrução " << pc << ": ";
            inst.print();
            
            bool instructionIssued = false;
            if (isBranch(inst.getType())) {
                // Para branches, precisamos aguardar que todas as instruções anteriores sejam completadas
                bool canIssue = true;
                for (const auto& rs : reservationStations) {
                    if (rs->busy && rs->instructionIndex < pc) {
                        canIssue = false;
                        break;
                    }
                }
                
                if (canIssue && areRegistersReady(inst)) {
                    handleBranch(inst);
                    instructionIssued = true;
                }
            } else if (isJump(inst.getType())) {
                handleJump(inst);
                instructionIssued = true;
            } else if (isLoadStore(inst.getType())) {
                handleLoadStore(inst);
                instructionIssued = true;
            } else {
                instructionIssued = issueInstruction();
            }
            
            if (instructionIssued) {
                updatePC();
            }
        }

        clock++;
    }
    std::cout << "\nExecução finalizada em " << clock << " ciclos.\n";
    printState();
}

void Tomasulo::printState() {
    std::cout << "\n=== Estado do Simulador ===" << std::endl;
    std::cout << "Ciclo: " << clock << std::endl;
    std::cout << "PC: " << pc << std::endl;
    
    std::cout << "\n--- Estações de Reserva ---" << std::endl;
    for (const auto& rs : reservationStations) {
        std::cout << rs->name << ": ";
        std::cout << "Busy=" << rs->busy << " ";
        if (rs->busy) {
            std::cout << "Op=" << static_cast<int>(rs->op) << " ";
            std::cout << "Vj=" << rs->vj << " ";
            std::cout << "Vk=" << rs->vk << " ";
            std::cout << "Qj=" << (rs->qj.empty() ? "-" : rs->qj) << " ";
            std::cout << "Qk=" << (rs->qk.empty() ? "-" : rs->qk) << " ";
            std::cout << "A=" << rs->address;
        }
        std::cout << std::endl;
    }

    std::cout << "\n--- Buffers de Load ---" << std::endl;
    for (const auto& buffer : loadBuffers) {
        std::cout << buffer->name << ": ";
        std::cout << "Busy=" << buffer->busy << " ";
        if (buffer->busy) {
            std::cout << "Op=" << static_cast<int>(buffer->op) << " ";
            std::cout << "Address=" << buffer->address << " ";
            std::cout << "QAddr=" << (buffer->qAddr.empty() ? "-" : buffer->qAddr) << " ";
            std::cout << "Dest=" << buffer->dest;
        }
        std::cout << std::endl;
    }

    std::cout << "\n--- Buffers de Store ---" << std::endl;
    for (const auto& buffer : storeBuffers) {
        std::cout << buffer->name << ": ";
        std::cout << "Busy=" << buffer->busy << " ";
        if (buffer->busy) {
            std::cout << "Op=" << static_cast<int>(buffer->op) << " ";
            std::cout << "Address=" << buffer->address << " ";
            std::cout << "QAddr=" << (buffer->qAddr.empty() ? "-" : buffer->qAddr) << " ";
            std::cout << "QValue=" << (buffer->qValue.empty() ? "-" : buffer->qValue) << " ";
            std::cout << "V=" << buffer->v;
        }
        std::cout << std::endl;
    }

    std::cout << "\n--- Unidades Funcionais ---" << std::endl;
    for (const auto& fu : functionalUnits) {
        std::cout << fu->name << ": ";
        std::cout << "Busy=" << fu->busy << " ";
        if (fu->busy) {
            std::cout << "Remaining=" << fu->remainingTime;
        }
        std::cout << std::endl;
    }

    std::cout << "\n--- Status dos Registradores ---" << std::endl;
    for (const auto& reg : registerStatus) {
        if (reg.second.busy) {
            std::cout << reg.first << ": ";
            std::cout << "Busy=" << reg.second.busy << " ";
            std::cout << "Qi=" << reg.second.reservationStation << std::endl;
        }
    }

    std::cout << "\n--- Valores dos Registradores ---" << std::endl;
    for (const auto& reg : registers) {
        std::cout << reg.first << "=" << reg.second << " ";
    }
    std::cout << std::endl;

    std::cout << "\n--- Memória ---" << std::endl;
    for (const auto& mem : memory) {
        std::cout << "Memory[" << mem.first << "]=" << mem.second << " ";
    }
    std::cout << std::endl;
}

