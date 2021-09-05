#include "dialog.h"
#include "ui_dialog.h"
#include "thread.h"
#include "stdio.h"
#include <QFont>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    //Variables initialization
    m_indexBlazeSpec = 0;
    minimumWavelenght = 0;
    maximumWavelenght = 0;
    IsTextActive = false;
    b_IsThereSpec = false;
    m_bSaveStatus = false;
    m_bRawAcqPressed = false;
    m_bSpecAcqPressed = false;

    //Initialize Font Title
    //QFont font = ui->l_title->font();
    //font.setPointSize(16);
    //font.setBold(true);
    //ui->l_title->setFont(font);

    // Initialize Thread
    mythread = new Thread(this);

    // Initialize  About dialog
    AboutDlg = new aboutdlg(this);

    // Initialize ListView
    ui->lw_specinfo->setModel(new QStringListModel(m_slData));

    // Initilize Graph
    ui->qcp_graph->addGraph(); // Red Line
    ui->qcp_graph->graph(0)->setPen(QPen(Qt::red));
    ui->qcp_graph->graph(0)->setAntialiasedFill(false);
    ui->qcp_graph->xAxis->setLabel("Wavelenght(nm)");
    ui->qcp_graph->yAxis->setLabel("Intensity(counts)");
    ui->qcp_graph->xAxis->setRange(359,1032);
    ui->qcp_graph->yAxis->setRange(0,65535);
    ui->qcp_graph->yAxis2->setVisible(true);
    ui->qcp_graph->yAxis2->setTickLabels(false);
    ui->qcp_graph->xAxis2->setVisible(true);
    ui->qcp_graph->xAxis2->setTickLabels(false);
    ui->qcp_graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // Initialize QTimer
    dataTimer = new QTimer(this);

    // Initialize controls
    // Main buttons ...
    ui->pb_refresh->setEnabled(false);
    ui->pb_selfolder->setEnabled(false);
     //Acquisition buttons ...
    ui->pb_refacq->setEnabled(false);
    ui->pb_specacq->setEnabled(false);
    ui->pb_newref->setEnabled(false);
    // Graph Control Buttons ..
    ui->pb_pause->setEnabled(false);
    ui->pb_play->setEnabled(false);
    ui->pb_playpause->setEnabled(false);
    ui->pb_autoscale->setEnabled(false);
    ui->pb_save_raw->setEnabled(false);
    // Signal Control Options ...
    ui->sb_average->setEnabled(false);
    ui->sb_boxcar->setEnabled(false);
    ui->sb_inttime->setEnabled(false);
    ui->pb_stopavg->setEnabled(false);
    ui->chkb_elecdark->setEnabled(false);
    ui->pb_savemanzero->setEnabled(false);
    ui->pb_submanzero->setEnabled(false);
    ui->pb_newmanzero->setEnabled(false);
    ui->pb_autoadjustment->setEnabled(false);
    ui->te_status->setReadOnly(true);

    // Flag Initialization
    b_folderselected = false; //Folder selected
    b_refacq = false; // Ref Acquired
    graphpaused = false; //Graph Paused

    // Connecting thread and main
    connect(mythread,SIGNAL(Data2Plot(double*,double*,int)),this,SLOT(PlottingData(double*,double*,int))); // For Display text box
    connect(mythread,SIGNAL(SendCalibratedValue(int)),this,SLOT(ReceivedCalibratedValue(int))); // For receiving calibrated value
    connect(mythread,SIGNAL(SendSavingFinished(bool)),this,SLOT(ReceivedSavingFinished(bool))); // For receiving saving status

    // TextLabel (QCustomPlot) initialization
    textLabel = new QCPItemText(ui->qcp_graph);
    textLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    textLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel->position->setCoords(0.85,0.1);
    textLabel->setText("");
    textLabel->setColor(QColor(255, 0, 0));
    QFont font_qcp = textLabel->font();
    font_qcp.setPointSize(10);
    font_qcp.setBold(true);
    textLabel->setFont(font_qcp);
    ui->qcp_graph->addItem(textLabel);

    // Verification of Spectrometer Attached
    *wrapperHandle = Wrapper_Create(); // Create Handle
    numberOfSpectrometersAttached = Wrapper_openAllSpectrometers(*wrapperHandle); //Open Spectrometers

    if(numberOfSpectrometersAttached > 0) // If exist spectrometer attached ...
    {
        b_IsThereSpec = true;
        //Grabbing Spec data ...
        GrabbingSpectrometerInformation(wrapperHandle);
        //Setting parameters ..
        int integrationTime = 100;
        int average         = 1;
        Wrapper_setIntegrationTime(*wrapperHandle,0,integrationTime*1000);
        Wrapper_setScansToAverage(*wrapperHandle,0,average);
        // Set Range in Integration Time Spin Box & Set Y Axis  in graph...
        minimumAllowedIntegrationTime = Wrapper_getMinimumIntegrationTime(*wrapperHandle,0);
        maximunAllowedIntegrationTime = Wrapper_getMaximumIntegrationTime(*wrapperHandle,0);
        maximumIntensity = Wrapper_getMaximumIntensity(*wrapperHandle,0);
        ui->sb_inttime->setRange(minimumAllowedIntegrationTime/1000,maximunAllowedIntegrationTime/1000);
        ui->qcp_graph->yAxis->setRange(0,maximumIntensity+100);
        // Set Domain & Set Axis in graph  & Set Blaze Index for Spectrometer
        DOUBLEARRAY_T wavelengthArrayHandle = DoubleArray_Create();
        int numberOfPixels = 0;
        double* wavelengthValues;
        m_indexBlazeSpec = 0;
        Wrapper_getWavelengths(*wrapperHandle,0,wavelengthArrayHandle); // Reading Data ...
        wavelengthValues = DoubleArray_getDoubleValues(wavelengthArrayHandle);
        numberOfPixels = DoubleArray_getLength(wavelengthArrayHandle);
        for(int a = 0; a<numberOfPixels; a++ ) // Locating Blaze index ...
        {
            if((wavelengthValues[a]>479.5)&&(wavelengthValues[a]<480.5))
            {
                    m_indexBlazeSpec = a;
                    break;
            }
        }
        minimumWavelenght = wavelengthValues[0];
        maximumWavelenght = wavelengthValues[numberOfPixels-1];
        ui->qcp_graph->xAxis->setRange(minimumWavelenght,maximumWavelenght);
        // Folder & Graph control Buttons ...
        ui->pb_selfolder->setEnabled(true);
        ui->pb_pause->setEnabled(true);
        ui->pb_play->setEnabled(false);
        ui->pb_playpause->setEnabled(false);
        ui->pb_autoscale->setEnabled(true);
        // Thread starting ...
        mythread->SetGraphics(wrapperHandle);
        mythread->start();
    }
    else
    {
        b_IsThereSpec = false;
        // Folder & Graph control Buttons ...
        ui->pb_refresh->setEnabled(true);
        ui->pb_selfolder->setEnabled(false);
        ui->pb_pause->setEnabled(false);
        ui->pb_play->setEnabled(false);
        ui->pb_playpause->setEnabled(false);
        Wrapper_closeAllSpectrometers(*wrapperHandle);  // Close Spectrometers
        Wrapper_Destroy(*wrapperHandle);                // Destroy Handle
    }

    if(b_IsThereSpec)
    {
        QFile temporalFile;
        temporalFile.setFileName("data1.nft");
        bool res = temporalFile.open(QIODevice::ReadOnly | QIODevice::Text);
        if(res)
        {
            QTextStream in(&temporalFile);
            QString line = in.readLine();
            if(line == "")
                //QMessageBox::warning(this,"jaja","jeje");
                b_folderselected = false;
            else
            {
                // Flag ..
                b_folderselected = true;
                // Acquisition Buttons ..
                ui->pb_refacq->setEnabled(true);
                ui->pb_specacq->setEnabled(false);
                ui->pb_newref->setEnabled(false);
                // Signal Control Buttons ..
                ui->sb_average->setEnabled(true);
                ui->sb_boxcar->setEnabled(false);
                ui->sb_inttime->setEnabled(true);
                ui->pb_stopavg->setEnabled(true);
                ui->pb_autoadjustment->setEnabled(true);
                ui->pb_save_raw->setEnabled(true);
                ui->chkb_elecdark->setEnabled(true);
                ui->pb_savemanzero->setEnabled(true);
                ui->pb_submanzero->setEnabled(false);
                ui->pb_newmanzero->setEnabled(false);
                mythread->SetDirectoryName(line);
                // Updating Status
                ui->te_status->append(QString("Directory selected ")+line);
            }

        }
        temporalFile.close();
    }

}

