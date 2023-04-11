#include <iostream>
#include <vector>
#include <stack>
#include <sstream>
#include <memory>

#include <QTimer>
#include <QMetaType>
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>

#include <chip8_pc.h>
#include <monitor.h>
#include <numberdisplay.h>

int main(int argc, char* argv[])
{
    QApplication app (argc, argv);

    time_t t;
    srand((unsigned) time(&t));

    /* Main widget */
    QWidget widget;

    /* Layouts */
    QHBoxLayout main_layout(&widget);
    QVBoxLayout vertical_layout;
    QHBoxLayout buttons_layout;
    vertical_layout.addLayout(&buttons_layout);
    main_layout.addLayout(&vertical_layout);

    /* Upper buttons */
    QPushButton load_button("Load");
    QPushButton start_button("Start");
    QPushButton stop_button("Stop");
    buttons_layout.addWidget(&load_button);
    buttons_layout.addWidget(&start_button);
    buttons_layout.addWidget(&stop_button);

    /* CHIP8 */
    CHIP8_PC pc;
    Monitor monitor(pc.getGraphicsWidth(), pc.getGraphicsHeight(), 10, &widget);
    vertical_layout.addWidget(&monitor);

    /* Current opcode display */
    NumberDisplay current_opcode_display;
    current_opcode_display.setName("Opcode");

    /* Program counter */
    NumberDisplay program_counter_display;
    program_counter_display.setName("Program Counter");

    /* Index register display */
    NumberDisplay index_register_display;
    index_register_display.setName("Index Register");

    QVBoxLayout subdisplays_layout;
    subdisplays_layout.addWidget(&current_opcode_display);
    subdisplays_layout.addWidget(&program_counter_display);
    subdisplays_layout.addWidget(&index_register_display);
    subdisplays_layout.addStretch();
    main_layout.addLayout(&subdisplays_layout);

    /* V_registers */
    std::vector<QVBoxLayout*> vregisters_layout;
    vregisters_layout.push_back(new QVBoxLayout());
    vregisters_layout.push_back(new QVBoxLayout());

    std::vector<NumberDisplay*> v_displays;
    for(int i = 0; i < 16; ++i) {
        NumberDisplay* dp = new NumberDisplay();
        std::stringstream ss;
        ss << std::hex << std::uppercase << i;
        dp->setName(QString("V") + QString::fromStdString(ss.str()) + QString(12, ' '));

        vregisters_layout.at(i / 8)->addWidget(dp);
        vregisters_layout.at(i / 8)->addStretch();
        v_displays.push_back(dp);
    }
    main_layout.addLayout(vregisters_layout.at(0));
    main_layout.addLayout(vregisters_layout.at(1));

    /* Signals and slots */
    QObject::connect(&pc, &CHIP8_PC::frameBufferChanged, &monitor, &Monitor::update);
    QObject::connect(&monitor, &Monitor::signalKeyPressEvent, &pc, &CHIP8_PC::receiveKeyPressEvent);
    QObject::connect(&monitor, &Monitor::signalKeyReleaseEvent, &pc, &CHIP8_PC::receiveKeyReleaseEvent);
    QObject::connect(&load_button, &QPushButton::released, &pc, &CHIP8_PC::load);
    QObject::connect(&start_button, &QPushButton::released, &pc, &CHIP8_PC::startCpu);
    QObject::connect(&stop_button, &QPushButton::released, &pc, &CHIP8_PC::stopCpu);
    QObject::connect(&pc, &CHIP8_PC::indexRegisterChanged, &index_register_display, &NumberDisplay::updateValue);
    QObject::connect(&pc, &CHIP8_PC::currentOpcodeChanged, &current_opcode_display, &NumberDisplay::updateValue);
    QObject::connect(&pc, &CHIP8_PC::programCounterChanged, &program_counter_display, &NumberDisplay::updateValue);
    QObject::connect(&pc, &CHIP8_PC::vRegisterChanged,
                     [&v_displays](int i, Byte_t value){v_displays.at(i)->updateValue(value);});

    widget.show();
    return app.exec();
}
