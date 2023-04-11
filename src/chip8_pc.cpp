#include "chip8_pc.h"
#include <QPoint>
#include <QBrush>
#include <QSize>
#include <QFileDialog>
#include <QKeyEvent>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <bitset>
#include <algorithm>
#include <type_traits>
#include <stdexcept>

#include <fmt/core.h>

namespace {
    /**********************
     ****** Constants *****
     **********************/

    // Memory locations
    const DByte_t user_memory_base = 0x0200;

    // Stack pointer base address
    const DByte_t stack_pointer_base = 0x0080 - 1;

    // Toggle to enable logging
    const bool debug = true;

    /**********************
     ******** Fonts *******
     **********************/

    static const std::vector<std::vector<Byte_t>> fonts {
        {0xF0,0x90,0x90,0x90,0xF0}, // 0
        {0xF0,0x10,0xF0,0x80,0xF0}, // 1
        {0x20,0x60,0x20,0x20,0x70}, // 2
        {0xF0,0x10,0xF0,0x10,0xF0}, // 3
        {0x90,0x90,0xF0,0x10,0x10}, // 4
        {0xF0,0x80,0xF0,0x10,0xF0}, // 5
        {0xF0,0x80,0xF0,0x90,0xF0}, // 6
        {0xF0,0x10,0x20,0x40,0x40}, // 7
        {0xF0,0x90,0xF0,0x90,0xF0}, // 8
        {0xF0,0x90,0xF0,0x10,0xF0}, // 9
        {0xF0,0x90,0xF0,0x90,0x90}, // A
        {0xE0,0x90,0xE0,0x90,0xE0}, // B
        {0xF0,0x80,0x80,0x80,0xF0}, // C
        {0xE0,0x90,0x90,0x90,0xE0}, // D
        {0xF0,0x80,0xF0,0x80,0xF0}, // E
        {0xF0,0x80,0xF0,0x80,0x80}  // F
    };

    /**********************
     ** Helper functions **
     **********************/

    /**
      * @brief  logging method. It is controlled by global variable debug
      * @param  const char* fmt
      *         Args... args
      *         (Global) debug -> true: logging is enabled
      *                        -> false: logging is disabled
      * @retval None
      */

    template<class... Args>
    void LOG(const char* fmt, Args... args) {
        if constexpr(debug) {
            fmt::print(fmt, args...);
        }
    }

    /**
      * @brief  Return the double-byte opcode at memory[address].
      *         The opcode is printed in hex.
      * @param  Memory
      *         Adress of the opcode
      * @retval the opcode
      */

    DByte_t getOpcode(std::vector<Byte_t>& memory, DByte_t address)
    {
        DByte_t opcode = 0;
        opcode = memory.at(address);
        opcode = opcode << 8;
        opcode = opcode + memory.at(address + 1);
        return opcode;
    }

    /**
      * @brief  Print the double-byte opcode at memory[address].
      *         The opcode is printed in hex.
      * @param  Opcode
      * @retval Nothing
      */
    void printOpcode(DByte_t opcode)
    {
        //std::cout << "opcode x" << std::hex << std::setfill('0') << std::setw(4) << (int) opcode << '\n';
        LOG("Executing {:#06x}: ", opcode);
    }

    /**
      * @brief  Convert a word to BCD format
      * @param  A word
      * @retval The word in BCD format
      */
    std::vector<Byte_t> convertToBcd(Byte_t num)
    {
        std::vector<Byte_t> result(3, 0);
        for(int i = 0; i < 3; ++i)
        {
            result.at(i) = num % 10;
            num /= 10;
        }
        return result;
    }

    /**
      * @brief  Extract a subsequence of hex values, between two indices,
      *         from a sequence of hex values
      * @param  A sequence of hex values
      *
      * @retval The subsequence
      */
    template<typename T>
    struct number_of_bits {
        static constexpr size_t value = sizeof(T) * 8;
    };

    template<typename T>
    T extractSubsequence(T value, size_t start, size_t len) {
        static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
                      "Integral required");
        auto isInBounds = [](size_t value, size_t lower_bound, size_t upper_bound) {
            return value >= lower_bound && value < upper_bound;
        };

        size_t max_len = sizeof(T) * 2;
        size_t end = start + len - 1;
        if(!isInBounds(start, 0, max_len) || !isInBounds(end, 0, max_len)) {
            throw std::invalid_argument("extractSubsequence : out of bound");
        }

