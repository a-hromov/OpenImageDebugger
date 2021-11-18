/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2019 OpenImageDebugger contributors
 * (https://github.com/OpenImageDebugger/OpenImageDebugger)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <cmath>
#include <iostream>

#include <QDateTime>
#include <QDebug>
#include <QFontDatabase>
#include <QSettings>
#include <QShortcut>
#include <QHostAddress>

#include "main_window.h"

#include "ui_main_window.h"


void MainWindow::initialize_settings()
{
    using BufferExpiration = QPair<QString, QDateTime>;

    qRegisterMetaTypeStreamOperators<QList<BufferExpiration>>(
        "QList<QPair<QString, QDateTime>>");

    QSettings settings(QSettings::Format::IniFormat,
                       QSettings::Scope::UserScope,
                       "OpenImageDebugger");

    settings.sync();

    // Load maximum framerate
    render_framerate_ =
        settings.value("Rendering/maximum_framerate", 60).value<double>();
    if (render_framerate_ <= 0.f) {
        render_framerate_ = 1.0;
    }

    // Default save suffix: Image
    settings.beginGroup("Export");
    if (settings.contains("default_export_suffix")) {
        default_export_suffix_ =
            settings.value("default_export_suffix").value<QString>();
    } else {
        default_export_suffix_ = "Image File (*.png)";
    }
    settings.endGroup();

#if !defined(IS_DEVELOPMENT)
    // Load previous session symbols (don't do that in case of OID development).
    QDateTime now = QDateTime::currentDateTime();
    QList<BufferExpiration> previous_buffers =
        settings.value("PreviousSession/buffers")
            .value<QList<BufferExpiration>>();

    for (const auto& previous_buffer : previous_buffers) {
        if (previous_buffer.second >= now) {
            previous_session_buffers_.insert(
                previous_buffer.first.toStdString());
        }
    }
#endif


    // Load window position/size.
    // Window is loaded with a fixed size and restored in timer.
    // This is needed to give application some time to run event loop
    // and redraw all widgets without changing overall geometry.
    settings.beginGroup("MainWindow");
    setFixedSize(settings.value("size", size()).toSize());
    move(settings.value("pos", pos()).toPoint());
    settings.endGroup();


    // Load UI geometry.
    settings.beginGroup("UI");
    if (settings.contains("list_position")) {

        const QString position_str = settings.value("list_position", "left").toString();

        if (position_str == "top" || position_str == "bottom")
            ui_->splitter->setOrientation(Qt::Vertical);

        if (position_str == "right" || position_str == "bottom")
            ui_->splitter->insertWidget(-1, ui_->frame_list);

        ui_->splitter->repaint();
    }
    if (settings.contains("splitter")) {

        const QList<QVariant> listSizesVariant = settings.value("splitter").toList();
        QList<int> listSizesInt;
        for (QVariant size : listSizesVariant)
            listSizesInt.append(size.toInt());

        ui_->splitter->setSizes(listSizesInt);
    }
    if (settings.contains("minmax_compact")) {
        if (settings.value("minmax_compact").toBool()) {

            bool isMinMaxVisible = true;
            if (settings.contains("minmax_visible"))
                isMinMaxVisible = settings.value("minmax_visible").toBool();

            if (isMinMaxVisible) {

                ui_->gridLayout_toolbar->addWidget(ui_->acToggle, 0, 0);
                ui_->gridLayout_toolbar->addWidget(ui_->linkViewsToggle, 0, 1);
                ui_->gridLayout_toolbar->addWidget(ui_->reposition_buffer, 0, 2);
                ui_->gridLayout_toolbar->addWidget(ui_->go_to_pixel, 1, 0);
                ui_->gridLayout_toolbar->addWidget(ui_->rotate_90_ccw, 1, 1);
                ui_->gridLayout_toolbar->addWidget(ui_->rotate_90_cw, 1, 2);
            }

            ui_->horizontalLayout_container_toolbar->addWidget(ui_->minMaxEditor, 2);
            ui_->horizontalLayout_container_toolbar->setStretch(0, 0);
            ui_->horizontalLayout_container_toolbar->setStretch(1, 1);
            ui_->horizontalLayout_container_toolbar->setStretch(2, 0);

            ui_->acEdit->hide();
        }
    }
    if (settings.contains("minmax_visible"))
        ui_->acEdit->setChecked(settings.value("minmax_visible").toBool());
    settings.endGroup();


    // Restore possibility to resize application in timer.
    // This is needed to give application some time to run event loop
    // and redraw all widgets without changing overall geometry.
    QTimer::singleShot(100, [this](){

        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    });
}


void MainWindow::initialize_ui_icons()
{
#define SET_FONT_ICON(ui_element, unicode_id) \
    ui_element->setFont(icons_font);          \
    ui_element->setText(unicode_id);

#define SET_VECTOR_ICON(ui_element, icon_file_name, width, height) \
    ui_element->setScaledContents(true);                           \
    ui_element->setMinimumWidth(std::round(width));                \
    ui_element->setMaximumWidth(std::round(width));                \
    ui_element->setMinimumHeight(std::round(height));              \
    ui_element->setMaximumHeight(std::round(height));              \
    ui_element->setPixmap(                                         \
        QIcon(":/resources/icons/" icon_file_name)                 \
            .pixmap(QSize(std::round(width* screen_dpi_scale),     \
                          std::round(height* screen_dpi_scale)))); \
    ui_element->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    if (QFontDatabase::addApplicationFont(":/resources/icons/fontello.ttf") <
        0) {
        qWarning() << "Could not load ionicons font file!";
    }

    qreal screen_dpi_scale = get_screen_dpi_scale();
    QFont icons_font;
    icons_font.setFamily("fontello");
    icons_font.setPointSizeF(10.f);

    SET_FONT_ICON(ui_->acEdit, "\ue803");
    SET_FONT_ICON(ui_->acToggle, "\ue804");
    SET_FONT_ICON(ui_->reposition_buffer, "\ue800");
    SET_FONT_ICON(ui_->linkViewsToggle, "\ue805");
    SET_FONT_ICON(ui_->rotate_90_cw, "\ue801");
    SET_FONT_ICON(ui_->rotate_90_ccw, "\ue802");
    SET_FONT_ICON(ui_->go_to_pixel, "\uf031");

    SET_FONT_ICON(ui_->ac_reset_min, "\ue808");
    SET_FONT_ICON(ui_->ac_reset_max, "\ue808");

    SET_VECTOR_ICON(ui_->label_c1_min, "label_red_channel.svg", 10, 10);
    SET_VECTOR_ICON(ui_->label_c1_max, "label_red_channel.svg", 10, 10);
    SET_VECTOR_ICON(ui_->label_c2_min, "label_green_channel.svg", 10, 10);
    SET_VECTOR_ICON(ui_->label_c2_max, "label_green_channel.svg", 10, 10);
    SET_VECTOR_ICON(ui_->label_c3_min, "label_blue_channel.svg", 10, 10);
    SET_VECTOR_ICON(ui_->label_c3_max, "label_blue_channel.svg", 10, 10);
    SET_VECTOR_ICON(ui_->label_c4_min, "label_alpha_channel.svg", 10, 10);
    SET_VECTOR_ICON(ui_->label_c4_max, "label_alpha_channel.svg", 10, 10);

    SET_VECTOR_ICON(ui_->label_minmax, "lower_upper_bound.svg", 8, 35);
}


void MainWindow::initialize_ui_settings()
{
    connect(ui_->splitter, &QSplitter::splitterMoved,
            this,
            &MainWindow::persist_settings_deferred);

    connect(ui_->acEdit, &QAbstractButton::clicked,
            this,
            &MainWindow::persist_settings_deferred);
}


void MainWindow::initialize_timers()
{
    connect(&settings_persist_timer_,
            SIGNAL(timeout()),
            this,
            SLOT(persist_settings()));
    settings_persist_timer_.setSingleShot(true);

    connect(&update_timer_, SIGNAL(timeout()), this, SLOT(loop()));
}


void MainWindow::initialize_shortcuts()
{
    QShortcut* symbol_list_focus_shortcut_ =
        new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this);
    connect(symbol_list_focus_shortcut_,
            SIGNAL(activated()),
            ui_->symbolList,
            SLOT(setFocus()));

    QShortcut* buffer_removal_shortcut_ =
        new QShortcut(QKeySequence(Qt::Key_Delete), ui_->imageList);
    connect(buffer_removal_shortcut_,
            SIGNAL(activated()),
            this,
            SLOT(remove_selected_buffer()));

    QShortcut* go_to_shortcut =
        new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this);
    connect(
        go_to_shortcut, SIGNAL(activated()), this, SLOT(toggle_go_to_dialog()));
    connect(go_to_widget_,
            SIGNAL(go_to_requested(float, float)),
            this,
            SLOT(go_to_pixel(float, float)));
}


