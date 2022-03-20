#include <iostream>
#include <sys/time.h>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <limits.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QPainter>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "json.hpp"

using json = nlohmann::json;


int64_t get_nanos() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (int64_t)(t.tv_sec) * (int64_t)1000000000 + (int64_t)(t.tv_nsec);
}





class Drawing {
public:
    int in_start;
    int in_last;
    int out_last;


    Drawing() {
        in_start = 0;
        in_last = -1;
        out_last = -1;
    }

    double start_time_ns;
    std::vector<double> t_arr_tmp = {};
    std::vector<double> x_arr_tmp = {};
    std::vector<double> y_arr_tmp = {};

    std::vector<std::vector<double> > t_arr = {};
    std::vector<std::vector<double> > x_arr = {};
    std::vector<std::vector<double> > y_arr = {};

    std::vector<double> draw_x_arr = {};
    std::vector<double> draw_y_arr = {};


    bool start_drawing(double x, double y, double pressure, double x_tilt, double y_tilt) {
        start_time_ns = get_nanos();
        //std::cout << "start drawing: nanos=" << start_time_ns << "(" << x << "," << y << "), p=" << pressure << " tile:("<< x_tilt << "," << y_tilt << ")" << std::endl;
        t_arr_tmp.push_back(0);
        x_arr_tmp.push_back(x);
        y_arr_tmp.push_back(y);
        return true;
    }

    bool continue_drawing(double x, double y, double pressure, double x_tilt, double y_tilt) {
        double nanos;
        nanos = get_nanos() - start_time_ns;
        //std::cout << "continue drawing: nanos=" << nanos << "("<< x << "," << y << "), p=" << pressure << " tile:("<< x_tilt << "," << y_tilt << ")" << std::endl;        
        t_arr_tmp.push_back(nanos);
        x_arr_tmp.push_back(x);
        y_arr_tmp.push_back(y);

        in_last += 1;
        double dt = (t_arr_tmp[in_last] - t_arr_tmp[in_start])/1000000.0;
        double dx = x_arr_tmp[in_last] - x_arr_tmp[in_start];
        double dy = y_arr_tmp[in_last] - y_arr_tmp[in_start];
        double dr = pow(pow(dx,2) + pow(dy,2), 0.5);


        //std::cout << "x,y:" << draw_x_arr[out_last] << "," << draw_y_arr[out_last] << ", dt:" << dt << ", dr:" << dr << std::endl;


        double tdiff_ms = 100;
        double rdiff = 3;

        if ((dt > tdiff_ms) | (dr > rdiff)) {
            do_update();
            return true;
        } else {
            return false;
        }
        //return true;
    }


    void do_update() {
        draw_x_arr.push_back(x_arr_tmp[in_last]);
        draw_y_arr.push_back(y_arr_tmp[in_last]);
        in_start = in_last;
        out_last += 1;
    }


    bool stop_drawing(double x, double y, double pressure, double x_tilt, double y_tilt) {
        double nanos;
        nanos = get_nanos() - start_time_ns;
        //std::cout << "stop drawing: nanos=" << nanos << "(" << x << "," << y << "), p=" << pressure << " tile:("<< x_tilt << "," << y_tilt << ")" << std::endl;
        t_arr_tmp.push_back(nanos);
        x_arr_tmp.push_back(x);
        y_arr_tmp.push_back(y);

        t_arr.push_back(t_arr_tmp);
        x_arr.push_back(x_arr_tmp);
        y_arr.push_back(y_arr_tmp);

        t_arr_tmp.clear();
        x_arr_tmp.clear();
        y_arr_tmp.clear();

        draw_x_arr.clear();
        draw_y_arr.clear();
        in_start = 0;
        in_last = -1;
        out_last = -1;

            
        return true;
    }







    

    void save() {

        //std::cout << "save" << std::endl;
        json j;
        j["t_arr"] = t_arr;
        j["x_arr"] = x_arr;
        j["y_arr"] = y_arr;
        //std::cout << j.dump(4) << std::endl;

        std::ofstream o("drawings.json");
        o << std::setw(4) << j << std::endl;
    }

    void load() {
        std::cout << "load" << std::endl;
    }

};


Drawing drawing;


MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {

    char hostname[32];
    gethostname(hostname, 32);
    std::cout << hostname << std::endl;
    if (!strcmp(hostname, "heisenbug")) {
        std::cout << "laptop" << std::endl;    
    } else {
        std::cout << "tablet" << std::endl;
        QApplication::setOverrideCursor(Qt::BlankCursor);
        QTimer::singleShot(0, this, SLOT(showFullScreen()));
    }
    
    setAttribute(Qt::WA_AcceptTouchEvents, true);
    
    auto *vbox = new QVBoxLayout(this);
    auto *hbox = new QHBoxLayout();

    auto *save = new QPushButton("save", this);
    auto *load = new QPushButton("load", this);
    auto *quit = new QPushButton("quit", this);
    hbox->addWidget(save, 1);
    hbox->addWidget(load, 1);
    hbox->addWidget(quit, 1);

    vbox->addStretch(1);
    vbox->addLayout(hbox, 0);

    setLayout(vbox);

    connect(save, &QPushButton::clicked, this, &MainWindow::save);
    connect(load, &QPushButton::clicked, this, &MainWindow::load);
    connect(quit, &QPushButton::clicked, this, &QApplication::quit);
}


void MainWindow::paintEvent(QPaintEvent *event) {
    QPainter qp(this);
    QPen pen(Qt::black, 2, Qt::SolidLine);  
    qp.setPen(pen);
    qp.drawLine(20, 40, 250, 40);

    for(int i=0; i < drawing.draw_x_arr.size(); i++){
        //std::cout << "x:" << drawing.x_arr_tmp[i] << ", y:" << drawing.y_arr_tmp[i] << std::endl;
        //qp.drawLine(drawing.x_arr_tmp[i-1], drawing.y_arr_tmp[i-1], drawing.x_arr_tmp[i], drawing.y_arr_tmp[i]);
        //qp.drawPoint(drawing.x_arr_tmp[i], drawing.y_arr_tmp[i]);
        qp.drawPoint(drawing.draw_x_arr[i], drawing.draw_y_arr[i]);
    }
    //std::cout << t_timer << std::endl;
}


MainWindow::~MainWindow() {
}

void MainWindow::save() {
    drawing.save();
}

void MainWindow::load() {
    drawing.load();
}


void MainWindow::tabletEvent(QTabletEvent *event) {
    bool must_repaint = false;
    switch (event->type()) {
    case QEvent::TabletPress:
        //std::cout << "tablet press, " << "pressure: " << event->pressure() << ", rotation" << event->rotation()
        //          << ", pos: " << event->position().x() << "," << event->position().y() << ", xtilt:" << event->xTilt()
        //          << ", ytilt: " << event->yTilt() << std::endl;
        must_repaint = drawing.start_drawing(event->position().x(), event->position().y(), event->pressure(), event->xTilt(), event->yTilt());
        break;
    case QEvent::TabletMove:
        //std::cout << "tablet move, " << "pressure: " << event->pressure() << ", rotation" << event->rotation()
        //          << ", pos: " << event->position().x() << "," << event->position().y() << ", xtilt:" << event->xTilt()
        //          << ", ytilt: " << event->yTilt() << std::endl;
        must_repaint = drawing.continue_drawing(event->position().x(), event->position().y(), event->pressure(), event->xTilt(), event->yTilt());
        break;
    case QEvent::TabletRelease:
        //std::cout << "tablet release, " << "pressure: " << event->pressure() << ", rotation" << event->rotation()
        //          << ", pos: " << event->position().x() << "," << event->position().y() << ", xtilt:" << event->xTilt()
        //          << ", ytilt: " << event->yTilt() << std::endl;
        //std::cout << "tablet deviceType: " << ((int)event->deviceType()) << std::endl;
        must_repaint = drawing.stop_drawing(event->position().x(), event->position().y(), event->pressure(), event->xTilt(), event->yTilt());
        // https://doc.qt.io/qt-6/qinputdevice.html#DeviceType-enum 16->0x10 -> stylus
        break;
    default:
        break;
    }
    if (must_repaint) {
        //repaint();
        update();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    //std::cout << "mouse move event" << std::endl;
    //bool must_repaint = drawing.continue_drawing(event->position().x(), event->position().y(), 0, 0, 0);
    //if (must_repaint) {
    //    //repaint();
    //	update();
    //}
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    //std::cout << "mouse press event" << std::endl;
    //bool must_repaint = drawing.start_drawing(event->position().x(), event->position().y(), 0, 0, 0);
    //if (must_repaint) {
    //    //repaint();
    //    update();
    //}
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    //std::cout << "mouse release event" << std::endl;
    //bool must_repaint = drawing.stop_drawing(event->position().x(), event->position().y(), 0, 0, 0);
    //if (must_repaint) {
    //    //repaint();
    //    update();
    //}
}
