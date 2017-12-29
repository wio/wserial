#include "test.h"
#include <QScrollBar>

void WorkerTest::processDataTest() {
    Worker worker;
    QSignalSpy outputSpy(&worker, &Worker::output);
    QByteArray buf = QString("Hello bro 1 2 3 4\r\n").toUtf8();
    worker.processData(buf);
    QCOMPARE(outputSpy.count(), 1);

    worker.plotEnabled = true;
    QSignalSpy plotPointSpy(&worker, &Worker::plotPoint);
    buf = QString("Hello bro 1.23 2.34 3.45 4.56\r\n").toUtf8();
    worker.processData(buf);
    QCOMPARE(plotPointSpy.count(), 4); // four points
}

void PlotterViewTest::plotPointTest() {
    PlotterView plotterView;
    plotterView.ui->xRangeSpinBox->setValue(10);
    plotterView.plotPoint(12, 0, false);
    plotterView.plotPoint(-10, 1, true);
    plotterView.ui->bestFitRadio->setChecked(true);
    plotterView.ui->xRangeSpinBox->setValue(1);
    plotterView.plotPoint(15, 0, false);
    plotterView.plotPoint(-110, 1, true);
    plotterView.plotPoint(15, 0, false);
    plotterView.plotPoint(-110, 1, true);
    plotterView.plotPoint(-150, 1, true); // should handle seen skipped line (line 0)
    plotterView.plotPoint(-10, 100, true); // should handle unseen skipped line (line 1-99)
    plotterView.clear();
}

void MainWindowTest::initTestCase() {
    mainWindow.show();
}

void MainWindowTest::uiTests() {
    mainWindow.ui->baudRate->setCurrentIndex(0);
    mainWindow.ui->portReload->click();
    mainWindow.ui->monitorButton->toggle();
    mainWindow.ui->plotterButton->toggle();
    QCOMPARE(mainWindow.ui->plotterButton->isChecked(), true);
    mainWindow.ui->plotterButton->toggle();
    mainWindow.ui->portReload->click();
}

void MainWindowTest::monitorViewTests() {
    QScrollBar* scroller = mainWindow.ui->plainTextEdit->verticalScrollBar();
    scroller->setSliderDown(true);
    // scroller pressed down - don't scroll to bottom
    mainWindow.output("Hello world 1");
    scroller->setSliderDown(false);
    // scroller released - scroll to bottom
    mainWindow.output("Hello world 2");
    // auto scroll disabled - don't scroll to bottom
    mainWindow.ui->autoScroll->setChecked(false);
    mainWindow.output("Hello world 3");
}

void MainWindowTest::cleanupTestCase() {
    mainWindow.close();
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    WorkerTest workerTest;
    PlotterViewTest plotterViewTest;
    MainWindowTest mainWindowTest;
    QTEST_SET_MAIN_SOURCE_PATH

    return QTest::qExec(&workerTest, argc, argv)
         + QTest::qExec(&plotterViewTest, argc, argv)
         + QTest::qExec(&mainWindowTest, argc, argv);
}
