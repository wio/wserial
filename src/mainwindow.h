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
#include <QtSerialPort/QSerialPortInfo>
#include "monitor.h"
#include "plotterview.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void startMonitor();
    void fetch();
    void open();

public slots:
    /**
     * Handles updating the monitor output text box
     *
     * @param val string to output
     */
    void handleOutput(const QString& val);

    /**
     * Handles I/O errors
     *
     * @param err the error message
     * @param monitorReset whether to call `resetMonitor`
     * @param warning whether this is a warning
     */
    void handleError(const QString& err, bool monitorReset = false, bool warning = false);

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
     * Handles toggles to the monitor button
     *
     * If checked, calls tryOpen and on success emits signal `startMonitor` to monitor serial input
     * Else, calls `stopMonitor` to stop monitoring serial input
     *
     * @param checked whether the button is checked
     */
    void handleMonitorToggle(bool checked);

    /**
     * Handles toggles to the plotter button
     *
     * If checked, opens the plotter in a new window beside the main window
     * Else, closes the plotter window
     *
     * @param checked whether the button is checked
     */
    void handlePlotter(bool checked);

    /**
     * Handles changes to the port combo box
     *
     * @param newIndex the current index selected in the combo box
     */

    /**
     * Handles changes to the port combo box
     *
     * Calls `resetMonitor`, and then sets the port if index is valid
     *
     * @param newIndex the current index selected in the combo box
     */
    void handlePortChanged(int newIndex);

    /**
     * Handles changes to the baud rate combo box
     *
     * If index is valid, obtains the monitor mutex lock and sets the baud rate
     *
     * @param newIndex the current index selected in the combo box
     */
    void handleBaudRateChanged(int);

private:
    void closeEvent(QCloseEvent*) override;

private:
    Ui::MainWindow *ui;

    /**
     * List of available ports from last query
     */
    QList<QSerialPortInfo> m_availablePorts;

    /**
     * Pointer to the Monitor object monitoring the serial input
     */
    Monitor* m_monitor;

    /**
     * Worker thread for the monitor
     */
    QThread m_monitorThread;

    /**
     * Pointer to the plotter view dialog
     */
    PlotterView* m_plotterView;

    /**
     * Fetches current port information, updates ui, and sets the current values in the monitor
     */
    void loadPortsAndSet();

    /**
     * Tells the monitor to stop and waits for the thread to finish executing
     */
    inline void stopMonitor();

    /**
     * Calls `stopMonitor`, then resets the monitor button
     */
    inline void resetMonitor();

    /**
     * Tells the monitor thread to open, waits, and generates an error on failure
     */
    inline bool tryOpen();
};

#endif // MAINWINDOW_H
