#ifndef AMBIENTLIGHT_H
#define AMBIENTLIGHT_H

#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>

namespace Delimiter
{
    const QChar Data(',');
}

namespace Ui {
class AmbientLight;
}

class AmbientLight : public QDialog
{
    Q_OBJECT

public:
    explicit AmbientLight(QWidget *parent = 0);
    ~AmbientLight();


private:
    Ui::AmbientLight *ui;
    void SetupUi(void);
    void Delay(quint32 sec);


    QSerialPort fSerialPort;
    QSerialPortInfo fSerialPortInfo;

    QString Status(bool value);

private slots:
    void OnPortSelected(QString port);
    void OnSlide(int brightnessValue);
    void setT();

    void OnDataReceived(void);
};

#endif // AMBIENTLIGHT_H
