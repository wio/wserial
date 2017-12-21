/**
 * @file mainwindow.h
 * @brief Implementation of PlotterView class
 *
 * The corresponding UI form is plotterview.ui
 *
 * @author Ambareesh Balaji
 * @date December 20, 2017
 * @bug No known bugs
 */

#include "plotterview.h"
#include "ui_plotterview.h"
#include <QToolButton>

#define DEFAULTXRANGE 500
#define YMAGNITUDEMAX 0.00001
#define CHARTMARGINFACTOR 1.1

using namespace QtCharts;

PlotterView::PlotterView(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlotterView),
    chartView(new QChartView),
    axisX(new QValueAxis),
    axisY(new QValueAxis),
    currX(0) {

    ui->setupUi(this);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui->verticalLayout->insertWidget(0, chartView);
    chart = chartView->chart();
    ui->xRangeSpinBox->setValue(DEFAULTXRANGE);
    axisX->setRange(0, DEFAULTXRANGE);
    axisY->setRange(-YMAGNITUDEMAX, YMAGNITUDEMAX);
    chart->setAxisX(axisX);
    chart->setAxisY(axisY);

    connect(ui->clearButton, &QToolButton::released, this, &PlotterView::clear);
    connect(ui->xRangeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterView::handleChangeXRange);
}

void PlotterView::handleChangeXRange(const int xRange) {
    // find the x value exactly one range before
    int oneRangeBefore = currX - xRange;
    if (oneRangeBefore >= 0) {
        // we have data exactly one range before, so we can adjust accordingly
        axisX->setRange(oneRangeBefore, currX);
    } else {
        // we set the range normally
        axisX->setRange(0, xRange);
    }
}

void PlotterView::clear() {
    chart->removeAllSeries();
    lines.clear();
    currX = 0;
    axisX->setRange(0, ui->xRangeSpinBox->value());
    axisY->setRange(-YMAGNITUDEMAX, YMAGNITUDEMAX);
}

void PlotterView::plotPoint(const qreal val, const int lineIndex, const bool increment) {
    // adjust the chart when the value is out of range
    if (val > axisY->max()) {
        axisY->setMax(val * CHARTMARGINFACTOR);
    } else if (val < axisY->min()) {
        axisY->setMin(val * CHARTMARGINFACTOR);
    }

    QLineSeries* currLine;
    if (lineIndex == lines.length()) {
        // this is a new line
        currLine = new QLineSeries;
        lines << currLine;
        currLine->setUseOpenGL();
        chart->addSeries(currLine);
        currLine->attachAxis(chart->axisX());
        currLine->attachAxis(chart->axisY());
        currLine->append(currX, val);
    } else {
        currLine = lines[lineIndex];
        QPointF newPoint(currX, val);
        currLine->append(newPoint);
        // get the position of the point on the chart;
        auto position = chart->mapToPosition(newPoint);
        auto plotArea = chart->plotArea();
        if (!plotArea.contains(position)) {
            // scroll point into view
            chart->scroll(position.x() - plotArea.right(), 0);
        }
    }

    currLine->setName(QString("%1").arg(val));
    if (increment) {
        ++currX;
    }
}

PlotterView::~PlotterView() {
    delete ui;
}
