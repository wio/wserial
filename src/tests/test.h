#ifndef TEST_H
#define TEST_H
#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include "worker.h"
#include "plotterview.h"
#include "ui_plotterview.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace Ui {
class PlotterViewTest;
}

class WorkerTest: public QObject {
    Q_OBJECT
private slots:
    void processDataTest();
};

class PlotterViewTest: public QObject {
    Q_OBJECT
private slots:
    void plotPointTest();
};

class MainWindowTest: public QObject {
    Q_OBJECT
    MainWindow mainWindow;

public:
    MainWindowTest();

private slots:
    void initTestCase();
    void uiTests();
    void monitorViewTests();
    void cleanupTestCase();
};

#endif // TEST_H
