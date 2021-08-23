#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDialog>

namespace Ui {
class aboutdlg;
}

class aboutdlg : public QDialog
{
    Q_OBJECT

public:
    explicit aboutdlg(QWidget *parent = 0);
    ~aboutdlg();

private:
    Ui::aboutdlg *ui;
};

#endif // ABOUTDLG_H
