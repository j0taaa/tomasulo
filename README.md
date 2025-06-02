# Simulador do Algoritmo de Tomasulo

## Índice
1. [Visão Geral](#visão-geral)
2. [O Algoritmo de Tomasulo](#o-algoritmo-de-tomasulo)
3. [Características do Simulador](#características-do-simulador)
4. [Arquitetura do Simulador](#arquitetura-do-simulador)
5. [Conjunto de Instruções](#conjunto-de-instruções)
6. [Formato de Entrada](#formato-de-entrada)
7. [Compilação e Execução](#compilação-e-execução)
8. [Saída do Simulador](#saída-do-simulador)
9. [Detalhes da Implementação](#detalhes-da-implementação)
10. [Exemplos de Uso](#exemplos-de-uso)
11. [Referências](#referências)

### Visão Geral
Este projeto implementa um simulador do Algoritmo de Tomasulo em C++. O Algoritmo de Tomasulo é uma técnica de escalonamento dinâmico que permite a execução fora de ordem das instruções, melhorando o desempenho da CPU ao reduzir os atrasos no pipeline. O simulador foi desenvolvido como parte de um projeto acadêmico para demonstrar os conceitos de superescalaridade e execução fora de ordem em processadores modernos.

### O Algoritmo de Tomasulo
O Algoritmo de Tomasulo foi desenvolvido por Robert Tomasulo na IBM em 1967 para o IBM System/360 Model 91. Suas principais características são:

1. **Renomeação de Registradores**
   - Elimina hazards de dados (WAR e WAW)
   - Permite execução fora de ordem
   - Utiliza um número maior de registradores virtuais

2. **Estações de Reserva**
   - Armazenam instruções e seus operandos
   - Permitem que instruções independentes executem em paralelo
   - Mantêm o estado de dependências entre instruções

3. **Common Data Bus (CDB)**
   - Permite broadcast de resultados
   - Reduz a necessidade de sincronização
   - Facilita o forwarding de dados

4. **Buffers de Load/Store**
   - Gerenciam acessos à memória
   - Permitem execução fora de ordem de operações de memória
   - Reduzem stalls no pipeline

### Características do Simulador
O simulador implementa todas as características principais do Algoritmo de Tomasulo:

1. **Escalonamento Dinâmico**
   - Emissão de instruções fora de ordem
   - Execução quando operandos estão disponíveis
   - Commit de resultados na ordem correta

2. **Renomeação de Registradores**
   - Implementação completa do mecanismo de renomeação
   - Suporte a hazards de dados
   - Gerenciamento de dependências

3. **Common Data Bus (CDB)**
   - Broadcast de resultados para todas as estações
   - Atualização imediata de valores
   - Redução de stalls no pipeline

4. **Unidades Funcionais Múltiplas**
   - Paralelismo de instruções
   - Diferentes latências por tipo de operação
   - Gerenciamento de recursos

5. **Buffers de Load/Store**
   - Gerenciamento de acessos à memória
   - Suporte a operações de memória fora de ordem
   - Prevenção de hazards de memória

6. **Predição de Desvio**
   - Suporte a instruções de branch
   - Flush do pipeline quando necessário
   - Atualização do PC

7. **Simulação Detalhada**
   - Visualização ciclo a ciclo
   - Estado completo do processador
   - Rastreamento de dependências

### Arquitetura do Simulador

#### Componentes de Hardware
1. **Estações de Reserva**
   - **Add/Sub**: 2 estações
     - Latência: 2 ciclos
     - Suporte a ADD, SUB, ADDI, SUBI
   - **Mult/Div**: 2 estações
     - Latência: 10 ciclos
     - Suporte a MUL, DIV, MULI, DIVI

2. **Buffers de Load/Store**
   - **Load**: 2 buffers
     - Latência: 2 ciclos
     - Suporte a LW
   - **Store**: 2 buffers
     - Latência: 2 ciclos
     - Suporte a SW

3. **Unidades Funcionais**
   - **Add/Sub**: 2 unidades
     - Operações aritméticas básicas
     - Latência de 2 ciclos
   - **Mult/Div**: 2 unidades
     - Operações de multiplicação e divisão
     - Latência de 10 ciclos
   - **Load/Store**: 2 unidades
     - Operações de memória
     - Latência de 2 ciclos

#### Estrutura de Dados
1. **Estações de Reserva**
   ```cpp
   struct ReservationStation {
       bool busy;
       std::string op;
       double vj, vk;
       std::string qj, qk;
       int address;
   };
   ```

2. **Buffers de Load/Store**
   ```cpp
   struct LoadStoreBuffer {
       bool busy;
       std::string op;
       int address;
       std::string q;
       double v;
   };
   ```

3. **Unidades Funcionais**
   ```cpp
   struct FunctionalUnit {
       bool busy;
       int remainingTime;
       std::shared_ptr<ReservationStation> rs;
   };
   ```

### Conjunto de Instruções
O simulador suporta um conjunto completo de instruções RISC:

1. **Instruções Aritméticas**
   - `ADD Rd, Rs, Rt`: Rd = Rs + Rt
   - `SUB Rd, Rs, Rt`: Rd = Rs - Rt
   - `MUL Rd, Rs, Rt`: Rd = Rs * Rt
   - `DIV Rd, Rs, Rt`: Rd = Rs / Rt

2. **Instruções Imediatas**
   - `ADDI Rd, Rs, imm`: Rd = Rs + imm
   - `SUBI Rd, Rs, imm`: Rd = Rs - imm
   - `MULI Rd, Rs, imm`: Rd = Rs * imm
   - `DIVI Rd, Rs, imm`: Rd = Rs / imm

3. **Instruções de Memória**
   - `LW Rd, offset(Rs)`: Rd = Memory[Rs + offset]
   - `SW Rd, offset(Rs)`: Memory[Rs + offset] = Rd

4. **Instruções de Desvio**
   - `BEQ Rs, Rt, offset`: if (Rs == Rt) PC += offset
   - `BNE Rs, Rt, offset`: if (Rs != Rt) PC += offset

### Formato de Entrada
O arquivo de entrada deve seguir o formato abaixo:

```
OP DEST SRC1 SRC2/IMM
```

Exemplos:
```
# Operações aritméticas
ADD R1 R2 R3    # R1 = R2 + R3
SUB R4 R5 R6    # R4 = R5 - R6
MUL R7 R8 R9    # R7 = R8 * R9
DIV R10 R11 R12 # R10 = R11 / R12

# Operações imediatas
ADDI R1 R2 10   # R1 = R2 + 10
SUBI R3 R4 5    # R3 = R4 - 5
MULI R5 R6 2    # R5 = R6 * 2
DIVI R7 R8 4    # R7 = R8 / 4

# Operações de memória
LW R1 R2 0      # R1 = Memory[R2 + 0]
SW R3 R4 8      # Memory[R4 + 8] = R3

# Operações de desvio
BEQ R1 R2 4     # if (R1 == R2) PC += 4
BNE R3 R4 -8    # if (R3 != R4) PC -= 8
```

### Compilação e Execução
1. **Requisitos**
   - Compilador C++11 ou superior
   - Sistema operacional Unix-like (Linux/MacOS)

2. **Compilação**
   ```bash
   g++ -std=c++11 main.cpp tomasulo.cpp -o tomasulo && ./tomasulo > output.txt
   ```
   ou para windows
   ```bash
    g++ -std=c++11 main.cpp tomasulo.cpp -o tomasulo; if ($?) { ./tomasulo > output.txt }
   ```
3. **Execução**
   ```bash
   ./tomasulo input.txt
   ```

### Saída do Simulador
O simulador fornece uma saída detalhada para cada ciclo de clock:

1. **Estado do Pipeline**
   - Instrução atual
   - Estágio de execução
   - Dependências ativas

2. **Estações de Reserva**
   - Status (ocupada/livre)
   - Operação
   - Valores dos operandos
   - Dependências

3. **Buffers de Load/Store**
   - Status
   - Endereço
   - Dados
   - Dependências

4. **Unidades Funcionais**
   - Status
   - Tempo restante
   - Instrução em execução

5. **Registradores**
   - Valores atuais
   - Status (renomeado/não renomeado)
   - Dependências

6. **Memória**
   - Valores armazenados
   - Endereços acessados

### Detalhes da Implementação

1. **Estrutura do Código**
   - Classes principais:
     - `Tomasulo`: Classe principal do simulador
     - `Instruction`: Representação de instruções
     - `ReservationStation`: Estações de reserva
     - `LoadStoreBuffer`: Buffers de memória
     - `FunctionalUnit`: Unidades funcionais

2. **Mecanismos Implementados**
   - Renomeação de registradores
   - Escalonamento dinâmico
   - Forwarding de dados
   - Predição de desvio
   - Gerenciamento de hazards

3. **Otimizações**
   - Uso de estruturas de dados eficientes
   - Minimização de cópias
   - Gerenciamento de memória otimizado

### Exemplos de Uso

1. **Exemplo Básico**
   ```
   ADD R1 R2 R3
   MUL R4 R1 R5
   ADD R6 R4 R7
   ```
   Este exemplo demonstra:
   - Dependência de dados
   - Renomeação de registradores
   - Execução fora de ordem

2. **Exemplo com Memória**
   ```
   LW R1 R2 0
   ADD R3 R1 R4
   SW R3 R5 8
   ```
   Este exemplo demonstra:
   - Operações de memória
   - Dependências de memória
   - Buffers de load/store

3. **Exemplo com Desvio**
   ```
   BEQ R1 R2 4
   ADD R3 R4 R5
   SUB R6 R7 R8
   ```
   Este exemplo demonstra:
   - Predição de desvio
   - Flush do pipeline
   - Execução especulativa

### Referências
1. Hennessy, J. L., & Patterson, D. A. (2017). Computer Architecture: A Quantitative Approach (6th ed.). Morgan Kaufmann.
2. Tomasulo, R. M. (1967). An Efficient Algorithm for Exploiting Multiple Arithmetic Units. IBM Journal of Research and Development.
3. Smith, J. E., & Sohi, G. S. (1995). The Microarchitecture of Superscalar Processors. Proceedings of the IEEE. 