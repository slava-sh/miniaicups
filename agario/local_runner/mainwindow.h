#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimerEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSvgGenerator>

#include "constants.h"
#include "strategymodal.h"
#include "mechanic.h"
#include "ui_mainwindow.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    Mechanic *mechanic = nullptr;
    StrategyModal *sm;
    QMessageBox *mbox;

    int timerId;
    bool is_paused;

    QMap<int, bool> player_vision;
public:
    explicit MainWindow(QWidget *parent = 0) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        mechanic(new Mechanic),
        sm(new StrategyModal),
        mbox(new QMessageBox),
        timerId(-1),
        is_paused(false)
    {
        ui->setupUi(this);
        this->setMouseTracking(true);
        this->setFixedSize(this->geometry().width(), this->geometry().height());

        if (Constants::instance().SEED.isEmpty()) {
            ui->txt_seed->setText(Constants::generate_seed());
        } else {
            ui->txt_seed->setText(Constants::instance().SEED);
        }

        ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);
        ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget->setFocusPolicy(Qt::NoFocus);
        ui->tableWidget->resizeColumnsToContents();
        ui->tableWidget->resizeRowsToContents();
        ui->tableWidget->setSortingEnabled(true);
        ui->tableWidget->sortByColumn(2, Qt::DescendingOrder);
        ui->tableWidget->horizontalHeader()->setSortIndicatorShown(true);
        ui->tableWidget->verticalHeader()->hide();

        ui->txt_ticks->setText("0");
        connect(ui->btn_start_pause, SIGNAL(pressed()), this, SLOT(start_or_pause_game()));
        connect(ui->btn_stop, SIGNAL(pressed()), this, SLOT(clear_game()));
        connect(ui->btn_step, SIGNAL(pressed()), this, SLOT(pause_and_step_game()));
        connect(ui->btn_svg, SIGNAL(pressed()), this, SLOT(save_svg()));

        connect(ui->cbx_forces, SIGNAL(stateChanged(int)), this, SLOT(update()));
        connect(ui->cbx_speed, SIGNAL(stateChanged(int)), this, SLOT(update()));
        connect(ui->cbx_fog, SIGNAL(stateChanged(int)), this, SLOT(update()));

        connect(ui->btn_strategies_settings, &QPushButton::clicked, sm, &QDialog::show);

        //ui->btn_start_pause->animateClick();
    }

    ~MainWindow() {
        if (ui) delete ui;
        if (mechanic) delete mechanic;
        if (sm) delete sm;
    }

public slots:
    void start_or_pause_game() {
        if (timerId > 0) {
            is_paused = !is_paused;
            return;
        }
        timerId = startTimer(Constants::instance().TICK_MS);

        QSharedPointer<ReplayLog> replay_log = nullptr;
        auto replay_log_txt = ui->txt_replay_log->text().trimmed();
        if (!replay_log_txt.isEmpty()) {
            replay_log = QSharedPointer<ReplayLog>(new ReplayLog(replay_log_txt));
            Constants::initialize(replay_log->env());
        }

        std::string seed = ui->txt_seed->text().toStdString();

        ui->tableWidget->setSortingEnabled(false);
        ui->tableWidget->setRowCount(0);

        mechanic = new Mechanic();
        if (replay_log != nullptr) {
            mechanic->set_replay_log(replay_log);
        }
        mechanic->init_objects(seed, [this, replay_log] (Player *player) {
            int pId = player->getId();
            player->set_color(sm->get_color(pId));

            Strategy *strategy = sm->get_strategy(pId);
            Custom *custom = dynamic_cast<Custom*>(strategy);
            if (custom != NULL) {
                connect(custom, SIGNAL(error(QString)), this, SLOT(on_error(QString)));
            }

            int row = ui->tableWidget->rowCount();
            ui->tableWidget->insertRow(row);

            int col = 0;

            ui->tableWidget->setItem(row, col++, new QTableWidgetItem(QString::number(pId)));
            ui->tableWidget->setItem(row, col++, new QTableWidgetItem(sm->get_color_name(pId)));
            ui->tableWidget->setItem(row, col++, new QTableWidgetItem("0"));

            //centered checkbox
            QWidget *widget = new QWidget(this);
            QCheckBox *checkBox = new QCheckBox(widget);
            QHBoxLayout *layout = new QHBoxLayout(widget);
            layout->addWidget(checkBox);
            layout->setAlignment(Qt::AlignCenter);
            layout->setContentsMargins(0,0,0,0);
            widget->setLayout(layout);
            checkBox->setProperty("pId", pId);
            checkBox->setChecked(player_vision.value(pId));
            connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(set_vision(bool)));
            ui->tableWidget->setCellWidget(row, col++, widget);

            return strategy;
        });

        for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
            for (int col = 0; col < ui->tableWidget->columnCount() - 1; ++col)
                ui->tableWidget->item(row, col)->setTextAlignment(Qt::AlignCenter);


        ui->tableWidget->resizeColumnsToContents();
        ui->tableWidget->resizeRowsToContents();
        ui->tableWidget->setSortingEnabled(true);

        this->update();
    }

    void on_error(QString msg) {
        is_paused = true;
        mbox->setStandardButtons(QMessageBox::Close);
        mbox->setText(msg);
        mbox->exec();
    }

    void set_vision(bool b) {
        auto cb = (QCheckBox*)QObject::sender();
        auto pid = cb->property("pId").toInt();
        player_vision[pid] = b;
        update();
    }

    void clear_game() {
        if (timerId > 0) {
            killTimer(timerId);
            is_paused = false;
            timerId = -1;
        }
        ui->txt_ticks->setText("");
        mechanic->clear_objects(false);
        ui->btn_start_pause->setText("Старт");
        this->update();
    }

    void pause_and_step_game() {
        start_or_pause_game();
        is_paused = true;
        step_game();
    }

    void finish_game() {
        killTimer(timerId);
        is_paused = false;
        timerId = -1;

        double max_score = 0;
        double maxId = -1;

        if (maxId != -1) {
            QString text = "Победителем стал ID=" + QString::number(maxId) + " со счетом " + QString::number(max_score);

            mbox->setStandardButtons(QMessageBox::Close);
            mbox->setText(text);
            mbox->exec();
        }
    }

    void save_svg() {
        QSvgGenerator generator;
        generator.setFileName("/tmp/game.svg");
        generator.setSize(QSize(
                    Constants::instance().GAME_WIDTH,
                    Constants::instance().GAME_HEIGHT));
        QPainter painter;
        painter.begin(&generator);
        paint_on(painter);
        painter.end();
    }