void Dialog::PlottingData(double* x,double* y,int numpix)
{
    QVector<double> X(numpix), Y(numpix);
    for (int i = 0; i<numpix; i++)
    {
        X[i]= x[i];
        Y[i]= y[i];
    }

    if((y[m_indexBlazeSpec] >= 52428)&&(!IsTextActive))
    {
        textLabel->setText("SATURATED IN 480nm\n(>80% Maximum Count)");
        IsTextActive = true;
    }
    else
    {
        if((y[m_indexBlazeSpec] < 52428)&&(IsTextActive))
        {
            textLabel->setText("");
            IsTextActive = false;
        }
    }

    ui->qcp_graph->graph(0)->setData(X,Y);
    ui->qcp_graph->replot();
}

void Dialog::ReceivedCalibratedValue(int cal_value)
{
    ui->sb_inttime->setValue(cal_value);
    QString calibrated;
    calibrated.sprintf("%d",cal_value);
    ui->te_status->append("Adjusted finished\nNew Integration Time value: "+calibrated);
}

void Dialog::on_pb_refresh_clicked()
{
    *wrapperHandle = Wrapper_Create(); // Create Handle
    numberOfSpectrometersAttached = Wrapper_openAllSpectrometers(*wrapperHandle); //Open Spectrometers

    if(numberOfSpectrometersAttached > 0) // If exist spectrometer attached ...
    {
        b_IsThereSpec = true;
        //Grabbing Spec data ...
        GrabbingSpectrometerInformation(wrapperHandle);
        //Setting parameters ..
        int integrationTime = 100;
        int average         = 1;
        Wrapper_setIntegrationTime(*wrapperHandle,0,integrationTime*1000);
        Wrapper_setScansToAverage(*wrapperHandle,0,average);
        // Set Range in Integration Time Spin Box & Set Axis Y in graph...
        minimumAllowedIntegrationTime = Wrapper_getMinimumIntegrationTime(*wrapperHandle,0);
        maximunAllowedIntegrationTime = Wrapper_getMaximumIntegrationTime(*wrapperHandle,0);
        maximumIntensity = Wrapper_getMaximumIntensity(*wrapperHandle,0);
        ui->sb_inttime->setRange(minimumAllowedIntegrationTime/1000,maximunAllowedIntegrationTime/1000);
        ui->qcp_graph->yAxis->setRange(0,maximumIntensity+100);
        // Set Domain & Set Axis in graph  & Set Blaze Index for Spectrometer
        DOUBLEARRAY_T wavelengthArrayHandle = DoubleArray_Create();
        int numberOfPixels = 0;
        double* wavelengthValues;
        m_indexBlazeSpec = 0;
        Wrapper_getWavelengths(*wrapperHandle,0,wavelengthArrayHandle); // Reading Data ...
        wavelengthValues = DoubleArray_getDoubleValues(wavelengthArrayHandle);
        numberOfPixels = DoubleArray_getLength(wavelengthArrayHandle);
        for(int a = 0; a<numberOfPixels; a++ ) // Locating Blaze index ...
        {
            if((wavelengthValues[a]>479.5)&&(wavelengthValues[a]<480.5))
            {
                    m_indexBlazeSpec = a;
                    break;
            }
        }
        minimumWavelenght = wavelengthValues[0];
        maximumWavelenght = wavelengthValues[numberOfPixels-1];
        ui->qcp_graph->xAxis->setRange(minimumWavelenght,maximumWavelenght);
        // Folder & Graph control Buttons ...
        ui->pb_selfolder->setEnabled(true);
        ui->pb_pause->setEnabled(true);
        ui->pb_play->setEnabled(false);
        ui->pb_playpause->setEnabled(false);
        ui->pb_autoscale->setEnabled(true);
        // Thread starting ...
        mythread->SetGraphics(wrapperHandle);
        mythread->start();
    }

    if(b_IsThereSpec)
    {
        QFile temporalFile;
        temporalFile.setFileName("data1.nft");
        bool res = temporalFile.open(QIODevice::ReadOnly | QIODevice::Text);
        if(res)
        {
            QTextStream in(&temporalFile);
            QString line = in.readLine();
            if(line == "")
                //QMessageBox::warning(this,"jaja","jeje");
                b_folderselected = false;
            else
            {
                // Flag ..
                b_folderselected = true;
                // Acquisition Buttons ..
                ui->pb_refacq->setEnabled(true);
                ui->pb_specacq->setEnabled(false);
                ui->pb_newref->setEnabled(false);
                // Signal Control Buttons ..
                ui->sb_average->setEnabled(true);
                ui->sb_boxcar->setEnabled(false); //
                ui->sb_inttime->setEnabled(true);
                ui->pb_stopavg->setEnabled(true);
                ui->pb_autoadjustment->setEnabled(true);
                ui->pb_save_raw->setEnabled(true);
                ui->chkb_elecdark->setEnabled(true);
                ui->pb_savemanzero->setEnabled(true);
                ui->pb_submanzero->setEnabled(false);
                ui->pb_newmanzero->setEnabled(false);
                mythread->SetDirectoryName(line);
                // Updating Status
                ui->te_status->append(QString("Directory selected ")+line);
            }

        }
        temporalFile.close();
    }
}

