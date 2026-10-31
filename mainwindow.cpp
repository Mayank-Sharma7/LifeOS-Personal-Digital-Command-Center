#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>

#include <QStackedWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QFrame>
#include <QWidget>
#include <QScrollArea>
#include <QAbstractItemView>
#include <QSignalBlocker>
#include <QSizePolicy>

#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

#include <QFont>
#include <QColor>
#include <QBrush>

#include <QDebug>

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    applyTheme();

    if (initializeDatabase())
    {
        refreshTasks();
        refreshDashboard();
        refreshNotes();
    }
    else
    {
        QMessageBox::critical(
            this,
            "Database Error",
            "LifeOS could not initialize its database.\n\n"
            "Please check the application permissions and try again."
        );
    }
}

// ------------------------------------------------------------
// Destructor
// ------------------------------------------------------------

MainWindow::~MainWindow()
{
    if (database.isValid())
    {
        const QString connectionName = database.connectionName();

        database.close();

        database = QSqlDatabase();

        QSqlDatabase::removeDatabase(connectionName);
    }
}

// ------------------------------------------------------------
// Database initialization
// ------------------------------------------------------------

bool MainWindow::initializeDatabase()
{
    const QString appDataPath =
        QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation
        );

    if (appDataPath.isEmpty())
    {
        qCritical() << "Could not determine application data directory.";
        return false;
    }

    if (!QDir().mkpath(appDataPath))
    {
        qCritical() << "Could not create application data directory:"
                    << appDataPath;

        return false;
    }

    const QString databasePath =
        appDataPath + "/lifeos.db";

    database = QSqlDatabase::addDatabase("QSQLITE");

    database.setDatabaseName(databasePath);

    if (!database.open())
    {
        qCritical() << "Database connection failed:"
                    << database.lastError().text();

        return false;
    }

    // Create the tasks table.
    QSqlQuery taskQuery(database);

    const QString createTasksSql = R"(
        CREATE TABLE IF NOT EXISTS tasks
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            priority TEXT NOT NULL DEFAULT 'Medium',
            due_date TEXT NOT NULL,
            completed INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL
        )
    )";

    if (!taskQuery.exec(createTasksSql))
    {
        qCritical() << "Could not create tasks table:"
                    << taskQuery.lastError().text();

        return false;
    }

    // Create the notes table.
    if (!createNotesTable())
    {
        return false;
    }

    qDebug() << "LifeOS database initialized at:"
             << databasePath;

    return true;
}

// ------------------------------------------------------------
// Notes database table
// ------------------------------------------------------------

bool MainWindow::createNotesTable()
{
    QSqlQuery query(database);

    const QString createNotesSql = R"(
        CREATE TABLE IF NOT EXISTS notes
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            content TEXT NOT NULL DEFAULT '',
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
    )";

    if (!query.exec(createNotesSql))
    {
        qCritical() << "Could not create notes table:"
                    << query.lastError().text();

        return false;
    }

    return true;
}

// ------------------------------------------------------------
// Application interface
// ------------------------------------------------------------

void MainWindow::setupUI()
{
    setWindowTitle("LifeOS - Personal Digital Command Center");

    resize(1400, 850);

    QWidget *centralWidget = new QWidget(this);

    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout =
        new QHBoxLayout(centralWidget);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    QWidget *sidebar = new QWidget();

    sidebar->setObjectName("sidebar");

    sidebar->setFixedWidth(235);

    QVBoxLayout *sidebarLayout =
        new QVBoxLayout(sidebar);

    sidebarLayout->setContentsMargins(18, 24, 18, 20);
    sidebarLayout->setSpacing(10);

    QLabel *logo = new QLabel("LifeOS");

    logo->setObjectName("logoLabel");

    QFont logoFont;
    logoFont.setPointSize(23);
    logoFont.setBold(true);

    logo->setFont(logoFont);

    QLabel *subtitle =
        new QLabel("PERSONAL COMMAND CENTER");

    subtitle->setObjectName("subtitleLabel");

    subtitle->setWordWrap(true);

    sidebarLayout->addWidget(logo);
    sidebarLayout->addWidget(subtitle);
    sidebarLayout->addSpacing(28);

    // Main page stack
    pages = new QStackedWidget();

    pages->addWidget(createDashboard());       // 0
    pages->addWidget(createTasksPage());       // 1

    pages->addWidget(
        createModulePage(
            "Calendar",
            "Plan your schedule and keep track of upcoming events."
        )
    );                                         // 2

    pages->addWidget(
        createModulePage(
            "Habit Tracker",
            "Build consistent routines and track your daily habits."
        )
    );                                         // 3

    pages->addWidget(
        createModulePage(
            "Personal Finance",
            "Manage your income, expenses, and financial goals."
        )
    );                                         // 4

    pages->addWidget(createNotesPage());       // 5

    pages->addWidget(
        createModulePage(
            "Analytics",
            "Review your productivity and personal progress."
        )
    );                                         // 6

    pages->addWidget(
        createModulePage(
            "Settings",
            "Customize your LifeOS experience."
        )
    );                                         // 7

    sidebarLayout->addWidget(
        createNavButton("Dashboard", 0)
    );

    sidebarLayout->addWidget(
        createNavButton("Tasks", 1)
    );

    sidebarLayout->addWidget(
        createNavButton("Calendar", 2)
    );

    sidebarLayout->addWidget(
        createNavButton("Habits", 3)
    );

    sidebarLayout->addWidget(
        createNavButton("Finance", 4)
    );

    sidebarLayout->addWidget(
        createNavButton("Notes", 5)
    );

    sidebarLayout->addWidget(
        createNavButton("Analytics", 6)
    );

    sidebarLayout->addWidget(
        createNavButton("Settings", 7)
    );

    sidebarLayout->addStretch();

    QLabel *sidebarFooter =
        new QLabel("Your life, organized.");

    sidebarFooter->setObjectName("sidebarFooter");

    sidebarLayout->addWidget(sidebarFooter);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(pages, 1);

    pages->setCurrentIndex(0);
}

