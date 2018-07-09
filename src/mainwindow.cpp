/**
 * @file mainwindow.cpp
 * @brief Implementation of MainWindow class
 *
 * The corresponding UI form is mainwindow.ui
 *
 * @author Ambareesh Balaji
 * @date December 20, 2017
 * @bug No known bugs
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtSerialPort/QSerialPort>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>
#include <QThread>
#include <QScrollBar>
#include <QListWidget>
#include <QMessageBox>
#include <QLineEdit>

// Logging modes
#define QMESSAGE 0
#define QDEBUG 1
#define STDOUT 2
#define STDERR 3
#define SILENT 4

MainWindow::MainWindow(const QString& port, const QString& baudRate, const bool immediate) :
    ui(new Ui::MainWindow),
    m_plotterView(nullptr),
    m_readFirstPass(true),
    m_monitorVerticalScrollBarGrabbing(false) {

    ui->setupUi(this);
    qint32 baudRates[] = {QSerialPort::Baud1200, QSerialPort::Baud2400, QSerialPort::Baud4800, QSerialPort::Baud9600, QSerialPort::Baud19200, QSerialPort::Baud38400, QSerialPort::Baud57600, QSerialPort::Baud115200};

    const int parsedRate = baudRate.toInt();
    int baudRateIndex = 3; // 9600 by default
    int i = 0;
    for (auto baudRate : baudRates) {
        if (parsedRate == baudRate) {
            baudRateIndex = i;
        }
        ui->baudRate->addItem(QString::number(baudRate), baudRate);
        i++;
    }
    ui->baudRate->setCurrentIndex(baudRateIndex);

    connect(ui->monitorButton, &QToolButton::toggled, this, &MainWindow::handleMonitorToggled);
    connect(ui->clearButton, &QToolButton::released, ui->plainTextEdit, &QPlainTextEdit::clear);
    connect(ui->sendButton, &QToolButton::released, this, &MainWindow::handleSend);
    connect(ui->portReload, &QToolButton::released, this, &MainWindow::handleReloadPorts);
    connect(ui->port, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::handlePortChanged);
    connect(ui->baudRate, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::handleBaudRateChanged);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::handleSend);
    connect(ui->plotterButton, &QToolButton::toggled, this, &MainWindow::handlePlotterToggled);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

    connect(ui->plainTextEdit->verticalScrollBar(), &QScrollBar::sliderPressed, this, &MainWindow::handleSliderPressed);
    connect(ui->plainTextEdit->verticalScrollBar(), &QScrollBar::sliderReleased, this, &MainWindow::handleSliderReleased);

    m_worker = new Worker;
    m_worker->moveToThread(&m_workerThread);
    m_workerThread.start();
    connect(this, &MainWindow::sendToWorker, m_worker, &Worker::processData);
    if (loadPortsAndSet(port) && immediate) {
        ui->monitorButton->setChecked(true);
        startMonitor();
    }

    ui->clearButton->setIcon(QIcon::fromTheme("edit-clear", QIcon(":/icons/edit-clear.svg")));
    ui->plotterButton->setIcon(QIcon::fromTheme("application-graphics", QIcon(":/icons/applications-graphics.svg")));
    ui->sendButton->setIcon(QIcon::fromTheme("network-transmit", QIcon(":/icons/network-transmit.svg")));
    ui->monitorButton->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/icons/media-playback-start.svg")));
    ui->portReload->setIcon(QIcon::fromTheme("reload", QIcon(":/icons/reload.svg")));
}

void MainWindow::closeEvent(QCloseEvent*) {
    stopMonitor();
    delete m_worker;
    m_workerThread.quit();
    m_workerThread.wait();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::handlePortChanged(int index) {
    resetMonitor();
    if (index != -1) {
        m_serialPort.setPortName(m_availablePorts[index].portName());
    }
}

void MainWindow::handleBaudRateChanged(int) {
    m_serialPort.setBaudRate(ui->baudRate->currentData().toInt());
}

void MainWindow::handleReloadPorts() {
    ui->port->clear();
    if (ui->monitorButton->isChecked()) {
        resetMonitor();
    }
    loadPortsAndSet();
}

void MainWindow::handleMonitorToggled(bool checked) {
    if (checked) {
        startMonitor();
    } else {
        stopMonitor();
    }
}

void MainWindow::handleReadyRead() {
    // the first pass generally has corrupted data, so clear the serial port's buffer
    if (m_readFirstPass) {
        m_readFirstPass = false;
        m_serialPort.clear();
        return;
    }
    QByteArray buf = m_serialPort.readAll();
    if (buf.length() > 0) {
        emit sendToWorker(buf);
    }
}

void MainWindow::handlePlotterToggled(bool checked) {
    if (checked) {
        if (m_plotterView == nullptr) {
            m_plotterView = new PlotterView(this);
        }
        connect(m_worker, &Worker::plotPoint, m_plotterView, &PlotterView::plotPoint);
        connect(m_plotterView, &PlotterView::finished, ui->plotterButton, &QToolButton::setChecked);
        m_plotterView->move(x() + 10 + width(), y());
        m_plotterView->show();
        m_worker->plotEnabled = true;
    } else {
        m_worker->plotEnabled = false;
        m_plotterView->close();
        disconnect(m_worker, &Worker::plotPoint, m_plotterView, &PlotterView::plotPoint);
        disconnect(m_plotterView, &PlotterView::finished, ui->plotterButton, &QToolButton::setChecked);
    }
}

void MainWindow::handleSend() {
    if (ui->lineEdit->text().length() != 0 &&
            m_availablePorts.length() != 0 &&
            tryOpen()) {
        m_serialPort.write(ui->lineEdit->text().toUtf8());
        if (!m_serialPort.error()) {
            ui->lineEdit->clear();
        }
    }
}

void MainWindow::handleSliderPressed() {
    m_monitorVerticalScrollBarGrabbing = true;
}

void MainWindow::handleSliderReleased() {
    m_monitorVerticalScrollBarGrabbing = false;
}

void MainWindow::handleError(QSerialPort::SerialPortError err) {
    if (err != QSerialPort::NoError) {
        resetMonitor();
    }
    switch(err) {
    case QSerialPort::DeviceNotFoundError:
        loadPortsAndSet();
        outputError("Device not found");
        break;
    case QSerialPort::PermissionError:
        outputError("Not enough permissions to access the device");
        break;
    case QSerialPort::OpenError:
        // this should never occur, else the code is buggy
        outputError("Device is already open in this program");
        break;
    case QSerialPort::NotOpenError:
        // this should never occur either
        outputError("Device is not open before performing actions");
        break;
    case QSerialPort::WriteError:
        outputError("Failed to write to device");
        break;
    case QSerialPort::ReadError:
        outputError("Failed to read from device");
        break;
    case QSerialPort::ResourceError:
        outputError("Failed to access device");
        break;
    case QSerialPort::QSerialPort::UnsupportedOperationError:
        outputError("Operation not supported, or prohibited by the OS");
        break;
    case QSerialPort::TimeoutError:
        // this should never happen either, since we are not polling the device
        outputError("Operation timed out");
        break;
    case QSerialPort::UnknownError:
        outputError("An unknown error occured");
    default:
        break;
    }
}

void MainWindow::output(const QString& val) {
    auto pte = ui->plainTextEdit;
    auto sb = ui->plainTextEdit->verticalScrollBar();
    auto sbVal = sb->value();
    pte->moveCursor(QTextCursor::End);
    pte->insertPlainText(val);
    pte->moveCursor(QTextCursor::End);
    if (ui->autoScroll->checkState() && !m_monitorVerticalScrollBarGrabbing) {
        sb->setValue(sb->maximum());
    } else {
        sb->setValue(sbVal);
    }
}

bool MainWindow::loadPortsAndSet(const QString& initialPort) {
    m_availablePorts = QSerialPortInfo::availablePorts();
    m_serialPort.setBaudRate(ui->baudRate->currentData().toInt());
    if (m_availablePorts.length() == 0) {
        ui->sendButton->setEnabled(false);
        ui->monitorButton->setEnabled(false);
        return false;
    } else {
        ui->sendButton->setEnabled(true);
        ui->monitorButton->setEnabled(true);
        int index = 0;
        for (int i = 0; i < m_availablePorts.length(); ++i) {
            auto portInfo = m_availablePorts[i];
            if (initialPort == portInfo.portName()) {
                index = i;
            }
            ui->port->addItem(QString("%1: %2").arg(portInfo.manufacturer()).arg(portInfo.portName()), i);
        }
        ui->port->setCurrentIndex(index);
        m_serialPort.setPortName(m_availablePorts[index].portName());
        return true;
    }
}

inline void MainWindow::startMonitor() {
    if (tryOpen()) {
        m_readFirstPass = true;
        connect(m_worker, &Worker::output, this, &MainWindow::output);
        connect(&m_serialPort, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
    }
}

inline void MainWindow::stopMonitor() {
    if (m_serialPort.isOpen()) {
        m_serialPort.close();
    }
    disconnect(&m_serialPort, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
    disconnect(m_worker, &Worker::output, this, &MainWindow::output);
}

inline void MainWindow::resetMonitor() {
    stopMonitor();
    ui->monitorButton->setChecked(false);
}

inline bool MainWindow::tryOpen() {
    return m_serialPort.isOpen() || m_serialPort.open(QIODevice::ReadWrite);
}

inline void MainWindow::outputError(const QString& errMesg) {
#if LOGGING_MODE == QMESSAGE
    QMessageBox::critical(this, "Error", errMesg, QMessageBox::Ok);
#elif LOGGING_MODE == QDEBUG
    qDebug() << errMesg;
#elif LOGGING_MODE == STDOUT
    QTextStream(stdout) << errMesg;
#elif LOGGING_MODE == STDERR
    QTextStream(stderr) << errMesg;
#elif LOGGING_MODE == SILENT
    Q_UNUSED(errMesg);
#endif
}