        size_t left_shift = (start * number_of_bits<Byte_t>::value / 2);
        size_t right_shift = (max_len - 1 - end) * number_of_bits<Byte_t>::value / 2;

        value &= (static_cast<T>(~0) >> left_shift);
        value >>= right_shift;
        return value;
    }
}

CHIP8_PC::CHIP8_PC(QObject *parent):
    QObject{parent},
    main_process_timer{nullptr},
    last_time{std::chrono::high_resolution_clock::now()},
    delay_timer{0},
    v_registers(16, 0),
    index_register(0),
    program_counter(user_memory_base),
    stack_pointer(stack_pointer_base),
    ram(ram_size, 0),
    frame_buffer{0},
    last_active_key(0),
    interested_keys_statuses(interested_keys.size(), 0)
{
    /* Load fonts to memory */
    auto it = ram.begin();
    for(auto font : fonts) {
        it = std::copy(font.begin(), font.end(), it);
    }
    LOG("Fonts loaded\n");
}

void CHIP8_PC::perform00E0(DByte_t opcode)
{
    printOpcode(opcode);
    clearFrameBuffer();
}

void CHIP8_PC::startCpu()
{
    LOG("Start CPU\n");
    if(!main_process_timer) {
        main_process_timer = new QTimer(this);
        QObject::connect(main_process_timer, &QTimer::timeout, this, &CHIP8_PC::process);
    }
    main_process_timer->start();
}

bool CHIP8_PC::isCpuRunning()
{
    return main_process_timer->isActive();
}

void CHIP8_PC::stopCpu()
{
    LOG("Stop CPU\n");
    if(main_process_timer) {
        main_process_timer->stop();
    }
}

void CHIP8_PC::receiveKeyPressEvent(int pressed_key)
{
    auto it = std::find(interested_keys.begin(), interested_keys.end(), pressed_key);
    if(it == interested_keys.end()) {
        return;
    }
    last_active_key = static_cast<int>(it - interested_keys.begin());
    LOG("Key {} pressed\n", last_active_key);
    interested_keys_statuses[last_active_key] = 1;
}

void CHIP8_PC::receiveKeyReleaseEvent(int pressed_key)
{
    auto it = std::find(interested_keys.begin(), interested_keys.end(), pressed_key);
    if(it == interested_keys.end()) {
        return;
    }
    last_active_key = static_cast<int>(it - interested_keys.begin());
    LOG("Key {} released\n", last_active_key);
    interested_keys_statuses[last_active_key] = 0;
    performFx0A(0x0000);
}