// ------------------------------------------------------------
// Navigation button
// ------------------------------------------------------------

QPushButton *MainWindow::createNavButton(
    const QString &text,
    int pageIndex
)
{
    QPushButton *button = new QPushButton(text);

    button->setObjectName("navButton");

    button->setCursor(Qt::PointingHandCursor);

    button->setMinimumHeight(43);

    connect(
        button,
        &QPushButton::clicked,
        this,
        [this, pageIndex]()
        {
            navigateTo(pageIndex);
        }
    );

    return button;
}

// ------------------------------------------------------------
// Navigate between pages
// ------------------------------------------------------------

void MainWindow::navigateTo(int pageIndex)
{
    if (!pages)
    {
        return;
    }

    pages->setCurrentIndex(pageIndex);

    if (pageIndex == 0)
    {
        refreshDashboard();
    }
    else if (pageIndex == 1)
    {
        refreshTasks();
    }
    else if (pageIndex == 5)
    {
        refreshNotes();
    }
}

// ------------------------------------------------------------
// Dashboard page
// ------------------------------------------------------------

QWidget *MainWindow::createDashboard()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(22);

    QLabel *heading =
        new QLabel("Good to see you!");

    heading->setObjectName("pageTitle");

    QLabel *description =
        new QLabel(
            "Your personal overview. Make today count."
        );

    description->setObjectName("mutedLabel");

    layout->addWidget(heading);
    layout->addWidget(description);

    // Summary cards
    QHBoxLayout *cardsLayout =
        new QHBoxLayout();

    cardsLayout->setSpacing(16);

    auto createCard =
        [](const QString &title,
           QLabel **valueLabel) -> QWidget *
    {
        QFrame *card = new QFrame();

        card->setObjectName("summaryCard");

        QVBoxLayout *cardLayout =
            new QVBoxLayout(card);

        cardLayout->setContentsMargins(20, 18, 20, 18);

        QLabel *titleLabel = new QLabel(title);

        titleLabel->setObjectName("mutedLabel");

        *valueLabel = new QLabel("0");

        (*valueLabel)->setObjectName("cardValue");

        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(*valueLabel);
        cardLayout->addStretch();

        card->setMinimumHeight(115);

        return card;
    };

    QLabel *tasksCardValue = nullptr;
    QLabel *completedCardValue = nullptr;
    QLabel *notesCardValue = nullptr;

    cardsLayout->addWidget(
        createCard("TOTAL TASKS", &tasksCardValue)
    );

    cardsLayout->addWidget(
        createCard("COMPLETED TASKS", &completedCardValue)
    );

    cardsLayout->addWidget(
        createCard("SAVED NOTES", &notesCardValue)
    );

    // Keep the task statistics label connected to the existing
    // dashboard refresh functionality.
    taskDashboardValue = tasksCardValue;

    // Use the additional cards for completion and notes totals.
    // These labels are children of their cards, so they can be
    // located when dashboard statistics are refreshed.
    completedCardValue->setObjectName("completedCardValue");
    notesCardValue->setObjectName("notesCardValue");

    layout->addLayout(cardsLayout);

    QLabel *focusHeading =
        new QLabel("Today's Focus");

    focusHeading->setObjectName("sectionTitle");

    layout->addWidget(focusHeading);

    QFrame *focusCard = new QFrame();

    focusCard->setObjectName("contentCard");

    QVBoxLayout *focusLayout =
        new QVBoxLayout(focusCard);

    focusLayout->setContentsMargins(20, 18, 20, 18);

    dashboardTasksLayout = focusLayout;

    layout->addWidget(focusCard, 1);

    // A small footer note
    QLabel *footer =
        new QLabel(
            "Stay focused. Small steps create big results."
        );

    footer->setObjectName("mutedLabel");

    layout->addWidget(footer);

    return page;
}

// ------------------------------------------------------------
// Task Manager page
// ------------------------------------------------------------

QWidget *MainWindow::createTasksPage()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(30, 26, 30, 26);
    layout->setSpacing(16);

    QLabel *heading =
        new QLabel("Task Manager");

    heading->setObjectName("pageTitle");

    QLabel *description =
        new QLabel(
            "Create tasks, organize priorities, and track completion."
        );

    description->setObjectName("mutedLabel");

    layout->addWidget(heading);
    layout->addWidget(description);

    QHBoxLayout *toolbar =
        new QHBoxLayout();

    taskSearch = new QLineEdit();

    taskSearch->setPlaceholderText(
        "Search tasks..."
    );

    taskSearch->setClearButtonEnabled(true);

    toolbar->addWidget(taskSearch, 1);

    QPushButton *addButton =
        new QPushButton("＋ Add Task");

    QPushButton *editButton =
        new QPushButton("Edit");

    QPushButton *deleteButton =
        new QPushButton("Delete");

    addButton->setObjectName("primaryButton");

    toolbar->addWidget(addButton);
    toolbar->addWidget(editButton);
    toolbar->addWidget(deleteButton);

    layout->addLayout(toolbar);

    taskTable = new QTableWidget();

    taskTable->setColumnCount(5);

    taskTable->setHorizontalHeaderLabels(
        {
            "ID",
            "Task",
            "Priority",
            "Due Date",
            "Completed"
        }
    );

    taskTable->setColumnHidden(0, true);

    taskTable->horizontalHeader()->setSectionResizeMode(
        1,
        QHeaderView::Stretch
    );

    taskTable->horizontalHeader()->setSectionResizeMode(
        2,
        QHeaderView::ResizeToContents
    );

    taskTable->horizontalHeader()->setSectionResizeMode(
        3,
        QHeaderView::ResizeToContents
    );

    taskTable->horizontalHeader()->setSectionResizeMode(
        4,
        QHeaderView::ResizeToContents
    );

    taskTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    taskTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    taskTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    taskTable->setAlternatingRowColors(true);

    taskTable->verticalHeader()->setVisible(false);

    layout->addWidget(taskTable, 1);

    taskStatsValue = new QLabel("0 tasks");

    taskStatsValue->setObjectName("mutedLabel");

    layout->addWidget(taskStatsValue);

    connect(
        addButton,
        &QPushButton::clicked,
        this,
        &MainWindow::addTask
    );

    connect(
        editButton,
        &QPushButton::clicked,
        this,
        &MainWindow::editTask
    );

    connect(
        deleteButton,
        &QPushButton::clicked,
        this,
        &MainWindow::deleteTask
    );

    connect(
        taskSearch,
        &QLineEdit::textChanged,
        this,
        &MainWindow::refreshTasks
    );

    connect(
        taskTable,
        &QTableWidget::itemChanged,
        this,
        [this](QTableWidgetItem *item)
        {
            if (loadingTasks || !item)
            {
                return;
            }

            if (item->column() == 4)
            {
                const int row = item->row();

                QTableWidgetItem *idItem =
                    taskTable->item(row, 0);

                if (!idItem)
                {
                    return;
                }

                const int taskId =
                    idItem->text().toInt();

                const bool completed =
                    item->checkState() == Qt::Checked;

                updateTaskCompletion(
                    taskId,
                    completed
                );
            }
        }
    );

    return page;
}

