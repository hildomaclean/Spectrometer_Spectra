#ifndef THREAD_H
#define THREAD_H

#include <QThread>
#include <QMutex>
#include <QDateTime>
#include "qcustomplot.h"
#include "ArrayTypes.h"
#include "Wrapper.h"
#include "ContinuousStrobeImpl.h"   // ocean optics
#include "BoardTemperatureImpl.h"   // ocean optics
#include "ThermoElectricImpl.h" //
#include <QFile>

class Thread : public QThread
{
    Q_OBJECT
public:
    explicit Thread(QObject *parent = 0);
    ~Thread(void);
    void Render(int,int,int,int);
    QDir FinalDirectory;


signals:
    void Data2Plot(double*,double*,int);
    void SendCalibratedValue(int);
    void SendSavingFinished(bool);

public slots:
    void SetGraphics(WRAPPER_T*);
    void SetDirectoryName(QString dir);
    void ControlGraph(int);
    void SavingControls(int,int,int);
    void SaveRaw(int);
    void CalibrationMode(int do_cal);
    void ZeroControls(int,int);
    void StopRunning(int);
    int GetReferenceFileNumber();
    int GetSpectrumFileNumber();
    int GetRawSpectrumFileNumber();
    int Autoadjusting(WRAPPER_T* wrapper,int min_ms, int max_ms, int step_ms, int min_nm, int max_nm);

private:
    void run();
    void SavingTXTfile(WRAPPER_T* wrapper,double* data , double* wave , int numpix, int ref_spec);
    WRAPPER_T* thr_wrapperHandle;
    QMutex mutex;
    QWaitCondition condition;
    double* ZeroValues;
    double* RefValues;
    int integration_time;
    int boxcar_width;
    int electrical_dark;
    int average;
    bool UpdateData;
    bool UpdateSave;
    bool UpdateZero;
    bool UpdateCal;
    int m_control;
    int refer_save;
    int spec_save;
    int save_zero;
    int sub_zero;
    int normalize;
    int countFiles_R;
    int countFiles_S;
    int countFiles_W;
    int stopThread;
    bool abort;
    QString directory_name;
    int do_Calibration;
    bool UpdateSaveRaw;
    int save_raw;
    bool IsFinalDirCreated;
};

#endif // THREAD_H
