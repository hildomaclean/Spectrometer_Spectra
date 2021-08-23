#include "thread.h"
#include "stdio.h"


Thread::Thread(QObject *parent) :
    QThread(parent)
{
    UpdateData  = false;
    UpdateCal   = false;
    UpdateSave  = false;
    UpdateSaveRaw = false;
    refer_save  = 0; //Saving
    spec_save   = 0; // Saving
    save_zero   = 0 ; //Zero
    sub_zero    = 0;  //Zero
    save_raw    = 0; //Save Raw


    ZeroValues = new double[3000];
    RefValues = new double[3000];

    countFiles_R = 0;
    countFiles_S = 0;
    countFiles_W = 0;

    stopThread = 0;
    abort = false;
    average = 1;
    integration_time = 100;

    IsFinalDirCreated = false;
}


Thread::~Thread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();
    wait();
}

void Thread::StopRunning(int stop)
{
    QMutexLocker locker(&mutex);
    stopThread = stop;
}

void Thread::CalibrationMode(int do_cal)
{
    QMutexLocker locker(&mutex);
    do_Calibration = do_cal;
    UpdateCal = true;
}

void Thread::SavingControls(int sv_ref, int sv_spec, int norm)
{
    QMutexLocker locker(&mutex);
    refer_save  = sv_ref;
    spec_save   = sv_spec;
    normalize   = norm;
    UpdateSave  = true;
}

void Thread::ZeroControls(int zr_save, int zr_sub)
{
    QMutexLocker locker(&mutex);
    save_zero   = zr_save;
    sub_zero    = zr_sub;
    UpdateZero  = true;
}

void Thread::SaveRaw(int sv_Raw)
{
    QMutexLocker locker(&mutex);
    save_raw = sv_Raw;
    UpdateSaveRaw = true;
}

void Thread::SetGraphics(WRAPPER_T * t_wh)
{
    thr_wrapperHandle = t_wh;
}

void Thread::SetDirectoryName(QString dir)
{
     QMutexLocker locker(&mutex);
     directory_name = dir;
     IsFinalDirCreated = false;
}

void Thread::Render(int int_time,int avg, int elec_dark, int boxcar)
{
    QMutexLocker locker(&mutex);
    integration_time    = int_time;
    average             = avg;
    boxcar_width        = boxcar;
    electrical_dark     = elec_dark;
    UpdateData          = true;

}

void Thread::ControlGraph(int control)
{
    if(control == 0)
    {
        m_control = 0;
        condition.wakeAll();
    }
    if(control == 1) //pause condition
        m_control = 1;
}