// ------------------------------------------------------------
// Generic placeholder page
// ------------------------------------------------------------

QWidget *MainWindow::createModulePage(
    const QString &title,
    const QString &description
)
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout =
        new QVBoxLayout(page);

    layout->setContentsMargins(35, 30, 35, 30);

    QLabel *heading = new QLabel(title);

    heading->setObjectName("pageTitle");

    QLabel *descriptionLabel =
        new QLabel(description);

    descriptionLabel->setObjectName("mutedLabel");

    descriptionLabel->setWordWrap(true);

    QFrame *card = new QFrame();

    card->setObjectName("contentCard");

    QVBoxLayout *cardLayout =
        new QVBoxLayout(card);

    QLabel *comingSoon =
        new QLabel("Coming Soon");

    comingSoon->setObjectName("sectionTitle");

    QLabel *details =
        new QLabel(
            "This module will be expanded in a future LifeOS update."
        );

    details->setObjectName("mutedLabel");

    details->setWordWrap(true);

    cardLayout->addWidget(comingSoon);
    cardLayout->addWidget(details);
    cardLayout->addStretch();

    layout->addWidget(heading);
    layout->addWidget(descriptionLabel);
    layout->addSpacing(15);
    layout->addWidget(card, 1);

    return page;
}

// ------------------------------------------------------------
// Add task
// ------------------------------------------------------------

void MainWindow::addTask()
{
    QDialog dialog(this);

    dialog.setWindowTitle("Create Task");

    dialog.setMinimumWidth(420);

    QVBoxLayout *layout =
        new QVBoxLayout(&dialog);

    QFormLayout *form =
        new QFormLayout();

    QLineEdit *titleInput = new QLineEdit();

    titleInput->setPlaceholderText(
        "Enter task title"
    );

    QLineEdit *descriptionInput = new QLineEdit();

    descriptionInput->setPlaceholderText(
        "Optional description"
    );

    QLineEdit *priorityInput = new QLineEdit();

    priorityInput->setText("Medium");

    priorityInput->setPlaceholderText(
        "Low, Medium, or High"
    );

    QLineEdit *dueDateInput = new QLineEdit();

    dueDateInput->setText(
        QDateTime::currentDateTime().date().toString("yyyy-MM-dd")
    );

    dueDateInput->setPlaceholderText("YYYY-MM-DD");

    form->addRow("Title:", titleInput);
    form->addRow("Description:", descriptionInput);
    form->addRow("Priority:", priorityInput);
    form->addRow("Due date:", dueDateInput);

    layout->addLayout(form);

    QDialogButtonBox *buttons =
        new QDialogButtonBox(
            QDialogButtonBox::Save |
            QDialogButtonBox::Cancel
        );

    layout->addWidget(buttons);

    connect(
        buttons,
        &QDialogButtonBox::accepted,
        &dialog,
        &QDialog::accept
    );

    connect(
        buttons,
        &QDialogButtonBox::rejected,
        &dialog,
        &QDialog::reject
    );

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const QString title =
        titleInput->text().trimmed();

    if (title.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Title",
            "Please enter a title for the task."
        );

        return;
    }

    const QString priority =
        priorityInput->text().trimmed().isEmpty()
            ? "Medium"
            : priorityInput->text().trimmed();

    const QString dueDate =
        dueDateInput->text().trimmed().isEmpty()
            ? QDateTime::currentDateTime().date().toString("yyyy-MM-dd")
            : dueDateInput->text().trimmed();

    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO tasks "
        "(title, description, priority, due_date, completed, created_at) "
        "VALUES (?, ?, ?, ?, 0, ?)"
    );

    query.addBindValue(title);
    query.addBindValue(descriptionInput->text().trimmed());
    query.addBindValue(priority);
    query.addBindValue(dueDate);

    query.addBindValue(
        QDateTime::currentDateTime().toString(Qt::ISODate)
    );

    if (!query.exec())
    {
        QMessageBox::critical(
            this,
            "Database Error",
            "Could not create the task.\n\n" +
                query.lastError().text()
        );

        return;
    }

    refreshTasks();
    refreshDashboard();
}

// ------------------------------------------------------------
// Edit task
// ------------------------------------------------------------

