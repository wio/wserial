/**
 * @file plotterview.cpp
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
// 10% margins on top and bottom
#define CHART_MARGIN 0.1

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
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundVisible(false);
    auto foregroundColor = QApplication::palette().text().color();
    chart->legend()->setLabelBrush(foregroundColor);

    ui->xRangeSpinBox->setValue(DEFAULTXRANGE);
    axisX->setRange(0, DEFAULTXRANGE);
    axisX->setLabelsBrush(foregroundColor);
    axisY->setRange(-YMAGNITUDEMAX, YMAGNITUDEMAX);
    axisY->setLabelsBrush(foregroundColor);
    chart->setAxisX(axisX);
    chart->setAxisY(axisY);

    connect(ui->clearButton, &QToolButton::released, this, &PlotterView::clear);
    connect(ui->xRangeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterView::handleChangeXRange);
    connect(ui->bestFitButton, &QToolButton::released, this, &PlotterView::bestFit);

#if defined _WIN32 || defined _WIN64 || defined(__APPLE__)
    ui->clearButton->setIcon(QIcon(":/icons/user-trash.svg"));
    ui->bestFitButton->setIcon(QIcon(":/icons/zoom-fit-best.svg"));
#endif
}

void PlotterView::bestFit() {
    int xRange = ui->xRangeSpinBox->value();
    if (lines.length() == 0) return;
    bool maxLeftFound = false;
    bool minLeftFound = false;
    bool maxRightFound = false;
    bool minRightFound = false;
    qreal maxLeft;
    qreal minLeft;
    qreal maxRight;
    qreal minRight;
    for (int i = 0; i < lines.length(); ++i) {
        auto line = lines[i];
        auto pts = line->pointsVector();
        int subtracted = currX - xRange;
        if (subtracted < 0) subtracted = 0;
        int lineStart = linesStart[i];
        subtracted -= lineStart;
        if (subtracted >= 0) {
            if (subtracted == pts.length()) {
                --subtracted;
            }
            qreal leftVal = pts[subtracted].y();
            if (minLeftFound) {
                if (leftVal < minLeft) {
                    minLeft = leftVal;
                }
            } else {
                minLeft = leftVal;
                minLeftFound = true;
            }
            if (maxLeftFound) {
                if (leftVal > maxLeft) {
                    maxLeft = leftVal;
                }
            } else {
                maxLeft = leftVal;
                maxLeftFound = true;
            }
        }
        subtracted = currX - lineStart;
        if (subtracted == pts.length()) {
            --subtracted;
        }
        qreal rightVal = pts[subtracted].y();
        if (minRightFound) {
            if (rightVal < minRight) {
                minRight = rightVal;
            }
        } else {
            minRight = rightVal;
            minRightFound = true;
        }
        if (maxRightFound) {
            if (rightVal > maxRight) {
                maxRight = rightVal;
            }
        } else {
            maxRight = rightVal;
            maxRightFound = true;
        }
    }

    /**
     *    1   |   2   |   3   |   4   |   5   |   6
     *     MR |    MR |    MR | ML    | ML    | ML
     *     mR |       |       |       |       | mL
     *        | ML    | ML    |    MR |    MR |
     *  ML    |    mR | mL    |    mR | mL    |    MR
     *  mL    | mL    |    mR | mL    |    mR |    mR
     */

    if (maxLeft <= maxRight) { // 1,2,3
        if (maxLeft <= minRight) {
            // 1
            yRangeSelect(minLeft, maxRight);
        } else if (minRight <= minLeft) {
            // 3
            yRangeSelect(minRight, maxRight);
        } else {
            // 2
            yRangeSelect(minLeft, maxRight);
        }
    } else if (maxRight <= maxLeft) { // 6,5,4
        if (maxRight <= minLeft) {
            // 6
            yRangeSelect(minRight, maxLeft);
        } else if (minLeft <= minRight) {
            // 4
            yRangeSelect(minLeft, maxLeft);
        } else {
            // 5
            yRangeSelect(minRight, maxLeft);
        }
    }
}

inline void PlotterView::yMinSelect(const qreal& min) {
    axisY->setMin((min - CHART_MARGIN * axisY->max()) / (1 - CHART_MARGIN));
}

inline void PlotterView::yMaxSelect(const qreal& max) {
    axisY->setMax((max - CHART_MARGIN * axisY->min()) / (1 - CHART_MARGIN));
}

/**
 * Let c := CHART_MARGIN
 *
 * y ------
 *     ^
 *     | -> c
 *     V
 * b ------
 *     ^
 *     | -> (1 - 2c)
 *     V
 * a ------
 *     ^
 *     | -> c
 *     V
 * x ------
 *
 * axisY->setRange(x, y)
 * where on solving:
 *
 * x = a - (b - a) * c / (1 - 2c)
 * y = b + (b - a) * c / (1 - 2c)
 *
 * c / (1 - 2c) is constant
 *
 */
inline void PlotterView::yRangeSelect(const qreal& a, const qreal& b) {
    axisY->setRange(a - (b - a) * CHART_MARGIN / (1 - 2 * CHART_MARGIN), b + (b - a) * CHART_MARGIN / (1 - 2 * CHART_MARGIN));
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
    linesStart.clear();
    linesLastX.clear();
    currX = 0;
    axisX->setRange(0, ui->xRangeSpinBox->value());
    axisY->setRange(-YMAGNITUDEMAX, YMAGNITUDEMAX);
}

void PlotterView::plotPoint(const qreal val, const int lineIndex, const bool increment) {
    QLineSeries* currLine;
    if (lineIndex == lines.length()) {
        // this is a new line
        currLine = new QLineSeries;
        lines << currLine;
        linesStart << currX;
        linesLastX << currX;
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
        if (position.x() > plotArea.right()) {
            // scroll point into view
            chart->scroll(position.x() - plotArea.right(), 0);
        }
        linesLastX[lineIndex] = currX;
    }

    currLine->setName(QString("%1").arg(val));
    // adjust the chart when the value is out of range
    if (val > axisY->max()) {
        if (ui->bestFitRadio->isChecked()) {
            bestFit();
        } else {
            yMaxSelect(val);
        }
    } else if (val < axisY->min()) {
        if (ui->bestFitRadio->isChecked()) {
            bestFit();
        } else {
            yMinSelect(val);
        }
    }
    if (increment) {
        for (int i = 0; i < lines.length(); ++i) {
            auto line = lines[i];
            if (linesLastX[i] != currX) {
                linesLastX[i] = currX;
                line->append(currX, 0);
                line->setName("0");
            }
        }
        ++currX;
    }
}

PlotterView::~PlotterView() {
    delete ui;
}