void Thread::run()
{
    DOUBLEARRAY_T spectrumArrayHandle = DoubleArray_Create();
    DOUBLEARRAY_T wavelengthArrayHandle = DoubleArray_Create();
    int numberOfPixels = 0;
    double* spectrumValues;
    double* wavelengthValues;
    double* zeroSpectrum;
    int Norm = 0;
    int CalibratedVal = 0;

    forever
    {
        // Grabbing external data ..
        // PARAMETERS ...
        mutex.lock();
        bool newData = UpdateData;
        int IntTime = integration_time;
        int Average = average;
        int BoxCarW = boxcar_width;
        int ElecDark = electrical_dark;
        if(newData)
            UpdateData = false;
        mutex.unlock();

        if(abort)      //
            return;    //
        if(stopThread == 1)
            return;

        // Setting parameters if necessary ...
        if(newData)
        {
            Wrapper_setIntegrationTime(*thr_wrapperHandle,0,IntTime*1000);
            Wrapper_setScansToAverage(*thr_wrapperHandle,0,Average);
            Wrapper_setCorrectForElectricalDark(*thr_wrapperHandle,0,ElecDark);
            //BOX CAR omitted
        }

        // Reading data ...
        Wrapper_getSpectrum(*thr_wrapperHandle,0,spectrumArrayHandle);
        spectrumValues = DoubleArray_getDoubleValues(spectrumArrayHandle);
        numberOfPixels = DoubleArray_getLength(spectrumArrayHandle);
        Wrapper_getWavelengths(*thr_wrapperHandle,0,wavelengthArrayHandle);
        wavelengthValues = DoubleArray_getDoubleValues(wavelengthArrayHandle);

        // PROCESS & SAVING OPERATIONS ...
        mutex.lock();
        int saveZero    = save_zero;
        int subZero     = sub_zero;
        int refSave     = refer_save;
        int specSave    = spec_save;
        int doCalibration = do_Calibration;
        int normalization   = normalize;
        int saveRaw = save_raw;
        bool update_save    = UpdateSave; //
        bool update_zero    = UpdateZero; //
        bool update_cal     = UpdateCal;  //
        bool update_saveraw = UpdateSaveRaw;
        if(update_zero)//
            UpdateZero  = false;//
        if(update_save)//
            UpdateSave  = false;//
        if(update_cal)
            UpdateCal   = false;
        if(update_saveraw)
            UpdateSaveRaw = false;
        mutex.unlock();

        // Saving Zero ...
        if(saveZero == 1)
        {
            ZeroValues = new double[numberOfPixels];
            for(int j=0; j<numberOfPixels; j++)
                ZeroValues[j]=spectrumValues[j];
            saveZero = 0;
        }

        // Processing Zero ...
        if(subZero == 1)
        {
            for(int j=0;j<numberOfPixels;j++)
                spectrumValues[j] = spectrumValues[j]-ZeroValues[j];
        }

        // Saving Raw ...
        if(saveRaw == 1)
        {
            saveRaw = 0;
            SavingTXTfile(thr_wrapperHandle,spectrumValues,wavelengthValues,numberOfPixels,2);
        }

        // Saving Reference ...
        if(refSave == 1)
        {
            RefValues = new double [numberOfPixels];
            for(int j=0; j<numberOfPixels; j++)
                RefValues[j]=spectrumValues[j];
            refSave = 0;
            Norm = 1;
            //SavingTXTfile(thr_wrapperHandle,RefValues,wavelengthValues,numberOfPixels,1);
        }

        // Processing Reflectance ...
        if(Norm == 1)
        {
            for(int j=0;j<numberOfPixels;j++)
                spectrumValues[j] = spectrumValues[j]/RefValues[j];

            if(specSave == 1)
            {   specSave = 0;
                SavingTXTfile(thr_wrapperHandle,spectrumValues,wavelengthValues,numberOfPixels,0);
            }
        }

        // No Normalization ...
        if (normalization == 0)
        {
            Norm = 0;
        }

        // Calibrate if needed ..
        if(doCalibration == 1)
        {
            // Doing Calibration ...                       min,max,step
            CalibratedVal = Autoadjusting(thr_wrapperHandle,10,500,10,300,700);
            // Setear con valor CalibratedVal ...
            Wrapper_setIntegrationTime(*thr_wrapperHandle,0,CalibratedVal*1000);
            //Mandar Valor Calibrado al dialog.cpp
            emit SendCalibratedValue(CalibratedVal);
            // Finish Calibration
            doCalibration = 0;
        }

        // Plotting data ...
        emit Data2Plot(wavelengthValues,spectrumValues,numberOfPixels);

        //VERIFICATION ...
        mutex.lock();
        int fv_control  = m_control;    // PAUSE!
        update_zero     = UpdateZero;   // Verifico si los Updates siguen en FALSE
        update_save     = UpdateSave;   // Si estuviesen en TRUE quiere decir que se recibio
        update_cal      = UpdateCal;    // Una nueva actualizacion durante el proceso RUN()
        update_saveraw  = UpdateSaveRaw; //
        if(fv_control == 1)
            condition.wait(&mutex);
        if((saveZero == 0) &&(!update_zero))    // Si el UPDATE sigue en FALSE, entonces se termin
            save_zero = 0;
        if ((refSave == 0) &&(!update_save))    //
            refer_save = 0;
        if((specSave == 0) &&(!update_save))    //
            spec_save = 0;
        if((doCalibration == 0)&&(!update_cal)) //
            do_Calibration = 0;
        if((saveRaw == 0)&&(!update_saveraw))     //
            save_raw = 0;
        mutex.unlock();
    }
}