void MainWindow::editTask()
{
    const int taskId = selectedTaskId();

    if (taskId < 0)
    {
        QMessageBox::information(
            this,
            "Select Task",
            "Please select a task to edit."
        );

        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "SELECT title, description, priority, due_date "
        "FROM tasks WHERE id = ?"
    );

    query.addBindValue(taskId);

    if (!query.exec() || !query.next())
    {
        QMessageBox::warning(
            this,
            "Task Not Found",
            "The selected task could not be loaded."
        );

        return;
    }

    QDialog dialog(this);

    dialog.setWindowTitle("Edit Task");

    dialog.setMinimumWidth(420);

    QVBoxLayout *layout =
        new QVBoxLayout(&dialog);

    QFormLayout *form =
        new QFormLayout();

    QLineEdit *titleInput =
        new QLineEdit(query.value(0).toString());

    QLineEdit *descriptionInput =
        new QLineEdit(query.value(1).toString());

    QLineEdit *priorityInput =
        new QLineEdit(query.value(2).toString());

    QLineEdit *dueDateInput =
        new QLineEdit(query.value(3).toString());

    form->addRow("Title:", titleInput);
    form->addRow("Description:", descriptionInput);
    form->addRow("Priority:", priorityInput);
    form->addRow("Due date:", dueDateInput);

    layout->addLayout(form);

    QDialogButtonBox *buttons =
        new QDialogButtonBox(
            QDialogButtonBox::Save |
            QDialogButtonBox::Cancel
        );

    layout->addWidget(buttons);

    connect(
        buttons,
        &QDialogButtonBox::accepted,
        &dialog,
        &QDialog::accept
    );

    connect(
        buttons,
        &QDialogButtonBox::rejected,
        &dialog,
        &QDialog::reject
    );

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const QString title =
        titleInput->text().trimmed();

    if (title.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Title",
            "Please enter a title for the task."
        );

        return;
    }

    QSqlQuery updateQuery(database);

    updateQuery.prepare(
        "UPDATE tasks "
        "SET title = ?, description = ?, priority = ?, due_date = ? "
        "WHERE id = ?"
    );

    updateQuery.addBindValue(title);
    updateQuery.addBindValue(descriptionInput->text().trimmed());
    updateQuery.addBindValue(priorityInput->text().trimmed());
    updateQuery.addBindValue(dueDateInput->text().trimmed());
    updateQuery.addBindValue(taskId);

    if (!updateQuery.exec())
    {
        QMessageBox::critical(
            this,
            "Database Error",
            "Could not update the task.\n\n" +
                updateQuery.lastError().text()
        );

        return;
    }

    refreshTasks();
    refreshDashboard();
}

// ------------------------------------------------------------
// Delete task
// ------------------------------------------------------------

void MainWindow::deleteTask()
{
    const int taskId = selectedTaskId();

    if (taskId < 0)
    {
        QMessageBox::information(
            this,
            "Select Task",
            "Please select a task to delete."
        );

        return;
    }

    const auto answer =
        QMessageBox::question(
            this,
            "Delete Task",
            "Are you sure you want to delete this task?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "DELETE FROM tasks WHERE id = ?"
    );

    query.addBindValue(taskId);

    if (!query.exec())
    {
        QMessageBox::critical(
            this,
            "Database Error",
            "Could not delete the task.\n\n" +
                query.lastError().text()
        );

        return;
    }

    refreshTasks();
    refreshDashboard();
}

// ------------------------------------------------------------
// Get selected task ID
// ------------------------------------------------------------

int MainWindow::selectedTaskId() const
{
    if (!taskTable)
    {
        return -1;
    }

    const int row =
        taskTable->currentRow();

    if (row < 0)
    {
        return -1;
    }

    QTableWidgetItem *idItem =
        taskTable->item(row, 0);

    if (!idItem)
    {
        return -1;
    }

    return idItem->text().toInt();
}

// ------------------------------------------------------------
// Mark task complete or incomplete
// ------------------------------------------------------------

void MainWindow::updateTaskCompletion(
    int taskId,
    bool completed
)
{
    QSqlQuery query(database);

    query.prepare(
        "UPDATE tasks SET completed = ? WHERE id = ?"
    );

    query.addBindValue(completed ? 1 : 0);
    query.addBindValue(taskId);

    if (!query.exec())
    {
        QMessageBox::warning(
            this,
            "Database Error",
            "Could not update task completion status."
        );

        return;
    }

    refreshTasks();
    refreshDashboard();
}

// ------------------------------------------------------------
// Refresh task table
// ------------------------------------------------------------

void MainWindow::refreshTasks()
{
    if (!taskTable || !database.isOpen())
    {
        return;
    }

    loadingTasks = true;

    const QSignalBlocker blocker(taskTable);

    taskTable->setRowCount(0);

    QSqlQuery query(database);

    const QString searchText =
        taskSearch
            ? taskSearch->text().trimmed()
            : QString();

    if (searchText.isEmpty())
    {
        query.prepare(
            "SELECT id, title, priority, due_date, completed "
            "FROM tasks ORDER BY id DESC"
        );
    }
    else
    {
        query.prepare(
            "SELECT id, title, priority, due_date, completed "
            "FROM tasks "
            "WHERE title LIKE ? OR description LIKE ? "
            "ORDER BY id DESC"
        );

        const QString pattern =
            "%" + searchText + "%";

        query.addBindValue(pattern);
        query.addBindValue(pattern);
    }

    if (!query.exec())
    {
        qWarning() << "Could not load tasks:"
                   << query.lastError().text();

        loadingTasks = false;

        return;
    }

    while (query.next())
    {
        const int row =
            taskTable->rowCount();

        taskTable->insertRow(row);

        const int taskId =
            query.value(0).toInt();

        auto *idItem =
            new QTableWidgetItem(
                QString::number(taskId)
            );

        auto *titleItem =
            new QTableWidgetItem(
                query.value(1).toString()
            );

        auto *priorityItem =
            new QTableWidgetItem(
                query.value(2).toString()
            );

        auto *dateItem =
            new QTableWidgetItem(
                query.value(3).toString()
            );

        auto *completedItem =
            new QTableWidgetItem();

        completedItem->setFlags(
            Qt::ItemIsEnabled |
            Qt::ItemIsSelectable |
            Qt::ItemIsUserCheckable
        );

        completedItem->setCheckState(
            query.value(4).toInt()
                ? Qt::Checked
                : Qt::Unchecked
        );

        taskTable->setItem(row, 0, idItem);
        taskTable->setItem(row, 1, titleItem);
        taskTable->setItem(row, 2, priorityItem);
        taskTable->setItem(row, 3, dateItem);
        taskTable->setItem(row, 4, completedItem);
    }

    loadingTasks = false;

    if (taskStatsValue)
    {
        taskStatsValue->setText(
            QString("%1 tasks")
                .arg(taskTable->rowCount())
        );
    }
}