void Dialog::GrabbingSpectrometerInformation(WRAPPER_T * wrapperhandle)
{
    // Prepare variables ..
    JSTRING spectrometerName    = JString_Create();
    JSTRING serialNumber        = JString_Create();
    JSTRING firmwareVersion     = JString_Create();
    JSTRING apiVersion          = JString_Create();
    int spectrometerIndex = 0;
    int darkpixels = 0;

    // Get information ...
    Wrapper_getName(*wrapperhandle,spectrometerIndex,spectrometerName);
    Wrapper_getSerialNumber(*wrapperhandle,spectrometerIndex,serialNumber);
    Wrapper_getFirmwareVersion(*wrapperhandle,spectrometerIndex,firmwareVersion);
    Wrapper_getApiVersion(*wrapperhandle,apiVersion);
    darkpixels = Wrapper_getNumberOfDarkPixels(*wrapperhandle, spectrometerIndex);

    // Show information ...
    m_slData.append(QString("Spectrometer Name: ") + QString(JString_getASCII(spectrometerName)));
    ((QStringListModel*) ui->lw_specinfo->model())->setStringList(m_slData);
    m_slData.append(QString("Serial Number: ") + QString(JString_getASCII(serialNumber)));
    ((QStringListModel*) ui->lw_specinfo->model())->setStringList(m_slData);
    m_slData.append(QString("Firmware Version: ") + QString(JString_getASCII(firmwareVersion)));
    ((QStringListModel*) ui->lw_specinfo->model())->setStringList(m_slData);
    m_slData.append(QString("API Version: ") + QString(JString_getASCII(apiVersion)));
    ((QStringListModel*) ui->lw_specinfo->model())->setStringList(m_slData);
    m_slData.append(QString("Number of dark pixels: ") + QString::number(darkpixels));
    ((QStringListModel*) ui->lw_specinfo->model())->setStringList(m_slData);
    ui->lw_specinfo->setEditTriggers(QAbstractItemView::NoEditTriggers);

    JString_Destroy(spectrometerName);
    JString_Destroy(serialNumber);
    JString_Destroy(firmwareVersion);
    JString_Destroy(apiVersion);

}

