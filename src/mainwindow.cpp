/**
 * @file mainwindow.h
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
#include <QDebug>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>
#include <QThread>
#include <QScrollBar>
#include <QListWidget>
#include <QMessageBox>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_plotterView(nullptr),
    m_readFirstPass(true),
    m_monitorVerticalScrollBarGrabbing(false) {

    ui->setupUi(this);
    QList<qint32> baudRates {QSerialPort::Baud1200, QSerialPort::Baud2400, QSerialPort::Baud4800, QSerialPort::Baud9600, QSerialPort::Baud19200, QSerialPort::Baud38400, QSerialPort::Baud57600, QSerialPort::Baud115200};
    for (auto baudRate : baudRates) {
        ui->baudRate->addItem(QString("%1").arg(baudRate), baudRate);
    }
    ui->baudRate->setCurrentIndex(3);
    loadPortsAndSet();

    connect(ui->monitorButton, &QToolButton::toggled, this, &MainWindow::handleMonitorToggled);
    connect(ui->clearButton, &QToolButton::released, ui->plainTextEdit, &QPlainTextEdit::clear);
    connect(ui->sendButton, &QToolButton::released, this, &MainWindow::handleSend);
    connect(ui->portReload, &QToolButton::released, this, &MainWindow::handleReloadPorts);
    connect(ui->port, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &MainWindow::handlePortChanged);
    connect(ui->baudRate, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &MainWindow::handleBaudRateChanged);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::handleSend);
    connect(ui->plotterButton, &QToolButton::toggled, this, &MainWindow::handlePlotterToggled);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

    connect(ui->plainTextEdit->verticalScrollBar(), &QScrollBar::sliderPressed, [this]() {
        m_monitorVerticalScrollBarGrabbing = true;
    });
    connect(ui->plainTextEdit->verticalScrollBar(), &QScrollBar::sliderReleased, [this]() {
        m_monitorVerticalScrollBarGrabbing = false;
    });

// Apply bundled icons on Windows and macOS
#if defined _WIN32 || defined __APPLE__
    ui->clearButton->setIcon(QIcon(":/icons/edit-clear.svg"));
    ui->plotterButton->setIcon(QIcon(":/icons/applications-graphics.svg"));
    ui->sendButton->setIcon(QIcon(":/icons/network-transmit.svg"));
    ui->monitorButton->setIcon(QIcon(":/icons/network-receive.svg"));
    ui->portReload->setIcon(QIcon(":/icons/reload.svg"));
#endif
}

void MainWindow::closeEvent(QCloseEvent*) {
    stopMonitor();
    if (m_plotterView != nullptr) {
        m_plotterView->close();
        delete m_plotterView;
    }
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
        const QString cur = QString::fromUtf8(buf);
        handleOutput(cur);
        if (ui->plotterButton->isChecked()) {
            // this regular expression checks for a comma/tab/space separated list
            // of decimal numbers followed by either CRLF or LF
            QRegularExpression re("(-?\\d+(\\.\\d+)?([, \\t]+-?\\d+(\\.\\d+)?)*)\\r?\\n");
            // if the output was broken up into separate packets
            // we need to keep track of the previous leftover value
            QString& combined = m_leftover += cur;
            QRegularExpressionMatchIterator matches = re.globalMatch(combined);
            int lastMatchIndex = -1;
            while (matches.hasNext()) {
                QRegularExpressionMatch match = matches.next();
                QString line = match.captured(1);
                QStringList words = line.split(QRegularExpression("[, \\t]+"));

                int i = 0;
                for (; i < words.length() - 1; ++i) {
                    QString word = words[i];
                    bool ok;
                    double number = word.toDouble(&ok);
                    if (ok) {
                        m_plotterView->plotPoint(qreal(number), i, false);
                    }
                }
                QString word = words[i];
                bool ok;
                double number = word.toDouble(&ok);
                if (ok) {
                    // true to increment the plot after last number in the line
                    m_plotterView->plotPoint(qreal(number), i, true);
                }
                // this returns the first index after the last match
                // or -1 if nothing was matched
                lastMatchIndex = match.capturedEnd(0);
            }
            if (lastMatchIndex == -1) ++lastMatchIndex;
            // mid returns the substring starting at lastMatchIndex to the end
            m_leftover = combined.mid(lastMatchIndex);
        }
    }
}

void MainWindow::handlePlotterToggled(bool checked) {
    if (checked) {
        m_plotterView = new PlotterView;
        m_plotterView->move(x() + 10 + width(), y());
        m_plotterView->show();
        connect(m_plotterView, &PlotterView::finished, ui->plotterButton, &QToolButton::setChecked);
    } else {
        m_plotterView->close();
        delete m_plotterView;
        m_plotterView = nullptr;
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

void MainWindow::handleOutput(const QString& val) {
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

void MainWindow::loadPortsAndSet() {
    m_availablePorts = QSerialPortInfo::availablePorts();
    if (m_availablePorts.length() == 0) {
        ui->sendButton->setEnabled(false);
        ui->monitorButton->setEnabled(false);
    } else {
        ui->sendButton->setEnabled(true);
        ui->monitorButton->setEnabled(true);
        for (int i = 0; i < m_availablePorts.length(); ++i) {
            auto portInfo = m_availablePorts[i];
            ui->port->addItem(QString("%1: %2").arg(portInfo.manufacturer()).arg(portInfo.portName()), i);
        }
        m_serialPort.setPortName(m_availablePorts[ui->port->currentIndex()].portName());
    }
    m_serialPort.setBaudRate(ui->baudRate->currentData().toInt());
}

inline void MainWindow::startMonitor() {
    if (tryOpen()) {
        m_readFirstPass = true;
        connect(&m_serialPort, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
    }
}

inline void MainWindow::stopMonitor() {
    if (m_serialPort.isOpen()) {
        m_serialPort.close();
    }
    disconnect(&m_serialPort, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
}

inline void MainWindow::resetMonitor() {
    stopMonitor();
    ui->monitorButton->setChecked(false);
}

inline bool MainWindow::tryOpen() {
    return m_serialPort.isOpen() || m_serialPort.open(QIODevice::ReadWrite);
}

inline void MainWindow::outputError(const QString& errMesg) {
    QMessageBox::critical(this, "Error", errMesg, QMessageBox::Ok);
}