// ------------------------------------------------------------
// Refresh dashboard
// ------------------------------------------------------------

void MainWindow::refreshDashboard()
{
    if (!database.isOpen())
    {
        return;
    }

    int totalTasks = 0;
    int completedTasks = 0;
    int totalNotes = 0;

    QSqlQuery taskQuery(database);

    if (taskQuery.exec(
            "SELECT COUNT(*), "
            "SUM(CASE WHEN completed = 1 THEN 1 ELSE 0 END) "
            "FROM tasks"
        ))
    {
        if (taskQuery.next())
        {
            totalTasks =
                taskQuery.value(0).toInt();

            completedTasks =
                taskQuery.value(1).toInt();
        }
    }

    QSqlQuery notesQuery(database);

    if (notesQuery.exec(
            "SELECT COUNT(*) FROM notes"
        ))
    {
        if (notesQuery.next())
        {
            totalNotes =
                notesQuery.value(0).toInt();
        }
    }

    if (taskDashboardValue)
    {
        taskDashboardValue->setText(
            QString::number(totalTasks)
        );
    }

    // Update the other dashboard cards.
    if (pages && pages->count() > 0)
    {
        QWidget *dashboard =
            pages->widget(0);

        if (dashboard)
        {
            QLabel *completedLabel =
                dashboard->findChild<QLabel *>(
                    "completedCardValue"
                );

            QLabel *notesLabel =
                dashboard->findChild<QLabel *>(
                    "notesCardValue"
                );

            if (completedLabel)
            {
                completedLabel->setText(
                    QString::number(completedTasks)
                );
            }

            if (notesLabel)
            {
                notesLabel->setText(
                    QString::number(totalNotes)
                );
            }
        }
    }

    if (!dashboardTasksLayout)
    {
        return;
    }

    // Remove previous dashboard task widgets.
    while (QLayoutItem *item =
               dashboardTasksLayout->takeAt(0))
    {
        if (item->widget())
        {
            item->widget()->deleteLater();
        }

        delete item;
    }

    QSqlQuery upcomingQuery(database);

    upcomingQuery.prepare(
        "SELECT title, due_date, priority "
        "FROM tasks "
        "WHERE completed = 0 "
        "ORDER BY due_date ASC, id DESC "
        "LIMIT 5"
    );

    if (!upcomingQuery.exec())
    {
        return;
    }

    bool hasTasks = false;

    while (upcomingQuery.next())
    {
        hasTasks = true;

        const QString title =
            upcomingQuery.value(0).toString();

        const QString dueDate =
            upcomingQuery.value(1).toString();

        const QString priority =
            upcomingQuery.value(2).toString();

        QLabel *taskLabel =
            new QLabel(
                QString("%1\nDue: %2  •  Priority: %3")
                    .arg(title, dueDate, priority)
            );

        taskLabel->setObjectName("dashboardTaskItem");

        taskLabel->setWordWrap(true);

        taskLabel->setMinimumHeight(52);

        dashboardTasksLayout->addWidget(taskLabel);
    }

    if (!hasTasks)
    {
        QLabel *emptyLabel =
            new QLabel(
                "You're all caught up! No pending tasks."
            );

        emptyLabel->setObjectName("mutedLabel");

        emptyLabel->setWordWrap(true);

        dashboardTasksLayout->addWidget(emptyLabel);
    }

    dashboardTasksLayout->addStretch();
}

// ============================================================
// NOTES MANAGER
// ============================================================

// ------------------------------------------------------------
// Notes page interface
// ------------------------------------------------------------

