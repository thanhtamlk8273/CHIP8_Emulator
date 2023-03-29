#include "numberdisplay.h"
#include <string>

NumberDisplay::NumberDisplay(QWidget *parent)
    : QFrame{parent}
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    setLineWidth(2);

    name_tag.setText("Unknown");
    name_tag.setFrameStyle(QFrame::Box);
    name_tag.setLineWidth(1);

    value_display.setAlignment(Qt::AlignRight);
    value_display.setFrameStyle(QFrame::Box);
    value_display.setLineWidth(1);
    value_display.setText(std::to_string(0).c_str());

    main_layout.addWidget(&name_tag);
    main_layout.addWidget(&value_display);

    setLayout(&main_layout);
}

void NumberDisplay::setName(const QString &name)
{
    name_tag.setText(name);
}

void NumberDisplay::updateValue(int value)
{
    char buff[30];
    sprintf(buff, "0x%X", value);
    value_display.setText(buff);
}
