#include "aboutdlg.h"
#include "ui_aboutdlg.h"

aboutdlg::aboutdlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutdlg)
{
    ui->setupUi(this);
    ui->l_aboutdlg->setOpenExternalLinks(true);
   /* ui->l_aboutdlg->setText("<p align=\"justify;\"><b>SPECTRA CIP V2.0</b></p>"
                            "<p align=\"justify;\">Developed by Gonzalo Cucho-Padin and Hildo Loayza-Loza</p>"
                            "<p align=\"justify;\">Developed under QT Creator 2.8.1 (QT 5.1.1)</p>"
                            "<p>Free Icons from <a href=\"http://www.fatcow.com/free-icons\" target=\"_new\">FatCow</a></p>"
                            );*/
}

aboutdlg::~aboutdlg()
{
    delete ui;
}