void MainWindow::initialize_networking()
{
    socket_.connectToHost(QString(host_settings_.url.c_str()),
                          host_settings_.port);
    const bool isConnected = socket_.waitForConnected();

    std::cout << "Connection status: " << host_settings_.url << ":" << host_settings_.port << " - " << (isConnected ? "connected" : "disconnected") << std::endl;
}


void MainWindow::initialize_symbol_completer()
{
    symbol_completer_ = new SymbolCompleter(this);

    symbol_completer_->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    symbol_completer_->setCompletionMode(QCompleter::PopupCompletion);
    symbol_completer_->setModelSorting(
        QCompleter::CaseInsensitivelySortedModel);

    ui_->symbolList->set_completer(symbol_completer_);
    connect(ui_->symbolList->completer(),
            SIGNAL(activated(QString)),
            this,
            SLOT(symbol_completed(QString)));
}


void MainWindow::initialize_left_pane()
{
    connect(ui_->imageList,
            SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this,
            SLOT(buffer_selected(QListWidgetItem*)));

    connect(ui_->symbolList,
            SIGNAL(editingFinished()),
            this,
            SLOT(symbol_selected()));

    ui_->imageList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_->imageList,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            this,
            SLOT(show_context_menu(const QPoint&)));
}


void MainWindow::initialize_auto_contrast_form()
{
    // Configure auto contrast inputs
    ui_->ac_c1_min->setValidator(new QDoubleValidator(ui_->ac_c1_min));
    ui_->ac_c2_min->setValidator(new QDoubleValidator(ui_->ac_c2_min));
    ui_->ac_c3_min->setValidator(new QDoubleValidator(ui_->ac_c3_min));

    ui_->ac_c1_max->setValidator(new QDoubleValidator(ui_->ac_c1_max));
    ui_->ac_c2_max->setValidator(new QDoubleValidator(ui_->ac_c2_max));
    ui_->ac_c3_max->setValidator(new QDoubleValidator(ui_->ac_c3_max));

    connect(ui_->ac_c1_min,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c1_min_update()));
    connect(ui_->ac_c1_max,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c1_max_update()));
    connect(ui_->ac_c2_min,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c2_min_update()));
    connect(ui_->ac_c2_max,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c2_max_update()));
    connect(ui_->ac_c3_min,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c3_min_update()));
    connect(ui_->ac_c3_max,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c3_max_update()));
    connect(ui_->ac_c4_min,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c4_min_update()));
    connect(ui_->ac_c4_max,
            SIGNAL(editingFinished()),
            this,
            SLOT(ac_c4_max_update()));

    connect(ui_->ac_reset_min, SIGNAL(clicked()), this, SLOT(ac_min_reset()));
    connect(ui_->ac_reset_max, SIGNAL(clicked()), this, SLOT(ac_max_reset()));
}


void MainWindow::initialize_toolbar()
{
    connect(ui_->acToggle, SIGNAL(clicked()), this, SLOT(ac_toggle()));

    connect(ui_->reposition_buffer,
            SIGNAL(clicked()),
            this,
            SLOT(recenter_buffer()));

    connect(ui_->linkViewsToggle,
            SIGNAL(clicked()),
            this,
            SLOT(link_views_toggle()));

    connect(ui_->rotate_90_cw, SIGNAL(clicked()), this, SLOT(rotate_90_cw()));
    connect(ui_->rotate_90_ccw, SIGNAL(clicked()), this, SLOT(rotate_90_ccw()));

    connect(
        ui_->go_to_pixel, SIGNAL(clicked()), this, SLOT(toggle_go_to_dialog()));
}


void MainWindow::initialize_visualization_pane()
{
    ui_->bufferPreview->set_main_window(this);
}


void MainWindow::initialize_status_bar()
{
    status_bar_ = new QLabel(this);
    status_bar_->setAlignment(Qt::AlignRight);

    statusBar()->addWidget(status_bar_, 1);
}


void MainWindow::initialize_go_to_widget()
{
    go_to_widget_ = new GoToWidget(ui_->bufferPreview);
}
