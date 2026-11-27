#pragma once

#include <QFileDialog>
#include <QMainWindow>

class QActionGroup;
class QDockWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class FindDialog;
class QLabel;
class QMenu;
class QTabWidget;
class Editor;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    Editor *newTab();
    bool openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *e) override;

private slots:
    void open();
    bool save();
    bool saveAs();
    void closeTab(int index);
    void onCurrentTabChanged();
    void updateStatus();
    void updateTabTitle(Editor *ed);

private:
    void buildMenus();
    void buildSearch();
    void buildAbout();
    void buildOptions();
    void rebuildLanguageMenu();
    void buildEncodingMenu();
    void reopenWithEncoding(const QString &id);
    bool createFileFormat(const QString &extension, Editor *forEditor);
    void askAboutUnknownFormat(Editor *ed);
    void applyShortcuts();
    void reloadPreferences();
    void reapplyColours();
    void showAbout();
    void saveCompressed();
    void backToFileMenu();
    void updateTabMemStats();
    void doFindNext(bool forward);
    void doFindAllInTabs();
    void doReplaceAllInTabs();
    void jumpToResult(QTreeWidgetItem *item);
    QString selectedText() const;
    QFileDialog::Options fileDialogOptions() const;
    Editor *current() const;
    Editor *editorAt(int index) const;
    bool saveEditor(Editor *ed, bool forceAsk);
    bool maybeSave(Editor *ed);
    void updateWindowTitle();

    QTabWidget *m_tabs;
    QLabel *m_posLabel;
    QLabel *m_langLabel;
    QLabel *m_encLabel;
    QActionGroup *m_encGroup = nullptr;
    QMenu *m_langMenu;
    QActionGroup *m_langGroup;
    QAction *m_wrapAction;
    FindDialog *m_findDlg = nullptr;
    QDockWidget *m_resultsDock = nullptr;
    QTreeWidget *m_results = nullptr;
    QMenu *m_fileMenu = nullptr;
    QLabel *m_statsLabel = nullptr;
    QTimer *m_statsTimer = nullptr;
    bool m_showWhitespace = false;
    QAction *m_aboutAction = nullptr;
};