QWidget *MainWindow::createNotesPage()
{
    QWidget *page = new QWidget();

    QVBoxLayout *mainLayout =
        new QVBoxLayout(page);

    mainLayout->setContentsMargins(28, 25, 28, 25);
    mainLayout->setSpacing(14);

    // Page heading
    QLabel *heading =
        new QLabel("Notes Manager");

    heading->setObjectName("pageTitle");

    QLabel *description =
        new QLabel(
            "Capture ideas, keep important information, "
            "and organize your thoughts."
        );

    description->setObjectName("mutedLabel");

    description->setWordWrap(true);

    mainLayout->addWidget(heading);
    mainLayout->addWidget(description);

    // Search bar and new-note button
    QHBoxLayout *toolbar =
        new QHBoxLayout();

    notesSearch = new QLineEdit();

    notesSearch->setPlaceholderText(
        "Search notes by title or content..."
    );

    notesSearch->setClearButtonEnabled(true);

    QPushButton *newNoteButton =
        new QPushButton("＋ New Note");

    newNoteButton->setObjectName("primaryButton");

    toolbar->addWidget(notesSearch, 1);
    toolbar->addWidget(newNoteButton);

    mainLayout->addLayout(toolbar);

    // Split the screen into note list and editor.
    QSplitter *splitter =
        new QSplitter(Qt::Horizontal);

    splitter->setChildrenCollapsible(false);

    // Left panel
    QFrame *listPanel = new QFrame();

    listPanel->setObjectName("contentCard");

    QVBoxLayout *listLayout =
        new QVBoxLayout(listPanel);

    listLayout->setContentsMargins(14, 14, 14, 14);
    listLayout->setSpacing(10);

    QLabel *listHeading =
        new QLabel("Your Notes");

    listHeading->setObjectName("sectionTitle");

    notesStatusValue =
        new QLabel("0 notes");

    notesStatusValue->setObjectName("mutedLabel");

    notesTable = new QTableWidget();

    notesTable->setColumnCount(3);

    notesTable->setHorizontalHeaderLabels(
        {
            "ID",
            "Title",
            "Last Updated"
        }
    );

    notesTable->setColumnHidden(0, true);

    notesTable->horizontalHeader()->setSectionResizeMode(
        1,
        QHeaderView::Stretch
    );

    notesTable->horizontalHeader()->setSectionResizeMode(
        2,
        QHeaderView::ResizeToContents
    );

    notesTable->verticalHeader()->setVisible(false);

    notesTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    notesTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    notesTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    notesTable->setAlternatingRowColors(true);

    listLayout->addWidget(listHeading);
    listLayout->addWidget(notesStatusValue);
    listLayout->addWidget(notesTable, 1);

    // Right panel: note editor
    QFrame *editorPanel = new QFrame();

    editorPanel->setObjectName("contentCard");

    QVBoxLayout *editorLayout =
        new QVBoxLayout(editorPanel);

    editorLayout->setContentsMargins(18, 18, 18, 18);
    editorLayout->setSpacing(12);

    QLabel *editorHeading =
        new QLabel("Note Editor");

    editorHeading->setObjectName("sectionTitle");

    QLabel *titleLabel =
        new QLabel("Note Title");

    titleLabel->setObjectName("mutedLabel");

    noteTitleInput = new QLineEdit();

    noteTitleInput->setPlaceholderText(
        "Give your note a meaningful title..."
    );

    QLabel *contentLabel =
        new QLabel("Content");

    contentLabel->setObjectName("mutedLabel");

    noteContentInput = new QTextEdit();

    noteContentInput->setPlaceholderText(
        "Start writing your thoughts here...\n\n"
        "You can use this space for ideas, reminders, "
        "meeting notes, plans, or anything important."
    );

    noteContentInput->setAcceptRichText(false);

    noteContentInput->setMinimumHeight(250);

    // Editor buttons
    QHBoxLayout *buttonLayout =
        new QHBoxLayout();

    QPushButton *saveButton =
        new QPushButton("Save Note");

    saveButton->setObjectName("primaryButton");

    QPushButton *deleteButton =
        new QPushButton("Delete");

    QPushButton *clearButton =
        new QPushButton("Clear Editor");

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(clearButton);

    QLabel *editorHint =
        new QLabel(
            "Select a note to edit it. Click Save Note "
            "to save your changes."
        );

    editorHint->setObjectName("mutedLabel");

    editorHint->setWordWrap(true);

    editorLayout->addWidget(editorHeading);
    editorLayout->addWidget(titleLabel);
    editorLayout->addWidget(noteTitleInput);
    editorLayout->addWidget(contentLabel);
    editorLayout->addWidget(noteContentInput, 1);
    editorLayout->addLayout(buttonLayout);
    editorLayout->addWidget(editorHint);

    splitter->addWidget(listPanel);
    splitter->addWidget(editorPanel);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    splitter->setSizes({420, 650});

    mainLayout->addWidget(splitter, 1);

    // Connect actions.
    connect(
        newNoteButton,
        &QPushButton::clicked,
        this,
        &MainWindow::clearNoteEditor
    );

    connect(
        saveButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (selectedNoteId() < 0)
            {
                addNote();
            }
            else
            {
                updateNote();
            }
        }
    );

    connect(
        deleteButton,
        &QPushButton::clicked,
        this,
        &MainWindow::deleteNote
    );

    connect(
        clearButton,
        &QPushButton::clicked,
        this,
        &MainWindow::clearNoteEditor
    );

    connect(
        notesSearch,
        &QLineEdit::textChanged,
        this,
        [this]()
        {
            refreshNotes();
        }
    );

    connect(
        notesTable,
        &QTableWidget::itemSelectionChanged,
        this,
        &MainWindow::loadSelectedNote
    );

    return page;
}

// ------------------------------------------------------------
// Get selected note ID
// ------------------------------------------------------------

int MainWindow::selectedNoteId() const
{
    if (!notesTable)
    {
        return -1;
    }

    const int row =
        notesTable->currentRow();

    if (row < 0)
    {
        return -1;
    }

    QTableWidgetItem *idItem =
        notesTable->item(row, 0);

    if (!idItem)
    {
        return -1;
    }

    bool ok = false;

    const int id =
        idItem->text().toInt(&ok);

    return ok ? id : -1;
}

// ------------------------------------------------------------
// Create a new note
// ------------------------------------------------------------

void MainWindow::addNote()
{
    if (!database.isOpen())
    {
        QMessageBox::warning(
            this,
            "Database Error",
            "The database is not available."
        );

        return;
    }

    const QString title =
        noteTitleInput->text().trimmed();

    const QString content =
        noteContentInput->toPlainText();

    if (title.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Title Required",
            "Please enter a title before saving your note."
        );

        noteTitleInput->setFocus();

        return;
    }

    const QString timestamp =
        QDateTime::currentDateTime().toString(Qt::ISODate);

    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO notes "
        "(title, content, created_at, updated_at) "
        "VALUES (?, ?, ?, ?)"
    );

    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(timestamp);
    query.addBindValue(timestamp);

    if (!query.exec())
    {
        QMessageBox::critical(
            this,
            "Could Not Save Note",
            "An error occurred while creating the note.\n\n" +
                query.lastError().text()
        );

        return;
    }

    const int newNoteId =
        query.lastInsertId().toInt();

    refreshNotes();

    // Select the newly created note.
    for (int row = 0; row < notesTable->rowCount(); ++row)
    {
        QTableWidgetItem *idItem =
            notesTable->item(row, 0);

        if (idItem &&
            idItem->text().toInt() == newNoteId)
        {
            notesTable->setCurrentCell(row, 1);

            break;
        }
    }

    loadSelectedNote();

    refreshDashboard();

    QMessageBox::information(
        this,
        "Note Saved",
        "Your note has been saved successfully."
    );
}

