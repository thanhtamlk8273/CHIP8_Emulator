#include <chip8_pc.h>
#include <gtest/gtest.h>

TEST(BasicTests, TestInstructions) {
    CHIP8_PC cpu;

    /* 1nnn */
    EXPECT_EQ(cpu.program_counter, 0x0200);
    cpu.perform1nnn(0x124F);
    EXPECT_EQ(cpu.program_counter, 0x024F);

    /* 2nnn */
    cpu.perform2nnn(0x234F);
    EXPECT_EQ(cpu.stack_pointer, 0x0081);
    uint16_t opcode = (cpu.ram.at(cpu.stack_pointer - 1) << 8) + cpu.ram.at(cpu.stack_pointer);
    EXPECT_EQ(opcode, 0x024F);
    EXPECT_EQ(cpu.program_counter, 0x34F);

    /* 6xkk */
    cpu.perform6xkk(0x6356);
    ASSERT_EQ(cpu.v_registers.at(0x3), 0x0056);

    /* 3xkk */
    cpu.perform3xkk(0x3356);
    EXPECT_EQ(cpu.program_counter, 0x351);
    cpu.perform3xkk(0x3358);
    EXPECT_EQ(cpu.program_counter, 0x351);

    /* 4xkk */
    cpu.perform4xkk(0x4358);
    EXPECT_EQ(cpu.program_counter, 0x353);
    cpu.perform4xkk(0x4356);
    EXPECT_EQ(cpu.program_counter, 0x353);

    /* 5xy0 */
    cpu.perform6xkk(0x6377);
    cpu.perform6xkk(0x6577);
    cpu.perform5xy0(0x5350);
    EXPECT_EQ(cpu.program_counter, 0x355);

    /* 7xkk */
    cpu.perform6xkk(0x6300);
    cpu.perform7xkk(0x7302);
    EXPECT_EQ(cpu.v_registers.at(0x3), 0x0002);

    /* 8xy0 */
    cpu.perform6xkk(0x6000);
    cpu.perform6xkk(0x6101);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x0);
    ASSERT_EQ(cpu.v_registers.at(0x1), 0x1);
    cpu.perform8xy0(0x8010);
    EXPECT_EQ(cpu.v_registers.at(0x0), cpu.v_registers.at(1));

    /* 8xy1 */
//    cpu.perform6xkk(0x60FF);
//    cpu.perform6xkk(0x6100);
//    ASSERT_EQ(cpu.v_registers.at(0), 0xFF);
//    ASSERT_EQ(cpu.v_registers.at(1), 0x00);
//    cpu.perform8xy1(0x8011);
//    EXPECT_EQ(cpu.v_registers.at(0), 0xFF);

    /* 8xy2 */
    cpu.perform6xkk(0x60F0);
    cpu.perform6xkk(0x6100);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0xF0);
    ASSERT_EQ(cpu.v_registers.at(0x1), 0x00);
    cpu.perform8xy2(0x8012);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x00);

    /* 8xy3 */
    cpu.perform6xkk(0x600C);
    cpu.perform6xkk(0x610A);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x0C);
    ASSERT_EQ(cpu.v_registers.at(0x1), 0x0A);
    cpu.perform8xy3(0x8013);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x06);

    /* 8xy4 - not overflow */
    cpu.perform6xkk(0x6002);
    cpu.perform6xkk(0x6101);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x02);
    ASSERT_EQ(cpu.v_registers.at(0x1), 0x01);
    cpu.perform8xy4(0x8014);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x03);
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x00);

    /* 8xy4 - overflow */
    cpu.perform6xkk(0x60FF);
    cpu.perform6xkk(0x6102);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0xFF);
    ASSERT_EQ(cpu.v_registers.at(0x1), 0x02);
    cpu.perform8xy4(0x8014);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x01);
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x01);

    /* 8xy5 - not borrow */
    cpu.perform6xkk(0x6003);
    cpu.perform6xkk(0x6102);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x03);
    ASSERT_EQ(cpu.v_registers.at(0x1), 0x02);
    cpu.perform8xy5(0x8015);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x01);
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x01);

    /* 8xy5 - borrow */
    cpu.perform6xkk(0x6002);
    cpu.perform6xkk(0x6103);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x02);
    ASSERT_EQ(cpu.v_registers.at(0x1), 0x03);
    cpu.perform8xy5(0x8015);
    EXPECT_EQ(cpu.v_registers.at(0x0), (uint8_t) (0x02 - 0x03));
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x00);

    /* 8xy6 */
    cpu.perform6xkk(0x6003);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x03);
    cpu.perform8xy6(0x8016);
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x01);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x01);

    cpu.perform6xkk(0x6002);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x02);
    cpu.perform8xy6(0x8016);
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x00);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x01);

    /* 8xyE */
    cpu.perform6xkk(0x6003);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x03);
    cpu.perform8xyE(0x802E);
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x00);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x06);

    cpu.perform6xkk(0x60F0);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0xF0);
    cpu.perform8xyE(0x802E);
    EXPECT_EQ(cpu.v_registers.at(0xF), 0x01);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0xE0);

    /* Annn */
    cpu.performAnnn(0xAFF2);
    EXPECT_EQ(cpu.index_register, 0x0FF2);
    cpu.performAnnn(0xAF52);
    EXPECT_EQ(cpu.index_register, 0x0F52);

    /* Bnnn */
    cpu.perform6xkk(0x6002);
    ASSERT_EQ(cpu.v_registers.at(0x0), 0x02);
    cpu.performBnnn(0xb304);
    EXPECT_EQ(cpu.program_counter, 0x0306);

    /* Cxkk */
    srand(1);
    cpu.performCxkk(0xc0FF);
    EXPECT_EQ(cpu.v_registers.at(0x0), 0x67);

    /* Fx1E */
    cpu.performAnnn(0xA001);
    ASSERT_EQ(cpu.index_register, 0x0001);
    cpu.perform6xkk(0x6001);
    cpu.performFx1E(0xF01E);
    ASSERT_EQ(cpu.index_register, 0x0002);
}