void Dialog::on_pb_play_clicked()
{
     mythread->ControlGraph(0);
     ui->pb_pause->setEnabled(true);
     ui->pb_play->setEnabled(false);
     ui->pb_playpause->setEnabled(false);
     graphpaused = false;
}

void Dialog::on_pb_pause_clicked()
{
    mythread->ControlGraph(1);
    ui->pb_pause->setEnabled(false);
    ui->pb_play->setEnabled(true);
    ui->pb_playpause->setEnabled(true);
    graphpaused = true;
}

void Dialog::on_sb_inttime_valueChanged(int arg1)
{
   int avg      = ui->sb_average->value();
   int boxcar   = ui->sb_boxcar->value();
   int elec_dark = 0;
   if(ui->chkb_elecdark->isChecked()) elec_dark = 1;
   else  elec_dark = 0;

   mythread->Render(arg1,avg,elec_dark,boxcar);
}

void Dialog::on_sb_average_valueChanged(int arg1)
{
    int inttime  = ui->sb_inttime->value();
    int boxcar   = ui->sb_boxcar->value();
    int elec_dark = 0;
    if(ui->chkb_elecdark->isChecked()) elec_dark = 1;
    else  elec_dark = 0;

    mythread->Render(inttime,arg1,elec_dark,boxcar);
}

void Dialog::on_pb_stopavg_clicked()
{
    ui->sb_average->setValue(1);
}