public:
    void step_game() {
        int tick = mechanic->tickEvent(is_paused);
        ui->txt_ticks->setText(QString::number(tick));
        this->update();

        if (tick % Constants::instance().BASE_TICK == 0 && tick != 0) {
            update_score();
        }
        if (tick % Constants::instance().GAME_TICKS == 0 && tick != 0) {
            finish_game();
        }
    }

    void paintEvent(QPaintEvent*) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(ui->viewport->x(), ui->viewport->y());
        painter.fillRect(ui->viewport->rect(), QBrush(Qt::white));
        painter.setClipRect(ui->viewport->rect());
        painter.scale((qreal) ui->viewport->width() / Constants::instance().GAME_WIDTH,
                      (qreal) ui->viewport->height() / Constants::instance().GAME_HEIGHT);
        paint_on(painter);
    }

    void paint_on(QPainter& painter) {
        bool show_speed = ui->cbx_speed->isChecked();
        bool show_cmd = ui->cbx_forces->isChecked();
        bool show_fogs = ui->cbx_fog->isChecked();
        mechanic->paintEvent(painter, show_speed, show_fogs, show_cmd, player_vision);
    }

    void timerEvent(QTimerEvent *event) {
        if (event->timerId() != timerId) {
            return;
        }
        if (is_paused) {
            ui->btn_start_pause->setText("Старт");
        } else {
            ui->btn_start_pause->setText("Пауза");
            step_game();
        }
    }

public:
    void mouseMoveEvent(QMouseEvent *event) {
        mousePressEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) {
        int x = (event->x() - ui->viewport->x()) / ((qreal) ui->viewport->width() / Constants::instance().GAME_WIDTH);
        int y = (event->y() - ui->viewport->y()) / ((qreal) ui->viewport->height() / Constants::instance().GAME_HEIGHT);
        mechanic->mouseMoveEvent(x, y);
    }

    void keyPressEvent(QKeyEvent *event) {
        if (event->key() == Qt::Key_Space) {
            pause_and_step_game();
        } else {
            mechanic->keyPressEvent(event);
        }
    }

    void update_score() {
        QMap<int, int> scores = mechanic->get_scores();

        ui->tableWidget->setSortingEnabled(false);

        for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
            int pId = ui->tableWidget->item(row, 0)->data(Qt::DisplayRole).toInt();
            ui->tableWidget->item(row, 2)->setData(Qt::DisplayRole, scores.value(pId));
        }

         ui->tableWidget->setSortingEnabled(true);
     }
};

#endif // MAINWINDOW_H
