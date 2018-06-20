/**
 * @file worker.cpp
 * @brief Implementation of Worker class
 *
 * @author Ambareesh Balaji
 * @date December 24, 2017
 * @bug No known bugs
 */

#include "worker.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

Worker::Worker() : plotEnabled(false) {}

void Worker::processData(const QByteArray& buf) {
    const QString cur = QString::fromUtf8(buf);
    emit output(cur);
    if (plotEnabled) {
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
                    emit plotPoint(qreal(number), i, false);
                }
            }
            QString word = words[i];
            bool ok;
            double number = word.toDouble(&ok);
            if (ok) {
                // true to increment the plot after last number in the line
                emit plotPoint(qreal(number), i, true);
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
