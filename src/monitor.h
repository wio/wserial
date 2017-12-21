/**
 * @file monitor.h
 * @brief The class that monitors serial input
 *
 * @author Ambareesh Balaji
 * @date December 20, 2017
 * @bug No known bugs
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <QtSerialPort/QSerialPort>
#include <QThread>
#include <QMutex>

class Monitor : public QObject
{
    Q_OBJECT
public:
    /**
     * Mutex for thread synchronization
     */
    QMutex mutex;

    /**
     * Flag to tell `read` to return
     */
    bool abortFlag;

    /**
     * Flag to check if `open` has returned
     */
    bool openingFlag;

    /**
     * Flag for whether to generate and send data to the plotter
     */
    bool plotFlag;

    /**
     * Default constructor
     */
    Monitor():
        abortFlag(false),
        openingFlag(false),
        plotFlag(false)
    {}

    /**
     * Serial port object
     */
    QSerialPort m_serialPort;

signals:
    void output(const QString& val);
    void error(const QString& err, bool monitorReset = false, bool warning = false);
    void plotPoint(const qreal val, const int index, bool increment);
    void incrementPlot();

public slots:

    /**
     * Reads data from serial, and parses numbers for the plotter
     *
     * Emits `output` to update the text box
     * Emits `plotPoint` to update the plotter
     */
    void readAndParse();

    /**
     * Tries to open the serial port, emits `error` on failure
     */
    void open();
};

#endif // MONITOR_H