void Thread::SavingTXTfile(WRAPPER_T* wrapper,double* data, double* wave, int numpix, int ref_spec_raw)
{

    // Creating and Opening file ...
    QFile file;
    QString number_filename;
    //QDate localDate_file = QDate::currentDate();
    QDateTime localDateTime_file = QDateTime::currentDateTime();

    if(!IsFinalDirCreated)
    {
        FinalDirectory.setPath(directory_name+"\\"+localDateTime_file.toString("SC_dd_MM_yyyy_hh_mm_ss"));
        if(!FinalDirectory.exists())
        {
            FinalDirectory.mkpath(".");
        }
        IsFinalDirCreated = true;
        countFiles_R = 0;
        countFiles_S = 0;
        countFiles_W = 0;
    }

    if(ref_spec_raw == 1) //Reference
    {   number_filename.sprintf("%03d",countFiles_R);
        file.setFileName(FinalDirectory.path()+"\\"+"R"+number_filename+".txt");
        countFiles_R++;
    }
    if(ref_spec_raw == 0)   // Spectrum
    {   number_filename.sprintf("%03d",countFiles_S);
        file.setFileName(FinalDirectory.path()+"\\"+"S"+number_filename+".txt");
        countFiles_S++;
    }
    if(ref_spec_raw == 2) //Raw
    {
        number_filename.sprintf("%03d",countFiles_W);
        file.setFileName(FinalDirectory.path()+"\\"+"W"+number_filename+".txt");
        countFiles_W++;
    }
    file.open(QIODevice::WriteOnly | QIODevice::Text);

    // Prepare variables ...
    QTextStream out(&file);
    JSTRING spectrometerName    = JString_Create();
    JSTRING serialNumber        = JString_Create();
    JSTRING firmwareVersion     = JString_Create();
    JSTRING apiVersion          = JString_Create();
    JSTRING bench               = JString_Create();
    JSTRING benchSize           = JString_Create();
    JSTRING benchFilter         = JString_Create();
    int spectrometerIndex = 0;
    int darkpixels = 0;
    mutex.lock();
    int int_time = integration_time;
    int avg = average;
    int boxcar = boxcar_width;
    int elec_dark = electrical_dark;
    mutex.unlock();
    int featureboardtemp;
    double temperatureBoard = 0;
    BOARDTEMPERATUREIMPL_T boardTemperatureImplHandle = BoardTemperatureImpl_Create();
    int featuredetectortemp;
    double temperaturedetector = 0;
    THERMOELECTRICIMPL_T temperaturedetectorImplHandle = ThermoElectricImpl_Create();
    BENCH_T  benchHandle;
    benchHandle = Bench_Create();
    QDateTime localDateTime_meta;

    // Get information ...
    Wrapper_getName(*wrapper,spectrometerIndex,spectrometerName);
    Wrapper_getSerialNumber(*wrapper,spectrometerIndex,serialNumber);
    Wrapper_getFirmwareVersion(*wrapper,spectrometerIndex,firmwareVersion);
    Wrapper_getApiVersion(*wrapper,apiVersion);
    darkpixels = Wrapper_getNumberOfDarkPixels(*wrapper, spectrometerIndex);
    featureboardtemp = Wrapper_isFeatureSupportedBoardTemperature(*wrapper,spectrometerIndex);
    if(featureboardtemp == 1)
    {   Wrapper_getFeatureControllerBoardTemperature(*wrapper,spectrometerIndex,boardTemperatureImplHandle);
        temperatureBoard = BoardTemperatureImpl_getBoardTemperatureCelsius(boardTemperatureImplHandle); }
    featuredetectortemp = Wrapper_isFeatureSupportedThermoElectric(*wrapper,spectrometerIndex);
    if(featuredetectortemp == 1)
    {Wrapper_getFeatureControllerThermoElectric(*wrapper, spectrometerIndex, temperaturedetectorImplHandle);
    temperaturedetector = ThermoElectricImpl_getDetectorTemperatureCelsius(temperaturedetectorImplHandle);}
    Wrapper_getBench(*wrapper,spectrometerIndex,benchHandle);
    Bench_getGrating(benchHandle,bench);
    Bench_getSlitSize(benchHandle,benchSize);
    Bench_getFilterWavelength(benchHandle,benchFilter);

    // Saving Metadata to file ...
    out<<"METADATA\n\n";
    out<<"Spectrometer Name: "<<JString_getASCII(spectrometerName)<<"\n";
    out<<"Serial Number: "<<JString_getASCII(serialNumber)<<"\n";
    out<<"Firmware Version: "<<JString_getASCII(firmwareVersion)<<"\n";
    out<<"Api Version: "<<JString_getASCII(apiVersion)<<"\n";
    out<<"Number of dark pixels: "<<darkpixels<<"\n";
    out<<"Integration time (ms): "<<int_time<<"\n";
    out<<"Scan to average: "<<avg<<"\n";
    out<<"Electric dark correction enabled(1)/disabled(0): "<<elec_dark<<"\n";
    if(featureboardtemp == 1)
    {   out<<"BoardTemperature feature is supported\n";
        out<<"TemperatureBoard: "<<temperatureBoard<<" C.\n";}
    else
        out<<"BoardTemperature feature is not supported\n";
    if(featuredetectortemp == 1)
    {   out<<"ThermoElectric feature is supported\n";
        out<<"TemperatureDetector: "<<temperaturedetector<<" C.\n";}
    else
        out<<"ThermoElectric feature is not supported\n";
    out<<"Grating Number: "<<JString_getASCII(bench)<<"\n";
    out<<"Grating Size: "<<JString_getASCII(benchSize)<<"\n";
    out<<"Filter Wavelenght: " <<JString_getASCII(benchFilter)<<"\n";
    out<<"Date & Hour: "<<localDateTime_meta.currentDateTime().toString()<<"\n";
    out<<"-------------------------\n";

    // Saving Data to file ...
    out<<"\nDATA\n\n";
    for(int i=0; i<numpix; i++)
        //out <<i<<" "<<wave[i]<<" "<<data[i] <<"\n";
        out <<wave[i]<<" "<<data[i] <<"\n";

    // Closing all ...
    file.close();
    JString_Destroy(spectrometerName);
    JString_Destroy(serialNumber);
    JString_Destroy(firmwareVersion);
    JString_Destroy(apiVersion);
    JString_Destroy(bench);
    JString_Destroy(benchSize);
    JString_Destroy(benchFilter);
    emit SendSavingFinished(true);
}