// ------------------------------------------------------------
// Update an existing note
// ------------------------------------------------------------

void MainWindow::updateNote()
{
    if (!database.isOpen())
    {
        QMessageBox::warning(
            this,
            "Database Error",
            "The database is not available."
        );

        return;
    }

    const int noteId =
        selectedNoteId();

    if (noteId < 0)
    {
        // No existing note is selected, so save as new.
        addNote();

        return;
    }

    const QString title =
        noteTitleInput->text().trimmed();

    const QString content =
        noteContentInput->toPlainText();

    if (title.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Title Required",
            "Please enter a title before saving your note."
        );

        noteTitleInput->setFocus();

        return;
    }

    const QString timestamp =
        QDateTime::currentDateTime().toString(Qt::ISODate);

    QSqlQuery query(database);

    query.prepare(
        "UPDATE notes "
        "SET title = ?, content = ?, updated_at = ? "
        "WHERE id = ?"
    );

    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(timestamp);
    query.addBindValue(noteId);

    if (!query.exec())
    {
        QMessageBox::critical(
            this,
            "Could Not Update Note",
            "An error occurred while updating the note.\n\n" +
                query.lastError().text()
        );

        return;
    }

    refreshNotes();

    // Keep the edited note selected.
    for (int row = 0; row < notesTable->rowCount(); ++row)
    {
        QTableWidgetItem *idItem =
            notesTable->item(row, 0);

        if (idItem &&
            idItem->text().toInt() == noteId)
        {
            notesTable->setCurrentCell(row, 1);

            break;
        }
    }

    loadSelectedNote();

    refreshDashboard();

    QMessageBox::information(
        this,
        "Note Updated",
        "Your changes have been saved successfully."
    );
}

// ------------------------------------------------------------
// Delete selected note
// ------------------------------------------------------------

void MainWindow::deleteNote()
{
    const int noteId =
        selectedNoteId();

    if (noteId < 0)
    {
        QMessageBox::information(
            this,
            "Select a Note",
            "Please select a note from the list before deleting."
        );

        return;
    }

    const QString title =
        noteTitleInput->text().trimmed();

    const auto answer =
        QMessageBox::question(
            this,
            "Delete Note",
            QString(
                "Are you sure you want to delete \"%1\"?\n\n"
                "This action cannot be undone."
            ).arg(title),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "DELETE FROM notes WHERE id = ?"
    );

    query.addBindValue(noteId);

    if (!query.exec())
    {
        QMessageBox::critical(
            this,
            "Could Not Delete Note",
            "An error occurred while deleting the note.\n\n" +
                query.lastError().text()
        );

        return;
    }

    clearNoteEditor();

    refreshNotes();

    refreshDashboard();

    QMessageBox::information(
        this,
        "Note Deleted",
        "The note has been deleted successfully."
    );
}

// ------------------------------------------------------------
// Load the selected note into the editor
// ------------------------------------------------------------

void MainWindow::loadSelectedNote()
{
    if (loadingNotes)
    {
        return;
    }

    const int noteId =
        selectedNoteId();

    if (noteId < 0)
    {
        return;
    }

    QSqlQuery query(database);

    query.prepare(
        "SELECT title, content "
        "FROM notes WHERE id = ?"
    );

    query.addBindValue(noteId);

    if (!query.exec() || !query.next())
    {
        QMessageBox::warning(
            this,
            "Note Not Found",
            "The selected note could not be loaded."
        );

        return;
    }

    noteTitleInput->setText(
        query.value(0).toString()
    );

    noteContentInput->setPlainText(
        query.value(1).toString()
    );
}

// ------------------------------------------------------------
// Clear the editor for a new note
// ------------------------------------------------------------

void MainWindow::clearNoteEditor()
{
    if (notesTable)
    {
        const QSignalBlocker blocker(notesTable);

        notesTable->clearSelection();
        notesTable->setCurrentItem(nullptr);
    }

    if (noteTitleInput)
    {
        noteTitleInput->clear();
    }

    if (noteContentInput)
    {
        noteContentInput->clear();
    }

    if (noteTitleInput)
    {
        noteTitleInput->setFocus();
    }
}

// ------------------------------------------------------------
// Refresh notes table and apply search
// ------------------------------------------------------------

void MainWindow::refreshNotes()
{
    if (!notesTable || !database.isOpen())
    {
        return;
    }

    loadingNotes = true;

    const int previouslySelectedId =
        selectedNoteId();

    const QSignalBlocker blocker(notesTable);

    notesTable->setRowCount(0);

    const QString searchText =
        notesSearch
            ? notesSearch->text().trimmed()
            : QString();

    QSqlQuery query(database);

    if (searchText.isEmpty())
    {
        query.prepare(
            "SELECT id, title, created_at, updated_at "
            "FROM notes "
            "ORDER BY updated_at DESC, id DESC"
        );
    }
    else
    {
        query.prepare(
            "SELECT id, title, created_at, updated_at "
            "FROM notes "
            "WHERE title LIKE ? OR content LIKE ? "
            "ORDER BY updated_at DESC, id DESC"
        );

        const QString pattern =
            "%" + searchText + "%";

        query.addBindValue(pattern);
        query.addBindValue(pattern);
    }

    if (!query.exec())
    {
        qWarning() << "Could not load notes:"
                   << query.lastError().text();

        loadingNotes = false;

        return;
    }

    int rowToRestore = -1;

    while (query.next())
    {
        const int row =
            notesTable->rowCount();

        notesTable->insertRow(row);

        const int noteId =
            query.value(0).toInt();

        const QString title =
            query.value(1).toString();

        const QString createdAt =
            query.value(2).toString();

        const QString updatedAt =
            query.value(3).toString();

        auto *idItem =
            new QTableWidgetItem(
                QString::number(noteId)
            );

        auto *titleItem =
            new QTableWidgetItem(title);

        auto *updatedItem =
            new QTableWidgetItem(updatedAt);

        // Display human-readable dates when possible.
        QDateTime updatedDateTime =
            QDateTime::fromString(
                updatedAt,
                Qt::ISODate
            );

        if (updatedDateTime.isValid())
        {
            updatedItem->setText(
                updatedDateTime.toString(
                    "dd MMM yyyy, hh:mm AP"
                )
            );
        }

        QDateTime createdDateTime =
            QDateTime::fromString(
                createdAt,
                Qt::ISODate
            );

        if (createdDateTime.isValid())
        {
            titleItem->setToolTip(
                "Created: " +
                createdDateTime.toString(
                    "dd MMM yyyy, hh:mm AP"
                )
            );
        }

        notesTable->setItem(row, 0, idItem);
        notesTable->setItem(row, 1, titleItem);
        notesTable->setItem(row, 2, updatedItem);

        if (noteId == previouslySelectedId)
        {
            rowToRestore = row;
        }
    }

    if (rowToRestore >= 0)
    {
        notesTable->setCurrentCell(rowToRestore, 1);
    }

    if (notesStatusValue)
    {
        const int count =
            notesTable->rowCount();

        notesStatusValue->setText(
            QString("%1 %2")
                .arg(count)
                .arg(count == 1 ? "note" : "notes")
        );
    }

    loadingNotes = false;
}

