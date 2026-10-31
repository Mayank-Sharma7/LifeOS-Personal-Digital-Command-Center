#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QString>

class QStackedWidget;
class QPushButton;
class QTableWidget;
class QLineEdit;
class QLabel;
class QVBoxLayout;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    // Main application
    QStackedWidget *pages = nullptr;

    // Task Manager
    QTableWidget *taskTable = nullptr;
    QLineEdit *taskSearch = nullptr;

    QLabel *taskStatsValue = nullptr;
    QLabel *taskDashboardValue = nullptr;
    QVBoxLayout *dashboardTasksLayout = nullptr;

    // Notes Manager
    QTableWidget *notesTable = nullptr;
    QLineEdit *notesSearch = nullptr;

    QLineEdit *noteTitleInput = nullptr;
    QTextEdit *noteContentInput = nullptr;

    QLabel *notesStatusValue = nullptr;

    // Database
    QSqlDatabase database;

    // Prevent unwanted table signals during refresh
    bool loadingTasks = false;
    bool loadingNotes = false;

    // Application setup
    void setupUI();
    void applyTheme();

    // Database
    bool initializeDatabase();
    bool createNotesTable();

    // Pages
    QWidget *createDashboard();
    QWidget *createTasksPage();
    QWidget *createNotesPage();

    QWidget *createModulePage(
        const QString &title,
        const QString &description
    );

    // Navigation
    QPushButton *createNavButton(
        const QString &text,
        int pageIndex
    );

    void navigateTo(int pageIndex);

    // Task Manager
    void addTask();
    void editTask();
    void deleteTask();

    void refreshTasks();
    void refreshDashboard();

    int selectedTaskId() const;

    void updateTaskCompletion(
        int taskId,
        bool completed
    );

    // Notes Manager
    void addNote();
    void updateNote();
    void deleteNote();

    void refreshNotes();
    void loadSelectedNote();
    void clearNoteEditor();

    int selectedNoteId() const;
};

#endif