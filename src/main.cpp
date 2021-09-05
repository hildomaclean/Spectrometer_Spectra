#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    w.setWindowFlags(Qt::Window); // add this
    w.show();

    return a.exec();
}