// ------------------------------------------------------------
// Apply the application theme
// ------------------------------------------------------------

void MainWindow::applyTheme()
{
    setStyleSheet(R"(
        QMainWindow {
            background-color: #100D1C;
            color: #F1EEFF;
        }

        QWidget {
            color: #F1EEFF;
            font-family: "Segoe UI";
            font-size: 10pt;
        }

        QWidget#sidebar {
            background-color: #171329;
            border-right: 1px solid #30264D;
        }

        QLabel#logoLabel {
            color: #C4B5FD;
            font-size: 23pt;
            font-weight: bold;
        }

        QLabel#subtitleLabel {
            color: #82789F;
            font-size: 8pt;
            letter-spacing: 1px;
        }

        QLabel#sidebarFooter {
            color: #82789F;
            font-size: 9pt;
        }

        QPushButton#navButton {
            background-color: transparent;
            color: #B9B1D4;
            border: none;
            border-radius: 8px;
            text-align: left;
            padding: 11px 13px;
        }

        QPushButton#navButton:hover {
            background-color: #29213F;
            color: #FFFFFF;
        }

        QPushButton {
            background-color: #29213F;
            color: #EDE9FE;
            border: 1px solid #40335F;
            border-radius: 7px;
            padding: 9px 15px;
        }

        QPushButton:hover {
            background-color: #382A57;
            border-color: #8B5CF6;
        }

        QPushButton:pressed {
            background-color: #211832;
        }

        QPushButton#primaryButton {
            background-color: #7C3AED;
            color: white;
            border: 1px solid #8B5CF6;
            font-weight: bold;
        }

        QPushButton#primaryButton:hover {
            background-color: #8B5CF6;
        }

        QLabel#pageTitle {
            color: #F5F3FF;
            font-size: 25pt;
            font-weight: bold;
        }

        QLabel#sectionTitle {
            color: #DDD6FE;
            font-size: 14pt;
            font-weight: bold;
        }

        QLabel#mutedLabel {
            color: #A39ABF;
        }

        QFrame#summaryCard {
            background-color: #1D1831;
            border: 1px solid #34294F;
            border-radius: 12px;
        }

        QFrame#contentCard {
            background-color: #1A152B;
            border: 1px solid #30264D;
            border-radius: 12px;
        }

        QLabel#cardValue {
            color: #C4B5FD;
            font-size: 23pt;
            font-weight: bold;
        }

        QLabel#dashboardTaskItem {
            background-color: #241D3B;
            color: #EDE9FE;
            border: 1px solid #3A2D59;
            border-radius: 7px;
            padding: 11px;
        }

        QLineEdit {
            background-color: #171329;
            color: #F5F3FF;
            border: 1px solid #40335F;
            border-radius: 7px;
            padding: 10px;
            selection-background-color: #7C3AED;
        }

        QLineEdit:focus {
            border: 1px solid #8B5CF6;
        }

        QTextEdit {
            background-color: #171329;
            color: #F5F3FF;
            border: 1px solid #40335F;
            border-radius: 7px;
            padding: 10px;
            selection-background-color: #7C3AED;
        }

        QTextEdit:focus {
            border: 1px solid #8B5CF6;
        }

        QTableWidget {
            background-color: #171329;
            alternate-background-color: #1D1831;
            color: #F1EEFF;
            gridline-color: #30264D;
            border: 1px solid #30264D;
            border-radius: 8px;
            selection-background-color: #49316F;
            selection-color: white;
        }

        QTableWidget::item {
            padding: 7px;
        }

        QHeaderView::section {
            background-color: #241D3B;
            color: #C4B5FD;
            border: none;
            border-bottom: 1px solid #40335F;
            padding: 10px;
            font-weight: bold;
        }

        QTableCornerButton::section {
            background-color: #241D3B;
            border: none;
        }

        QScrollBar:vertical {
            background-color: #171329;
            width: 10px;
            margin: 0;
        }

        QScrollBar::handle:vertical {
            background-color: #49316F;
            border-radius: 4px;
            min-height: 25px;
        }

        QScrollBar::handle:vertical:hover {
            background-color: #7C3AED;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0;
        }

        QSplitter::handle {
            background-color: #30264D;
            width: 4px;
        }

        QSplitter::handle:hover {
            background-color: #7C3AED;
        }

        QDialog {
            background-color: #171329;
        }

        QDialogButtonBox QPushButton {
            min-width: 85px;
        }

        QMessageBox {
            background-color: #171329;
        }

        QToolTip {
            background-color: #241D3B;
            color: #F5F3FF;
            border: 1px solid #7C3AED;
            padding: 5px;
        }
    )");
}