int Thread::GetReferenceFileNumber()
{
    mutex.lock();
    int num_ref_file = countFiles_R;
    mutex.unlock();
    return num_ref_file;
}

int Thread::GetSpectrumFileNumber()
{
    mutex.lock();
    int num_spec_file = countFiles_S;
    mutex.unlock();
    return num_spec_file;
}

int Thread::GetRawSpectrumFileNumber()
{
    mutex.lock();
    int num_raw_file = countFiles_W;
    mutex.unlock();
    return num_raw_file;
}

int Thread::Autoadjusting(WRAPPER_T* wrapper,int min_ms, int max_ms, int step_ms, int min_nm, int max_nm)
{
    DOUBLEARRAY_T spectrumArrayHandle = DoubleArray_Create();
    DOUBLEARRAY_T wavelengthArrayHandle = DoubleArray_Create();
    int numberOfPixels = 0;
    double* spectrumValues;
    double* wavelengthValues;
    bool saturated = false;
    int ms_saturated = min_ms;
    int indexBlaze_nm = 0;

    // Reading Data ...
    Wrapper_getWavelengths(*thr_wrapperHandle,0,wavelengthArrayHandle);
    wavelengthValues = DoubleArray_getDoubleValues(wavelengthArrayHandle);
    numberOfPixels = DoubleArray_getLength(wavelengthArrayHandle);
    // Locating Blaze index ...
    for(int a = 0; a<numberOfPixels; a++ )
    {
        if((wavelengthValues[a]>479.5)&&(wavelengthValues[a]<480.5))
        {
                indexBlaze_nm = a;
                break;
        }
    }

    // Searching at different milliseconds
    for (int i = min_ms; i < max_ms; i = i+step_ms)
    {
        // Setting Parameters
        Wrapper_setIntegrationTime(*wrapper,0,i*1000);

        // Sleep for a while
        // Sleep(20);

        // Reading Data
        Wrapper_getSpectrum(*wrapper,0,spectrumArrayHandle);
        spectrumValues = DoubleArray_getDoubleValues(spectrumArrayHandle);
        numberOfPixels = DoubleArray_getLength(spectrumArrayHandle);
        Wrapper_getWavelengths(*thr_wrapperHandle,0,wavelengthArrayHandle);
        wavelengthValues = DoubleArray_getDoubleValues(wavelengthArrayHandle);

        // Processing

        if(spectrumValues[indexBlaze_nm]>=52428)
        {
           saturated = true;
           if(i == min_ms)
               ms_saturated = min_ms;
           else
               ms_saturated = i-step_ms;

           break;
        }

    }

    if(!saturated)
        return max_ms;

    if((saturated) && (ms_saturated != 10))
        return ms_saturated;

    if((saturated) && (ms_saturated == 10))
    {
        // Searching in a finest way
        for (int i = 1; i < 10; i++)
        {
            // Setting Parameters
            Wrapper_setIntegrationTime(*wrapper,0,i*1000);
            // Reading Data
            Wrapper_getSpectrum(*wrapper,0,spectrumArrayHandle);
            spectrumValues = DoubleArray_getDoubleValues(spectrumArrayHandle);
            numberOfPixels = DoubleArray_getLength(spectrumArrayHandle);
            Wrapper_getWavelengths(*thr_wrapperHandle,0,wavelengthArrayHandle);
            wavelengthValues = DoubleArray_getDoubleValues(wavelengthArrayHandle);

            // Processing
            if(spectrumValues[indexBlaze_nm]>=52428)
            {
                if(i == 1)
                   ms_saturated = 1;
                else
                   ms_saturated = i-1;

                break;
            }
        }

        return ms_saturated;
    }

}
