#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFrame>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QDate>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QProgressBar>
#include <QScrollArea>
#include <QHash>
#include <QList>
#include <QVariant>
#include <QStatusBar>
#include <QSizePolicy>
#include <QDebug>
#include <QPainter>
#include <QColor>
#include <QGraphicsDropShadowEffect>
#include <QTime>
#include <QPaintEvent>
#include <QCalendarWidget>
#include <QShortcut>
#include <QKeySequence>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <algorithm>
#include <cmath>

class AnalyticsChart : public QWidget
{
public:
    explicit AnalyticsChart(QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName("analyticsChart");
        setMinimumHeight(230);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void setValues(int completed, int pending, int habitsDone,
                   int habitsTotal, double income, double expenses)
    {
        m_completed = qMax(0, completed);
        m_pending = qMax(0, pending);
        m_habitsDone = qMax(0, habitsDone);
        m_habitsTotal = qMax(0, habitsTotal);
        m_income = qMax(0.0, income);
        m_expenses = qMax(0.0, expenses);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), Qt::transparent);

        const int left = 18;
        const int right = width() - 18;
        const int usable = qMax(100, right - left);
        const int gap = 18;
        const int cardWidth = qMax(100, (usable - gap * 2) / 3);
        const int top = 10;
        const int cardHeight = height() - 20;

        struct Metric { QString title; QString firstLabel; QString secondLabel; double first; double second; };
        const Metric metrics[] = {
            {"TASKS", "Completed", "Pending", double(m_completed), double(m_pending)},
            {"HABITS", "Done today", "Remaining", double(m_habitsDone), double(qMax(0, m_habitsTotal - m_habitsDone))},
            {"FINANCE", "Income", "Expenses", m_income, m_expenses}
        };

        for (int i = 0; i < 3; ++i) {
            const int x = left + i * (cardWidth + gap);
            QRect card(x, top, cardWidth, cardHeight);
            p.setPen(QPen(QColor("#34343D"), 1));
            p.setBrush(QColor("#19191D"));
            p.drawRoundedRect(card, 10, 10);

            p.setPen(QColor("#FF6871"));
            QFont titleFont = p.font();
            titleFont.setBold(true);
            titleFont.setPointSize(9);
            p.setFont(titleFont);
            p.drawText(x + 14, top + 26, metrics[i].title);

            const double maxValue = qMax(1.0, qMax(metrics[i].first, metrics[i].second));
            const int barX = x + 14;
            const int barWidth = qMax(40, cardWidth - 28);
            int barY = top + 52;
            const int barHeight = 12;

            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#303038"));
            p.drawRoundedRect(barX, barY, barWidth, barHeight, 5, 5);
            p.setBrush(QColor("#E63240"));
            p.drawRoundedRect(barX, barY, int(barWidth * metrics[i].first / maxValue), barHeight, 5, 5);

            p.setPen(QColor("#D8D8E0"));
            QFont bodyFont = p.font();
            bodyFont.setBold(false);
            bodyFont.setPointSize(9);
            p.setFont(bodyFont);
            QString firstValue = i == 2 ? QString("₹%1").arg(metrics[i].first, 0, 'f', 0) : QString::number(int(metrics[i].first));
            p.drawText(barX, barY + 29, metrics[i].firstLabel);
            p.drawText(x + cardWidth - 65, barY + 29, 52, 16, Qt::AlignRight, firstValue);

            barY += 48;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#303038"));
            p.drawRoundedRect(barX, barY, barWidth, barHeight, 5, 5);
            p.setBrush(QColor("#777783"));
            p.drawRoundedRect(barX, barY, int(barWidth * metrics[i].second / maxValue), barHeight, 5, 5);
            p.setPen(QColor("#D8D8E0"));
            QString secondValue = i == 2 ? QString("₹%1").arg(metrics[i].second, 0, 'f', 0) : QString::number(int(metrics[i].second));
            p.drawText(barX, barY + 29, metrics[i].secondLabel);
            p.drawText(x + cardWidth - 65, barY + 29, 52, 16, Qt::AlignRight, secondValue);
        }
    }

private:
    int m_completed = 0;
    int m_pending = 0;
    int m_habitsDone = 0;
    int m_habitsTotal = 0;
    double m_income = 0.0;
    double m_expenses = 0.0;
};

class LifeOS : public QMainWindow
{
public:
    LifeOS(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("LifeOS 3.0 | Professional Edition");
        setMinimumSize(1100, 720);
        resize(1450, 900);

        if (!openDatabase()) {
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
            return;
        }
        createInterface();
        applyTheme();
        refreshEverything();
        showPage("Dashboard");
        setupReminders();
    }

private:

    QSqlDatabase database;
    QString databasePath;
    QSystemTrayIcon *trayIcon = nullptr;
    QTimer *reminderTimer = nullptr;
    QDate lastReminderDate;

    QWidget *root = nullptr;
    QStackedWidget *pages = nullptr;
    QWidget *dashboardPage = nullptr;

    QHash<QString, QWidget*> pageMap;
    QHash<QString, QTableWidget*> tableMap;
    QHash<QString, QLineEdit*> searchMap;

    QLabel *welcomeLabel = nullptr;
    QLabel *dateLabel = nullptr;

    QLabel *taskCountLabel = nullptr;
    QLabel *completedCountLabel = nullptr;
    QLabel *noteCountLabel = nullptr;
    QLabel *eventCountLabel = nullptr;
    QLabel *balanceLabel = nullptr;
    QLabel *insightLabel = nullptr;

    QProgressBar *taskProgress = nullptr;

    struct Module
    {
        QString name;
        QString table;
        QStringList fields;
        QStringList headers;
        QStringList searchFields;
    };

    QList<Module> modules;

    QString timestamp() const
    {
        return QDateTime::currentDateTime().toString(Qt::ISODate);
    }

    // -------------------------------------------------------
    // DATABASE
    // -------------------------------------------------------

    bool openDatabase()
    {
        QString folder =
            QStandardPaths::writableLocation(
                QStandardPaths::AppDataLocation
            );

        if (folder.isEmpty())
            folder = QDir::homePath() + "/LifeOS";

        QDir().mkpath(folder);

        databasePath = folder + "/lifeos.db";
        database = QSqlDatabase::addDatabase("QSQLITE");
        database.setDatabaseName(databasePath);

        if (!database.open()) {
            QMessageBox::critical(
                nullptr,
                "Database Error",
                database.lastError().text()
            );
            return false;
        }

        QStringList statements = {

            R"(
                CREATE TABLE IF NOT EXISTS tasks (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    title TEXT NOT NULL,
                    description TEXT DEFAULT '',
                    priority TEXT NOT NULL DEFAULT 'Medium',
                    due_date TEXT NOT NULL DEFAULT '',
                    completed INTEGER NOT NULL DEFAULT 0,
                    created_at TEXT NOT NULL DEFAULT ''
                )
            )",

            R"(
                CREATE TABLE IF NOT EXISTS notes (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    title TEXT NOT NULL,
                    content TEXT NOT NULL DEFAULT '',
                    created_at TEXT NOT NULL,
                    updated_at TEXT NOT NULL
                )
            )",

