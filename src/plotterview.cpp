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
    m_chartView(new QChartView),
    m_axisX(new QValueAxis),
    m_axisY(new QValueAxis),
    m_currX(0) {

    ui->setupUi(this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    ui->verticalLayout->insertWidget(0, m_chartView);

    m_chart = m_chartView->chart();
    m_chart->layout()->setContentsMargins(0, 0, 0, 0);
    m_chart->setBackgroundVisible(false);
    auto foregroundColor = QApplication::palette().text().color();
    m_chart->legend()->setLabelBrush(foregroundColor);

    ui->xRangeSpinBox->setValue(DEFAULTXRANGE);
    m_axisX->setRange(0, DEFAULTXRANGE);
    m_axisX->setLabelsBrush(foregroundColor);
    m_axisY->setRange(-YMAGNITUDEMAX, YMAGNITUDEMAX);
    m_axisY->setLabelsBrush(foregroundColor);
    m_chart->setAxisX(m_axisX);
    m_chart->setAxisY(m_axisY);

    connect(ui->clearButton, &QToolButton::released, this, &PlotterView::clear);
    connect(ui->xRangeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterView::handleChangeXRange);
    connect(ui->bestFitButton, &QToolButton::released, this, &PlotterView::bestFit);

    ui->clearButton->setIcon(QIcon::fromTheme("user-trash", QIcon(":/icons/user-trash.svg")));
    ui->bestFitButton->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/icons/zoom-fit-best.svg")));
}

void PlotterView::bestFit() {
    int xRange = ui->xRangeSpinBox->value();
    if (m_lines.length() == 0) return;
    bool maxLeftFound = false;
    bool minLeftFound = false;
    bool maxRightFound = false;
    bool minRightFound = false;
    qreal maxLeft;
    qreal minLeft;
    qreal maxRight;
    qreal minRight;
    for (int i = 0; i < m_lines.length(); ++i) {
        auto line = m_lines[i];
        auto pts = line->pointsVector();
        int subtracted = m_currX - xRange;
        if (subtracted < 0) subtracted = 0;
        int lineStart = m_linesStart[i];
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
        subtracted = m_currX - lineStart;
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
    m_axisY->setMin((min - CHART_MARGIN * m_axisY->max()) / (1 - CHART_MARGIN));
}

inline void PlotterView::yMaxSelect(const qreal& max) {
    m_axisY->setMax((max - CHART_MARGIN * m_axisY->min()) / (1 - CHART_MARGIN));
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
    m_axisY->setRange(a - (b - a) * CHART_MARGIN / (1 - 2 * CHART_MARGIN), b + (b - a) * CHART_MARGIN / (1 - 2 * CHART_MARGIN));
}

void PlotterView::handleChangeXRange(const int xRange) {
    // find the x value exactly one range before
    int oneRangeBefore = m_currX - xRange;
    if (oneRangeBefore >= 0) {
        // we have data exactly one range before, so we can adjust accordingly
        m_axisX->setRange(oneRangeBefore, m_currX);
    } else {
        // we set the range normally
        m_axisX->setRange(0, xRange);
    }
}

void PlotterView::clear() {
    m_chart->removeAllSeries();
    m_lines.clear();
    m_linesStart.clear();
    m_linesLastX.clear();
    m_currX = 0;
    m_axisX->setRange(0, ui->xRangeSpinBox->value());
    m_axisY->setRange(-YMAGNITUDEMAX, YMAGNITUDEMAX);
}

void PlotterView::plotPoint(const qreal val, const int lineIndex, const bool increment) {
    QLineSeries* currLine;
    if (lineIndex == m_lines.length()) {
        // this is a new line
        currLine = new QLineSeries;
        m_lines << currLine;
        m_linesStart << m_currX;
        m_linesLastX << m_currX;
        currLine->setUseOpenGL();
        m_chart->addSeries(currLine);
        currLine->attachAxis(m_chart->axisX());
        currLine->attachAxis(m_chart->axisY());
        currLine->append(m_currX, val);
    } else {
        currLine = m_lines[lineIndex];
        QPointF newPoint(m_currX, val);
        currLine->append(newPoint);
        // get the position of the point on the chart;
        auto position = m_chart->mapToPosition(newPoint);
        auto plotArea = m_chart->plotArea();
        if (position.x() > plotArea.right()) {
            // scroll point into view
            m_chart->scroll(position.x() - plotArea.right(), 0);
        }
        m_linesLastX[lineIndex] = m_currX;
    }

    currLine->setName(QString("%1").arg(val));
    // adjust the chart when the value is out of range
    if (val > m_axisY->max()) {
        if (ui->bestFitRadio->isChecked()) {
            bestFit();
        } else {
            yMaxSelect(val);
        }
    } else if (val < m_axisY->min()) {
        if (ui->bestFitRadio->isChecked()) {
            bestFit();
        } else {
            yMinSelect(val);
        }
    }
    if (increment) {
        for (int i = 0; i < m_lines.length(); ++i) {
            auto line = m_lines[i];
            if (m_linesLastX[i] != m_currX) {
                m_linesLastX[i] = m_currX;
                line->append(m_currX, 0);
                line->setName("0");
            }
        }
        ++m_currX;
    }
}

PlotterView::~PlotterView() {
    delete ui;
}
