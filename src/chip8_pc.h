#ifndef CHIP8_PC_H
#define CHIP8_PC_H

#include <vector>
#include <QApplication>
#include <QBitmap>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QByteArray>
#include <QTimer>

#include <gtest/gtest_prod.h>

typedef uint8_t Byte_t;
typedef uint16_t DByte_t;

class CHIP8_PC: public QObject
{
    Q_OBJECT

public:
    CHIP8_PC(QObject *parent = nullptr);
    ~CHIP8_PC() = default;
    CHIP8_PC(const CHIP8_PC &) = default;
    unsigned getGraphicsWidth() { return graphics_width; }
    unsigned getGraphicsHeight() { return graphics_height; }

private:
    FRIEND_TEST(BasicTests, TestInstructions);
    /* Instructions processing method */
    void perform00E0(DByte_t opcode);
    void perform00EE(DByte_t opcode);
    void perform1nnn(DByte_t opcode);
    void perform2nnn(DByte_t opcode);
    void perform3xkk(DByte_t opcode);
    void perform4xkk(DByte_t opcode);
    void perform5xy0(DByte_t opcode);
    void perform6xkk(DByte_t opcode);
    void perform7xkk(DByte_t opcode);
    void perform8xy0(DByte_t opcode);
    void perform8xy1(DByte_t opcode);
    void perform8xy2(DByte_t opcode);
    void perform8xy3(DByte_t opcode);
    void perform8xy4(DByte_t opcode);
    void perform8xy5(DByte_t opcode);
    void perform8xy6(DByte_t opcode);
    void perform8xyE(DByte_t opcode);
    void perform9xy0(DByte_t opcode);
    void performAnnn(DByte_t opcode);
    void performBnnn(DByte_t opcode);
    void performCxkk(DByte_t opcode);
    void performDxyn(DByte_t opcode);
    void performFx1E(DByte_t opcode);
    void performFx33(DByte_t opcode);
    void performFx55(DByte_t opcode);
    void performFx65(DByte_t opcode);
    /* frame_buffer method */
    void clearFrameBuffer();
    void sendSignalToMonitor();
    /* Testing methods */

signals:
    void frameBufferChanged(uint8_t* buffer);
    void indexRegisterChanged(int value);
    void programCounterChanged(int value);
    void currentOpcodeChanged(int value);
    void vRegisterChanged(int i, int value);

public slots:
    void load();
    void startCPU();
    void stopCPU();

private slots:
    void process();

private:
    /* Timers */
    QTimer* main_process_timer;

    /* 8-bit registes V0-VF */
    const unsigned number_of_8bit_registers = 16;
    std::vector<Byte_t> v_registers;
    DByte_t index_register;
    DByte_t program_counter;

    /* Stack pointer */
    Byte_t stack_pointer;

    /* Memory */
    const unsigned ram_size = 4096;
    std::vector<Byte_t> ram;
    void setVRegister(int i, Byte_t value);
    Byte_t readVRegister(int i);

    /* Graphics */
    const unsigned graphics_width = 64;
    const unsigned graphics_height = 32;

    /* Framebuffer */
    std::vector<Byte_t> frame_buffer;
};

#endif // CHIP8_PC_H
