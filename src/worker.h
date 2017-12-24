/**
 * @file worker.h
 * @brief Worker class for processing input in a separate thread
 *
 * @author Ambareesh Balaji
 * @date December 20, 2017
 * @bug No known bugs
 */

#ifndef WORKER_H
#define WORKER_H

#include <QObject>

class Worker : public QObject
{
    Q_OBJECT
public:
    /**
     * Default constructor, takes no arguments
     */
    Worker();

    /**
     * Whether the plotter is enabled, set by main thread
     */
    bool plotEnabled;

signals:
    void output(const QString& val);
    void plotPoint(const qreal, const int, const bool);

public slots:
    /**
     * Processes the given buffer, and scans and parses numbers when the plotter is enabled
     *
     * @param buf the input buffer
     */
    void processData(const QByteArray& buf);

private:
    /**
     * Data left over from the last job when scanning for numbers
     */
    QString m_leftover;
};

#endif // WORKER_H
