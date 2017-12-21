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
    m_monitor(new Monitor),
    m_plotterView(nullptr) {

    ui->setupUi(this);
    QList<qint32> baudRates {QSerialPort::Baud1200, QSerialPort::Baud2400, QSerialPort::Baud4800, QSerialPort::Baud9600, QSerialPort::Baud19200, QSerialPort::Baud38400, QSerialPort::Baud57600, QSerialPort::Baud115200};
    for (auto baudRate : baudRates) {
        ui->baudRate->addItem(QString("%1").arg(baudRate), baudRate);
    }
    ui->baudRate->setCurrentIndex(3);
    loadPortsAndSet();

    connect(this, &MainWindow::startMonitor, m_monitor, &Monitor::readAndParse);
    m_monitor->moveToThread(&m_monitorThread);
    m_monitorThread.start();

    connect(ui->monitorButton, &QToolButton::toggled, this, &MainWindow::handleMonitorToggle);
    connect(ui->clearButton, &QToolButton::released, ui->plainTextEdit, &QPlainTextEdit::clear);
    connect(ui->sendButton, &QToolButton::released, this, &MainWindow::handleSend);
    connect(ui->portReload, &QToolButton::released, this, &MainWindow::handleReloadPorts);
    connect(ui->port, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &MainWindow::handlePortChanged);
    connect(this, &MainWindow::open, m_monitor, &Monitor::open);
    connect(ui->baudRate, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &MainWindow::handleBaudRateChanged);
    connect(m_monitor, &Monitor::output, this, &MainWindow::handleOutput);
    connect(m_monitor, &Monitor::error, this, &MainWindow::handleError);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::handleSend);
    connect(ui->plotterButton, &QToolButton::toggled, this, &MainWindow::handlePlotter);

#if defined _WIN32 || defined __APPLE__
    ui->clearButton->setIcon(QIcon(":/icons/edit-clear.svg"));
    ui->plotterButton->setIcon(QIcon(":/icons/applications-graphics.svg"));
    ui->sendButton->setIcon(QIcon(":/icons/network-transmit.svg"));
    ui->monitorButton->setIcon(QIcon(":/icons/network-receive.svg"));
    ui->portReload->setIcon(QIcon(":/icons/reload.svg"));
#endif
}

void MainWindow::handleReloadPorts() {
    ui->port->clear();
    if (ui->monitorButton->isChecked()) {
        resetMonitor();
    }
    loadPortsAndSet();
}

inline bool MainWindow::tryOpen() {
    if (!m_monitor->m_serialPort.isOpen()) {
        m_monitor->openingFlag = true;
        emit open();
        while (m_monitor->openingFlag);
        if (!m_monitor->m_serialPort.isOpen()) return false;
    }
    return true;
}

void MainWindow::handleMonitorToggle(bool checked) {
    if (checked) {
        if (tryOpen()) {
            emit startMonitor();
        }
    } else {
        stopMonitor();
    }
}

void MainWindow::handlePlotter(bool checked) {
    if (checked) {
        m_plotterView = new PlotterView;
        m_plotterView->move(x() + 10 + width(), y());
        m_plotterView->show();
        connect(m_plotterView, &PlotterView::finished, ui->plotterButton, &QToolButton::setChecked);
        connect(m_monitor, &Monitor::plotPoint, m_plotterView, &PlotterView::plotPoint);
    } else {
        m_plotterView->close();
        delete m_plotterView;
        m_plotterView = nullptr;
    }
    m_monitor->plotFlag = checked;
}

void MainWindow::handleSend() {
    if (ui->lineEdit->text().length() != 0 &&
            m_availablePorts.length() != 0 &&
            tryOpen()) {
        m_monitor->m_serialPort.write(ui->lineEdit->text().toUtf8());
        if (m_monitor->m_serialPort.error() == QSerialPort::WriteError || !m_monitor->m_serialPort.waitForBytesWritten(5000)) {
            handleError("Failed to write to port");
        }

        ui->lineEdit->clear();
    }
}

void MainWindow::handleError(const QString& err, bool monitorReset, bool warning) {
    if (monitorReset) {
        resetMonitor();
    }
    if (warning) {
        QMessageBox::warning(this, "Warning", err, QMessageBox::Ok);
    } else {
        QMessageBox::critical(this, "Error", err, QMessageBox::Ok);
    }
}

void MainWindow::handleOutput(const QString& val) {
    auto pte = ui->plainTextEdit;
    auto sb = ui->plainTextEdit->verticalScrollBar();
    auto sbVal = sb->value();
    pte->moveCursor(QTextCursor::End);
    pte->insertPlainText(val);
    pte->moveCursor(QTextCursor::End);
    if (ui->autoScroll->checkState()) {
        sb->setValue(sb->maximum());
    } else {
        sb->setValue(sbVal);
    }
}

void MainWindow::closeEvent(QCloseEvent*) {
    stopMonitor();
    m_monitorThread.quit();
    m_monitorThread.wait();
    delete m_monitor;
    if (m_plotterView != nullptr) {
        handlePlotter(false);
    }
}

MainWindow::~MainWindow() {
    delete ui;
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
        m_monitor->m_serialPort.setPortName(m_availablePorts[ui->port->currentIndex()].portName());
    }
    m_monitor->m_serialPort.setBaudRate(ui->baudRate->currentData().toInt());
}

void MainWindow::handlePortChanged(int index) {
    resetMonitor();
    if (index != -1) {
        m_monitor->m_serialPort.setPortName(m_availablePorts[index].portName());
    }
}

void MainWindow::handleBaudRateChanged(int) {
    m_monitor->mutex.lock();
    m_monitor->m_serialPort.setBaudRate(ui->baudRate->currentData().toInt());
    m_monitor->mutex.unlock();
}

inline void MainWindow::stopMonitor() {
    m_monitor->abortFlag = true;
    m_monitor->mutex.lock();
    m_monitor->abortFlag = false;
    m_monitor->mutex.unlock();
}

inline void MainWindow::resetMonitor() {
    stopMonitor();
    ui->monitorButton->setChecked(false);
}