            R"(
                CREATE TABLE IF NOT EXISTS events (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    title TEXT NOT NULL,
                    event_date TEXT NOT NULL,
                    event_time TEXT NOT NULL DEFAULT '',
                    location TEXT DEFAULT '',
                    description TEXT DEFAULT '',
                    created_at TEXT NOT NULL
                )
            )",

            R"(
                CREATE TABLE IF NOT EXISTS habits (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    title TEXT NOT NULL,
                    frequency TEXT NOT NULL DEFAULT 'Daily',
                    completed_today INTEGER NOT NULL DEFAULT 0,
                    streak INTEGER NOT NULL DEFAULT 0,
                    created_at TEXT NOT NULL
                )
            )",

            R"(
                CREATE TABLE IF NOT EXISTS transactions (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    title TEXT NOT NULL,
                    type TEXT NOT NULL,
                    amount REAL NOT NULL DEFAULT 0,
                    category TEXT DEFAULT '',
                    transaction_date TEXT NOT NULL,
                    notes TEXT DEFAULT ''
                )
            )"
        };

        for (const QString &sql : statements) {
            QSqlQuery query(database);

            if (!query.exec(sql)) {
                qWarning() << query.lastError().text();
                return false;
            }
        }

        return true;
    }

    int countRows(const QString &table,
                  const QString &condition = QString())
    {
        QString sql = "SELECT COUNT(*) FROM " + table;

        if (!condition.isEmpty())
            sql += " WHERE " + condition;

        QSqlQuery query(database);

        if (!query.exec(sql) || !query.next())
            return 0;

        return query.value(0).toInt();
    }

    double financeTotal(const QString &type)
    {
        QSqlQuery query(database);

        query.prepare(
            "SELECT COALESCE(SUM(amount), 0) "
            "FROM transactions WHERE type = ?"
        );

        query.addBindValue(type);

        if (!query.exec() || !query.next())
            return 0.0;

        return query.value(0).toDouble();
    }

    // -------------------------------------------------------
    // INTERFACE
    // -------------------------------------------------------

    void createInterface()
    {
        root = new QWidget;
        root->setObjectName("root");
        setCentralWidget(root);

        auto *mainLayout = new QHBoxLayout(root);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // SIDEBAR

        auto *sidebar = new QWidget;
        sidebar->setObjectName("sidebar");
        sidebar->setFixedWidth(235);

        auto *sideLayout = new QVBoxLayout(sidebar);
        sideLayout->setContentsMargins(18, 24, 18, 22);
        sideLayout->setSpacing(9);

        auto *brand = new QLabel("LIFEOS");
        brand->setObjectName("brand");

        auto *brandSubtitle =
            new QLabel("PERSONAL COMMAND CENTER");

        brandSubtitle->setObjectName("muted");
        brandSubtitle->setWordWrap(true);

        sideLayout->addWidget(brand);
        sideLayout->addWidget(brandSubtitle);
        sideLayout->addSpacing(14);

        auto *globalSearchButton = new QPushButton("⌕   Search everything");
        globalSearchButton->setObjectName("secondaryButton");
        globalSearchButton->setMinimumHeight(40);
        globalSearchButton->setToolTip("Search tasks, notes, events, habits and transactions (Ctrl+K)");
        connect(globalSearchButton, &QPushButton::clicked, this, [this]() { showGlobalSearch(); });
        sideLayout->addWidget(globalSearchButton);
        auto *searchShortcut = new QShortcut(QKeySequence("Ctrl+K"), this);
        connect(searchShortcut, &QShortcut::activated, this, [this]() { showGlobalSearch(); });
        sideLayout->addSpacing(12);

        auto *navHeading = new QLabel("WORKSPACE");
        navHeading->setObjectName("sectionLabel");

        sideLayout->addWidget(navHeading);

        QStringList navigation = {
            "Dashboard",
            "Tasks",
            "Notes",
            "Calendar",
            "Habits",
            "Finance",
            "Analytics",
            "Settings"
        };

        for (const QString &name : navigation) {
            auto *button = new QPushButton("   " + name);
            button->setObjectName("navButton");
            button->setCursor(Qt::PointingHandCursor);
            button->setMinimumHeight(43);

            connect(button, &QPushButton::clicked,
                    this, [this, name]() {
                showPage(name);
            });

            sideLayout->addWidget(button);
        }

        sideLayout->addStretch();

        auto *version = new QLabel(
            "LIFEOS DESKTOP\nVersion 3.0"
        );

        version->setObjectName("muted");
        sideLayout->addWidget(version);

        // MAIN STACK

        pages = new QStackedWidget;
        pages->setObjectName("pages");

        dashboardPage = createDashboard();
        pages->addWidget(dashboardPage);
        pageMap["Dashboard"] = dashboardPage;

        configureModules();

        for (const Module &module : modules)
            createModulePage(module);

        createAnalyticsPage();
        createSettingsPage();

        mainLayout->addWidget(sidebar);
        mainLayout->addWidget(pages, 1);

        statusBar()->showMessage(
            "LifeOS ready  |  Your personal command center"
        );
    }

    QWidget *createPageContainer(const QString &title,
                                 const QString &subtitle)
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);

        layout->setContentsMargins(30, 26, 30, 26);
        layout->setSpacing(18);

        auto *heading = new QLabel(title);
        heading->setObjectName("pageTitle");

        auto *description = new QLabel(subtitle);
        description->setObjectName("muted");
        description->setWordWrap(true);

        layout->addWidget(heading);
        layout->addWidget(description);

        return page;
    }

    void showPage(const QString &name)
    {
        if (!pageMap.contains(name))
            return;

        pages->setCurrentWidget(pageMap[name]);

        statusBar()->showMessage(
            "LifeOS  /  " + name,
            2500
        );

        if (name == "Dashboard")
            refreshDashboard();

        if (name == "Analytics")
            refreshAnalytics();
    }

    // -------------------------------------------------------
    // PREMIUM UI HELPERS
    // -------------------------------------------------------

    void applyPremiumShadow(QWidget *widget)
    {
        if (!widget)
            return;

        auto *shadow = new QGraphicsDropShadowEffect(widget);
        shadow->setBlurRadius(26);
        shadow->setOffset(0, 5);
        shadow->setColor(QColor(230, 50, 64, 38));
        widget->setGraphicsEffect(shadow);
    }

    // -------------------------------------------------------
    // DASHBOARD
    // -------------------------------------------------------

    QWidget *createDashboard()
    {
        // A scrollable dashboard keeps every premium panel visible on smaller screens.
        auto *page = new QWidget;
        auto *outerLayout = new QVBoxLayout(page);
        outerLayout->setContentsMargins(0, 0, 0, 0);
        outerLayout->setSpacing(0);

        auto *scroll = new QScrollArea(page);
        scroll->setObjectName("dashboardScroll");
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);

        auto *content = new QWidget;
        auto *layout = new QVBoxLayout(content);

        layout->setContentsMargins(30, 25, 30, 25);
        layout->setSpacing(20);

        auto *topRow = new QHBoxLayout;

        auto *headingBox = new QVBoxLayout;

        welcomeLabel = new QLabel("Welcome back.");
        welcomeLabel->setObjectName("pageTitle");

        dateLabel = new QLabel;
        dateLabel->setObjectName("muted");

        headingBox->addWidget(welcomeLabel);
        headingBox->addWidget(dateLabel);

        auto *quickTask = new QPushButton("+ New Task");
        quickTask->setObjectName("primaryButton");

        connect(quickTask, &QPushButton::clicked,
                this, [this]() {
            addRecord("Tasks");
        });

        auto *quickNote = new QPushButton("+ New Note");
        quickNote->setObjectName("secondaryButton");

        connect(quickNote, &QPushButton::clicked,
                this, [this]() {
            addRecord("Notes");
        });

        topRow->addLayout(headingBox);
        topRow->addStretch();
        topRow->addWidget(quickNote);
        topRow->addWidget(quickTask);

        layout->addLayout(topRow);

        // HERO PANEL

        auto *hero = new QFrame;
        hero->setObjectName("heroPanel");
        applyPremiumShadow(hero);

        auto *heroLayout = new QVBoxLayout(hero);
        heroLayout->setContentsMargins(26, 23, 26, 23);
        heroLayout->setSpacing(12);

        auto *heroTag = new QLabel("YOUR DAY. YOUR DIRECTION.");
        heroTag->setObjectName("heroTag");

        auto *heroTitle = new QLabel(
            "Make today count."
        );

        heroTitle->setObjectName("heroTitle");

        auto *heroDescription = new QLabel(
            "Plan smarter, build better habits, and keep "
            "everything important in one place."
        );

        heroDescription->setObjectName("heroDescription");
        heroDescription->setWordWrap(true);

        auto *heroButtons = new QHBoxLayout;

        auto *viewTasks = new QPushButton("View my tasks  →");
        viewTasks->setObjectName("primaryButton");

        connect(viewTasks, &QPushButton::clicked,
                this, [this]() {
            showPage("Tasks");
        });

        auto *viewAnalytics =
            new QPushButton("Explore analytics");

        viewAnalytics->setObjectName("secondaryButton");

        connect(viewAnalytics, &QPushButton::clicked,
                this, [this]() {
            showPage("Analytics");
        });

        heroButtons->addWidget(viewTasks);
        heroButtons->addWidget(viewAnalytics);
        heroButtons->addStretch();

        heroLayout->addWidget(heroTag);
        heroLayout->addWidget(heroTitle);
        heroLayout->addWidget(heroDescription);
        heroLayout->addLayout(heroButtons);

        layout->addWidget(hero);

        // STAT CARDS

        auto *statsGrid = new QGridLayout;
        statsGrid->setSpacing(14);

        taskCountLabel = new QLabel("0");
        completedCountLabel = new QLabel("0");
        noteCountLabel = new QLabel("0");
        eventCountLabel = new QLabel("0");
        balanceLabel = new QLabel("₹0.00");

        statsGrid->addWidget(
            createStatCard("TOTAL TASKS", taskCountLabel, "↗"),
            0, 0
        );

        statsGrid->addWidget(
            createStatCard("COMPLETED TASKS",
                           completedCountLabel, "✓"),
            0, 1
        );

        statsGrid->addWidget(
            createStatCard("SAVED NOTES", noteCountLabel, "✎"),
            0, 2
        );

        statsGrid->addWidget(
            createStatCard("UPCOMING EVENTS", eventCountLabel, "◷"),
            0, 3
        );

        statsGrid->addWidget(
            createStatCard("FINANCIAL BALANCE",
                           balanceLabel, "₹"),
            0, 4
        );

        layout->addLayout(statsGrid);

        // PROGRESS PANEL

        auto *progressCard = new QFrame;
        progressCard->setObjectName("card");

        auto *progressLayout = new QVBoxLayout(progressCard);
        progressLayout->setContentsMargins(22, 18, 22, 18);
        progressLayout->setSpacing(10);

        auto *progressTitle = new QLabel("Today's momentum");
        progressTitle->setObjectName("cardTitle");

        auto *progressSubtitle = new QLabel(
            "Your task completion progress"
        );
        progressSubtitle->setObjectName("muted");

        taskProgress = new QProgressBar;
        taskProgress->setRange(0, 100);
        taskProgress->setValue(0);
        taskProgress->setTextVisible(true);
        taskProgress->setFormat("%p% complete");

        progressLayout->addWidget(progressTitle);
        progressLayout->addWidget(progressSubtitle);
        progressLayout->addWidget(taskProgress);

        applyPremiumShadow(progressCard);
        layout->addWidget(progressCard);

        // SMART DAILY INSIGHT
        auto *insightCard = new QFrame;
        insightCard->setObjectName("insightCard");
        applyPremiumShadow(insightCard);

        auto *insightLayout = new QVBoxLayout(insightCard);
        insightLayout->setContentsMargins(20, 16, 20, 16);
        insightLayout->setSpacing(7);

        auto *insightHeading = new QLabel("✦  LIFEOS DAILY INTELLIGENCE");
        insightHeading->setObjectName("insightHeading");

        insightLabel = new QLabel(
            "Your personalized productivity insight will appear here."
        );
        insightLabel->setObjectName("insightText");
        insightLabel->setWordWrap(true);
        insightLabel->setMinimumHeight(38);

        insightLayout->addWidget(insightHeading);
        insightLayout->addWidget(insightLabel);
        layout->addWidget(insightCard);

        // TODAY'S TASKS

        auto *bottomRow = new QHBoxLayout;
        bottomRow->setSpacing(16);

        auto *tasksCard = new QFrame;
        tasksCard->setObjectName("card");
        applyPremiumShadow(tasksCard);

        auto *tasksLayout = new QVBoxLayout(tasksCard);
        tasksLayout->setContentsMargins(20, 18, 20, 18);

        auto *todayTitle = new QLabel("Recent tasks");
        todayTitle->setObjectName("cardTitle");

        auto *todayTasks = new QLabel;
        todayTasks->setObjectName("dashboardList");
        todayTasks->setWordWrap(true);
        todayTasks->setMinimumHeight(100);

        auto *openTasks = new QPushButton("Open Task Manager  →");
        openTasks->setObjectName("textButton");

        connect(openTasks, &QPushButton::clicked,
                this, [this]() {
            showPage("Tasks");
        });

        tasksLayout->addWidget(todayTitle);
        tasksLayout->addWidget(todayTasks);
        tasksLayout->addWidget(openTasks);
        tasksLayout->addStretch();

        auto *actionsCard = new QFrame;
        actionsCard->setObjectName("card");
        applyPremiumShadow(actionsCard);

        auto *actionsLayout = new QVBoxLayout(actionsCard);
        actionsLayout->setContentsMargins(20, 18, 20, 18);

        auto *actionsTitle = new QLabel("Quick actions");
        actionsTitle->setObjectName("cardTitle");

        actionsLayout->addWidget(actionsTitle);

        QStringList actionNames = {
            "＋  Create a task",
            "✎  Write a note",
            "◷  Add an event",
            "₹  Record a transaction",
            "◎  Track a habit"
        };

        QStringList destinations = {
            "Tasks", "Notes", "Calendar", "Finance", "Habits"
        };

        for (int i = 0; i < actionNames.size(); ++i) {
            auto *button = new QPushButton(actionNames[i]);
            button->setObjectName("actionButton");
            button->setMinimumHeight(37);

            QString destination = destinations[i];

            connect(button, &QPushButton::clicked,
                    this, [this, destination]() {
                addRecord(destination);
            });

            actionsLayout->addWidget(button);
        }

        bottomRow->addWidget(tasksCard, 3);
        bottomRow->addWidget(actionsCard, 2);

        layout->addLayout(bottomRow);
        layout->addStretch();

        // Store dashboard task list as a child label for refreshing.
        todayTasks->setObjectName("recentTasksLabel");

        scroll->setWidget(content);
        outerLayout->addWidget(scroll);
        return page;
    }

    QFrame *createStatCard(const QString &title,
                           QLabel *value,
                           const QString &symbol)
    {
        auto *card = new QFrame;
        card->setObjectName("statCard");
        card->setMinimumHeight(120);
        applyPremiumShadow(card);

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(15, 14, 15, 14);
        layout->setSpacing(8);

        auto *top = new QHBoxLayout;

        auto *name = new QLabel(title);
        name->setObjectName("statLabel");
        name->setWordWrap(true);

        auto *icon = new QLabel(symbol);
        icon->setObjectName("statIcon");

        top->addWidget(name, 1);
        top->addWidget(icon);

        value->setObjectName("statValue");

        layout->addLayout(top);
        layout->addWidget(value);
        layout->addStretch();

        return card;
    }

    void refreshDashboard()
    {
        if (!taskCountLabel)
            return;

        const int total = countRows("tasks");
        const int completed =
            countRows("tasks", "completed = 1");

        taskCountLabel->setText(QString::number(total));
        completedCountLabel->setText(QString::number(completed));
        noteCountLabel->setText(
            QString::number(countRows("notes"))
        );

        eventCountLabel->setText(
            QString::number(
                countRows(
                    "events",
                    "event_date >= '" +
                    QDate::currentDate().toString("yyyy-MM-dd") + "'"
                )
            )
        );

        const double income = financeTotal("Income");
        const double expenses = financeTotal("Expense");

        balanceLabel->setText(
            QString("₹%1").arg(income - expenses, 0, 'f', 2)
        );

        const int percent = total > 0
            ? (completed * 100 / total)
            : 0;

        taskProgress->setValue(percent);

        const QTime currentTime = QTime::currentTime();
        QString greeting;
        if (currentTime.hour() < 12)
            greeting = "Good morning";
        else if (currentTime.hour() < 17)
            greeting = "Good afternoon";
        else
            greeting = "Good evening";
        welcomeLabel->setText(greeting + ".");

        dateLabel->setText(
            QDate::currentDate().toString("dddd, dd MMMM yyyy")
        );

        // Build insights from the user's actual local database.
        const QString today = QDate::currentDate().toString("yyyy-MM-dd");
        const int pending = qMax(0, total - completed);
        const int overdue = countRows(
            "tasks",
            "completed = 0 AND due_date <> '' AND due_date < '" + today + "'"
        );
        const int totalHabits = countRows("habits");
        const int habitsDone = countRows("habits", "completed_today = 1");
        QString insight;

        if (total == 0) {
            insight = "Start small: add one important task for today. A clear first step makes it easier to build momentum.";
        } else if (overdue > 0) {
            insight = QString("You have %1 overdue task%2. Review the deadlines and either finish, reschedule, or remove anything that is no longer relevant.")
                .arg(overdue)
                .arg(overdue == 1 ? "" : "s");
        } else if (pending == 0) {
            insight = "Excellent work — every task currently in your list is marked complete. Choose one meaningful priority for your next session.";
        } else if (percent >= 75) {
            insight = QString("You're building strong momentum: %1% of your tasks are complete. Finish one of the remaining %2 task%3 to keep the progress going.")
                .arg(percent)
                .arg(pending)
                .arg(pending == 1 ? "" : "s");
        } else if (percent >= 40) {
            insight = QString("You're making progress: %1 of %2 tasks are complete. Pick the most important remaining task and give it your next focused block.")
                .arg(completed)
                .arg(total);
        } else {
            insight = QString("Keep the day manageable. You have %1 pending task%2; start with the smallest high-priority task rather than trying to do everything at once.")
                .arg(pending)
                .arg(pending == 1 ? "" : "s");
        }

        if (totalHabits > 0) {
            if (habitsDone == totalHabits)
                insight += " All tracked habits are marked complete today — great consistency.";
            else
                insight += QString(" Habits: %1 of %2 completed today.")
                    .arg(habitsDone)
                    .arg(totalHabits);
        }

        if (income - expenses < 0)
            insight += " Your recorded expenses are currently higher than your recorded income.";

        if (insightLabel)
            insightLabel->setText(insight);

        auto *recent = findChild<QLabel*>("recentTasksLabel");

        if (recent) {
            QSqlQuery query(database);

            query.exec(
                "SELECT title, completed, priority, due_date "
                "FROM tasks ORDER BY id DESC LIMIT 5"
            );

            QString text;

            while (query.next()) {
                const QString title = query.value(0).toString();
                const bool done = query.value(1).toInt() == 1;
                const QString priority = query.value(2).toString();
                const QString due = query.value(3).toString();

                text += QString("%1  %2\n")
                    .arg(done ? "✓" : "○")
                    .arg(title);

                text += QString("     %1 • %2\n\n")
                    .arg(priority)
                    .arg(due.isEmpty() ? "No due date" : due);
            }

            if (text.isEmpty())
                text = "No tasks yet.\nCreate your first task to get started.";

            recent->setText(text.trimmed());
        }
    }

    // -------------------------------------------------------
    // MODULE DEFINITIONS
    // -------------------------------------------------------

    void configureModules()
    {
        modules = {
            {
                "Tasks",
                "tasks",
                {"title","description","priority","due_date","completed"},
                {"Task","Description","Priority","Due date","Completed"},
                {"title","description"}
            },
            {
                "Notes",
                "notes",
                {"title","content"},
                {"Title","Content"},
                {"title","content"}
            },
            {
                "Calendar",
                "events",
                {"title","event_date","event_time","location","description"},
                {"Event","Date","Time","Location","Description"},
                {"title","location","description"}
            },
            {
                "Habits",
                "habits",
                {"title","frequency","completed_today","streak"},
                {"Habit","Frequency","Done today","Streak"},
                {"title","frequency"}
            },
            {
                "Finance",
                "transactions",
                {"title","type","amount","category","transaction_date","notes"},
                {"Transaction","Type","Amount","Category","Date","Notes"},
                {"title","category","notes"}
            }
        };
    }

    // -------------------------------------------------------
    // MODULE PAGES
    // -------------------------------------------------------

    void createModulePage(const Module &module)
    {
        auto *page = createPageContainer(
            module.name,
            moduleDescription(module.name)
        );

        auto *layout =
            qobject_cast<QVBoxLayout*>(page->layout());

        if (module.name == "Calendar") {
            auto *calendar = new QCalendarWidget;
            calendar->setObjectName("lifeosCalendar");
            calendar->setGridVisible(true);
            calendar->setMinimumHeight(230);
            calendar->setMaximumHeight(270);
            layout->addWidget(calendar);
            connect(calendar, &QCalendarWidget::selectionChanged,
                    this, [this, calendar]() {
                if (searchMap.contains("Calendar")) {
                    const QString selected = calendar->selectedDate().toString("yyyy-MM-dd");
                    searchMap["Calendar"]->setText(selected);
                    statusBar()->showMessage("Calendar date selected: " + selected, 2500);
                }
            });
        }

        auto *toolbar = new QHBoxLayout;

        auto *search = new QLineEdit;
        search->setPlaceholderText(
            "Search " + module.name.toLower() + "..."
        );

        search->setClearButtonEnabled(true);
        searchMap[module.name] = search;

        auto *add = new QPushButton("+ Add");
        add->setObjectName("primaryButton");

        auto *edit = new QPushButton("Edit");
        edit->setObjectName("secondaryButton");

        auto *remove = new QPushButton("Delete");
        remove->setObjectName("dangerButton");

        toolbar->addWidget(search, 1);
        toolbar->addWidget(add);
        toolbar->addWidget(edit);
        toolbar->addWidget(remove);

        layout->addLayout(toolbar);

        auto *table = new QTableWidget;
        table->setColumnCount(module.headers.size() + 1);

        QStringList headers = {"ID"};
        headers.append(module.headers);

        table->setHorizontalHeaderLabels(headers);
        table->setColumnHidden(0, true);
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setSectionResizeMode(
            QHeaderView::ResizeToContents
        );
        table->horizontalHeader()->setStretchLastSection(true);

        tableMap[module.name] = table;

        layout->addWidget(table, 1);

        auto *footer = new QLabel("0 records");
        footer->setObjectName("muted");

        layout->addWidget(footer);

        pageMap[module.name] = page;
        pages->addWidget(page);

        connect(search, &QLineEdit::textChanged,
                this, [this, name = module.name]() {
            refreshModule(name);
        });

        connect(add, &QPushButton::clicked,
                this, [this, name = module.name]() {
            addRecord(name);
        });

        connect(edit, &QPushButton::clicked,
                this, [this, name = module.name]() {
            editRecord(name);
        });

        connect(remove, &QPushButton::clicked,
                this, [this, name = module.name]() {
            deleteRecord(name);
        });

        connect(table, &QTableWidget::cellDoubleClicked,
                this, [this, name = module.name]() {
            editRecord(name);
        });

        footer->setObjectName("footer_" + module.name);
    }

    QString moduleDescription(const QString &name) const
    {
        if (name == "Tasks")
            return "Organize your priorities and track completion.";

        if (name == "Notes")
            return "Keep your ideas, thoughts, and important information.";

        if (name == "Calendar")
            return "Manage events, appointments, and upcoming plans.";

        if (name == "Habits")
            return "Build consistency by tracking your daily habits.";

        if (name == "Finance")
            return "Track income, expenses, and your overall balance.";

        return "Manage your LifeOS workspace.";
    }

    Module moduleFor(const QString &name) const
    {
        for (const Module &module : modules) {
            if (module.name == name)
                return module;
        }

        return Module();
    }

    void refreshModule(const QString &name)
    {
        const Module module = moduleFor(name);
        if (module.name.isEmpty() || !tableMap.contains(name))
            return;

        QTableWidget *table = tableMap.value(name, nullptr);
        if (!table)
            return;

        table->setRowCount(0);

        QStringList selectedColumns{"id"};
        selectedColumns.append(module.fields);
        const QString sql = "SELECT " + selectedColumns.join(", ") +
                            " FROM " + module.table + " ORDER BY id DESC";

        QSqlQuery query(database);
        if (!query.exec(sql)) {
            statusBar()->showMessage(
                "Could not refresh " + name + ": " + query.lastError().text(), 7000);
            return;
        }

        const QString filter = searchMap.contains(name) && searchMap.value(name)
            ? searchMap.value(name)->text().trimmed().toLower()
            : QString();

        int shown = 0;
        while (query.next()) {
            // Keep visible cell values and searchable values separate.
            // Search-only date fields must never increase the visible column count.
            QStringList displayValues;
            QStringList searchableValues;
            displayValues.reserve(module.fields.size());

            for (int i = 0; i < module.fields.size(); ++i) {
                const QString field = module.fields.at(i);
                const QVariant rawValue = query.value(i + 1);
                QString displayValue;

                if (field == "completed" || field == "completed_today")
                    displayValue = rawValue.toInt() == 1 ? "Yes" : "No";
                else if (field == "amount")
                    displayValue = QString("₹%1").arg(rawValue.toDouble(), 0, 'f', 2);
                else
                    displayValue = rawValue.toString();

                displayValues.append(displayValue);
                searchableValues.append(displayValue);
                searchableValues.append(rawValue.toString());
            }

            // The date is already among the selected fields, but explicitly
            // include it in matching to make date searches intuitive.
            const QString dateField = name == "Calendar" ? "event_date" :
                                      name == "Tasks" ? "due_date" :
                                      name == "Finance" ? "transaction_date" : QString();
            if (!dateField.isEmpty()) {
                const int fieldIndex = module.fields.indexOf(dateField);
                if (fieldIndex >= 0)
                    searchableValues.append(query.value(fieldIndex + 1).toString());
            }

            if (!filter.isEmpty()) {
                bool matches = false;
                for (const QString &value : searchableValues) {
                    if (value.toLower().contains(filter)) {
                        matches = true;
                        break;
                    }
                }
                if (!matches)
                    continue;
            }

            const int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));

            for (int col = 0; col < displayValues.size(); ++col) {
                auto *item = new QTableWidgetItem(displayValues.at(col));
                item->setToolTip(displayValues.at(col));
                const QString field = module.fields.at(col);
                if ((field == "completed" || field == "completed_today") &&
                    displayValues.at(col) == "Yes") {
                    item->setForeground(QColor("#ff626b"));
                }
                table->setItem(row, col + 1, item);
            }
            ++shown;
        }

        if (auto *footer = findChild<QLabel*>("footer_" + name))
            footer->setText(QString("%1 record(s)").arg(shown));
    }

    // -------------------------------------------------------
    // ADD / EDIT DIALOG
    // -------------------------------------------------------

    QWidget *createEditor(const QString &field,
                          const QVariant &value,
                          QWidget *parent)
    {
        if (field == "description" ||
            field == "content" ||
            field == "notes") {

            auto *editor = new QTextEdit(parent);
            editor->setMinimumHeight(95);
            editor->setPlainText(value.toString());
            return editor;
        }

        if (field == "priority") {
            auto *combo = new QComboBox(parent);
            combo->addItems({"Low", "Medium", "High", "Urgent"});
            combo->setCurrentText(
                value.toString().isEmpty()
                    ? "Medium"
                    : value.toString()
            );
            return combo;
        }

        if (field == "type") {
            auto *combo = new QComboBox(parent);
            combo->addItems({"Income", "Expense"});
            combo->setCurrentText(
                value.toString().isEmpty()
                    ? "Expense"
                    : value.toString()
            );
            return combo;
        }

        if (field == "frequency") {
            auto *combo = new QComboBox(parent);
            combo->addItems({"Daily", "Weekly", "Weekdays", "Custom"});
            combo->setCurrentText(
                value.toString().isEmpty()
                    ? "Daily"
                    : value.toString()
            );
            return combo;
        }

        if (field == "completed" ||
            field == "completed_today") {
            auto *combo = new QComboBox(parent);
            combo->addItem("No", 0);
            combo->addItem("Yes", 1);

            int index = combo->findData(value.toInt());
            combo->setCurrentIndex(index < 0 ? 0 : index);

            return combo;
        }

        if (field == "streak") {
            auto *spin = new QDoubleSpinBox(parent);
            spin->setDecimals(0);
            spin->setRange(0, 100000);
            spin->setValue(value.toDouble());
            return spin;
        }

        if (field == "amount") {
            auto *spin = new QDoubleSpinBox(parent);
            spin->setRange(0, 999999999);
            spin->setDecimals(2);
            spin->setPrefix("₹ ");
            spin->setValue(value.toDouble());
            return spin;
        }

        if (field == "due_date" ||
            field == "event_date" ||
            field == "transaction_date") {

            auto *date = new QDateEdit(parent);
            date->setCalendarPopup(true);
            date->setDisplayFormat("yyyy-MM-dd");

            QDate parsed = QDate::fromString(
                value.toString(),
                "yyyy-MM-dd"
            );

            date->setDate(
                parsed.isValid() ? parsed : QDate::currentDate()
            );

            return date;
        }

        if (field == "event_time") {
            auto *edit = new QLineEdit(parent);
            edit->setPlaceholderText("e.g. 14:30");
            edit->setText(value.toString());
            return edit;
        }

        auto *edit = new QLineEdit(parent);
        edit->setText(value.toString());

        if (field == "title")
            edit->setPlaceholderText("Enter a title");

        return edit;
    }

    QVariant editorValue(const QString &field, QWidget *editor)
    {
        if (auto *text = qobject_cast<QTextEdit*>(editor))
            return text->toPlainText();

        if (auto *combo = qobject_cast<QComboBox*>(editor)) {
            if (field == "completed" ||
                field == "completed_today") {
                return combo->currentData();
            }

            return combo->currentText();
        }

        if (auto *spin = qobject_cast<QDoubleSpinBox*>(editor))
            return spin->value();

        if (auto *date = qobject_cast<QDateEdit*>(editor))
            return date->date().toString("yyyy-MM-dd");

        if (auto *line = qobject_cast<QLineEdit*>(editor))
            return line->text().trimmed();

        return {};
    }

    void addRecord(const QString &name)
    {
        Module module = moduleFor(name);

        if (module.name.isEmpty())
            return;

        QDialog dialog(this);
        dialog.setWindowTitle("Create " + name);
        dialog.resize(520, 500);

        auto *layout = new QVBoxLayout(&dialog);
        auto *form = new QFormLayout;
        form->setSpacing(12);

        QHash<QString, QWidget*> editors;

        for (const QString &field : module.fields) {
            QWidget *editor = createEditor(field, {}, &dialog);
            editors[field] = editor;

            QString label = field;
            label.replace("_", " ");
            label = label.left(1).toUpper() + label.mid(1);

            form->addRow(label, editor);
        }

        layout->addLayout(form);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Save |
            QDialogButtonBox::Cancel
        );

        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted,
                &dialog, &QDialog::accept);

        connect(buttons, &QDialogButtonBox::rejected,
                &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted)
            return;

        if (editorValue("title", editors["title"])
                .toString().trimmed().isEmpty()) {
            QMessageBox::warning(
                this, "Required field",
                "Please enter a title before saving."
            );
            return;
        }

        if (module.table == "transactions") {
            const double amount = editorValue("amount", editors.value("amount")).toDouble();
            if (!std::isfinite(amount) || amount <= 0.0) {
                QMessageBox::warning(this, "Invalid amount",
                                     "Enter an amount greater than zero.");
                return;
            }
        }

        QStringList columns = module.fields;
        QVariantList values;

        for (const QString &field : module.fields)
            values.append(editorValue(field, editors[field]));

        if (module.table == "tasks") {
            columns << "created_at";
            values << timestamp();
        }
        else if (module.table == "notes") {
            columns << "created_at" << "updated_at";
            values << timestamp() << timestamp();
        }
        else if (module.table == "events") {
            columns << "created_at";
            values << timestamp();
        }
        else if (module.table == "habits") {
            columns << "created_at";
            values << timestamp();
        }

        QStringList placeholders;

        for (int i = 0; i < columns.size(); ++i)
            placeholders << "?";

        QString sql =
            "INSERT INTO " + module.table +
            " (" + columns.join(", ") + ") VALUES (" +
            placeholders.join(", ") + ")";

        QSqlQuery query(database);
        query.prepare(sql);

        for (const QVariant &value : values)
            query.addBindValue(value);

        if (!query.exec()) {
            QMessageBox::critical(
                this, "Save failed",
                query.lastError().text()
            );
            return;
        }

        refreshEverything();
        statusBar()->showMessage(name + " saved successfully.", 3000);
    }

    void editRecord(const QString &name)
    {
        Module module = moduleFor(name);

        if (!tableMap.contains(name))
            return;

        QTableWidget *table = tableMap[name];
        int row = table->currentRow();

        if (row < 0) {
            QMessageBox::information(
                this, "Select a record",
                "Select a row first, then click Edit."
            );
            return;
        }

        const int id =
            table->item(row, 0)->text().toInt();

        QSqlQuery get(database);
        get.prepare(
            "SELECT " + module.fields.join(", ") +
            " FROM " + module.table + " WHERE id = ?"
        );

        get.addBindValue(id);

        if (!get.exec() || !get.next()) {
            QMessageBox::warning(
                this, "Not found",
                "The selected record could not be loaded."
            );
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle("Edit " + name);
        dialog.resize(520, 500);

        auto *layout = new QVBoxLayout(&dialog);
        auto *form = new QFormLayout;
        form->setSpacing(12);

        QHash<QString, QWidget*> editors;

        for (int i = 0; i < module.fields.size(); ++i) {
            const QString field = module.fields[i];

            QWidget *editor = createEditor(
                field, get.value(i), &dialog
            );

            editors[field] = editor;

            QString label = field;
            label.replace("_", " ");
            label = label.left(1).toUpper() + label.mid(1);

            form->addRow(label, editor);
        }

        layout->addLayout(form);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Save |
            QDialogButtonBox::Cancel
        );

        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted,
                &dialog, &QDialog::accept);

        connect(buttons, &QDialogButtonBox::rejected,
                &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted)
            return;

        if (module.table == "transactions") {
            const double amount = editorValue("amount", editors.value("amount")).toDouble();
            if (!std::isfinite(amount) || amount <= 0.0) {
                QMessageBox::warning(this, "Invalid amount",
                                     "Enter an amount greater than zero.");
                return;
            }
        }

        QStringList assignments;

        for (const QString &field : module.fields)
            assignments << field + " = ?";

        if (module.table == "notes")
            assignments << "updated_at = ?";

        QSqlQuery update(database);

        update.prepare(
            "UPDATE " + module.table +
            " SET " + assignments.join(", ") +
            " WHERE id = ?"
        );

        for (const QString &field : module.fields)
            update.addBindValue(
                editorValue(field, editors[field])
            );

        if (module.table == "notes")
            update.addBindValue(timestamp());

        update.addBindValue(id);

        if (!update.exec()) {
            QMessageBox::critical(
                this, "Update failed",
                update.lastError().text()
            );
            return;
        }

        refreshEverything();
        statusBar()->showMessage(name + " updated.", 3000);
    }

    void deleteRecord(const QString &name)
    {
        Module module = moduleFor(name);

        if (!tableMap.contains(name))
            return;

        QTableWidget *table = tableMap[name];
        int row = table->currentRow();

        if (row < 0) {
            QMessageBox::information(
                this, "Select a record",
                "Select a row first, then click Delete."
            );
            return;
        }

        int answer = QMessageBox::question(
            this,
            "Confirm deletion",
            "Delete this " + name.toLower() +
            " record permanently?"
        );

        if (answer != QMessageBox::Yes)
            return;

        const int id =
            table->item(row, 0)->text().toInt();

        QSqlQuery query(database);
        query.prepare(
            "DELETE FROM " + module.table + " WHERE id = ?"
        );

        query.addBindValue(id);

        if (!query.exec()) {
            QMessageBox::critical(
                this, "Delete failed",
                query.lastError().text()
            );
            return;
        }

        refreshEverything();
        statusBar()->showMessage(name + " deleted.", 3000);
    }

    // -------------------------------------------------------
    // ANALYTICS
    // -------------------------------------------------------

    void createAnalyticsPage()
    {
        auto *page = createPageContainer(
            "Analytics",
            "A simple overview of your productivity and finances."
        );

        auto *layout =
            qobject_cast<QVBoxLayout*>(page->layout());

        auto *card = new QFrame;
        card->setObjectName("card");

        auto *box = new QVBoxLayout(card);
        box->setContentsMargins(24, 22, 24, 22);
        box->setSpacing(18);

        auto *title = new QLabel("Your productivity overview");
        title->setObjectName("cardTitle");

        auto *stats = new QLabel;
        stats->setObjectName("analyticsText");
        stats->setWordWrap(true);

        auto *chart = new AnalyticsChart;
        chart->setObjectName("analyticsVisualChart");

        auto *refresh = new QPushButton("Refresh analytics");
        refresh->setObjectName("primaryButton");

        connect(refresh, &QPushButton::clicked,
                this, [this]() {
            refreshAnalytics();
        });

        box->addWidget(title);
        box->addWidget(stats);
        box->addWidget(chart);
        box->addWidget(refresh);
        box->addStretch();

        layout->addWidget(card, 1);

        pageMap["Analytics"] = page;
        pages->addWidget(page);
    }

    void refreshAnalytics()
    {
        QWidget *page = pageMap.value("Analytics", nullptr);

        if (!page)
            return;

        auto *label = page->findChild<QLabel*>("analyticsText");

        if (!label)
            return;

        int totalTasks = countRows("tasks");
        int doneTasks = countRows("tasks", "completed = 1");
        int pendingTasks = totalTasks - doneTasks;

        double income = financeTotal("Income");
        double expenses = financeTotal("Expense");
        double balance = income - expenses;

        int totalHabits = countRows("habits");
        int habitsDone = countRows("habits", "completed_today = 1");

        int percentage = totalTasks > 0
            ? doneTasks * 100 / totalTasks
            : 0;

        QString report;

        report += "TASK PERFORMANCE\n";
        report += "────────────────────────────────\n";
        report += QString("Total tasks:             %1\n").arg(totalTasks);
        report += QString("Completed tasks:         %1\n").arg(doneTasks);
        report += QString("Pending tasks:           %1\n").arg(pendingTasks);
        report += QString("Completion rate:         %1%\n\n").arg(percentage);

        report += "HABIT TRACKING\n";
        report += "────────────────────────────────\n";
        report += QString("Total habits:             %1\n").arg(totalHabits);
        report += QString("Marked done today:        %1\n\n").arg(habitsDone);

        report += "FINANCIAL OVERVIEW\n";
        report += "────────────────────────────────\n";
        report += QString("Total income:            ₹%1\n")
            .arg(income, 0, 'f', 2);
        report += QString("Total expenses:          ₹%1\n")
            .arg(expenses, 0, 'f', 2);
        report += QString("Net balance:             ₹%1\n\n")
            .arg(balance, 0, 'f', 2);

        report += "NOTES & EVENTS\n";
        report += "────────────────────────────────\n";
        report += QString("Saved notes:              %1\n")
            .arg(countRows("notes"));
        report += QString("Scheduled events:         %1\n")
            .arg(countRows("events"));

        label->setText(report);

        if (auto *chartWidget = page->findChild<QWidget*>("analyticsVisualChart")) {
            auto *chart = static_cast<AnalyticsChart*>(chartWidget);
            chart->setValues(doneTasks, pendingTasks, habitsDone,
                             totalHabits, income, expenses);
        }
    }

    // -------------------------------------------------------
    // GLOBAL SEARCH, BACKUP / RESTORE, AND REMINDERS
    // -------------------------------------------------------

    void showGlobalSearch()
    {
        QDialog dialog(this);
        dialog.setWindowTitle("Search all LifeOS data");
        dialog.resize(760, 520);
        auto *layout = new QVBoxLayout(&dialog);
        auto *input = new QLineEdit;
        input->setPlaceholderText("Search tasks, notes, events, habits and transactions...");
        auto *results = new QTableWidget(0, 3);
        results->setHorizontalHeaderLabels({"TYPE", "TITLE", "DETAIL"});
        results->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        results->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        results->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        results->setEditTriggers(QAbstractItemView::NoEditTriggers);
        results->setSelectionBehavior(QAbstractItemView::SelectRows);
        results->setAlternatingRowColors(true);
        layout->addWidget(input);
        layout->addWidget(results, 1);

        auto runSearch = [this, results](const QString &term) {
            results->setRowCount(0);
            const QString pattern = "%" + term.trimmed() + "%";
            if (term.trimmed().isEmpty()) return;
            struct SearchSpec { QString type, sql; int binds; };
            const QList<SearchSpec> specs = {
                {"Task", "SELECT title, COALESCE(description,'') FROM tasks WHERE title LIKE ? OR description LIKE ? ORDER BY id DESC LIMIT 40", 2},
                {"Note", "SELECT title, COALESCE(content,'') FROM notes WHERE title LIKE ? OR content LIKE ? ORDER BY id DESC LIMIT 40", 2},
                {"Event", "SELECT title, COALESCE(description,'') FROM events WHERE title LIKE ? OR description LIKE ? OR location LIKE ? ORDER BY id DESC LIMIT 40", 3},
                {"Habit", "SELECT title, frequency FROM habits WHERE title LIKE ? OR frequency LIKE ? ORDER BY id DESC LIMIT 40", 2},
                {"Transaction", "SELECT title, type || ' • ₹' || amount FROM transactions WHERE title LIKE ? OR category LIKE ? OR type LIKE ? ORDER BY id DESC LIMIT 40", 3}
            };
            for (const auto &spec : specs) {
                QSqlQuery query(database);
                query.prepare(spec.sql);
                for (int i = 0; i < spec.binds; ++i) query.addBindValue(pattern);
                if (!query.exec()) continue;
                while (query.next()) {
                    int row = results->rowCount();
                    results->insertRow(row);
                    results->setItem(row, 0, new QTableWidgetItem(spec.type));
                    results->setItem(row, 1, new QTableWidgetItem(query.value(0).toString()));
                    QString detail = query.value(1).toString();
                    if (detail.size() > 180) detail = detail.left(180) + "…";
                    results->setItem(row, 2, new QTableWidgetItem(detail));
                }
            }
            if (results->rowCount() == 0) {
                results->insertRow(0);
                results->setItem(0, 0, new QTableWidgetItem("No matches"));
                results->setItem(0, 1, new QTableWidgetItem("Try another search term"));
            }
        };
        connect(input, &QLineEdit::textChanged, &dialog, runSearch);
        input->setFocus();
        dialog.exec();
    }

    void backupDatabase()
    {
        const QString target = QFileDialog::getSaveFileName(
            this, "Save LifeOS backup", QDir::homePath() + "/LifeOS-backup-" +
            QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".db",
            "SQLite database (*.db)");
        if (target.isEmpty()) return;
        if (QFile::exists(target)) QFile::remove(target);
        if (QFile::copy(databasePath, target))
            QMessageBox::information(this, "Backup created", "Your LifeOS database backup was saved to:\n" + target);
        else
            QMessageBox::critical(this, "Backup failed", "Could not copy the database. Check the destination folder and permissions.");
    }

    void restoreDatabase()
    {
        const QString source = QFileDialog::getOpenFileName(this, "Choose LifeOS backup", QDir::homePath(), "SQLite database (*.db)");
        if (source.isEmpty()) return;
        if (QFileInfo(source).absoluteFilePath() == QFileInfo(databasePath).absoluteFilePath()) {
            QMessageBox::warning(this, "Restore", "That is the currently active database.");
            return;
        }
        if (QMessageBox::warning(this, "Confirm restore",
            "Restoring replaces your current LifeOS data. A safety backup of the current database will be created first. Continue?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

        const QString safety = databasePath + ".before-restore-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
        database.close();
        const bool backedUp = QFile::copy(databasePath, safety);
        const bool removed = QFile::remove(databasePath);
        const bool copied = removed && QFile::copy(source, databasePath);
        database.open();
        if (!copied) {
            QMessageBox::critical(this, "Restore failed", "The selected backup could not be restored. Your previous database safety copy is at:\n" + safety);
            return;
        }
        refreshEverything();
        QMessageBox::information(this, "Restore complete", "Backup restored successfully. Please restart LifeOS if any page still shows old information.");
    }

    void setupReminders()
    {
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            trayIcon = new QSystemTrayIcon(QApplication::windowIcon(), this);
            trayIcon->setToolTip("LifeOS reminders");
            auto *menu = new QMenu(this);
            QAction *openAction = menu->addAction("Open LifeOS");
            connect(openAction, &QAction::triggered, this, [this]() { showNormal(); raise(); activateWindow(); });
            menu->addSeparator();
            QAction *quitAction = menu->addAction("Exit");
            connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
            trayIcon->setContextMenu(menu);
            trayIcon->show();
        }
        reminderTimer = new QTimer(this);
        reminderTimer->setInterval(60000);
        connect(reminderTimer, &QTimer::timeout, this, [this]() { checkReminders(); });
        reminderTimer->start();
        QTimer::singleShot(2500, this, [this]() { checkReminders(); });
    }

    void checkReminders()
    {
        const QDate today = QDate::currentDate();
        if (lastReminderDate == today) return;
        const QString date = today.toString("yyyy-MM-dd");
        QSqlQuery events(database);
        events.prepare("SELECT title, event_time FROM events WHERE event_date = ? ORDER BY event_time");
        events.addBindValue(date);
        QStringList due;
        if (events.exec()) while (events.next()) {
            const QString time = events.value(1).toString();
            due << (time.isEmpty() ? events.value(0).toString() : time + " — " + events.value(0).toString());
        }
        const int overdue = countRows("tasks", "completed = 0 AND due_date <> '' AND due_date < '" + date + "'");
        if (!due.isEmpty() || overdue > 0) {
            QString message;
            if (!due.isEmpty()) message += "Today's events: " + due.join("; ") + ". ";
            if (overdue > 0) message += QString("You have %1 overdue task(s).").arg(overdue);
            if (trayIcon && trayIcon->isVisible()) trayIcon->showMessage("LifeOS daily reminder", message, QSystemTrayIcon::Information, 10000);
            statusBar()->showMessage(message, 15000);
        }
        lastReminderDate = today;
    }

    // -------------------------------------------------------
    // SETTINGS
    // -------------------------------------------------------

    void createSettingsPage()
    {
        auto *page = createPageContainer(
            "Settings",
            "LifeOS preferences and application information."
        );

        auto *layout =
            qobject_cast<QVBoxLayout*>(page->layout());

        auto *card = new QFrame;
        card->setObjectName("card");

        auto *box = new QVBoxLayout(card);
        box->setContentsMargins(24, 22, 24, 22);
        box->setSpacing(14);

        auto *title = new QLabel("Application preferences");
        title->setObjectName("cardTitle");

        auto *theme = new QLabel(
            "ACTIVE THEME\n"
            "Crimson • Plum • Midnight Gradient\n\n"
            "The theme is applied consistently across "
            "the dashboard, navigation, forms, and tables."
        );

        theme->setWordWrap(true);
        theme->setObjectName("analyticsText");

        auto *databaseInfo = new QLabel(
            "LOCAL DATA STORAGE\n"
            "LifeOS uses an SQLite database on this computer. "
            "Your records are stored locally and persist "
            "between application launches."
        );

        databaseInfo->setWordWrap(true);
        databaseInfo->setObjectName("analyticsText");

        auto *backupButton = new QPushButton("Create database backup");
        backupButton->setObjectName("primaryButton");
        connect(backupButton, &QPushButton::clicked, this, [this]() { backupDatabase(); });

        auto *restoreButton = new QPushButton("Restore from backup");
        restoreButton->setObjectName("secondaryButton");
        connect(restoreButton, &QPushButton::clicked, this, [this]() { restoreDatabase(); });

        auto *refresh = new QPushButton("Refresh all data");
        refresh->setObjectName("primaryButton");

        connect(refresh, &QPushButton::clicked,
                this, [this]() {
            refreshEverything();

            QMessageBox::information(
                this,
                "LifeOS",
                "All visible modules have been refreshed."
            );
        });

        auto *about = new QPushButton("About LifeOS");
        about->setObjectName("secondaryButton");

        connect(about, &QPushButton::clicked,
                this, [this]() {
            QMessageBox::about(
                this,
                "About LifeOS",
                "LifeOS Personal Command Center\n\n"
                "Tasks • Notes • Calendar • Habits\n"
                "Finance • Analytics • Settings\n\n"
                "Built with C++ and Qt 6."
            );
        });

        box->addWidget(title);
        box->addWidget(theme);
        box->addWidget(databaseInfo);
        box->addWidget(backupButton, 0, Qt::AlignLeft);
        box->addWidget(restoreButton, 0, Qt::AlignLeft);
        box->addWidget(refresh, 0, Qt::AlignLeft);
        box->addWidget(about, 0, Qt::AlignLeft);
        box->addStretch();

        layout->addWidget(card, 1);

        pageMap["Settings"] = page;
        pages->addWidget(page);
    }

    // -------------------------------------------------------
    // REFRESH
    // -------------------------------------------------------

    void refreshEverything()
    {
        for (const Module &module : modules)
            refreshModule(module.name);

        refreshDashboard();
        refreshAnalytics();
    }

    // -------------------------------------------------------
    // RED AND BLACK THEME
    // -------------------------------------------------------

    void applyTheme()
    {
        qApp->setStyle("Fusion");

        setStyleSheet(R"(
            QWidget {
                background-color: transparent;
                color: #F1F1F4;
                font-family: "Segoe UI";
                font-size: 13px;
            }

            /* Prevent black rectangles behind labels and headings. */
            QLabel {
                background-color: transparent;
                border: none;
            }

            QToolTip {
                background-color: #242027;
                color: #FFFFFF;
                border: 1px solid #A93440;
                padding: 6px 8px;
                border-radius: 5px;
            }

            QWidget#root {
                background-color: qlineargradient(
                    x1: 0, y1: 0, x2: 1, y2: 1,
                    stop: 0 #171421,
                    stop: 0.45 #201522,
                    stop: 1 #101923
                );
            }

            QWidget#sidebar {
                background-color: qlineargradient(
                    x1: 0, y1: 0, x2: 0, y2: 1,
                    stop: 0 #211722,
                    stop: 0.55 #191721,
                    stop: 1 #121923
                );
                border-right: 1px solid #453044;
            }

            QLabel#brand {
                color: #FF424D;
                font-size: 31px;
                font-weight: 900;
                letter-spacing: 2px;
            }

            QLabel#brandSub {
                color: #85858F;
                font-size: 9px;
                letter-spacing: 1px;
            }

            QLabel#muted {
                color: #92929D;
                font-size: 12px;
            }

            QLabel#sectionLabel {
                color: #777781;
                font-size: 10px;
                font-weight: bold;
                letter-spacing: 1px;
                padding-top: 5px;
                padding-bottom: 5px;
            }

            QPushButton#navButton {
                text-align: left;
                background-color: transparent;
                color: #C8C8D0;
                border: 1px solid transparent;
                border-radius: 8px;
                padding: 10px 11px;
                font-size: 13px;
            }

            QPushButton#navButton:hover {
                background-color: #2A1D22;
                color: #FF777F;
                border: 1px solid #63303A;
            }

            QPushButton#navButton:checked {
                background-color: qlineargradient(
                    x1: 0, y1: 0, x2: 1, y2: 0,
                    stop: 0 #713044,
                    stop: 1 #3B2948
                );
                color: #FFFFFF;
                border: 1px solid #A74762;
                border-radius: 9px;
            }

            QLabel#pageTitle {
                color: #FFFFFF;
                font-size: 28px;
                font-weight: 800;
            }

            QFrame#heroPanel {
                background-color: qlineargradient(
                    x1: 0, y1: 0, x2: 1, y2: 1,
                    stop: 0 #51212F,
                    stop: 0.45 #302034,
                    stop: 1 #1B2435
                );
                border: 1px solid #A74762;
                border-radius: 18px;
            }

            QFrame#insightCard {
                background-color: qlineargradient(
                    x1: 0, y1: 0, x2: 1, y2: 0,
                    stop: 0 #38202F,
                    stop: 0.55 #282033,
                    stop: 1 #1C2232
                );
                border: 1px solid #654054;
                border-left: 4px solid #FF536B;
                border-radius: 14px;
            }

            QLabel#insightHeading {
                color: #FF737C;
                font-size: 10px;
                font-weight: 800;
                letter-spacing: 1.4px;
            }

            QLabel#insightText {
                color: #E5E1E5;
                font-size: 12px;
                line-height: 1.4;
            }

            QLabel#heroTag {
                color: #FF626B;
                font-size: 10px;
                font-weight: bold;
                letter-spacing: 2px;
            }

            QLabel#heroTitle {
                color: #FFFFFF;
                font-size: 30px;
                font-weight: 800;
            }

            QLabel#heroDescription {
                color: #C9BFC1;
                font-size: 13px;
            }

            QFrame#statCard {
                background-color: qlineargradient(
                    x1: 0, y1: 0, x2: 0, y2: 1,
                    stop: 0 #292333,
                    stop: 1 #1A1C29
                );
                border: 1px solid #413448;
                border-radius: 15px;
            }

            QFrame#statCard:hover {
                border: 1px solid #D84B6A;
                background-color: #302333;
            }

            QLabel#statLabel {
                color: #9A9AA5;
                font-size: 10px;
                font-weight: bold;
                letter-spacing: 1px;
            }

            QLabel#statValue {
                color: #FFFFFF;
                font-size: 24px;
                font-weight: 800;
            }

            QLabel#statIcon {
                color: #FF525D;
                font-size: 20px;
                font-weight: bold;
            }

            QFrame#card {
                background-color: qlineargradient(
                    x1: 0, y1: 0, x2: 1, y2: 1,
                    stop: 0 #292333,
                    stop: 1 #191D2A
                );
                border: 1px solid #3E3549;
                border-radius: 15px;
            }

            QLabel#cardTitle {
                color: #FFFFFF;
                font-size: 16px;
                font-weight: bold;
            }

            QLabel#dashboardList {
                color: #D5D5DC;
                font-size: 12px;
            }

            QLabel#analyticsText {
                color: #D5D5DC;
                font-size: 14px;
                line-height: 1.5;
            }

            QPushButton {
                background-color: #292333;
                color: #EAEAF0;
                border: 1px solid #38383F;
                border-radius: 7px;
                padding: 9px 13px;
            }

            QPushButton:hover {
                background-color: #34343B;
                border-color: #FF525D;
            }

            QPushButton:pressed {
                background-color: #4A2026;
            }

            QPushButton#primaryButton {
                background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #F04451, stop:1 #C92736);
                color: white;
                font-weight: bold;
                border: 1px solid #FF6872;
                border-radius: 8px;
                padding: 10px 15px;
            }

            QPushButton#primaryButton:hover {
                background-color: #F34A57;
                border-color: #FF9097;
            }

            QPushButton#secondaryButton {
                background-color: #25252B;
                border: 1px solid #45454D;
                padding: 10px 15px;
            }

            QPushButton#dangerButton {
                background-color: #30181C;
                color: #FF727A;
                border: 1px solid #6D2A33;
            }

            QPushButton#dangerButton:hover {
                background-color: #4A2026;
            }

            QPushButton#actionButton {
                text-align: left;
                background-color: #202024;
                border: 1px solid #34343B;
                padding: 10px;
            }

            QPushButton#actionButton:hover {
                background-color: #321B20;
                color: #FF6871;
                border-color: #7A3039;
            }

            QPushButton#textButton {
                color: #FF626B;
                background-color: transparent;
                border: none;
                text-align: left;
                padding-left: 0;
            }

            QLineEdit, QTextEdit, QDateEdit,
            QDoubleSpinBox, QComboBox {
                background-color: #211D2B;
                color: #F1F1F4;
                border: 1px solid #393940;
                border-radius: 6px;
                padding: 8px;
                selection-background-color: #D93240;
            }

            QLineEdit:focus, QTextEdit:focus,
            QDateEdit:focus, QDoubleSpinBox:focus,
            QComboBox:focus {
                border: 1px solid #FF525D;
            }

            QComboBox QAbstractItemView {
                background-color: #202024;
                color: #FFFFFF;
                selection-background-color: #A72B35;
            }

            QCalendarWidget QWidget {
                background-color: #19191D;
                color: #E8E8EE;
                alternate-background-color: #25252B;
            }

            QCalendarWidget QToolButton {
                color: #FF777F;
                background-color: #252025;
                border: 1px solid #393940;
                border-radius: 5px;
                padding: 5px;
            }

            QCalendarWidget QAbstractItemView:enabled {
                color: #E8E8EE;
                selection-background-color: #D93240;
                selection-color: white;
            }

            QTableWidget {
                background-color: #1B1B28;
                alternate-background-color: #242131;
                color: #E8E8EE;
                gridline-color: #303037;
                border: 1px solid #33333A;
                border-radius: 8px;
                selection-background-color: #63242C;
                selection-color: #FFFFFF;
            }

            QTableWidget::item {
                padding: 7px;
            }

            QHeaderView::section {
                background-color: #252025;
                color: #FF777F;
                padding: 10px;
                border: none;
                border-bottom: 1px solid #493038;
                font-weight: bold;
            }

            QProgressBar {
                background-color: #29292F;
                color: white;
                border: 1px solid #393940;
                border-radius: 6px;
                height: 20px;
                text-align: center;
            }

            QProgressBar::chunk {
                background-color: #E63240;
                border-radius: 5px;
            }

            QDialog {
                background-color: #151518;
            }

            QStatusBar {
                background-color: #17171A;
                color: #92929D;
                border-top: 1px solid #303036;
            }

            QScrollBar:vertical {
                background: #17171B;
                width: 10px;
                margin: 0;
            }

            QScrollBar::handle:vertical {
                background: #48434A;
                min-height: 25px;
                border-radius: 4px;
            }

            QScrollBar::handle:vertical:hover {
                background: #B9323D;
            }
        )");
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("LifeOS");

    LifeOS window;
    window.show();

    return app.exec();
}