void Dialog::on_chkb_elecdark_clicked()
{
    int inttime  = ui->sb_inttime->value();
    int boxcar   = ui->sb_boxcar->value();
    int avg      = ui->sb_average->value();
    int elec_dark = 0;
    if(ui->chkb_elecdark->isChecked()) elec_dark = 1;
    else  elec_dark = 0;

    mythread->Render(inttime,avg,elec_dark,boxcar);
}

void Dialog::on_pb_autoscale_clicked()
{
    if(!b_refacq)
    {
      //  ui->qcp_graph->xAxis->setRange(359,1032);
        ui->qcp_graph->xAxis->setRange(minimumWavelenght,maximumWavelenght);
        ui->qcp_graph->yAxis->setRange(0,maximumIntensity+100);
    }
    else
    {
       // ui->qcp_graph->xAxis->setRange(359,1032);
        ui->qcp_graph->xAxis->setRange(minimumWavelenght,maximumWavelenght);
        ui->qcp_graph->yAxis->setRange(0,1.5);
    }
        ui->qcp_graph->replot();
}





void Dialog::on_pb_selfolder_clicked()
{
    dirName =  QFileDialog::getExistingDirectory(this,tr("Saving Directory"),"/home",QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);

    if(dirName!= "")
    {
        // Flag ..
        b_folderselected = true;
        // Acquisition Buttons ..
        ui->pb_refacq->setEnabled(true);
        ui->pb_specacq->setEnabled(false);
        ui->pb_newref->setEnabled(false);
        // Signal Control Buttons ..
        ui->sb_average->setEnabled(true);
        ui->sb_boxcar->setEnabled(false); //
        ui->sb_inttime->setEnabled(true);
        ui->pb_stopavg->setEnabled(true);
        ui->pb_autoadjustment->setEnabled(true);
        ui->pb_save_raw->setEnabled(true);
        ui->chkb_elecdark->setEnabled(true);
        ui->pb_savemanzero->setEnabled(true);
        ui->pb_submanzero->setEnabled(false);
        ui->pb_newmanzero->setEnabled(false);
        mythread->SetDirectoryName(dirName);
        // Updating Status
        ui->te_status->append(QString("Directory selected ")+dirName);
        // Updating data1.nft
        QFile temporalFile("data1.nft");
        temporalFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&temporalFile);
        out << dirName;
        temporalFile.close();

    }
    else
    {
        if(!b_folderselected)
        {
            // Flag ..
            b_folderselected = false;
            // Acquisition Buttons ..
            ui->pb_refacq->setEnabled(false);
            ui->pb_specacq->setEnabled(false);
            ui->pb_newref->setEnabled(false);
            // Signal Control Buttons ..
            ui->sb_average->setEnabled(false);
            ui->sb_boxcar->setEnabled(false);
            ui->sb_inttime->setEnabled(false);
            ui->pb_stopavg->setEnabled(false);
            ui->chkb_elecdark->setEnabled(false);
            ui->pb_savemanzero->setEnabled(false);
            ui->pb_submanzero->setEnabled(false);
        }
    }
}

void Dialog::on_pb_refacq_clicked()
{
    if(graphpaused)
    {   QMessageBox::warning(this,"Saving Reference","Cannot save a white reference in pause mode",QMessageBox::Ok);
        return;
    }

    int resp = QMessageBox::information(this,"Saving Reference","INFORMATION:\nSaving a white reference.",QMessageBox::Ok|QMessageBox::Cancel);

    if (resp == QMessageBox::Ok)
    {
        // Before taking a reference we have to activate electrical dark correction ...
        if(!ui->chkb_elecdark->isChecked())
        {
            ui->chkb_elecdark->setChecked(true);
            on_chkb_elecdark_clicked();
            Sleep(500);
        }
        ui->pb_save_raw->setEnabled(false);
        ui->pb_refacq->setEnabled(false);
        ui->pb_specacq->setEnabled(true);
        ui->pb_newref->setEnabled(true);
        // Saving Reference & Normalization & Flag ...
        mythread->SavingControls(1,0,1);
        b_refacq = true;
        // Setting Range in Graph & Color ...
        ui->qcp_graph->yAxis->setRange(0,1.5);
        ui->qcp_graph->graph(0)->setPen(QPen(Qt::blue));
        // Updating status ...
        int number_file = mythread->GetReferenceFileNumber();
        QString numQS;
        numQS.sprintf("%03d",number_file);
       // ui->te_status->append("Saving White Reference R"+numQS+".txt");
    }
}

