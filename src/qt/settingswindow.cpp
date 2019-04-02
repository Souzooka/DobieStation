#include <QVBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QDialog>
#include <QWidget>
#include <QGroupBox>
#include <QRadioButton>

#include "settingswindow.hpp"
#include "settings.hpp"

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent)
{
    QRadioButton* jit_checkbox = new QRadioButton(tr("JIT"));
    QRadioButton* interpreter_checkbox = new QRadioButton(tr("Interpreter"));
    QLabel* warning = new QLabel(tr("NOTE: Change will take effect the next time you load a game."));

    bool vu1_jit = Settings::instance().vu1_jit_enabled;
    jit_checkbox->setChecked(vu1_jit);
    interpreter_checkbox->setChecked(!vu1_jit);

    connect(jit_checkbox, &QRadioButton::clicked, this, [=] (){
        Settings::instance().vu1_jit_enabled = true;
    });

    connect(interpreter_checkbox, &QRadioButton::clicked, this, [=] (){
        Settings::instance().vu1_jit_enabled = false;
    });

    connect(&Settings::instance(), &Settings::reload, this, [=]() {
        bool vu1_jit_enabled = Settings::instance().vu1_jit_enabled;
        jit_checkbox->setChecked(vu1_jit_enabled);
        interpreter_checkbox->setChecked(!vu1_jit_enabled);
    });

    QVBoxLayout* vu1_layout = new QVBoxLayout;
    vu1_layout->addWidget(jit_checkbox);
    vu1_layout->addWidget(interpreter_checkbox);
    vu1_layout->addWidget(warning);

    QGroupBox* vu1_groupbox = new QGroupBox(tr("VU1"));
    vu1_groupbox->setLayout(vu1_layout);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(vu1_groupbox);
    layout->addStretch(1);

    setLayout(layout);

    setMinimumWidth(400);
}

PathTab::PathTab(QWidget* parent)
    : QWidget(parent)
{
    path_list = new QListWidget;
    path_list->setSpacing(1);
    path_list->insertItems(0, Settings::instance().rom_directories);

    QVBoxLayout* edit_layout = new QVBoxLayout;
    edit_layout->addWidget(path_list);

    QPushButton* add_directory = new QPushButton(tr("Add"));
    QPushButton* remove_directory = new QPushButton(tr("Remove"));

    connect(remove_directory, &QAbstractButton::clicked, this, [=]() {
        int row = path_list->currentRow();

        auto widget = path_list->takeItem(row);
        if (!widget)
            return;

        Settings::instance().remove_rom_directory(widget->text());
    });

    connect(add_directory, &QAbstractButton::clicked, this, [=]() {
        QString path = QFileDialog::getExistingDirectory(
            this, tr("Open Rom Directory"),
            Settings::instance().last_used_directory
        );

        Settings::instance().add_rom_directory(path);
    });

    connect(&Settings::instance(), &Settings::rom_directory_added, this, [=](QString path) {
        path_list->insertItems(path_list->count(), { path });
    });

    connect(&Settings::instance(), &Settings::bios_changed, this, [=](QString path) {
        BiosReader bios_reader(path);
        bios_info->setText(bios_reader.to_string());
    });

    connect(&Settings::instance(), &Settings::reload, this, [=]() {
        BiosReader bios_reader(Settings::instance().bios_path);
        bios_info->setText(bios_reader.to_string());

        path_list->clear();
        path_list->insertItems(0, Settings::instance().rom_directories);
    });

    QHBoxLayout* button_layout = new QHBoxLayout;
    button_layout->addStretch();
    button_layout->addWidget(add_directory);
    button_layout->addWidget(remove_directory);

    edit_layout->addLayout(button_layout);

    QPushButton* browse_button = new QPushButton(tr("Browse"));
    connect(browse_button, &QAbstractButton::clicked, [=]() {
        QString path = QFileDialog::getOpenFileName(
            this, tr("Open Bios"), Settings::instance().last_used_directory,
            tr("Bios File (*.bin)")
        );

        Settings::instance().set_bios_path(path);
    });

    BiosReader bios_reader(Settings::instance().bios_path);
    bios_info = new QLabel(bios_reader.to_string());

    QPushButton* screenshot_button = new QPushButton(tr("Browse"));
    connect(screenshot_button, &QAbstractButton::clicked, [=]() {
        QString directory = QFileDialog::getExistingDirectory(
            this, tr("Choose Screenshot Directory"),
            Settings::instance().last_used_directory
        );

        Settings::instance().set_screenshot_directory(directory);
    });

    auto screenshot_label =
        new QLabel(Settings::instance().screenshot_directory);

    connect(&Settings::instance(), &Settings::screenshot_directory_changed, [=](QString directory) {
        screenshot_label->setText(directory);
    });

    QGridLayout* other_layout = new QGridLayout;
    other_layout->addWidget(new QLabel(tr("Bios:"), 0, 0));
    other_layout->addWidget(bios_info, 0, 2);
    other_layout->addWidget(browse_button, 0, 3);
    other_layout->addWidget(new QLabel("Screenshots:"), 1, 0);
    other_layout->addWidget(screenshot_label, 1, 2);
    other_layout->addWidget(screenshot_button, 1, 3);

    other_layout->setColumnStretch(1, 1);
    other_layout->setAlignment(Qt::AlignTop);

    QGroupBox* path_groupbox = new QGroupBox(tr("Rom Directories"));
    path_groupbox->setLayout(edit_layout);

    QGroupBox* other_groupbox = new QGroupBox(tr("Other"));
    other_groupbox->setLayout(other_layout);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(path_groupbox);
    layout->addWidget(other_groupbox);

    setLayout(layout);
}

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    general_tab = new GeneralTab(this);
    path_tab = new PathTab(this);

    tab_widget = new QTabWidget(this);
    tab_widget->addTab(general_tab, tr("General"));
    tab_widget->addTab(path_tab, tr("Paths"));
    tab_widget->setCurrentIndex(TAB::GENERAL);

    QDialogButtonBox* close_buttons = new QDialogButtonBox(
        QDialogButtonBox::Close | QDialogButtonBox::Ok, this
    );
    connect(close_buttons, &QDialogButtonBox::accepted, this, [=]() {
        Settings::instance().save();
        accept();
    });
    connect(close_buttons, &QDialogButtonBox::rejected, this, [=]() {
        Settings::instance().reset();
        reject();
    });

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(tab_widget);
    layout->addWidget(close_buttons);

    setLayout(layout);
}

void SettingsWindow::show_path_tab()
{
    tab_widget->setCurrentIndex(TAB::PATH);
}