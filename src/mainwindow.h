/**
 * @file mainwindow.h
 * @brief The main window of WSerial
 *
 * The corresponding UI form is mainwindow.ui
 *
 * @author Ambareesh Balaji
 * @date December 20, 2017
 * @bug No known bugs
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "plotterview.h"
#include <QThread>
#include "worker.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    /**
     * Handles changes to the baud rate combo box
     *
     * If index is valid, obtains the monitor mutex lock and sets the baud rate
     *
     * @param newIndex the current index selected in the combo box
     */
    void handleBaudRateChanged(int);

    /**
     * Handles serial port errors
     *
     * @param err the error enum which corresponds to the error
     */
    void handleError(QSerialPort::SerialPortError err);

    /**
     * Handles toggles to the monitor button
     *
     * If checked, calls `startMonitor` to monitor serial input
     * Else, calls `stopMonitor` to stop monitoring serial input
     *
     * @param checked whether the button is checked
     */
    void handleMonitorToggled(bool checked);

    /**
     * Handles toggles to the plotter button
     *
     * If checked, opens the plotter in a new window beside the main window
     * Else, closes the plotter window
     *
     * @param checked whether the button is checked
     */
    void handlePlotterToggled(bool checked);

    /**
     * Handles changes to the port combo box
     *
     * Calls `resetMonitor`, and then sets the port if index is valid
     *
     * @param newIndex the current index selected in the combo box
     */
    void handlePortChanged(int newIndex);

    /**
     *  Reads new data from the port when available
     *  and sends it to the worker
     */
    void handleReadyRead();

    /**
     * Handles reloading the available ports
     *
     * Resets UI combo box and calls `loadPorts`
     */
    void handleReloadPorts();

    /**
     * Handles writing output to board
     *
     * Uses the value in the text box, which is cleared on success
     */
    void handleSend();

    /**
     * Updates the textbox, and handles auto-scrolling
     *
     * @param val new string to be appended to the textbox
     */
    void output(const QString& val);

signals:
    /**
     * Sends the newly read input buffer to the worker for processing
     *
     * @param buf
     */
    void sendToWorker(const QByteArray& buf);

private:
    /**
     * Worker object which processes incoming data off the main thread
     */
    Worker* worker;

    /**
     * Thread for the worker object
     */
    QThread workerThread;

    /**
     * Qt UI object that gives access the the UI form
     */
    Ui::MainWindow *ui;

    /**
     * List of available ports from last query
     */
    QList<QSerialPortInfo> m_availablePorts;

    /**
     * Serial port object for serial communication
     */
    QSerialPort m_serialPort;

    /**
     * Pointer to the plotter view dialog
     */
    PlotterView* m_plotterView;

    /**
     * Whether or not this is the first read after we started listening
     *
     * Used to clear the buffer, which may have old, corrupted data
     */
    bool m_readFirstPass;

    /**
     * Whether the user is currently grabbing the scrollbar in the monitor
     *
     * Lets the user scroll up while autoscrolling is still enabled
     */
    bool m_monitorVerticalScrollBarGrabbing;

    /**
     * An event handler for when the main window is closed
     *
     * Used to clean up before closing
     */
    void closeEvent(QCloseEvent*) override;

    /**
     * Fetches current port information, updates ui, and sets the current values in the monitor
     */
    void loadPortsAndSet();

    /**
     * Updates the monitor output text box
     *
     * @param val string to output
     */

    /**
     * Opens the serial port of not already open
     *
     * @return whether the port is open without any errors
     */
    inline bool tryOpen();

    /**
     * Starts listening to input from the serial port
     */
    inline void startMonitor();

    /**
     * Stops listening to input from the serial port, closes the serial port is open
     */
    inline void stopMonitor();

    /**
     * Calls `stopMonitor`, then resets the monitor button
     */
    inline void resetMonitor();

    /**
     * A helper function that creates a new QMessageBox::critical with the error
     */
    inline void outputError(const QString& errMesg);
};

#endif // MAINWINDOW_H
