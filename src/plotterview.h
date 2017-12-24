/**
 * @file plotterview.h
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

    /**
     * Takes the visible portion of the graph,
     * and tries to fit it as snugly as possible within the given margins
     */
    void bestFit();
private:
    /**
     * Qt UI object that gives access the the UI form
     */
    Ui::PlotterView *ui;

    /**
     * The chart view Qt widget which lets us draw line graphs
     */
    QChartView* chartView;

    /**
     * The actual chart inside the chart widget on which we can plot lines
     */
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
     * The x value at which each line starts
     */
    QVector<int> linesStart;

    /**
     * The last plotted x-value of the given line
     */
    QVector<int> linesLastX;

    /**
     * The current x-value
     */
    int currX;

    /**
     * Given a minimum limit of the graph, calculates and sets a range [x, y],
     * where x is the newly calculated minimum just within the chart margin,
     * and y is the current maximum limit of the y-axis
     *
     * @param min the given minimum
     */
    inline void yMinSelect(const qreal& min);

    /**
     * Given a maximum limit of the graph, calculates and sets a range [x, y],
     * where y is the newly calculated maximum just within the chart margin,
     * and x is the current minimum limit of the y-axis
     *
     * @param max the given maximum
     */
    inline void yMaxSelect(const qreal& max);

    /**
     * Given a range [a,b], this calculates and sets the y-axis range to [x,y],
     * such that a and b are just within the chart margin
     *
     * @param a the lower limit of the range (minimum)
     * @param b the upper limit of the range (maximum)
     */
    inline void yRangeSelect(const qreal& a, const qreal& b);
};

#endif // PLOTTERVIEW_H