void CHIP8_PC::process()
{
    DByte_t opcode = getOpcode(ram, program_counter);
    emit currentOpcodeChanged(opcode);
    emit programCounterChanged(program_counter);

    program_counter += 2;

    if(opcode == 0x0000)
    {
    }
    else if(opcode == 0x00E0) {
        perform00E0(opcode);
    }
    else if(opcode == 0x00EE) {
        perform00EE(opcode);
    }
    else if((opcode & 0xF000) == 0x1000) {
        perform1nnn(opcode);
    }
    else if((opcode & 0xF000) == 0x2000)
    {
        perform2nnn(opcode);
    }
    else if((opcode & 0xF000) == 0x3000) {
        perform3xkk(opcode);
    }
    else if((opcode & 0xF000) == 0x4000) {
        perform4xkk(opcode);
    }
    else if((opcode & 0xF000) == 0x5000) {
        perform5xy0(opcode);
    }
    else if((opcode & 0xF000) == 0x6000) {
        perform6xkk(opcode);
    }
    else if((opcode & 0xF000) == 0x7000) {
        perform7xkk(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x0)
    {
        perform8xy0(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x1)
    {
        perform8xy1(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x2)
    {
        perform8xy2(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x3)
    {
        perform8xy3(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x4)
    {
        perform8xy4(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x5)
    {
        perform8xy5(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x6)
    {
        perform8xy6(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0x7)
    {
        perform8xy7(opcode);
    }
    else if((opcode & 0xF000) == 0x8000 && (opcode & 0x000F) == 0xE)
    {
        perform8xyE(opcode);
    }
    else if((opcode & 0xF000) == 0x9000)
    {
        perform9xy0(opcode);
    }
    else if((opcode & 0xF000) == 0xa000) {
        performAnnn(opcode);
    }
    else if((opcode & 0xF000) == 0xb000) {
        performBnnn(opcode);
    }
    else if((opcode & 0xF000) == 0xc000) {
        performCxkk(opcode);
    }
    else if((opcode & 0xF000) == 0xd000) {
        performDxyn(opcode);
    }
    else if((opcode & 0xF000) == 0xE000 && (opcode & 0xFF) == 0x9E) {
        performEx9e(opcode);
    }
    else if((opcode & 0xF000) == 0xE000 && (opcode & 0xFF) == 0xA1) {
        performExa1(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x07) {
        performFx07(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x0A) {
        performFx0A(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x15) {
        performFx15(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x1E)
    {
        performFx1E(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x29)
    {
        performFx29(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x33) {
        performFx33(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x55)
    {
        performFx55(opcode);
    }
    else if((opcode & 0xF000) == 0xF000 && (opcode & 0xFF) == 0x65)
    {
        performFx65(opcode);
    }
    else
    {
        LOG("Unknow instruction {:#06x}\n" , opcode);
        abort();
    }

    auto duration = std::chrono::duration_cast
            <std::chrono::duration<DByte_t, std::ratio<1,60>>>
            (std::chrono::high_resolution_clock::now() - last_time);
    if(duration.count() > delay_timer) {
        delay_timer = 0;
    }
    else {
        delay_timer -= duration.count();
    }

    sendSignalToMonitor();

    if(program_counter == ram_size)
    {
        stopCpu();
    }
}

void CHIP8_PC::load()
{
    std::string filepath{QFileDialog::getOpenFileName(nullptr, "Open source file", "~", "CHIP8 sources (*.ch8 *.txt)").toStdString()};
    if(filepath.empty()) return;

    LOG("Load source file {}\n", filepath);
    std::ifstream input(filepath, std::ios::binary);

    std::vector<Byte_t>(ram.size(), 0);
    auto it = ram.begin() + user_memory_base;
    auto end = std::copy(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>(), it);

    /* Clear frambuffer */
    clearFrameBuffer();

    it = ram.begin() + user_memory_base;

    LOG("{} bytes loaded\n", end - it);
    for(;it < end; ++it) {
        DByte_t address = it - ram.begin();
        LOG("[{:#06x}] {:#04x}\n", address, *it);
    }
}

void CHIP8_PC::perform00EE(DByte_t opcode)
{
    printOpcode(opcode);
    uint16_t new_pc_value = ram.at(stack_pointer);
    --stack_pointer;
    new_pc_value = (((uint16_t) ram.at(stack_pointer)) << 8) + new_pc_value;
    --stack_pointer;

    program_counter = new_pc_value;
    if(stack_pointer < stack_pointer_base - 1) {
        LOG("error: stack_pointer decreased below the the base\n");
    }

    LOG("Return from subroutine. Program counter is set to {:#06x}\n", program_counter);
}

void CHIP8_PC::perform6xkk(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    auto new_value = extractSubsequence(opcode, 2, 2);
    setVRegister(register_id, new_value);

    LOG("Set V{:02d} to {:#04x}\n", register_id, new_value);
}

void CHIP8_PC::performAnnn(DByte_t opcode)
{
    printOpcode(opcode);
    auto new_value = extractSubsequence(opcode, 1, 3);

    index_register = new_value;
    emit indexRegisterChanged(static_cast<int>(index_register));

    LOG("Set index register to {:#04x}\n", new_value);
}

void CHIP8_PC::performBnnn(DByte_t opcode)
{
    printOpcode(opcode);
    auto new_value = extractSubsequence(opcode, 1, 3);

    program_counter = new_value;
    program_counter += readVRegister(0x0);

    LOG("Set program counter to {:#06x}", program_counter);
}

void CHIP8_PC::performCxkk(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    auto new_value = extractSubsequence(opcode, 2, 2);

    Byte_t random_value = rand();
    setVRegister(register_id, random_value & new_value);

    LOG("Set V{:02d} to {:#04x}\n", register_id, readVRegister(register_id));
}

void CHIP8_PC::perform3xkk(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    auto value = extractSubsequence(opcode, 2, 2);
    if(readVRegister(register_id) == value) {
        program_counter += 2;
        LOG("Skip next instruction\n");
    }
    else {
        LOG("Do not skip next instruction\n");
    }
}

void CHIP8_PC::perform4xkk(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    auto value = extractSubsequence(opcode, 2, 2);
    if(readVRegister(register_id) != value) {
        program_counter += 2;
        LOG("Skip next instruction\n");
    }
    else {
        LOG("Do not skip next instruction\n");
    }
}

void CHIP8_PC::perform5xy0(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);
    if(readVRegister(vx_id) == readVRegister(vy_id))
    {
        program_counter += 2;
        LOG("Skip next instruction\n");
    }
    else {
        LOG("Do not skip next instruction\n");
    }
}

void CHIP8_PC::performDxyn(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);
    auto n_op = extractSubsequence(opcode, 3, 1);

    int x = readVRegister(vx_id) % 64;
    int y = readVRegister(vy_id) % 32;
    int n = n_op;

    LOG("Draw at coordinate x: {}, y: {}, {} bytes\n", x, y, n);

    for(int i = 0; i < n; ++i)
    {
        Byte_t value = ram.at(index_register + i);

        Byte_t* row = &frame_buffer[y][0];

        unsigned int byte_index = x / 8;
        unsigned int offset = x % 8;

        if(offset == 0)
        {
            row[byte_index] ^= value;
        }
        else
        {
            unsigned char first_half = value >> offset;
            unsigned char second_half = value << (8 - offset);

            row[byte_index] ^= first_half;
            row[(byte_index + 1) % 8] ^= second_half;
        }
        y = (y + 1) % 32;
    }
}

void CHIP8_PC::performEx9e(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    if(interested_keys_statuses[readVRegister(register_id)] == 1) {
        program_counter += 2;
        LOG("Skip the next instruction\n");
    }
    else {
        LOG("Do nothing\n");
    }
}

void CHIP8_PC::performExa1(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    if(interested_keys_statuses[readVRegister(register_id)] == 0) {
        program_counter += 2;
        LOG("Skip the next instruction\n");
    }
    else {
        LOG("Do nothing\n");
    }
}

void CHIP8_PC::performFx07(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    setVRegister(register_id, delay_timer);
    LOG("Set V{} to value of delay timer {:#04x}\n", register_id, delay_timer);
}

void CHIP8_PC::performFx0A(DByte_t opcode)
{
    static DByte_t last_opcode = 0x0000;

    if(opcode == 0x0000 && isCpuRunning()) return;

    if(last_opcode == 0x0000) {
        printOpcode(opcode);
        last_opcode = opcode;
        sendSignalToMonitor(CheckPeriod::No);
        stopCpu();
    }
    else {
        auto register_id = extractSubsequence(last_opcode, 1, 1);
        setVRegister(register_id, static_cast<Byte_t>(last_active_key));
        startCpu();
        last_opcode = 0x0000;
    }
}

void CHIP8_PC::performFx15(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    delay_timer = readVRegister(register_id);
    LOG("Set delay timer to value of V{} {:#04x}\n", register_id, delay_timer);
}

void CHIP8_PC::perform7xkk(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    auto value = extractSubsequence(opcode, 2, 2);
    setVRegister(register_id,
                 readVRegister(register_id) + value);
    LOG("Add {:#04x} to V{}\n", value, register_id);
}

void CHIP8_PC::perform1nnn(DByte_t opcode)
{
    printOpcode(opcode);
    auto value = extractSubsequence(opcode, 1, 3);
    program_counter = value;
    LOG("Jump to {:#04x}\n", value);
}

void CHIP8_PC::perform8xy0(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);
    setVRegister(vx_id, readVRegister(vy_id));
    LOG("Set V{} to the value of V{}\n", vx_id, vy_id);
}

void CHIP8_PC::perform8xy1(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);
    setVRegister(vx_id,
                 readVRegister(vx_id) | readVRegister(vy_id));
    LOG("Set V{} to V{} | V{}\n", vx_id, vx_id, vy_id);
}

void CHIP8_PC::perform8xy2(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);
    setVRegister(vx_id,
                 readVRegister(vx_id) & readVRegister(vy_id));
    LOG("Set V{} to V{} & V{}\n", vx_id, vx_id, vy_id);
}

void CHIP8_PC::perform8xy3(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);
    setVRegister(vx_id,
                 readVRegister(vx_id) ^ readVRegister(vy_id));
    LOG("Set V{} to V{} ^ V{}\n", vx_id, vx_id, vy_id);
}

void CHIP8_PC::perform8xy4(DByte_t opcode)
{
    printOpcode(opcode);

    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);

    DByte_t result = readVRegister(vx_id) + readVRegister(vy_id);
    if(result > 0xFF)
        setVRegister(0xF, 1);
    else
        setVRegister(0xF, 0);
    setVRegister(vx_id, static_cast<Byte_t>(result));
}

void CHIP8_PC::perform8xy5(DByte_t opcode)
{
    printOpcode(opcode);

    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);

    Byte_t vx_value = readVRegister(vx_id);
    Byte_t vy_value = readVRegister(vy_id);

    setVRegister(vx_id, vx_value - vy_value);
    if(vx_value < vy_value)
        setVRegister(0xF, 0x0);
    else
        setVRegister(0xF, 0x1);
}

void CHIP8_PC::perform8xy6(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    setVRegister(0xF, readVRegister(vx_id) & 0x1);
    setVRegister(vx_id, readVRegister(vx_id) >> 1);
}

void CHIP8_PC::perform8xy7(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);

    Byte_t vx_value = readVRegister(vx_id);
    Byte_t vy_value = readVRegister(vy_id);

    setVRegister(vx_id, vy_value - vx_value);
    if(vy_value < vx_value)
        setVRegister(0xF, 0x0);
    else
        setVRegister(0xF, 0x1);
}

void CHIP8_PC::perform8xyE(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    setVRegister(0xF, (readVRegister(vx_id) & 0x80) >> 7);
    setVRegister(vx_id, readVRegister(vx_id) << 1);
}

void CHIP8_PC::perform9xy0(DByte_t opcode)
{
    printOpcode(opcode);
    auto vx_id = extractSubsequence(opcode, 1, 1);
    auto vy_id = extractSubsequence(opcode, 2, 1);

        if(readVRegister(vx_id) != readVRegister(vy_id)) {
            program_counter += 2;
            LOG("Skip the next instruction\n");
        }
        else {
            LOG("Do not skip the next instruction\n");
        }
}

void CHIP8_PC::perform2nnn(DByte_t opcode)
{
    printOpcode(opcode);

    ++stack_pointer;
    ram.at(stack_pointer) = (program_counter & 0x0F00) >> 8;
    ++stack_pointer;
    ram.at(stack_pointer) = (program_counter & 0x00FF);

    program_counter = (opcode & 0x0FFF);

    LOG("Call subroutine at {:#06x}\n", (opcode & 0x0FFF));
}

void CHIP8_PC::performFx1E(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    index_register += v_registers.at(register_id);
    emit indexRegisterChanged(static_cast<int>(index_register));
}

void CHIP8_PC::performFx29(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);
    auto value = readVRegister(register_id);
    index_register = value * std::size(fonts[0]);
}

void CHIP8_PC::performFx33(DByte_t opcode)
{
    printOpcode(opcode);
    auto register_id = extractSubsequence(opcode, 1, 1);

    LOG("Convert {} to BCF format\n", readVRegister(register_id));
    auto num_in_bcd = convertToBcd(readVRegister(register_id));

    for(int i = 0; i < 3; ++i)
    {
        ram.at(index_register + i) = num_in_bcd.at(2 -  i);
        LOG("Set {:#04x} to ram[{:#06x}]\n", num_in_bcd.at(i), index_register + i);
    }
}

void CHIP8_PC::performFx55(DByte_t opcode)
{
    printOpcode(opcode);

    auto upper_register_id = extractSubsequence(opcode, 1, 1);

    for(int i = 0; i <= upper_register_id; ++i)
    {
        ram.at(index_register + i) = readVRegister(i);
    }

    LOG("\n");
}

void CHIP8_PC::performFx65(DByte_t opcode)
{
    printOpcode(opcode);

    auto upper_register_id = extractSubsequence(opcode, 1, 1);

    for(int i = 0; i <= upper_register_id; ++i)
    {
        setVRegister(i, ram.at(index_register + i));
    }

    LOG("\n");
}

void CHIP8_PC::clearFrameBuffer()
{
    std::fill( &frame_buffer[0][0], &frame_buffer[0][0] + sizeof(frame_buffer), 0 );
}

void CHIP8_PC::sendSignalToMonitor(CheckPeriod check_period)
{
    static auto lastUpdated = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdated);
    if(ms.count() > 100 || check_period == CheckPeriod::No) {
        emit frameBufferChanged(&frame_buffer[0][0]);
        lastUpdated = now;
    }
}

void CHIP8_PC::setVRegister(int i, Byte_t value)
{
    v_registers.at(i) = value;
    emit vRegisterChanged(i, value);
}

Byte_t CHIP8_PC::readVRegister(int i)
{
    return v_registers.at(i);
}
