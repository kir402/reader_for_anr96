#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <string>
#include <QtSerialPort>
#include "ANR.h"
#include<QFile>
#include<QTextStream>
#include<QtEndian>

#include<QtWidgets>
//#include <QIntValidator>
/*При создании формы обнуляется вызуальный статус проверки порта
 * и выполняется поиск доступных COM портов, а также заполняются структуры для запросов
 * */
QDate GL_DATE = QDate::currentDate();
QTimer * QRT;
const int ANR_SIZE = 7;         //количество пакетов на отправку
const int DATA_SIZE = 117;      //количество переменных
const int TIME_PERIOD = 100;   //период опроса и записи данных
const int SAVE_PERIOD = 60;
int CELL_2 = 2;     //глобальная переменная определяющая строку
QFile My_File;
QTextStream My_OUT;


ANR_WRITE ANR_LIST[ANR_SIZE];
QSerialPort serialPort;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_3->setText("Проверка порта: Не выполнена!");
    foreach (const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
           {
               ui->ComPortBox->addItem(serialPortInfo.portName());
           }
    ui->spinBox->setMinimum(1);
    ui->spinBox->setMaximum(255);
    QRT = new QTimer(this);
    connect(QRT,SIGNAL(timeout()), SLOT(slotQRT()));
}

MainWindow::~MainWindow()
{
    delete ui;
}



/*
 * При нажатии кнопки выполняется обнуление визуального статуса доступности порта
 * и обновляется список доступных COM портов
 */
void MainWindow::on_ComButton_clicked()
{
    ui->ComPortBox->clear();
    ui->label_3->setText("Проверка порта: Не выполнена!");
    foreach (const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
           {
               ui->ComPortBox->addItem(serialPortInfo.portName());
           }
}

/*
 * Проверка доступности выбранного порта на открытие в режиме чтение/запись
 */
void MainWindow::on_ComButton_2_clicked()
{

        serialPort.setPortName(ui->ComPortBox->currentText());
        serialPort.setBaudRate(QSerialPort::Baud9600);
        if (!serialPort.open(QIODevice::ReadWrite)) {

            ui->label_3->setText("Проверка порта: Порт недоступен");
               return;
           }

        ui->label_3->setText("Проверка порта: Порт открыт");
        serialPort.close();
}


void MainWindow::on_pushButton_3_clicked()//создать файл
{
    serialPort.setPortName(ui->ComPortBox->currentText());
    serialPort.setBaudRate(QSerialPort::Baud9600);
    if (!serialPort.open(QIODevice::ReadWrite)) {

        ui->label_3->setText("Проверка порта: Порт недоступен");
           return;
       }
    ui->label_6->setText("!!!!");
    for(int i = 0; i < ANR_SIZE; ++i)
    {
        ANR_LIST[i].ANR_Adress = ui->spinBox->value();
        ANR_LIST[i].ANR_CODE = 0x03;
        ANR_LIST[i].ANR_NUM_REG = My_SWAP(&ANR_NUM_REG_LIST[i]);
        ANR_LIST[i].ANR_REG_Adress = My_SWAP(&ADRESS_REG[i]);

        ANR_LIST[i].ANR_CRC16 = CRC16((unsigned char *)&ANR_LIST[i],6);
    }
    ui->label_6->setText("Готово");
    My_File.setFileName("Data\\" + QDate::currentDate().toString("dd.MM.yyyy")+ ".csv");
    My_File.open(QFile::ReadWrite | QFile::Text);
    My_OUT.setDevice(&My_File);
    My_OUT << NAME_REG[0]; //Запись "Время"
    for (int g = 1; g < DATA_SIZE; ++g) {
        My_OUT << '\t' << NAME_REG[g]; //Запись остального титула
    }


    QRT->start(TIME_PERIOD);
}

void MainWindow::on_pushButton_4_clicked()//закрыть файл
{
    QRT->stop();
    My_File.flush();
    My_File.close();
    serialPort.close();
    CELL_2 = 2;

}

void MainWindow::slotQRT()
{
   QRT->stop();
    if(!serialPort.isOpen())
    {
        ui->label->setText("Порт закрыт");
        My_File.flush();
        My_File.close();
        return;
    }
    //int CELL_1 = 2;
    QByteArray Data;
    My_OUT << QTime::currentTime().toString("hh:mm:ss"); // Запись времени
    for (int g = 0; g < ANR_SIZE; ++g) { //for ANR_SIZE
        serialPort.write((char *)&ANR_LIST[g], 8);
        while (serialPort.waitForReadyRead(msec_[g])) { //while
                    Data.append(serialPort.readAll());
                }//while

        float * buf64 = (float*)(Data.data() + 3);
        for (int q = 0; q < (int)(ANR_NUM_REG_LIST[g]/2); ++q)
        {//for q
            My_OUT << '\t' << qToBigEndian(buf64[q]);

        }//for q

    } //for ANR_SIZE
    My_OUT << '\n';
    ui->label_5->setText(QString::number(CELL_2 - 1));
    ++CELL_2;
    QDate Now_Date = QDate::currentDate();
    if (Now_Date != GL_DATE) { //if
        My_File.flush();
        My_File.close();
        My_File.setFileName("Data\\" + Now_Date.toString("dd.MM.yyyy") + ".cvs");
        My_File.open(QFile::ReadWrite | QFile::Text);
        My_OUT.setDevice(&My_File);
        My_OUT << NAME_REG[0];
        for (int g = 1; g < DATA_SIZE; ++g) {
            My_OUT << '\t' << NAME_REG[g];
        }
        CELL_2 = 2;
        GL_DATE = QDate::currentDate();
    }// if
    if ((CELL_2 % SAVE_PERIOD) == 0) My_File.flush();
    QRT->start(TIME_PERIOD);
}

void MainWindow::on_pushButton_clicked()//Пауза
{
    if(ui->pushButton->text() == "Пауза"){
        QRT->stop();
        My_File.flush();
        ui->pushButton->setText("Продолжить");
    } else if (ui->pushButton->text() == "Продолжить") {
        ui->pushButton->setText("Пауза");
        QRT->start(TIME_PERIOD);
    }

}
