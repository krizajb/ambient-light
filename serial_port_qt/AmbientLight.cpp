#include "AmbientLight.h"
#include "ui_AmbientLight.h"

#include <QDebug>
#include <QTime>
#include <QTimer>

AmbientLight::AmbientLight(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AmbientLight)
{
    ui->setupUi(this);
    this->SetupUi();

    connect(&this->fSerialPort, SIGNAL(readyRead()), this, SLOT(OnDataReceived()));
}

AmbientLight::~AmbientLight()
{
    delete ui;
    fSerialPort.close();
}

void AmbientLight::SetupUi()
{
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo port, portList)
    {
        ui->serialPortCmbBox->addItem(port.portName());
    }

    ui->brightnessSlider->setRange(0, 255);
//    ui->brightnessSlider->setTickInterval(50);
//    ui->brightnessSlider->setSingleStep(1);
    ui->brightnessSlider->setEnabled(false);

    connect(ui->serialPortCmbBox, SIGNAL(activated(QString)), this, SLOT(OnPortSelected(QString)));
    connect(ui->brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(OnSlide(int)));
}

void AmbientLight::Delay(quint32 sec)
{
    QTime dieTime= QTime::currentTime().addMSecs(sec);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

QString AmbientLight::Status(bool value)
{
    QString status = "Succeeced";
    if (!value) { status = "Failed"; }

    return status;
}

void AmbientLight::OnPortSelected(QString port)
{
    //qDebug() << "Port selected" << port;

    if (0 != this->ui->serialPortCmbBox->currentText().compare(this->fSerialPort.portName()))
    {
        this->fSerialPort.close();


        this->fSerialPort.setPortName(port);
        this->fSerialPort.setFlowControl(QSerialPort::NoFlowControl);


        bool isConnected = this->fSerialPort.open(QIODevice::ReadWrite);
        this->fSerialPort.setDataTerminalReady(false);

        qDebug() << this->fSerialPort.pinoutSignals();
        qDebug() << "Connection status to" << port << Status(isConnected);

        if (isConnected)
        {
            qDebug() << this->fSerialPort.isOpen();
            qDebug() << this->fSerialPort.isWritable();

            //this->Delay(1000);   // without delay the receiver "data avail" signal isn't emited
            qDebug() << this->fSerialPort.write("I");   // tell its the initial
            qDebug() << this->fSerialPort.flush();

            ui->brightnessSlider->setEnabled(true);
        }

//        int fadeAmount = 20;    // how many points to fade the LED by
//        int brightness = 0;
//        while(1)
//        {
//            brightness = brightness + fadeAmount;

//              // reverse the direction of the fading at the ends of the fade:
//              if (brightness == 0 || brightness > 220) {
//                fadeAmount = -fadeAmount ;
//              }
//              qDebug() << brightness;

//              QString data(QString::number(brightness) + Delimiter::Data);
//              this->fSerialPort.write(data.toStdString().c_str());

//              this->Delay(400);
//        }
    }
}

void AmbientLight::OnSlide(int brightnessValue)
{
    //qDebug() << brightnessValue;
    if (this->fSerialPort.isOpen())
    {
        QString data(QString::number(brightnessValue) + Delimiter::Data);
        //qDebug() << data;
        this->fSerialPort.write(data.toStdString().c_str());
        /*qDebug() <<*/ this->fSerialPort.flush();
    }
}

void AmbientLight::setT()
{
    ui->brightnessSlider->blockSignals(false);
}

void AmbientLight::OnDataReceived()
{
    char buf[1024];
    memset(&buf, 0, sizeof(buf));

    this->fSerialPort.read(buf, sizeof(buf));

    bool ok;
    qDebug() << "Received" << buf;
    QByteArray data(buf);
    qint32 value = data.toInt(&ok);
    qDebug() << "Conversion status" << Status(ok) << "Value" << value;
    if (ok)
    {
        ui->brightnessSlider->blockSignals(true);
        ui->brightnessSlider->setValue(value);
        ui->brightnessSlider->blockSignals(false);
    }
}
