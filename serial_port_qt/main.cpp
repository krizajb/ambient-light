#include "AmbientLight.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AmbientLight w;
    w.show();

    return a.exec();
}