void Dialog::on_pb_newref_clicked()
{
    ui->pb_save_raw->setEnabled(true);
    ui->pb_refacq->setEnabled(true);
    ui->pb_specacq->setEnabled(false);
    ui->pb_newref->setEnabled(false);
    // Stoping Normalization & Flag
    mythread->SavingControls(0,0,0);
    b_refacq = false;
    // Setting Range in Graph & Color
    ui->qcp_graph->yAxis->setRange(0,maximumIntensity);
    ui->qcp_graph->graph(0)->setPen(QPen(Qt::red));
    // Updating Status ...
    ui->te_status->append(QString("Deleting White Reference ... done!"));
    ui->te_status->append(QString("You can grab a new White Reference"));
}

void Dialog::on_pb_playpause_clicked()
{
    mythread->ControlGraph(0);
    Sleep(500);
    mythread->ControlGraph(1);
    graphpaused = true;
}

void Dialog::on_pb_savemanzero_clicked()
{
    int resp = QMessageBox::information(this,"Saving Manual Zero","INFORMATION:\n Saving Manual Zero, please block the light",QMessageBox::Ok|QMessageBox::Cancel);

    if(resp == QMessageBox::Ok)
    {
        ui->pb_savemanzero->setEnabled(false);
        ui->pb_submanzero->setEnabled(true);
        ui->pb_newmanzero->setEnabled(false);
        mythread->ZeroControls(1,0);
        ui->te_status->append(QString("Saving Manual Zero ... done!"));
    }
}

void Dialog::on_pb_submanzero_clicked()
{
    ui->pb_savemanzero->setEnabled(false);
    ui->pb_submanzero->setEnabled(false);
    ui->pb_newmanzero->setEnabled(true);

    mythread->ZeroControls(0,1);
    ui->te_status->append(QString("Substracting Manual Zero ... done!"));
}

void Dialog::on_pb_newmanzero_clicked()
{
    ui->pb_savemanzero->setEnabled(true);
    ui->pb_submanzero->setEnabled(false);
    ui->pb_newmanzero->setEnabled(false);
    mythread->ZeroControls(0,0);
    ui->te_status->append(QString("Deleting Manual Zero ... done!"));
    ui->te_status->append(QString("You can select new Manual Zero"));
}

void Dialog::on_pb_cleanstatus_clicked()
{
    ui->te_status->clear();
}

void Dialog::on_pb_autoadjustment_clicked()
{
    mythread->CalibrationMode(1);
    ui->te_status->append("Adjusting graph wait for results ...");
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pb_about_clicked()
{
    AboutDlg->exec();
}

void Dialog::on_pb_save_raw_clicked()
{
    mythread->SaveRaw(1);

    ui->pb_save_raw->setEnabled(false);
    m_bRawAcqPressed = true;
    //

}

void Dialog::on_pb_specacq_clicked()
{
    if(graphpaused)
    {   QMessageBox::warning(this,"Saving Spectrum","WARNING:\nCannot save a spectrum in pause mode",QMessageBox::Ok);
        return;
    }
    // Saving Spectrum & Normalization & Flag
    m_bSaveStatus = false;
    mythread->SavingControls(0,1,1);

    ui->pb_specacq->setEnabled(false);
    m_bSpecAcqPressed = true;

}
void Dialog::ReceivedSavingFinished(bool SaveStatus)
{
    m_bSaveStatus = SaveStatus;


    // Updating status ...
    if(m_bSpecAcqPressed)
    {   int number_file = mythread->GetSpectrumFileNumber();
        number_file=number_file-1;
        QString numQS;
        numQS.sprintf("%03d",number_file);
        ui->te_status->append("Saving Spectrum S"+numQS+".txt");
        ui->pb_specacq->setEnabled(true);
        m_bSpecAcqPressed = false;
    }

    if(m_bRawAcqPressed)
    {   int number_file = mythread->GetRawSpectrumFileNumber();
        number_file=number_file-1;
        QString numQS;
        numQS.sprintf("%03d",number_file);
        ui->te_status->append("Saving Raw Spectrum W"+numQS+".txt");
        ui->pb_save_raw->setEnabled(true);
        m_bRawAcqPressed = false;
    }
}


