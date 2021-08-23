#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTimer>
#include "thread.h"
#include "qcustomplot.h"
#include "ArrayTypes.h"
#include "Wrapper.h"
#include "aboutdlg.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    int numberOfSpectrometersAttached;
    bool b_folderselected;
    bool b_refacq;
    int minimumAllowedIntegrationTime;
    int maximunAllowedIntegrationTime;
    int maximumIntensity;
    QString dirName;
    int m_indexBlazeSpec;
    int minimumWavelenght;
    int maximumWavelenght;
    bool IsTextActive;
    bool b_IsThereSpec;
    QCPItemText *textLabel;
    bool m_bSaveStatus;
    bool m_bRawAcqPressed;
    bool m_bSpecAcqPressed;

private slots:

       void PlottingData(double*,double*,int);
       void GrabbingSpectrometerInformation(WRAPPER_T *);
       void ReceivedCalibratedValue(int);
       void ReceivedSavingFinished(bool);
       void on_pb_refresh_clicked();
       void on_pb_play_clicked();
       void on_pb_pause_clicked();
       void on_sb_inttime_valueChanged(int arg1);
       void on_sb_average_valueChanged(int arg1);
       void on_pb_stopavg_clicked();
       void on_chkb_elecdark_clicked();
       void on_pb_autoscale_clicked();
       void on_pb_selfolder_clicked();
       void on_pb_refacq_clicked();
       void on_pb_newref_clicked();
       void on_pb_playpause_clicked();
       void on_pb_savemanzero_clicked();
       void on_pb_submanzero_clicked();
       void on_pb_newmanzero_clicked();
       void on_pb_cleanstatus_clicked();
       void on_pb_specacq_clicked();
       void on_pb_autoadjustment_clicked();
       void on_pb_save_raw_clicked();
       void on_pb_about_clicked();

private:
    Ui::Dialog *ui;
    Thread *mythread;  
    QStringList m_slData;
    QTimer *dataTimer;
    WRAPPER_T *wrapperHandle;
    bool graphpaused;
    aboutdlg *AboutDlg;


};

#endif // DIALOG_H
