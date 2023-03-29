#ifndef NUMBERDISPLAY_H
#define NUMBERDISPLAY_H

#include <QFrame>
#include <QLabel>
#include <QLCDNumber>
#include <QVBoxLayout>

class NumberDisplay : public QFrame
{
    Q_OBJECT
public:
    explicit NumberDisplay(QWidget *parent = nullptr);
    void setName(const QString& name);
    QString getName() {return name_tag.text(); }

public slots:
    void updateValue(int value);

private:
    QVBoxLayout main_layout;
    QLabel name_tag;
    QLabel value_display;
};

#endif // NUMBERDISPLAY_H
