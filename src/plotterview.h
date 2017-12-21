/**
 * @file mainwindow.h
 * @brief The dialog box of the plotter, manages drawing and updating the chart
 *
 * The corresponding UI form is plotterview.ui
 *
 * @author Ambareesh Balaji
 * @date December 20, 2017
 * @bug No known bugs
 */

#ifndef PLOTTERVIEW_H
#define PLOTTERVIEW_H

#include <QDialog>
#include <QtCharts>
#include <QVector>

using namespace QtCharts;

namespace Ui {
class PlotterView;
}

class PlotterView : public QDialog {
    Q_OBJECT
public:
    explicit PlotterView(QWidget *parent = 0);
    ~PlotterView();

public slots:
    /**
     * Plots the given value on the chart
     *
     * @param val y-value of the point to plot
     * @param lineIndex index of the line to plot the point on
     * @param increment whether to increment the currX
     */
    void plotPoint(const qreal val, const int lineIndex, const bool increment);

    /**
     * Clears the chart
     */
    void clear();

    /**
     * Handles changes to the x-axis range
     *
     * @param xRange the new x-axis range
     */
    void handleChangeXRange(const int xRange);

private:
    Ui::PlotterView *ui;
    QChartView* chartView;
    QChart* chart;

    /**
     * x-axis
     */
    QValueAxis* axisX;

    /**
     * y-axis
     */
    QValueAxis* axisY;

    /**
     * The list of line series
     */
    QVector<QLineSeries*> lines;

    /**
     * The current x-value
     */
    int currX;
};

#endif // PLOTTERVIEW_H
